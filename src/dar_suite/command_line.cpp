//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2052 Denis Corbin
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "getopt_decision.h"

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
    char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif
} // end extern "C"

#include <string>
#include <algorithm>
#include <vector>
#include <deque>
#include <iostream>
#include <new>

#include "deci.hpp"
#include "command_line.hpp"
#include "user_interaction.hpp"
#include "tools.hpp"
#include "line_tools.hpp"
#include "dar.hpp"
#include "dar_suite.hpp"
#include "integers.hpp"
#include "no_comment.hpp"
#include "config_file.hpp"
#include "shell_interaction.hpp"
#include "dar.hpp"
#include "libdar.hpp"
#include "cygwin_adapt.hpp"
#include "mask_list.hpp"
#include "crit_action_cmd_line.hpp"
#include "criterium.hpp"
#include "fichier_local.hpp"

#define OPT_STRING "c:A:x:d:t:l:v::z::y::nw::p::k::R:s:S:X:I:P:bhLWDru:U:VC:i:o:OT::E:F:K:J:Y:Z:B:fm:NH::a::eQG:Mg:j#:*:,[:]:+:@:$:~:%:q/:^:_:01:2:.:3:9:<:>:=:4:5::6:7:"

#define ONLY_ONCE "Only one -%c is allowed, ignoring this extra option"
#define MISSING_ARG "Missing argument to -%c option"
#define INVALID_ARG "Invalid argument given to -%c option"
#define INVALID_SIZE "Invalid size given with option -%c"

#define DEFAULT_CRYPTO_SIZE 10240


using namespace std;
using namespace libdar;
struct pre_mask
{
    bool included;      // whether it is a include or exclude entry
    string mask;        // the string (either a mask or a filename)
    bool case_sensit;   // whether comparison is case sensitive
    bool file_listing;  // whether the corresponding string is a filename containing a list of file to match to
    bool glob_exp;      // whether this is a glob (not regex) expression
};

struct mask_opt
{
    bool case_sensit;
    bool file_listing;
    const path & prefix;
    bool glob_exp;

    mask_opt(const path & ref) : prefix(ref) {};

    void read_from(struct pre_mask m) { case_sensit = m.case_sensit; file_listing = m.file_listing; glob_exp = m.glob_exp; };
};

static const U_I min_compr_size_default = 100;
    // the default value for --mincompr

static const U_I sparse_file_min_size_default = 15;
    // the default value for --sparse-file-min-size

    // return a newly allocated memory (to be deleted by the caller)
static void show_license(shell_interaction & dialog);
static void show_warranty(shell_interaction & dialog);
static void show_version(shell_interaction & dialog, const char *command_name);
static void usage(shell_interaction & dialog, const char *command_name);
static void split_compression_algo(const char *arg, compression & algo, U_I & level);
static fsa_scope string_to_fsa(const string & arg);

#if HAVE_GETOPT_LONG
const struct option *get_long_opt();
#endif

struct recursive_param
{
	// input parameters
    shell_interaction *dialog;
    const char *home;
    const vector<string> dar_dcf_path;
    const vector<string> dar_duc_path;
	// output parameters
    vector<string> inclusions;
    deque<pre_mask> name_include_exclude;
    deque<pre_mask> path_include_exclude;
    deque<pre_mask> ea_include_exclude;
    deque<pre_mask> compr_include_exclude;
    deque<pre_mask> backup_hook_include_exclude;
    bool readconfig;
    bool glob_mode;
    vector<string> non_options;
    bool ordered_filters;
    bool case_sensit;
    bool fixed_date_mode;
    bool sparse_file_reactivation;
    U_I suffix_base;
    bool ea_erase;
    bool only_more_recent;
    bool detruire;
    bool no_inter;

    recursive_param(shell_interaction & x_dialog,
		    const char *x_home,
		    const vector<string> & x_dar_dcf_path,
		    const vector<string> & x_dar_duc_path): dar_dcf_path(x_dar_dcf_path), dar_duc_path(x_dar_duc_path)
    {
	dialog = new (nothrow) shell_interaction(x_dialog);
	if(dialog == NULL)
	    throw Ememory("recursive_param::recursive_param");
	home = x_home;

	    //
	readconfig = true;
	glob_mode = true; // defaults to glob expressions
	ordered_filters = false;
	case_sensit = true;
	fixed_date_mode = false;
	sparse_file_reactivation = false;
	suffix_base = TOOLS_BIN_SUFFIX;
	ea_erase = false;
	only_more_recent = false;
	detruire = true;
	no_inter = false;
    };

    recursive_param(const recursive_param & ref): dar_dcf_path(ref.dar_dcf_path), dar_duc_path(ref.dar_duc_path)
    {
	throw SRC_BUG;
    };

    ~recursive_param()
    {
	if(dialog != NULL)
	    delete dialog;
    }
};

static bool get_args_recursive(recursive_param & rec,
			       line_param & p,
			       S_I argc,
			       char  * const argv[]);


static void make_args_from_file(user_interaction & dialog,
                                operation op,
				const vector<string> & targets,
				const string & filename,
				S_I & argc,
                                char **&argv,
				bool info_details);
static void destroy(S_I argc, char **argv);
static void skip_getopt(S_I argc, char *const argv[], S_I next_to_read);
static bool update_with_config_files(recursive_param & rec, line_param & p);

static mask *make_include_exclude_name(const string & x, mask_opt opt);
static mask *make_exclude_path_ordered(const string & x, mask_opt opt);
static mask *make_exclude_path_unordered(const string & x, mask_opt opt);
static mask *make_include_path(const string & x, mask_opt opt);
static mask *make_ordered_mask(deque<pre_mask> & listing, mask *(*make_include_mask) (const string & x, mask_opt opt), mask *(*make_exclude_mask)(const string & x, mask_opt opt), const path & prefix);
static mask *make_unordered_mask(deque<pre_mask> & listing, mask *(*make_include_mask) (const string & x, mask_opt opt), mask *(*make_exclude_mask)(const string & x, mask_opt opt), const path & prefix);
static void add_non_options(S_I argc, char * const argv[], vector<string> & non_options);

// #define DEBOGGAGE
#ifdef DEBOGGAGE
static void show_args(S_I argc, char *argv[]);
#endif

bool get_args(shell_interaction & dialog,
              const char *home,
	      const vector<string> & dar_dcf_path,
	      const vector<string> & dar_duc_path,
	      S_I argc,
	      char *const argv[],
	      line_param & p)
{
    string cmd = path(argv[0]).basename();
    recursive_param rec = recursive_param(dialog, home, dar_dcf_path, dar_duc_path);

	// initializing parameters to their default values
    p.op = noop;
    p.file_size = 0;
    p.first_file_size = 0;
    p.filename = "";
    p.allow_over = true;
    p.warn_over = true;
    p.info_details = false;
    p.display_treated = false;
    p.display_treated_only_dir = false;
    p.display_skipped = false;
    p.algo = none;
    p.compression_level = 9;
    p.pause = 0;
    p.beep = false;
    p.empty_dir = false;
    p.input_pipe = "";
    p.output_pipe = "";
    p.what_to_check = inode::cf_all;
    p.execute = "";
    p.execute_ref = "";
    p.pass.clear();
    p.gnupg_key_size = 0;
    p.signatories.clear();
    p.blind_signatures = false;
    p.pass_ref.clear();
    p.flat = false;
    p.min_compr_size = min_compr_size_default;
    p.nodump = false;
    p.exclude_by_ea = false;
    p.ea_name_for_exclusion = "";
    p.hourshift = 0;
    p.warn_remove_no_match = true;
    p.filter_unsaved = false;
    p.empty = false;
    p.alter_atime = true;
    p.same_fs = false;
    p.snapshot = false;
    p.cache_directory_tagging = false;
    p.crypto_size = DEFAULT_CRYPTO_SIZE;
    p.crypto_size_ref = DEFAULT_CRYPTO_SIZE;
    p.list_mode = archive_options_listing::normal;
    p.aux_pass.clear();
    p.aux_execute = "";
    p.aux_crypto_size = DEFAULT_CRYPTO_SIZE;
    p.keep_compressed = false;
    p.fixed_date = 0;
    p.quiet = false;
    p.slice_perm = "";
    p.slice_user = "";
    p.slice_group = "";
    p.repeat_count = 3; // 3 retry by default
    p.repeat_byte = 1;  // 1 wasted byte allowed by default
    p.decremental = false;
#if FURTIVE_READ_MODE_AVAILABLE
    p.furtive_read_mode = true;
#else
    p.furtive_read_mode = false;
#endif
    p.lax = false;
    p.use_sequential_marks = true;
    p.sequential_read = false;
    p.sparse_file_min_size = sparse_file_min_size_default;
    p.dirty = dirtyb_warn;
    p.security_check = true;
    p.user_comment = "";
    p.hash = hash_none;
    p.num_digits = 0;
    p.ref_num_digits = 0;
    p.aux_num_digits = 0;
    p.only_deleted = false;
    p.not_deleted = false;
    p.backup_hook_mask = NULL;
    p.backup_hook_execute = "";
    p.list_ea = false;
    p.ignore_unknown_inode = false;
    p.no_compare_symlink_date = true;
    p.scope = all_fsa_families();

    try
    {
	opterr = 0;

	if(!get_args_recursive(rec, p, argc, argv))
	    return false;

	    // checking and updating options with configuration file if any
	if(rec.readconfig)
	    if(! update_with_config_files(rec, p))
		return false;

	    // this cannot be done sooner, because "info_details" would always be equal to false
	    // as command-line would not have been yet parsed.

	if(p.info_details)
	{
	    if(!rec.non_options.empty())
	    {
		vector<string>::iterator it = rec.non_options.begin();
		bool init_message_done = false;

		while(it != rec.non_options.end())
		{
		    if(*it != "auxiliary" && *it != "reference") // theses two targets are stored as "user targets" but are reserved targets not user's
		    {
			if(!init_message_done)
			{
			    dialog.warning(gettext("User target found on command line:"));
			    init_message_done = true;
			}
			dialog.printf("\t%S", &(*it)); // yes, strange syntax, where "&(*it)" is not equal to "it" ... :-)
		    }
		    ++it;
		}

		if(!init_message_done)
		    dialog.warning(gettext("No user target found on command line"));
	    }
	}

	    // some sanity checks

	if(p.filename == "" || p.sauv_root == NULL || p.op == noop)
	    throw Erange("get_args", tools_printf(gettext("Missing -c -x -d -t -l -C -+ option, see `%S -h' for help"), &cmd));
	if(p.filename == "-" && p.file_size != 0)
	    throw Erange("get_args", gettext("Slicing (-s option), is not compatible with archive on standard output (\"-\" as filename)"));
	if(p.filename != "-" && (p.op != create && p.op != isolate && p.op != merging))
	    if(p.sauv_root == NULL)
		throw SRC_BUG;
	if(p.filename != "-")
	    tools_check_basename(dialog, *p.sauv_root, p.filename, EXTENSION);
	if((p.op == merging || p.op == create) && p.aux_filename != NULL)
	{
	    if(p.aux_root == NULL)
		throw SRC_BUG;
	    else
		tools_check_basename(dialog, *p.aux_root, *p.aux_filename, EXTENSION);
	}

	if(p.fs_root == NULL)
	{
	    p.fs_root = new (nothrow) path(".");
	    if(p.fs_root == NULL)
		throw Ememory("get_args");
	}
	if(rec.fixed_date_mode && p.op != create)
	    throw Erange("get_args", gettext("-af option is only available with -c"));
	if(p.ref_filename != NULL && p.op == listing)
	    dialog.warning(gettext("-A option is not available with -l"));
	if(p.op == isolate && p.ref_filename == NULL)
	    throw Erange("get_args", gettext("with -C option, -A option is mandatory"));
	if(p.op == merging && p.ref_filename == NULL)
	    throw Erange("get_args", gettext("with -+ option, -A option is mandatory"));
	if(p.op != extract && !p.warn_remove_no_match)
	    dialog.warning(gettext("-wa is only useful with -x option"));
	if(p.filename == "-" && p.ref_filename != NULL && *p.ref_filename == "-"
	   && p.output_pipe == "" && !p.sequential_read)
	    throw Erange("get_args", gettext("-o is mandatory when using \"-A -\" with \"-c -\" \"-C -\" or \"-+ -\""));
	if(p.ref_filename != NULL && *p.ref_filename != "-")
	{
	    if(p.ref_root == NULL)
		throw SRC_BUG;
	    else
		tools_check_basename(dialog, *p.ref_root, *p.ref_filename, EXTENSION);
	}

	if(p.algo != none && p.op != create && p.op != isolate && p.op != merging)
	    dialog.warning(gettext("-z or -y need only to be used with -c -C or -+ options"));
	if(p.first_file_size != 0 && p.file_size == 0)
	    throw Erange("get_args", gettext("-S option requires the use of -s"));
	if(p.what_to_check != inode::cf_all && (p.op == isolate || (p.op == create && p.ref_root == NULL) || p.op == test || p.op == listing || p.op == merging))
	    dialog.warning(gettext("ignoring -O option, as it is useless in this situation"));
	if(p.what_to_check == inode::cf_all
	   && p.op == extract
	   && capability_CHOWN(dialog, p.info_details)
	   && getuid() != 0) // uid == 0 for root
	{
	    p.what_to_check = inode::cf_ignore_owner;
	    string msg = tools_printf(gettext("File ownership will not be restored as %s has not the CHOWN capability nor is running as root. to avoid this message use -O option"), cmd.c_str());
	    dialog.pause(msg);
	}
	if(p.furtive_read_mode
	   && capability_FOWNER(dialog, p.info_details) != libdar::capa_set
	   && getuid() != 0)
	{
	    if(p.op == create || p.op == diff)
		dialog.printf(gettext("Furtive read mode has been disabled as %s has not the FOWNER capability nor is running as root"), cmd.c_str());
	    p.furtive_read_mode = false;
	}

	if(p.execute_ref != "" && p.ref_filename == NULL)
	    dialog.warning(gettext("-F is only useful with -A option, for the archive of reference"));
	if(p.pass_ref != "" && p.ref_filename == NULL)
	{
	    dialog.warning(gettext("-J is only useful with -A option, for the archive of reference"));
	}
	if(p.flat && p.op != extract)
	    dialog.warning(gettext("-f in only available with -x option, ignoring"));
	if(p.min_compr_size != min_compr_size_default && p.op != create && p.op != merging)
	    dialog.warning(gettext("-m is only useful with -c"));
	if(p.hourshift > 0)
	{
	    if(p.op == create)
	    {
		if(p.ref_filename == NULL)
		    dialog.warning(gettext("-H is only useful with -A option when making a backup"));
	    }
	    else
		if(p.op == extract)
		{
		    if(!rec.only_more_recent)
			dialog.warning(gettext("-H is only useful with -r option when extracting"));
		}
		else
		    if(p.op != diff)
			dialog.warning(gettext("-H is only useful with -c, -d or -x"));
	}

	if(p.filter_unsaved && p.op != listing)
	    dialog.warning(gettext("-as is only available with -l, ignoring -as option"));
	if(p.empty && p.op != create && p.op != extract && p.op != merging && p.op != test)
	    dialog.warning(gettext("-e is only useful with -x, -c or -+ options"));
	if(!p.alter_atime && p.op != create && p.op != diff)
	    dialog.warning(gettext("-ac is only useful with -c or -d"));
	if(p.same_fs && p.op != create)
	    dialog.warning(gettext("-M is only useful with -c"));
	if(p.snapshot && p.op != create)
	    dialog.warning(gettext("The snapshot backup (-A +) is only available with -c option, ignoring"));
	if(p.cache_directory_tagging && p.op != create)
	    dialog.warning(gettext("The Cache Directory Tagging Standard is only useful while performing a backup, ignoring it here"));

	if((p.aux_root != NULL || p.aux_filename != NULL) && p.op != merging && p.op != create)
	    throw Erange("get_args", gettext("-@ is only available with -+ and -c options"));
	if(p.aux_pass != "" && p.op != merging && p.op != create)
	    throw Erange("get_args", gettext("-$ is only available with -+ option and -c options"));
	if(p.aux_execute != "" && p.op != merging && p.op != create)
	    throw Erange("get_args", gettext("-~ is only available with -+ and -c options"));
	if(p.aux_crypto_size != DEFAULT_CRYPTO_SIZE && p.op != merging && p.op != create)
	    throw Erange("get_args", tools_printf(gettext("-%% is only available with -+ option")));

	if(p.aux_pass != "" && p.aux_filename == NULL)
	    dialog.warning(gettext("-$ is only useful with -@ option, for the auxiliary archive of reference"));
	if(p.aux_crypto_size != DEFAULT_CRYPTO_SIZE && p.aux_filename == NULL)
	    dialog.printf(gettext("-%% is only useful with -@ option, for the auxiliary archive of reference"));
	if(p.aux_execute != "" && p.aux_filename == NULL)
	    dialog.warning(gettext("-~ is only useful with -@ option, for the auxiliary archive of reference"));
	if(p.keep_compressed && p.op != merging)
	{
	    dialog.warning(gettext("-ak is only available while merging (operation -+), ignoring -ak"));
	    p.keep_compressed = false;
	}

	if(p.algo != none && p.op == merging && p.keep_compressed)
	    dialog.warning(gettext("Compression option (-z or -y) is useless and ignored when using -ak option"));

	    // sparse files handling

	if(p.sparse_file_min_size != sparse_file_min_size_default)
	{
	    if(p.op != merging && p.op != create)
		dialog.warning(gettext("--sparse-file-min-size only available while saving or merging archives, ignoring"));
	    else
		if(p.op == merging && !rec.sparse_file_reactivation)
		    dialog.warning(gettext("To use --sparse-file-min-size while merging archive, you need to use -ah option too, please check man page for details"));
	}
	if(p.op == merging && !rec.sparse_file_reactivation)
	    p.sparse_file_min_size = 0; // disabled by default for archive merging

	if((p.not_deleted || p.only_deleted) && p.op != extract)
	    dialog.warning(gettext("-k option is only useful with -x option"));

	if(p.not_deleted && p.only_deleted)
	    throw Erange("get_args", gettext("-konly and -kignore cannot be used at the same time"));

	if(rec.no_inter && p.pause > 0)
	    throw Erange("get_args", gettext("-p and -Q options are mutually exclusives"));

	    //////////////////////
	    // generating masks
	    // for filenames
	    //
	if(rec.ordered_filters)
	    p.selection = make_ordered_mask(rec.name_include_exclude,
					    &make_include_exclude_name,
					    &make_include_exclude_name,
					    tools_relative2absolute_path(*p.fs_root, tools_getcwd()));
	else // unordered filters
	    p.selection = make_unordered_mask(rec.name_include_exclude,
					      &make_include_exclude_name,
					      &make_include_exclude_name,
					      tools_relative2absolute_path(*p.fs_root, tools_getcwd()));


	    /////////////////////////
	    // generating masks for
	    // directory tree
	    //

	if(rec.ordered_filters)
	    p.subtree = make_ordered_mask(rec.path_include_exclude,
					  &make_include_path,
					  &make_exclude_path_ordered,
					  p.op != test && p.op != merging && p.op != listing ? tools_relative2absolute_path(*p.fs_root, tools_getcwd()) : "<ROOT>");
	else // unordered filters
	    p.subtree = make_unordered_mask(rec.path_include_exclude,
					    &make_include_path,
					    &make_exclude_path_unordered,
					    p.op != test && p.op != merging && p.op != listing ? tools_relative2absolute_path(*p.fs_root, tools_getcwd()) : "<ROOT>");


	    ////////////////////////////////
	    // generating mask for
	    // compression selected files
	    //
	if(p.algo == none)
	{
	    if(!rec.compr_include_exclude.empty())
		dialog.warning(gettext("-Y and -Z are only useful with compression (-z or -y option for example), ignoring any -Y and -Z option"));
	    if(p.min_compr_size != min_compr_size_default)
		dialog.warning(gettext("-m is only useful with compression (-z or -y option for example), ignoring -m"));
	}

	if(p.algo != none)
	    if(rec.ordered_filters)
		p.compress_mask = make_ordered_mask(rec.compr_include_exclude,
						    &make_include_exclude_name,
						    &make_include_exclude_name,
						    tools_relative2absolute_path(*p.fs_root, tools_getcwd()));
	    else
		p.compress_mask = make_unordered_mask(rec.compr_include_exclude,
						      &make_include_exclude_name,
						      &make_include_exclude_name,
						      tools_relative2absolute_path(*p.fs_root, tools_getcwd()));
	else
	{
	    p.compress_mask = new (nothrow) bool_mask(true);
	    if(p.compress_mask == NULL)
		throw Ememory("get_args");
	}

	    ////////////////////////////////
	    // generating mask for EA
	    //
	    //

	if(rec.ordered_filters)
	    p.ea_mask = make_ordered_mask(rec.ea_include_exclude,
					  &make_include_exclude_name,
					  &make_include_exclude_name,
					  tools_relative2absolute_path(*p.fs_root, tools_getcwd()));
	else // unordered filters
	    p.ea_mask = make_unordered_mask(rec.ea_include_exclude,
					    &make_include_exclude_name,
					    &make_include_exclude_name,
					    tools_relative2absolute_path(*p.fs_root, tools_getcwd()));


	    ////////////////////////////////
	    // generating mask for backup hook
	    //
	    //

	if(rec.backup_hook_include_exclude.size() == 0)
	{
	    p.backup_hook_mask = NULL;
	    if(p.backup_hook_execute != "")
	    {
		p.backup_hook_execute = "";
		if(p.op != create)
		    dialog.warning(gettext("-= option is valid only while saving files, thus in conjunction with -c option, ignoring"));
		else
		    dialog.warning(gettext("-= option will be ignored as it is useless if you do not specify to which files or directories this backup hook is to be applied, thanks to -< and -> options. See man page for more details."));
	    }
	}
	else
	    if(p.op != create)
	    {
		dialog.warning(gettext("backup hook feature (-<, -> or -= options) is only available when saving files, ignoring"));
		p.backup_hook_mask = NULL;
		p.backup_hook_execute = "";
	    }
	    else
		if(rec.ordered_filters)
		    p.backup_hook_mask = make_ordered_mask(rec.backup_hook_include_exclude,
							   &make_exclude_path_unordered, // no mistake here about *exclude*, nor *unordered*
							   &make_exclude_path_unordered, // no mistake here about *exclude*, nor *unordered*
							   tools_relative2absolute_path(*p.fs_root, tools_getcwd()));
		else
		    p.backup_hook_mask = make_unordered_mask(rec.backup_hook_include_exclude,
							     &make_exclude_path_unordered,// no mistake here about *exclude*
							     &make_exclude_path_unordered,
							     tools_relative2absolute_path(*p.fs_root, tools_getcwd()));


	    ////////////////////////////////
	    // overwriting policy
	    //

	switch(p.op)
	{
	case merging:
	    if(p.overwrite == NULL)
	    {
		p.overwrite = new (nothrow) crit_constant_action(data_preserve, EA_merge_preserve);
		if(p.overwrite == NULL)
		    throw Ememory("get_args");
	    }
	    break;
	case extract:
	    if(p.overwrite == NULL)
	    {
		line_tools_4_4_build_compatible_overwriting_policy(p.allow_over,
								   rec.detruire,
								   rec.only_more_recent,
								   p.hourshift,
								   rec.ea_erase,
								   p.overwrite);
		if(p.overwrite == NULL)
		    throw Ememory("get_args");
	    }
	    break;
	default:
	    if(p.overwrite != NULL)
	    {
		delete p.overwrite;
		p.overwrite = NULL;
		dialog.warning(gettext("-/ option is only useful with -+ option, ignoring"));
	    }
	}

	    ////////////////////////////////
	    // user comment
	    //
	    //

	if(p.user_comment != "")
	    if(p.op != create && p.op != merging && p.op != isolate)
		dialog.warning(gettext("-. option is only useful when merging, creating or isolating an archive, ignoring"));
	    else
	    {
		p.user_comment = line_tools_expand_user_comment(p.user_comment, argc, argv);
		if(p.info_details)
		    dialog.printf(gettext("The following user comment will be placed in clear text in the archive: %S"), &p.user_comment);
	    }
	else
	    p.user_comment = "N/A";

	    ////////////////////////////////
	    // security check
	    //
	    //

	if(!p.alter_atime)
	    p.security_check = false;
    }
    catch(Erange & e)
    {
        dialog.warning(string(gettext("Parse error: ")) + e.get_message());
        return false;
    }
    return true;
}

const char *get_short_opt() { return OPT_STRING; }



static bool get_args_recursive(recursive_param & rec,
			       line_param & p,
			       S_I argc,
			       char  * const argv[])
{
    S_I lu;
    S_I rec_c;
    char **rec_v = NULL;
    pre_mask tmp_pre_mask;
    U_I tmp;
    string tmp_string, tmp_string2;

	// fetching first the targets (non optional argument of command line)
    add_non_options(argc, argv, rec.non_options);

#if HAVE_GETOPT_LONG
    while((lu = getopt_long(argc, argv, OPT_STRING, get_long_opt(), NULL)) != EOF)
#else
        while((lu = getopt(argc, argv, OPT_STRING)) != EOF)
#endif
        {
            switch(lu)
            {
            case 'c':
            case 'x':
            case 'd':
            case 't':
            case 'l':
            case 'C':
	    case '+':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(p.filename != "" || p.sauv_root != NULL)
                    throw Erange("get_args", gettext(" Only one option of -c -d -t -l -C -x or -+ is allowed"));
                if(string(optarg) != string(""))
                    tools_split_path_basename(optarg, p.sauv_root, p.filename);
                else
                    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
                switch(lu)
                {
                case 'c':
                    p.op = create;
                    break;
                case 'x':
                    p.op = extract;
                    break;
                case 'd':
                    p.op = diff;
                    break;
                case 't':
                    p.op = test;
                    break;
                case 'l':
                    p.op = listing;
                    break;
                case 'C':
                    p.op = isolate;
                    break;
		case '+':
		    p.op = merging;
		    break;
                default:
                    throw SRC_BUG;
                }
                break;
            case 'A':
                if(p.ref_filename != NULL || p.ref_root != NULL || p.snapshot || p.fixed_date != 0)
                    throw Erange("get_args", gettext("Only one -A option is allowed"));
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(strcmp("", optarg) == 0)
                    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
		if(rec.fixed_date_mode)
		{
		    try
		    {
			try
			{
				// trying to read a simple integer
			        // note that the namespace specification is necessary
			        // due to similar existing name in std namespace under
			        // certain OS (FreeBSD 10.0)
			    libdar::deci tmp = string(optarg);
			    p.fixed_date = tmp.computer();
			}
			catch(Edeci & e)
			{
				// fallback to human readable string

			    p.fixed_date = tools_convert_date(optarg);
			}
		    }
		    catch(Egeneric & e)
		    {
			throw Erange("get_args", string(gettext("Error while parsing -A argument as a date: ")+ e.get_message()));
		    }
		}
		else
		    if(strcmp("+", optarg) == 0)
			p.snapshot = true;
		    else
		    {
			p.ref_filename = new (nothrow) string();
			if(p.ref_filename == NULL)
			    throw Ememory("get_args");
			try
			{
			    tools_split_path_basename(optarg, p.ref_root, *p.ref_filename);
			    rec.non_options.push_back("reference");
			}
			catch(...)
			{
			    delete p.ref_filename;
			    p.ref_filename = NULL;
			    throw;
			}
		    }
                break;
            case 'v':
		if(optarg == NULL)
		{
		    p.info_details = true;
		    p.display_treated = true;
		    p.display_treated_only_dir = false;
		}
		else
		    if(strcasecmp("skipped", optarg) == 0 || strcasecmp("s", optarg) == 0)
			p.display_skipped = true;
		    else if(strcasecmp("treated", optarg) == 0 || strcasecmp("t", optarg) == 0)
		    {
			p.display_treated = true;
			p.display_treated_only_dir = false;
		    }
		    else if(strcasecmp("messages", optarg) == 0 || strcasecmp("m", optarg) == 0)
			p.info_details = true;
		    else if(strcasecmp("dir", optarg) == 0 || strcasecmp("d", optarg) == 0)
		    {
			p.display_treated = true;
			p.display_treated_only_dir = true;
		    }
		    else if(strcasecmp("all", optarg) == 0 || strcasecmp("a", optarg) == 0)
		    {
			p.info_details = true;
			p.display_skipped = true;
			p.display_treated = true;
			p.display_treated_only_dir = false;
		    }
		    else
			throw Erange("command_line.cpp:get_args_recursive", tools_printf(gettext(INVALID_ARG), char(lu)));
                break;
            case 'z':
		if(optarg != NULL)
		    split_compression_algo(optarg, p.algo, p.compression_level);
		else
		    if(p.algo == none)
			p.algo = gzip;
		    else
			throw Erange("get_args", gettext("Choose only one compression algorithm"));
                break;
            case 'y':
		rec.dialog->warning(gettext("Note that -y option is deprecated it will be removed in future release, please use -z option instead (read man page for details)"));
                if(p.algo == none)
                    p.algo = bzip2;
                else
                    throw Erange("get_args", gettext("Choose either -z or -y not both"));
                if(optarg != NULL)
                    if(! tools_my_atoi(optarg, p.compression_level) || p.compression_level > 9 || p.compression_level < 1)
                        throw Erange("get_args", gettext("Compression level must be between 1 and 9, included"));
                break;
            case 'n':
                p.allow_over = false;
                if(!p.warn_over)
                {
                    rec.dialog->warning(gettext("-w option is useless with -n"));
                    p.warn_over = false;
                }
                break;
            case 'w':
                p.warn_over = false;
                if(optarg != NULL)
                {
                    if(strcmp(optarg, "a") == 0 || strcmp(optarg, "all") == 0)
                        p.warn_remove_no_match = false;
                    else
                        if(strcmp(optarg, "d") != 0 && strcmp(optarg, "default") != 0)
                            throw Erange("get_args", string(gettext("Unknown argument given to -w: ")) + optarg);
                        // else this is the default -w
                }
                break;
            case 'p':
		if(optarg != NULL)
		{
			// note that the namespace specification is necessary
			// due to similar existing name in std namespace under
			// certain OS (FreeBSD 10.0)
		    libdar::deci conv = string(optarg);
		    p.pause = conv.computer();
		}
		else
		    p.pause = 1;
                break;
            case 'k':
		if(optarg == NULL) // -k without argument
		{
 		    if(p.only_deleted)
			throw Erange("command_line.cpp:get_args_recursive", string(gettext("\"-k\" (or \"-kignore\") and \"-konly\" are not compatible")));
		    p.not_deleted = true;
		}
		else
		    if(strcasecmp(optarg, "ignore") == 0)
		    {
			if(p.only_deleted)
			    throw Erange("command_line.cpp:get_args_recursive", string(gettext("\"-k\" (or \"-kignore\") and \"-konly\" are not compatible")));
			p.not_deleted = true;
		    }
		    else
			if(strcasecmp(optarg, "only") == 0)
			{
			    if(p.not_deleted)
				throw Erange("command_line.cpp:get_args_recursive", string(gettext("\"-k\" (or \"-kignore\") and \"-konly\" are not compatible")));
			    p.only_deleted = true;
			}
			else
			    throw Erange("command_line.cpp:get_args_recursive", tools_printf(gettext("Unknown argument given to -k : %s"), optarg));
                break;
            case 'R':
                if(p.fs_root != NULL)
                    throw Erange("get_args", gettext("Only one -R option is allowed"));
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                else
                    p.fs_root = new (nothrow) path(optarg, true);
                if(p.fs_root == NULL)
                    throw Ememory("get_args");
                break;
            case 's':
                if(p.file_size != 0)
                    throw Erange("get_args", gettext("Only one -s option is allowed"));
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                else
                {
                    try
                    {
                        p.file_size = tools_get_extended_size(optarg, rec.suffix_base);
                        if(p.first_file_size == 0)
                            p.first_file_size = p.file_size;
                    }
                    catch(Edeci &e)
                    {
                        rec.dialog->warning(tools_printf(gettext(INVALID_SIZE), char(lu)));
                        return false;
                    }
                }
                break;
            case 'S':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(p.first_file_size == 0)
                    p.first_file_size = tools_get_extended_size(optarg, rec.suffix_base);
                else
                    if(p.file_size == 0)
                        throw Erange("get_args", gettext("Only one -S option is allowed"));
                    else
                        if(p.file_size == p.first_file_size)
                        {
                            try
                            {
                                p.first_file_size = tools_get_extended_size(optarg, rec.suffix_base);
                                if(p.first_file_size == p.file_size)
                                    rec.dialog->warning(gettext("Giving to -S option the same value as the one given to -s option is useless"));
                            }
                            catch(Egeneric &e)
                            {
                                rec.dialog->warning(tools_printf(gettext(INVALID_SIZE), char(lu)));
                                return false;
                            }

                        }
                        else
                            throw Erange("get_args", gettext("Only one -S option is allowed"));
                break;
            case 'X':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                tmp_pre_mask.file_listing = false;
                tmp_pre_mask.case_sensit = rec.case_sensit;
                tmp_pre_mask.included = false;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = rec.glob_mode;
                rec.name_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'I':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                tmp_pre_mask.file_listing = false;
                tmp_pre_mask.case_sensit = rec.case_sensit;
                tmp_pre_mask.included = true;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = rec.glob_mode;
                rec.name_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'P':
            case ']':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                tmp_pre_mask.file_listing = lu == ']';
                tmp_pre_mask.case_sensit = rec.case_sensit;
                tmp_pre_mask.included = false;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = rec.glob_mode;
                rec.path_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'g':
            case '[':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                tmp_pre_mask.file_listing = lu == '[';
                tmp_pre_mask.case_sensit = rec.case_sensit;
                tmp_pre_mask.included = true;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = rec.glob_mode;
                rec.path_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'b':
                p.beep = true;
                break;
            case 'h':
                usage(*rec.dialog, argv[0]);
		p.op = version_or_help;
                return false;
            case 'L':
                show_license(*rec.dialog);
                return false;
            case 'W':
                show_warranty(*rec.dialog);
                return false;
            case 'D':
                if(p.empty_dir)
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    p.empty_dir = true;
                break;
            case 'r':
                if(!p.allow_over)
                    rec.dialog->warning(gettext("-r is useless with -n"));
                if(rec.only_more_recent)
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
		    rec.only_more_recent = true;
                break;
            case 'u':
            case 'U':
		if(optarg == NULL)
		    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		tmp_pre_mask.file_listing = false;
		tmp_pre_mask.case_sensit = rec.case_sensit;
		tmp_pre_mask.included = lu == 'U';
		tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = rec.glob_mode;
		rec.ea_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'V':
                show_version(*rec.dialog, argv[0]);
		p.op = version_or_help;
                return false;
            case 'i':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(p.input_pipe == "")
                    p.input_pipe = optarg;
                else
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                break;
            case 'o':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(p.output_pipe == "")
                    p.output_pipe = optarg;
                else
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                break;
            case 'O':
                if(p.what_to_check != inode::cf_all)
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
		    if(optarg == NULL)
			p.what_to_check = inode::cf_ignore_owner;
		    else
			if(strcasecmp(optarg, "ignore-owner") == 0)
			    p.what_to_check = inode::cf_ignore_owner;
			else
			    if(strcasecmp(optarg, "mtime") == 0)
				p.what_to_check = inode::cf_mtime;
			    else
				if(strcasecmp(optarg, "inode-type") == 0)
				    p.what_to_check = inode::cf_inode_type;
				else
				    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));

                break;
            case 'T':
                if(optarg == NULL)
                    p.list_mode = archive_options_listing::tree;
		else
		    if(strcasecmp("normal", optarg) == 0)
                	p.list_mode = archive_options_listing::normal;
		    else
			if (strcasecmp("tree", optarg) == 0)
			    p.list_mode = archive_options_listing::tree;
			else
			    if (strcasecmp("xml", optarg) == 0)
				p.list_mode = archive_options_listing::xml;
			    else
				throw Erange("command_line.cpp:get_args_recursive", tools_printf(gettext(INVALID_ARG), char(lu)));
                break;
            case 'E':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		line_tools_split_at_first_space(optarg, tmp_string, tmp_string2);
		tmp_string = line_tools_get_full_path_from_PATH(rec.dar_duc_path, tmp_string.c_str()) + " " + tmp_string2;
                if(p.execute == "")
                    p.execute = tmp_string;
                else
                    p.execute += string(" ; ") + tmp_string;
                break;
            case 'F':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(p.execute_ref == "")
                    p.execute_ref = optarg;
                else
                    p.execute_ref += string(" ; ") + optarg;
                break;
            case 'J':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(p.pass_ref == "")
                    p.pass_ref = secu_string(optarg, strlen(optarg));
                else
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                break;
            case 'K':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(p.pass == "")
                    p.pass = secu_string(optarg, strlen(optarg));
                else
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                break;
            case 'Y':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                tmp_pre_mask.file_listing = false;
                tmp_pre_mask.case_sensit = rec.case_sensit;
                tmp_pre_mask.included = true;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = rec.glob_mode;
                rec.compr_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'Z':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                tmp_pre_mask.file_listing = false;
                tmp_pre_mask.case_sensit = rec.case_sensit;
                tmp_pre_mask.included = false;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = rec.glob_mode;
                rec.compr_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'B':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		tmp_string = line_tools_get_full_path_from_PATH(rec.dar_dcf_path, optarg);
                if(find(rec.inclusions.begin(), rec.inclusions.end(), tmp_string) != rec.inclusions.end())
                    throw Erange("get_args", tools_printf(gettext("File inclusion loop detected. The file %s includes itself directly or through other files (-B option)"), optarg));
                else
                {
                    bool ret;
		    try
		    {
			make_args_from_file(*rec.dialog, p.op, rec.non_options, tmp_string, rec_c, rec_v, p.info_details);
		    }
		    catch(Erange & e)
		    {
			Erange modif = Erange("get_args", tools_printf(gettext("Error in included file (%s): "), optarg) + e.get_message());
			throw modif;
		    }
#if DEBOGGAGE
                    show_args(rec_c, rec_v);
#endif
                    S_I optind_mem = line_tools_reset_getopt(); // save the external variable to use recursivity (see getopt)
                        // reset getopt module

                    try
                    {
                        rec.inclusions.push_back(tmp_string);
			try
			{
			    ret = get_args_recursive(rec, p, rec_c, rec_v);
			}
			catch(Erange & e)
			{
			    Erange more = Erange(e.get_source(), tools_printf(gettext("In included file %S: "), &tmp_string) + e.get_message());
			    rec.inclusions.pop_back();
			    throw more;
			}
			catch(...)
			{
			    rec.inclusions.pop_back();
			    throw;
			}
                        rec.inclusions.pop_back();
                    }
                    catch(...)
                    {
                        destroy(rec_c, rec_v);
			rec_v = NULL;
			rec_c = 0;
                        skip_getopt(argc, argv, optind_mem);
                        throw;
                    }
                    destroy(rec_c, rec_v);
		    rec_v = NULL;
		    rec_c = 0;
                    skip_getopt(argc, argv, optind_mem); // restores getopt after recursion

                    if(!ret)
                        return false;
                }
                break;
            case 'f':
                if(p.flat)
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    p.flat = true;
                break;
            case 'm':
                if(p.min_compr_size != min_compr_size_default)
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                {
                    if(optarg == NULL)
                        throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                    p.min_compr_size = tools_get_extended_size(optarg, rec.suffix_base);
                    if(p.min_compr_size == min_compr_size_default)
                        rec.dialog->warning(tools_printf(gettext("%d is the default value for -m, no need to specify it on command line, ignoring"), min_compr_size_default));
                    break;
                }
            case 'N':
                if(!rec.readconfig)
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    rec.readconfig = false;
                break;
            case ' ':
#ifdef LIBDAR_NODUMP_FEATURE
                if(p.nodump)
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    p.nodump = true;
                break;
#else
                throw Ecompilation(gettext("--nodump feature has not been activated at compilation time, it is thus not available"));
#endif
            case 'H':
                if(optarg == NULL)
                    p.hourshift = 1;
                else
                {
                    try
                    {
			    // note that the namespace specification is necessary
			    // due to similar existing name in std namespace under
			    // certain OS (FreeBSD 10.0)
                        p.hourshift = libdar::deci(string(optarg)).computer();
                    }
                    catch(Edeci & e)
                    {
                        throw Erange("command_line.cpp:get_args_recursive", gettext("Argument given to -H is not a positive integer number"));
                    }
                }
                break;
            case 'a':
                if(optarg == NULL)
                    throw Erange("command_line.cpp:get_args_recursive", gettext("-a option requires an argument"));
                if(strcasecmp("SI-unit", optarg) == 0 || strcasecmp("SI", optarg) == 0 || strcasecmp("SI-units", optarg) == 0)
                    rec.suffix_base = TOOLS_SI_SUFFIX;
                else if(strcasecmp("binary-unit", optarg) == 0 || strcasecmp("binary", optarg) == 0 || strcasecmp("binary-units", optarg) == 0)
		    rec.suffix_base = TOOLS_BIN_SUFFIX;
		else if(strcasecmp("atime", optarg) == 0 || strcasecmp("a", optarg) == 0)
		{
		    p.alter_atime = true;
		    p.furtive_read_mode = false;
		}
		else if(strcasecmp("ctime", optarg) == 0 || strcasecmp("c", optarg) == 0)
		{
		    p.alter_atime = false;
		    p.furtive_read_mode = false;
		}
		else if(strcasecmp("m", optarg) == 0 || strcasecmp("mask", optarg) == 0)
		{
		    if(rec.ordered_filters)
			rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
		    else
			rec.ordered_filters = true;
		}
		else if(strcasecmp("n", optarg) == 0 || strcasecmp("no-case", optarg) == 0 || strcasecmp("no_case", optarg) == 0)
		    rec.case_sensit = false;
		else if(strcasecmp("case", optarg) == 0)
		    rec.case_sensit = true;
		else if(strcasecmp("s", optarg) == 0 || strcasecmp("saved", optarg) == 0)
		{
		    if(p.filter_unsaved)
			rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
		    else
			p.filter_unsaved = true;
		}
		else if(strcasecmp("e", optarg) == 0 || strcasecmp("erase_ea", optarg) == 0)
		{
		    if(rec.ea_erase)
			rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
		    else
			rec.ea_erase = true;
		}
		else if(strcasecmp("g", optarg) == 0 || strcasecmp("glob", optarg) == 0)
		    rec.glob_mode = true;
		else if(strcasecmp("r", optarg) == 0 || strcasecmp("regex", optarg) == 0)
		    rec.glob_mode = false;
		else if(strcasecmp("k", optarg) == 0 || strcasecmp("keep-compressed", optarg) == 0)
		{
		    if(p.keep_compressed)
			rec.dialog->warning(gettext("-ak option need not be specified more than once, ignoring extra -ak options"));
		    p.keep_compressed = true;
		}
		else if(strcasecmp("f", optarg) == 0 || strcasecmp("fixed-date", optarg) == 0)
		{
		    if(p.ref_filename != NULL || p.ref_root != NULL || p.snapshot)
			throw Erange("get_args", gettext("-af must be present before -A option not after!"));
		    if(rec.fixed_date_mode)
			rec.dialog->warning(gettext("-af option need not be specified more than once, ignoring extra -af options"));
		    rec.fixed_date_mode = true;
		}
		else if(strcasecmp("d", optarg) == 0 || strcasecmp("decremental", optarg) == 0)
		    p.decremental = true;
		else if(strcasecmp("l", optarg) == 0 || strcasecmp("lax", optarg) == 0)
		    p.lax = true;
		else if(strcasecmp("t", optarg) == 0 || strcasecmp("tape-marks", optarg) == 0)
		    p.use_sequential_marks = false;
		else if(strcasecmp("h", optarg) == 0 || strcasecmp("holes-recheck", optarg) == 0)
		    rec.sparse_file_reactivation = true;
		else if(strcasecmp("secu", optarg) == 0)
		    p.security_check = false;
		else if(strcasecmp("list-ea", optarg) == 0)
		    p.list_ea = true;
		else if(strcasecmp("i", optarg) == 0 || strcasecmp("ignore-unknown-inode-type", optarg) == 0)
		    p.ignore_unknown_inode = true;
		else if(strcasecmp("do-not-compare-symlink-mtime", optarg) == 0)
		    p.no_compare_symlink_date = false;
		else if(strcasecmp("test-self-reported-bug", optarg) == 0)
		    throw SRC_BUG; // testing the way a internal error is reported
		else if(strcasecmp("b", optarg) == 0 || strcasecmp("blind-to-signatures", optarg) == 0)
		    p.blind_signatures = true;
		else
		    throw Erange("command_line.cpp:get_args_recursive", tools_printf(gettext("Unknown argument given to -a : %s"), optarg));
                break;
            case 'e':
                if(p.empty)
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    p.empty = true;
                break;
            case 'Q':
		rec.no_inter = true;
		break;
            case 'j':
                break;  // ignore this option already parsed during initialization (dar_suite.cpp)
            case 'G':
		throw Erange("get_args", gettext("-G option is obsolete, use -@ option instead, see man page for details"));
            case 'M':
                if(p.same_fs)
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    p.same_fs = true;
                break;
            case '#':
                if(! tools_my_atoi(optarg, tmp))
                    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
                else
                    p.crypto_size = (U_32)tmp;
                break;
            case '*':
                if(! tools_my_atoi(optarg, tmp))
                    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
                else
                    p.crypto_size_ref = (U_32)tmp;
                break;
            case ',':
                if(p.cache_directory_tagging)
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    p.cache_directory_tagging = true;
                break;
	    case '@':
		if(p.aux_filename != NULL || p.aux_root != NULL)
		    throw Erange("get_args", gettext("Only one -@ option is allowed"));
		if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		if(strcmp("", optarg) == 0)
                    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
		else
		{
		    p.aux_filename = new (nothrow) string();
		    if(p.aux_filename == NULL)
			throw Ememory("get_args");
		    try
		    {
			tools_split_path_basename(optarg, p.aux_root, *p.aux_filename);
			rec.non_options.push_back("auxiliary");
		    }
		    catch(...)
		    {
			delete p.aux_filename;
			p.aux_filename = NULL;
			throw;
		    }
		}
		break;
	    case '~':
		if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(p.aux_execute == "")
                    p.aux_execute = optarg;
                else
                    p.aux_execute += string(" ; ") + optarg;
		break;
	    case '$':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(p.aux_pass == "")
                    p.aux_pass = secu_string(optarg, strlen(optarg));
                else
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                break;
	    case '%':
                if(! tools_my_atoi(optarg, tmp))
                    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
                else
                    p.aux_crypto_size = (U_32)tmp;
                break;
	    case '/':
		if(optarg == NULL)
		    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		if(p.overwrite == NULL)
		{
		    try
		    {
			p.overwrite = crit_action_create_from_string(*rec.dialog, crit_action_canonize_string(optarg), p.hourshift);
		    }
		    catch(Erange & e)
		    {
			throw Erange(e.get_source(), string(gettext("Syntax error in overwriting policy: ") + e.get_message()));
		    }
		}
		else
                    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                break;
	    case '^':
		if(optarg == NULL)
		    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		line_tools_slice_ownership(string(optarg), p.slice_perm, p.slice_user, p.slice_group);
		break;
	    case '_':
		if(optarg == NULL)
		    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		line_tools_repeat_param(string(optarg), p.repeat_count, p.repeat_byte);
		break;
	    case '0':
		if(optarg == NULL)
		    p.sequential_read = true;
		else
		    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
		break;
	    case '1':
		if(p.sparse_file_min_size != sparse_file_min_size_default)
		    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
		else
		{
		    if(optarg == NULL)
                        throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		    try
		    {
			p.sparse_file_min_size = tools_get_extended_size(optarg, rec.suffix_base);
			if(p.sparse_file_min_size == sparse_file_min_size_default)
			    rec.dialog->warning(tools_printf(gettext("%d is the default value for --sparse-file-min-size, no need to specify it on command line, ignoring"), sparse_file_min_size_default));
		    }
		    catch(Edeci & e)
		    {
			throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
		    }
		}
		break;
	    case '2':
		if(p.dirty != dirtyb_warn)
		    rec.dialog->warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
		else
		{
		    if(optarg == NULL)
			throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		    if(strcasecmp("ignore", optarg) == 0)
			p.dirty = dirtyb_ignore;
		    else
			if(strcasecmp("no-warn", optarg) == 0)
			    p.dirty = dirtyb_ok;
			else
			    throw Erange("command_line.cpp:get_args_recursive", tools_printf(gettext("Unknown argument given to -2 : %s"), optarg));
		}
		break;
	    case '"':
		if(optarg == NULL)
		    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		else
		{
		    tlv_list tmp;
		    argc_argv arg;
		    bool ret;

		    tools_read_from_pipe(*rec.dialog, tools_str2int(optarg), tmp);
		    line_tools_tlv_list2argv(*rec.dialog, tmp, arg);

		    S_I optind_mem = line_tools_reset_getopt(); // save the external variable to use recursivity (see getopt)
                        // reset getopt module

		    ret = get_args_recursive(rec, p, arg.argc(), arg.argv());
		    skip_getopt(argc, argv, optind_mem); // restores getopt after recursion

		    if(!ret)
			return false;
		}
		break;
	    case 'q':
		p.quiet = true;
		break;
	    case '.':
		if(optarg == NULL)
		    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		if(p.user_comment != "")
		    p.user_comment += " ";
		p.user_comment += optarg;
		break;
	    case '3':
		if(optarg == NULL)
		    throw Erange("get_args", tools_printf(gettext("Missing argument to --hash"), char(lu)));
		if(strcasecmp(optarg, "md5") == 0)
		    p.hash = hash_md5;
		else
		    if(strcasecmp(optarg, "sha1") == 0)
			p.hash = hash_sha1;
		    else
			throw Erange("get_args", string(gettext("Unknown parameter given to --hash option: ")) + optarg);
		break;
	    case '9':
		if(optarg == NULL)
		    throw Erange("get_args", tools_printf(gettext("Missing argument to --min-digits"), char(lu)));
		else
		{
		    try
		    {
			line_tools_get_min_digits(optarg, p.num_digits, p.ref_num_digits, p.aux_num_digits);
		    }
		    catch(Erange & e)
		    {
			throw Erange("get_args", string(gettext("Error while parsing --min-digits option: ")) + e.get_message());
		    }
		}
		break;
	    case '=':
		if(optarg == NULL)
		    throw Erange("get_args", tools_printf(gettext("Missing argument to --backup-hook-execute"), char(lu)));
		line_tools_split_at_first_space(optarg, tmp_string, tmp_string2);
		tmp_string = line_tools_get_full_path_from_PATH(rec.dar_duc_path, tmp_string.c_str()) + " " + tmp_string2;
		if(p.backup_hook_execute == "")
		    p.backup_hook_execute = tmp_string;
		else
		    p.backup_hook_execute += string(" ; ") + tmp_string;
		break;
	    case '>':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                tmp_pre_mask.file_listing = false;
                tmp_pre_mask.case_sensit = rec.case_sensit;
                tmp_pre_mask.included = false;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = rec.glob_mode;
                rec.backup_hook_include_exclude.push_back(tmp_pre_mask);
		break;
	    case '<':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                tmp_pre_mask.file_listing = false;
                tmp_pre_mask.case_sensit = rec.case_sensit;
                tmp_pre_mask.included = true;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = rec.glob_mode;
                rec.backup_hook_include_exclude.push_back(tmp_pre_mask);
		break;
	    case '4':
		if(optarg == NULL)
		    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		p.scope = string_to_fsa(optarg);
		if(p.info_details)
		{
		    string list;
		    set<fsa_family>::iterator it = p.scope.begin();
		    while(it != p.scope.end())
		    {
			list += " ";
			list += fsa_family_to_string(*it);
			++it;
		    }
		    rec.dialog->warning(string("FSA family in scope:") + list);
		}
		break;
	    case '5':
		p.exclude_by_ea = true;
		if(optarg != NULL)
		    p.ea_name_for_exclusion = optarg;
		else
		    p.ea_name_for_exclusion = "";
		break;
	    case '6':
		if(optarg != NULL)
		{
		    libdar::deci conv = string(optarg);
		    infinint ks = conv.computer();
		    p.gnupg_key_size = 0;
		    ks.unstack(p.gnupg_key_size);
		    if(ks > 0)
			throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
		}
		else
		    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		break;
	    case '7':
		if(optarg != NULL)
		{
		    if(strlen(optarg) != 0)
			p.signatories = line_tools_split(optarg, ',');
		    else
			throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
		}
		else
		    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		break;
            case ':':
                throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(optopt)));
            case '?':
                rec.dialog->warning(tools_printf(gettext("Ignoring unknown option -%c"),char(optopt)));
                break;
            default:
                rec.dialog->warning(tools_printf(gettext("Ignoring unknown option -%c"),char(lu)));
            }
        }

    return true;
}

static void usage(shell_interaction & dialog, const char *command_name)
{
    string name;
    tools_extract_basename(command_name, name);
    dialog.change_non_interactive_output(&cout);

    dialog.printf(gettext("usage: %s [ -c | -x | -d | -t | -l | -C | -+ ] [<path>/]<basename> [options...]\n"), name.c_str());
    dialog.printf("       %s -h\n", name.c_str());
    dialog.printf("       %s -V\n", name.c_str());
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("Commands are:\n"));
    dialog.printf(gettext("   -c  creates an archive\n"));
    dialog.printf(gettext("   -x  extracts files from the archive\n"));
    dialog.printf(gettext("   -d  compares the archive with the existing filesystem\n"));
    dialog.printf(gettext("   -t  tests the archive integrity\n"));
    dialog.printf(gettext("   -l  lists the contents of the archive\n"));
    dialog.printf(gettext("   -C  isolates the catalogue from an archive\n"));
    dialog.printf(gettext("   -+  merge two archives / create a sub archive\n"));
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("   -h  displays this help information\n"));
    dialog.printf(gettext("   -V  displays version information\n"));
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("Common options:\n"));
    dialog.printf(gettext("   -v\t\t   verbose output\n"));
    dialog.printf(gettext("   -q\t\t   suppress final statistics report\n"));
    dialog.printf(gettext("   -vs\t\t   display skipped files\n"));
    dialog.printf(gettext("   -R <path>\t   filesystem root directory (current dir by default)\n"));
    dialog.printf(gettext("   -X <mask>\t   files to exclude from the operation (none by default)\n"));
    dialog.printf(gettext("   -I <mask>\t   files to include in the operation (all by default)\n"));
    dialog.printf(gettext("   -P <path>\t   subdirectory to exclude from the operation\n"));
    dialog.printf(gettext("   -g <path>\t   subdirectory to include in the operation\n"));
    dialog.printf(gettext("   -[ <filename>   filename contains a list of files to include\n"));
    dialog.printf(gettext("   -] <path>\t   filename contains a list of files to exclude\n"));
    dialog.printf(gettext("   -n\t\t   don't overwrite files\n"));
    dialog.printf(gettext("   -w\t\t   don't warn before overwriting files\n"));
    dialog.printf(gettext("   -wa\t\t   don't warn before overwriting and removing files\n"));
    dialog.printf(gettext("   -b\t\t   ring the terminal bell when user action is required\n"));
    dialog.printf(gettext("   -O[ignore-owner | mtime | inode-type] do not consider user and group\n"));
    dialog.printf(gettext("\t\t   ownership\n"));
    dialog.printf(gettext("   -H [N]\t   ignore shift in dates of an exact number of hours\n"));
    dialog.printf(gettext("   -E <string>\t   command to execute between slices\n"));
    dialog.printf(gettext("   -F <string>\t   same as -E but for the archive of reference\n"));
    dialog.printf(gettext("   -u <mask>\t   mask to ignore certain EA\n"));
    dialog.printf(gettext("   -U <mask>\t   mask to allow certain EA\n"));
    dialog.printf(gettext("   -K <string>\t   use <string> as key to encrypt/decrypt\n"));
    dialog.printf(gettext("   -J <string>\t   same as -K but it does concern the archive of reference\n"));
    dialog.printf(gettext("   -# <integer>    encryption block size\n"));
    dialog.printf(gettext("   -* <integer>    same as -# but for archive of reference\n"));
    dialog.printf(gettext("   -B <filename>   read options from given file\n"));
    dialog.printf(gettext("   -N\t\t   do not read ~/.darrc nor /etc/darrc configuration file\n"));
    dialog.printf(gettext("   -e\t\t   dry run, fake execution, nothing is produced\n"));
    dialog.printf(gettext("   -Q\t\t   suppress the initial warning when not launched from a tty\n"));
    dialog.printf(gettext("   -aa\t\t   do not try to preserve atime of file open for reading.\n"));
    dialog.printf(gettext("   -ac\t\t   do not try to preserve ctime (default behavior).\n"));
    dialog.printf(gettext("   -am\t\t   set ordered mode for all filters\n"));
    dialog.printf(gettext("   -an\t\t   the masks that follow are now case insensitive\n"));
    dialog.printf(gettext("   -acase\t   the masks that follow are now case sensitive\n"));
    dialog.printf(gettext("   -ar\t\t   set the following masks to be regex expressions\n"));
    dialog.printf(gettext("   -ag\t\t   set the following masks to be glob expressions\n"));
    dialog.printf(gettext("   -j\t\t   ask user what to do when memory is exhausted\n"));
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("Saving / Isolation / merging options (to use with -c, -C or -+):\n"));
    dialog.printf(gettext("   -A [path/]<basename> archive to take as reference\n"));
    dialog.printf(gettext("   -@ [path/]<basename> auxiliary archive of reference for merging\n"));
    dialog.printf(gettext("   -$ <string>\t   encryption key for auxiliary archive\n"));
    dialog.printf(gettext("   -~ <string>\t   command between slices of the auxiliary archive\n"));
    dialog.printf(gettext("   -z [[algo:]level]\t compress data in archive. -z = -z9 = -zgzip:9\n"));
    dialog.printf(gettext("      Available algo: gzip,bzip2,lzo. Exemples: -zlzo -zbzip2:5 -z1 -z\n"));
    dialog.printf(gettext("   -s <integer>    split the archive in several files of size <integer>\n"));
    dialog.printf(gettext("   -S <integer>    first file size (if different from following ones)\n"));
    dialog.printf(gettext("   -aSI \t   slice size suffixes k, M, T, G, etc. are power of 10\n"));
    dialog.printf(gettext("   -abinary\t   slice size suffixes k, M, T, G, etc. are power of 2\n"));
    dialog.printf(gettext("   -p\t\t   pauses before writing to a new file\n"));
    dialog.printf(gettext("   -D\t\t   excluded directories are stored as empty directories\n"));
    dialog.printf(gettext("   -Z <mask>\t   do not compress the matching filenames\n"));
    dialog.printf(gettext("   -Y <mask>\t   do only compress the matching filenames\n"));
    dialog.printf(gettext("   -m <number>\t   do not compress file smaller than <number>\n"));
    dialog.printf(gettext("   --nodump\t   do not backup, files having the nodump 'd' flag set\n"));
    dialog.printf(gettext("   -@ [path/]<basename> Do on-fly catalogue isolation of the resulting archive\n"));
    dialog.printf(gettext("   -M\t\t   stay in the same filesystem while scanning directories\n"));
    dialog.printf(gettext("   -,\t\t   ignore directories that follow the Directory Tagging\n"));
    dialog.printf(gettext("\t\t   Standard\n"));
    dialog.printf(gettext("   -/ <string>\t   which way dar can overwrite files at archive merging or\n"));
    dialog.printf(gettext("\t\t   extraction time\n"));
    dialog.printf(gettext("   -^ <string>\t   permission[:user[:group]] of created slices\n"));
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("Restoring options (to use with -x) :\n"));
    dialog.printf(gettext("   -k\t\t   do not remove files destroyed since the reference backup\n"));
    dialog.printf(gettext("   -r\t\t   do not restore file older than those on filesystem\n"));
    dialog.printf(gettext("   -f\t\t   do not restore directory structure\n"));
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("Reading options (to use with -x, -d, -t, -l, -A)\n"));
    dialog.printf(gettext("   -i <named pipe> pipe to use instead of std input to read data from dar_slave\n"));
    dialog.printf(gettext("   -o <named pipe> pipe to use instead of std output to orders dar_slave\n"));
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("Listing options (to use with -l):\n"));
    dialog.printf(gettext("   -T\t\t   tree output format\n"));
    dialog.printf(gettext("   -as\t\t   only list files saved in the archive\n"));
    dialog.printf(gettext("\n\n"));
    dialog.printf(gettext("Type \"man dar\" for more details and for all other available options.\n"));
}

static void show_warranty(shell_interaction & dialog)
{
    dialog.change_non_interactive_output(&cout);
    dialog.printf("                     NO WARRANTY\n");
    dialog.printf("\n");
    dialog.printf("  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n");
    dialog.printf("FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n");
    dialog.printf("OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n");
    dialog.printf("PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n");
    dialog.printf("OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n");
    dialog.printf("MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n");
    dialog.printf("TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n");
    dialog.printf("PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n");
    dialog.printf("REPAIR OR CORRECTION.\n");
    dialog.printf("\n");
    dialog.printf("  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n");
    dialog.printf("WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n");
    dialog.printf("REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n");
    dialog.printf("INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n");
    dialog.printf("OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n");
    dialog.printf("TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n");
    dialog.printf("YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n");
    dialog.printf("PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n");
    dialog.printf("POSSIBILITY OF SUCH DAMAGES.\n");
    dialog.printf("\n");
}

static void show_license(shell_interaction & dialog)
{
    dialog.change_non_interactive_output(&cout);
    dialog.printf("             GNU GENERAL PUBLIC LICENSE\n");
    dialog.printf("                Version 2, June 1991\n");
    dialog.printf("\n");
    dialog.printf(" Copyright (C) 1989, 1991 Free Software Foundation, Inc.\n");
    dialog.printf("                       59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n");
    dialog.printf(" Everyone is permitted to copy and distribute verbatim copies\n");
    dialog.printf(" of this license document, but changing it is not allowed.\n");
    dialog.printf("\n");
    dialog.printf("                     Preamble\n");
    dialog.printf("\n");
    dialog.printf("  The licenses for most software are designed to take away your\n");
    dialog.printf("freedom to share and change it.  By contrast, the GNU General Public\n");
    dialog.printf("License is intended to guarantee your freedom to share and change free\n");
    dialog.printf("software--to make sure the software is free for all its users.  This\n");
    dialog.printf("General Public License applies to most of the Free Software\n");
    dialog.printf("Foundation's software and to any other program whose authors commit to\n");
    dialog.printf("using it.  (Some other Free Software Foundation software is covered by\n");
    dialog.printf("the GNU Library General Public License instead.)  You can apply it to\n");
    dialog.printf("your programs, too.\n");
    dialog.printf("\n");
    dialog.printf("  When we speak of free software, we are referring to freedom, not\n");
    dialog.printf("price.  Our General Public Licenses are designed to make sure that you\n");
    dialog.printf("have the freedom to distribute copies of free software (and charge for\n");
    dialog.printf("this service if you wish), that you receive source code or can get it\n");
    dialog.printf("if you want it, that you can change the software or use pieces of it\n");
    dialog.printf("in new free programs; and that you know you can do these things.\n");
    dialog.printf("\n");
    dialog.printf("  To protect your rights, we need to make restrictions that forbid\n");
    dialog.printf("anyone to deny you these rights or to ask you to surrender the rights.\n");
    dialog.printf("These restrictions translate to certain responsibilities for you if you\n");
    dialog.printf("distribute copies of the software, or if you modify it.\n");
    dialog.printf("\n");
    dialog.printf("  For example, if you distribute copies of such a program, whether\n");
    dialog.printf("gratis or for a fee, you must give the recipients all the rights that\n");
    dialog.printf("you have.  You must make sure that they, too, receive or can get the\n");
    dialog.printf("source code.  And you must show them these terms so they know their\n");
    dialog.printf("rights.\n");
    dialog.printf("\n");
    dialog.printf("  We protect your rights with two steps: (1) copyright the software, and\n");
    dialog.printf("(2) offer you this license which gives you legal permission to copy,\n");
    dialog.printf("distribute and/or modify the software.\n");
    dialog.printf("\n");
    dialog.printf("  Also, for each author's protection and ours, we want to make certain\n");
    dialog.printf("that everyone understands that there is no warranty for this free\n");
    dialog.printf("software.  If the software is modified by someone else and passed on, we\n");
    dialog.printf("want its recipients to know that what they have is not the original, so\n");
    dialog.printf("that any problems introduced by others will not reflect on the original\n");
    dialog.printf("authors' reputations.\n");
    dialog.printf("\n");
    dialog.printf("  Finally, any free program is threatened constantly by software\n");
    dialog.printf("patents.  We wish to avoid the danger that redistributors of a free\n");
    dialog.printf("program will individually obtain patent licenses, in effect making the\n");
    dialog.printf("program proprietary.  To prevent this, we have made it clear that any\n");
    dialog.printf("patent must be licensed for everyone's free use or not licensed at all.\n");
    dialog.printf("\n");
    dialog.printf("  The precise terms and conditions for copying, distribution and\n");
    dialog.printf("modification follow.\n");
    dialog.printf("\n");
    dialog.printf("             GNU GENERAL PUBLIC LICENSE\n");
    dialog.printf("   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION\n");
    dialog.printf("\n");
    dialog.printf("  0. This License applies to any program or other work which contains\n");
    dialog.printf("a notice placed by the copyright holder saying it may be distributed\n");
    dialog.printf("under the terms of this General Public License.  The \"Program\", below,\n");
    dialog.printf("refers to any such program or work, and a \"work based on the Program\"\n");
    dialog.printf("means either the Program or any derivative work under copyright law:\n");
    dialog.printf("that is to say, a work containing the Program or a portion of it,\n");
    dialog.printf("either verbatim or with modifications and/or translated into another\n");
    dialog.printf("language.  (Hereinafter, translation is included without limitation in\n");
    dialog.printf("the term \"modification\".)  Each licensee is addressed as \"you\".\n");
    dialog.printf("\n");
    dialog.printf("Activities other than copying, distribution and modification are not\n");
    dialog.printf("covered by this License; they are outside its scope.  The act of\n");
    dialog.printf("running the Program is not restricted, and the output from the Program\n");
    dialog.printf("is covered only if its contents constitute a work based on the\n");
    dialog.printf("Program (independent of having been made by running the Program).\n");
    dialog.printf("Whether that is true depends on what the Program does.\n");
    dialog.printf("\n");
    dialog.printf("  1. You may copy and distribute verbatim copies of the Program's\n");
    dialog.printf("source code as you receive it, in any medium, provided that you\n");
    dialog.printf("conspicuously and appropriately publish on each copy an appropriate\n");
    dialog.printf("copyright notice and disclaimer of warranty; keep intact all the\n");
    dialog.printf("notices that refer to this License and to the absence of any warranty;\n");
    dialog.printf("and give any other recipients of the Program a copy of this License\n");
    dialog.printf("along with the Program.\n");
    dialog.printf("\n");
    dialog.printf("You may charge a fee for the physical act of transferring a copy, and\n");
    dialog.printf("you may at your option offer warranty protection in exchange for a fee.\n");
    dialog.printf("\n");
    dialog.printf("  2. You may modify your copy or copies of the Program or any portion\n");
    dialog.printf("of it, thus forming a work based on the Program, and copy and\n");
    dialog.printf("distribute such modifications or work under the terms of Section 1\n");
    dialog.printf("above, provided that you also meet all of these conditions:\n");
    dialog.printf("\n");
    dialog.printf("    a) You must cause the modified files to carry prominent notices\n");
    dialog.printf("    stating that you changed the files and the date of any change.\n");
    dialog.printf("\n");
    dialog.printf("    b) You must cause any work that you distribute or publish, that in\n");
    dialog.printf("    whole or in part contains or is derived from the Program or any\n");
    dialog.printf("    part thereof, to be licensed as a whole at no charge to all third\n");
    dialog.printf("    parties under the terms of this License.\n");
    dialog.printf("\n");
    dialog.printf("    c) If the modified program normally reads commands interactively\n");
    dialog.printf("    when run, you must cause it, when started running for such\n");
    dialog.printf("    interactive use in the most ordinary way, to print or display an\n");
    dialog.printf("    announcement including an appropriate copyright notice and a\n");
    dialog.printf("    notice that there is no warranty (or else, saying that you provide\n");
    dialog.printf("    a warranty) and that users may redistribute the program under\n");
    dialog.printf("    these conditions, and telling the user how to view a copy of this\n");
    dialog.printf("    License.  (Exception: if the Program itself is interactive but\n");
    dialog.printf("    does not normally print such an announcement, your work based on\n");
    dialog.printf("    the Program is not required to print an announcement.)\n");
    dialog.printf("\n");
    dialog.printf("These requirements apply to the modified work as a whole.  If\n");
    dialog.printf("identifiable sections of that work are not derived from the Program,\n");
    dialog.printf("and can be reasonably considered independent and separate works in\n");
    dialog.printf("themselves, then this License, and its terms, do not apply to those\n");
    dialog.printf("sections when you distribute them as separate works.  But when you\n");
    dialog.printf("distribute the same sections as part of a whole which is a work based\n");
    dialog.printf("on the Program, the distribution of the whole must be on the terms of\n");
    dialog.printf("this License, whose permissions for other licensees extend to the\n");
    dialog.printf("entire whole, and thus to each and every part regardless of who wrote it.\n");
    dialog.printf("\n");
    dialog.printf("Thus, it is not the intent of this section to claim rights or contest\n");
    dialog.printf("your rights to work written entirely by you; rather, the intent is to\n");
    dialog.printf("exercise the right to control the distribution of derivative or\n");
    dialog.printf("collective works based on the Program.\n");
    dialog.printf("\n");
    dialog.printf("In addition, mere aggregation of another work not based on the Program\n");
    dialog.printf("with the Program (or with a work based on the Program) on a volume of\n");
    dialog.printf("a storage or distribution medium does not bring the other work under\n");
    dialog.printf("the scope of this License.\n");
    dialog.printf("\n");
    dialog.printf("  3. You may copy and distribute the Program (or a work based on it,\n");
    dialog.printf("under Section 2) in object code or executable form under the terms of\n");
    dialog.printf("Sections 1 and 2 above provided that you also do one of the following:\n");
    dialog.printf("\n");
    dialog.printf("    a) Accompany it with the complete corresponding machine-readable\n");
    dialog.printf("    source code, which must be distributed under the terms of Sections\n");
    dialog.printf("    1 and 2 above on a medium customarily used for software interchange; or,\n");
    dialog.printf("\n");
    dialog.printf("    b) Accompany it with a written offer, valid for at least three\n");
    dialog.printf("    years, to give any third party, for a charge no more than your\n");
    dialog.printf("    cost of physically performing source distribution, a complete\n");
    dialog.printf("    machine-readable copy of the corresponding source code, to be\n");
    dialog.printf("    distributed under the terms of Sections 1 and 2 above on a medium\n");
    dialog.printf("    customarily used for software interchange; or,\n");
    dialog.printf("\n");
    dialog.printf("    c) Accompany it with the information you received as to the offer\n");
    dialog.printf("    to distribute corresponding source code.  (This alternative is\n");
    dialog.printf("    allowed only for noncommercial distribution and only if you\n");
    dialog.printf("    received the program in object code or executable form with such\n");
    dialog.printf("    an offer, in accord with Subsection b above.)\n");
    dialog.printf("\n");
    dialog.printf("The source code for a work means the preferred form of the work for\n");
    dialog.printf("making modifications to it.  For an executable work, complete source\n");
    dialog.printf("code means all the source code for all modules it contains, plus any\n");
    dialog.printf("associated interface definition files, plus the scripts used to\n");
    dialog.printf("control compilation and installation of the executable.  However, as a\n");
    dialog.printf("special exception, the source code distributed need not include\n");
    dialog.printf("anything that is normally distributed (in either source or binary\n");
    dialog.printf("form) with the major components (compiler, kernel, and so on) of the\n");
    dialog.printf("operating system on which the executable runs, unless that component\n");
    dialog.printf("itself accompanies the executable.\n");
    dialog.printf("\n");
    dialog.printf("If distribution of executable or object code is made by offering\n");
    dialog.printf("access to copy from a designated place, then offering equivalent\n");
    dialog.printf("access to copy the source code from the same place counts as\n");
    dialog.printf("distribution of the source code, even though third parties are not\n");
    dialog.printf("compelled to copy the source along with the object code.\n");
    dialog.printf("\n");
    dialog.printf("  4. You may not copy, modify, sublicense, or distribute the Program\n");
    dialog.printf("except as expressly provided under this License.  Any attempt\n");
    dialog.printf("otherwise to copy, modify, sublicense or distribute the Program is\n");
    dialog.printf("void, and will automatically terminate your rights under this License.\n");
    dialog.printf("However, parties who have received copies, or rights, from you under\n");
    dialog.printf("this License will not have their licenses terminated so long as such\n");
    dialog.printf("parties remain in full compliance.\n");
    dialog.printf("\n");
    dialog.printf("  5. You are not required to accept this License, since you have not\n");
    dialog.printf("signed it.  However, nothing else grants you permission to modify or\n");
    dialog.printf("distribute the Program or its derivative works.  These actions are\n");
    dialog.printf("prohibited by law if you do not accept this License.  Therefore, by\n");
    dialog.printf("modifying or distributing the Program (or any work based on the\n");
    dialog.printf("Program), you indicate your acceptance of this License to do so, and\n");
    dialog.printf("all its terms and conditions for copying, distributing or modifying\n");
    dialog.printf("the Program or works based on it.\n");
    dialog.printf("\n");
    dialog.printf("  6. Each time you redistribute the Program (or any work based on the\n");
    dialog.printf("Program), the recipient automatically receives a license from the\n");
    dialog.printf("original licensor to copy, distribute or modify the Program subject to\n");
    dialog.printf("these terms and conditions.  You may not impose any further\n");
    dialog.printf("restrictions on the recipients' exercise of the rights granted herein.\n");
    dialog.printf("You are not responsible for enforcing compliance by third parties to\n");
    dialog.printf("this License.\n");
    dialog.printf("\n");
    dialog.printf("  7. If, as a consequence of a court judgment or allegation of patent\n");
    dialog.printf("infringement or for any other reason (not limited to patent issues),\n");
    dialog.printf("conditions are imposed on you (whether by court order, agreement or\n");
    dialog.printf("otherwise) that contradict the conditions of this License, they do not\n");
    dialog.printf("excuse you from the conditions of this License.  If you cannot\n");
    dialog.printf("distribute so as to satisfy simultaneously your obligations under this\n");
    dialog.printf("License and any other pertinent obligations, then as a consequence you\n");
    dialog.printf("may not distribute the Program at all.  For example, if a patent\n");
    dialog.printf("license would not permit royalty-free redistribution of the Program by\n");
    dialog.printf("all those who receive copies directly or indirectly through you, then\n");
    dialog.printf("the only way you could satisfy both it and this License would be to\n");
    dialog.printf("refrain entirely from distribution of the Program.\n");
    dialog.printf("\n");
    dialog.printf("If any portion of this section is held invalid or unenforceable under\n");
    dialog.printf("any particular circumstance, the balance of the section is intended to\n");
    dialog.printf("apply and the section as a whole is intended to apply in other\n");
    dialog.printf("circumstances.\n");
    dialog.printf("\n");
    dialog.printf("It is not the purpose of this section to induce you to infringe any\n");
    dialog.printf("patents or other property right claims or to contest validity of any\n");
    dialog.printf("such claims; this section has the sole purpose of protecting the\n");
    dialog.printf("integrity of the free software distribution system, which is\n");
    dialog.printf("implemented by public license practices.  Many people have made\n");
    dialog.printf("generous contributions to the wide range of software distributed\n");
    dialog.printf("through that system in reliance on consistent application of that\n");
    dialog.printf("system; it is up to the author/donor to decide if he or she is willing\n");
    dialog.printf("to distribute software through any other system and a licensee cannot\n");
    dialog.printf("impose that choice.\n");
    dialog.printf("\n");
    dialog.printf("This section is intended to make thoroughly clear what is believed to\n");
    dialog.printf("be a consequence of the rest of this License.\n");
    dialog.printf("\n");
    dialog.printf("  8. If the distribution and/or use of the Program is restricted in\n");
    dialog.printf("certain countries either by patents or by copyrighted interfaces, the\n");
    dialog.printf("original copyright holder who places the Program under this License\n");
    dialog.printf("may add an explicit geographical distribution limitation excluding\n");
    dialog.printf("those countries, so that distribution is permitted only in or among\n");
    dialog.printf("countries not thus excluded.  In such case, this License incorporates\n");
    dialog.printf("the limitation as if written in the body of this License.\n");
    dialog.printf("\n");
    dialog.printf("  9. The Free Software Foundation may publish revised and/or new versions\n");
    dialog.printf("of the General Public License from time to time.  Such new versions will\n");
    dialog.printf("be similar in spirit to the present version, but may differ in detail to\n");
    dialog.printf("address new problems or concerns.\n");
    dialog.printf("\n");
    dialog.printf("Each version is given a distinguishing version number.  If the Program\n");
    dialog.printf("specifies a version number of this License which applies to it and \"any\n");
    dialog.printf("later version\", you have the option of following the terms and conditions\n");
    dialog.printf("either of that version or of any later version published by the Free\n");
    dialog.printf("Software Foundation.  If the Program does not specify a version number of\n");
    dialog.printf("this License, you may choose any version ever published by the Free Software\n");
    dialog.printf("Foundation.\n");
    dialog.printf("\n");
    dialog.printf("  10. If you wish to incorporate parts of the Program into other free\n");
    dialog.printf("programs whose distribution conditions are different, write to the author\n");
    dialog.printf("to ask for permission.  For software which is copyrighted by the Free\n");
    dialog.printf("Software Foundation, write to the Free Software Foundation; we sometimes\n");
    dialog.printf("make exceptions for this.  Our decision will be guided by the two goals\n");
    dialog.printf("of preserving the free status of all derivatives of our free software and\n");
    dialog.printf("of promoting the sharing and reuse of software generally.\n");
    dialog.printf("\n");
    show_warranty(dialog);
    dialog.printf("              END OF TERMS AND CONDITIONS\n");
    dialog.printf("\n");
    dialog.printf("     How to Apply These Terms to Your New Programs\n");
    dialog.printf("\n");
    dialog.printf("  If you develop a new program, and you want it to be of the greatest\n");
    dialog.printf("possible use to the public, the best way to achieve this is to make it\n");
    dialog.printf("free software which everyone can redistribute and change under these terms.\n");
    dialog.printf("\n");
    dialog.printf("  To do so, attach the following notices to the program.  It is safest\n");
    dialog.printf("to attach them to the start of each source file to most effectively\n");
    dialog.printf("convey the exclusion of warranty; and each file should have at least\n");
    dialog.printf("the \"copyright\" line and a pointer to where the full notice is found.\n");
    dialog.printf("\n");
    dialog.printf("    <one line to give the program's name and a brief idea of what it does.>\n");
    dialog.printf("    Copyright (C) <year>  <name of author>\n");
    dialog.printf("\n");
    dialog.printf("    This program is free software; you can redistribute it and/or modify\n");
    dialog.printf("    it under the terms of the GNU General Public License as published by\n");
    dialog.printf("    the Free Software Foundation; either version 2 of the License, or\n");
    dialog.printf("    (at your option) any later version.\n");
    dialog.printf("\n");
    dialog.printf("    This program is distributed in the hope that it will be useful,\n");
    dialog.printf("    but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    dialog.printf("    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    dialog.printf("    GNU General Public License for more details.\n");
    dialog.printf("\n");
    dialog.printf("    You should have received a copy of the GNU General Public License\n");
    dialog.printf("    along with this program; if not, write to the Free Software\n");
    dialog.printf("    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n");
    dialog.printf("\n");
    dialog.printf("\n");
    dialog.printf("Also add information on how to contact you by electronic and paper mail.\n");
    dialog.printf("\n");
    dialog.printf("If the program is interactive, make it output a short notice like this\n");
    dialog.printf("when it starts in an interactive mode:\n");
    dialog.printf("\n");
    dialog.printf("    Gnomovision version 69, Copyright (C) year name of author\n");
    dialog.printf("    Gnomovision comes with ABSOLUTELY NO WARRANTY; for details type `show w'.\n");
    dialog.printf("    This is free software, and you are welcome to redistribute it\n");
    dialog.printf("    under certain conditions; type `show c' for details.\n");
    dialog.printf("\n");
    dialog.printf("The hypothetical commands `show w' and `show c' should show the appropriate\n");
    dialog.printf("parts of the General Public License.  Of course, the commands you use may\n");
    dialog.printf("be called something other than `show w' and `show c'; they could even be\n");
    dialog.printf("mouse-clicks or menu items--whatever suits your program.\n");
    dialog.printf("\n");
    dialog.printf("You should also get your employer (if you work as a programmer) or your\n");
    dialog.printf("school, if any, to sign a \"copyright disclaimer\" for the program, if\n");
    dialog.printf("necessary.  Here is a sample; alter the names:\n");
    dialog.printf("\n");
    dialog.printf("  Yoyodyne, Inc., hereby disclaims all copyright interest in the program\n");
    dialog.printf("  `Gnomovision' (which makes passes at compilers) written by James Hacker.\n");
    dialog.printf("\n");
    dialog.printf("  <signature of Ty Coon>, 1 April 1989\n");
    dialog.printf("  Ty Coon, President of Vice\n");
    dialog.printf("\n");
    dialog.printf("This General Public License does not permit incorporating your program into\n");
    dialog.printf("proprietary programs.  If your program is a subroutine library, you may\n");
    dialog.printf("consider it more useful to permit linking proprietary applications with the\n");
    dialog.printf("library.  If this is what you want to do, use the GNU Library General\n");
    dialog.printf("Public License instead of this License.\n");
    dialog.printf("\n");
}

static void show_version(shell_interaction & dialog, const char *command_name)
{
    string name;
    tools_extract_basename(command_name, name);
    U_I maj, med, min;

    get_version(maj, med, min);
    dialog.change_non_interactive_output(&cout);
    dialog.warning(tools_printf("\n %s version %s, Copyright (C) 2002-2052 Denis Corbin\n",  name.c_str(), ::dar_version())
		   + "   " + dar_suite_command_line_features()
		   + "\n"
		   + (maj > 2 ? tools_printf(gettext(" Using libdar %u.%u.%u built with compilation time options:"), maj, med, min)
		      : tools_printf(gettext(" Using libdar %u.%u built with compilation time options:"), maj, min)));
    tools_display_features(dialog);
    dialog.printf("\n");
    dialog.warning(tools_printf(gettext(" compiled the %s with %s version %s\n"), __DATE__, CC_NAT, __VERSION__)
		   + tools_printf(gettext(" %s is part of the Disk ARchive suite (Release %s)\n"), name.c_str(), PACKAGE_VERSION)
		   + tools_printf(gettext(" %s comes with ABSOLUTELY NO WARRANTY; for details\n type `%s -W'."), name.c_str(), name.c_str())
		   + tools_printf(gettext(" This is free software, and you are welcome\n to redistribute it under certain conditions;"))
		   + tools_printf(gettext(" type `%s -L | more'\n for details.\n\n"), name.c_str()));
}

#if HAVE_GETOPT_LONG
const struct option *get_long_opt()
{
    static const struct option ret[] = {
        {"beep", no_argument, NULL, 'b' },
        {"create", required_argument, NULL, 'c'},
        {"diff", required_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {"input", required_argument, NULL, 'i'},
        {"deleted", optional_argument, NULL, 'k'},
        {"no-delete", no_argument, NULL, 'k'},     // backward compatiblity
        {"list", required_argument, NULL, 'l'},
        {"no-overwrite", no_argument, NULL, 'n'},
        {"output", required_argument, NULL, 'o'},
        {"pause", optional_argument, NULL, 'p'},
        {"recent", no_argument, NULL, 'r'},
        {"slice", required_argument, NULL, 's'},
        {"test", required_argument, NULL, 't'},
        {"exclude-ea", required_argument, NULL, 'u'},
        {"verbose", optional_argument, NULL, 'v'},
        {"no-warn", optional_argument, NULL, 'w'},
        {"extract", required_argument, NULL, 'x'},
        {"bzip2", optional_argument, NULL, 'y'},  // backward compatibility
        {"gzip", optional_argument, NULL, 'z'},   // backward compatibility
	{"compression", required_argument, NULL, 'z'},
        {"ref", required_argument, NULL, 'A'},
        {"isolate", required_argument, NULL, 'C'},
        {"empty-dir", no_argument, NULL, 'D'},
        {"include", required_argument, NULL, 'I'},
        {"prune", required_argument, NULL, 'P'},
        {"fs-root", required_argument, NULL, 'R'},
        {"first-slice", required_argument, NULL, 'S'},
        {"include-ea", required_argument, NULL, 'U'},
        {"version", no_argument, NULL, 'V'},
        {"exclude", required_argument, NULL, 'X'},
        {"ignore-owner", no_argument, NULL, 'O'},
	{"comparison-field", optional_argument, NULL, 'O'},
        {"tree-format", no_argument, NULL, 'T'},
        {"list-format", required_argument, NULL, 'T'},
        {"execute", required_argument, NULL, 'E'},
        {"execute-ref",required_argument, NULL, 'F'},
        {"ref-execute",required_argument, NULL, 'F'},
        {"key", required_argument, NULL, 'K'},
        {"key-ref", required_argument, NULL, 'J'},
        {"ref-key", required_argument, NULL, 'J'},
        {"include-compression", required_argument, NULL, 'Y'},
        {"exclude-compression", required_argument, NULL, 'Z'},
        {"batch", required_argument, NULL, 'B'},
        {"flat", no_argument, NULL, 'f'},
        {"mincompr", required_argument, NULL, 'm'},
        {"noconf", no_argument, NULL, 'N'},
        {"nodump", no_argument, NULL, ' '},
        {"hour", optional_argument, NULL, 'H'},
        {"alter", optional_argument, NULL, 'a'},
        {"empty", no_argument, NULL, 'e'},
        {"dry-run", no_argument, NULL, 'e'},
        {"on-fly-isolate", required_argument, NULL, '@'},
        {"no-mount-points", no_argument, NULL, 'M'},
        {"go-into", required_argument, NULL, 'g'},
        {"jog", no_argument, NULL, 'j'},
        {"crypto-block", required_argument, NULL, '#'},
        {"ref-crypto-block", required_argument, NULL, '*'},
        {"crypto-block-ref", required_argument, NULL, '*'},
        {"cache-directory-tagging", no_argument, NULL, ','},
        {"include-from-file", required_argument, NULL, '['},
        {"exclude-from-file", required_argument, NULL, ']'},
	{"merge", required_argument, NULL, '+'},
	{"aux", required_argument, NULL, '@'},
	{"aux-ref", required_argument, NULL, '@'},
	{"aux-key", required_argument, NULL, '$'},
	{"aux-execute", required_argument, NULL, '~'},
	{"aux-crypto-block", required_argument, NULL, '%'},
	{"quiet", no_argument, NULL, 'q'},
	{"overwriting-policy", required_argument, NULL, '/' },
	{"slice-mode", required_argument, NULL, '^' },
	{"retry-on-change", required_argument, NULL, '_' },
	{"pipe-fd", required_argument, NULL, '"' },
	{"sequential-read", no_argument, NULL, '0'},
	{"sparse-file-min-size", required_argument, NULL, '1'},
	{"dirty-behavior", required_argument, NULL, '2'},
	{"user-comment", required_argument, NULL, '.'},
	{"hash", required_argument, NULL, '3'},
	{"min-digits", required_argument, NULL, '9'},
	{"backup-hook-include", required_argument, NULL, '<'},
	{"backup-hook-exclude", required_argument, NULL, '>'},
	{"backup-hook-execute", required_argument, NULL, '='},
	{"fsa-scope", required_argument, NULL, '4'},
	{"exclude-by-ea", optional_argument, NULL, '5'},
	{"key-length", required_argument, NULL, '6' },
	{"sign", required_argument, NULL, '7' },
        { NULL, 0, NULL, 0 }
    };

    return ret;
}
#endif

static void make_args_from_file(user_interaction & dialog, operation op, const vector<string> & targets, const string & filename, S_I & argc, char **&argv, bool info_details)
{
    vector <string> cibles;
    argv = NULL;
    argc = 0;

    fichier_local conf = fichier_local(filename, false); // the object conf will close fd

        ////////
        // removing the comments from file
        //
    no_comment sousconf = no_comment(conf);

        ////////
        // defining the conditional syntax targets
        // that will be considered in the file
    cibles = targets;
    cibles.push_back("all");
    switch(op)
    {
    case noop:
        cibles.push_back("default");
        break;
    case create:
        cibles.push_back("create");
        break;
    case extract:
        cibles.push_back("extract");
        break;
    case diff:
        cibles.push_back("diff");
        break;
    case test:
        cibles.push_back("test");
        break;
    case listing:
        cibles.push_back("listing");
        cibles.push_back("list");
        break;
    case merging:
	cibles.push_back("merge");
	break;
    case isolate:
        cibles.push_back("isolate");
        break;
    default:
        throw SRC_BUG;
    }


        //////
        //  hide the targets we don't want to see
        //
    config_file surconf = config_file(cibles, sousconf);

        //////
        //  now we have surconf -> sousconf -> conf -> fd
        //  which makes the job to remove comments
        //  and hide unwanted conditional statements on the fly
        //  surconf can be used as a normal file.
        //

    const char *command = "dar";
    char *pseudo_command = NULL;

    try
    {
	vector <string> mots;


            // now parsing the file and cutting words
            // taking care of quotes
            //
	mots = tools_split_in_words(surconf);


	    // now converting the mots of type vector<string> to argc/argv arguments
	    //
        argc = mots.size()+1;
	if(argc < 0)
	    throw SRC_BUG; // integer overflow occurred
        argv = new (nothrow) char *[argc];
        if(argv == NULL)
            throw Ememory("make_args_from_file");
	for(S_I i = 0; i < argc; ++i)
	    argv[i] = NULL;

            // adding a fake "dar" word as first argument (= argv[0])
            //
	char *pseudo_command = new (nothrow) char[strlen(command)+1];
	if(pseudo_command == NULL)
	    throw Ememory("make_args_from_file");
        strncpy(pseudo_command, command, strlen(command));
        pseudo_command[strlen(command)] = '\0';
	argv[0] = pseudo_command;
	pseudo_command = NULL;

        if(info_details)
            dialog.printf(gettext("Arguments read from %S :"), &filename);
        for(U_I i = 0; i < mots.size(); ++i)
        {
            argv[i+1] = tools_str2charptr(mots[i]); // mots[i] goes to argv[i+1] !
            if(info_details)
                dialog.printf(" \"%s\"", argv[i+1]);
        }
        if(info_details)
            dialog.printf("\n");
    }
    catch(...)
    {
        if(argv != NULL)
        {
	    for(S_I i = 0; i < argc; ++i)
		if(argv[i] != NULL)
		    delete[] argv[i];
	    delete[] argv;
	    argv = NULL;
        }
	argc = 0;
	if(pseudo_command != NULL)
	{
	    delete[] pseudo_command;
	    pseudo_command = NULL;
	}
        throw;
    }
}


static void destroy(S_I argc, char **argv)
{
    register S_I i = 0;
    for(i = 0; i < argc; ++i)
        delete [] argv[i];
    delete [] argv;
}

static void skip_getopt(S_I argc, char * const argv[], S_I next_to_read)
{
    (void)line_tools_reset_getopt();
#if HAVE_GETOPT_LONG
    while(getopt_long(argc, argv, OPT_STRING, get_long_opt(), NULL) != EOF && optind < next_to_read)
        ;
#else
    while(getopt(argc, argv, OPT_STRING) != EOF && optind < next_to_read)
        ;
#endif
}

#ifdef DEBOGGAGE
static void show_args(S_I argc, char *argv[])
{
    register S_I i;
    for(i = 0; i < argc; ++i)
        dialog.printf("[%s]\n", argv[i]);
}
#endif


static bool update_with_config_files(recursive_param & rec, line_param & p)
{
    string buffer;
    enum { syntax, ok, unknown } retour = unknown;
    S_I rec_c = 0;
    char **rec_v = NULL;

    (void)line_tools_reset_getopt();

	// trying to open $HOME/.darrc

    buffer = string(rec.home) + "/.darrc";

    try
    {
	make_args_from_file(*rec.dialog, p.op, rec.non_options, buffer, rec_c, rec_v, p.info_details);

	try
	{
	    try
	    {
		if(! get_args_recursive(rec, p, rec_c, rec_v))
		    retour = syntax;
		else
		    retour = ok;
	    }
	    catch(Erange & e)
	    {
		Erange more = Erange(e.get_source(), tools_printf(gettext("In included file %S: "), &buffer) + e.get_message());
		throw more;
	    }
	}
	catch(...)
	{
	    if(rec_v != NULL)
	    {
		destroy(rec_c, rec_v);
		rec_v = NULL;
		rec_c = 0;
	    }
	    throw;
	}
	if(rec_v != NULL)
	{
	    destroy(rec_c, rec_v);
	    rec_v = NULL;
	    rec_c = 0;
	}
    }
    catch(Erange & e)
    {
	if(e.get_source() != "make_args_from_file")
	    throw;
	    // else:
	    // failed openning the file,
	    // nothing to do,
	    // we will try the other config file
	    // below
    }

    rec_c = 0;
    rec_v = NULL;

    if(retour == unknown)
    {
	    // trying to open DAR_SYS_DIR/darrc

	buffer = string(DAR_SYS_DIR) + "/darrc";

	try
	{

	    make_args_from_file(*rec.dialog, p.op, rec.non_options, buffer, rec_c, rec_v, p.info_details);

	    try
	    {
		(void)line_tools_reset_getopt(); // reset getopt call

		try
		{
		    if(! get_args_recursive(rec, p, rec_c, rec_v))
			retour = syntax;
		    else
			retour = ok;
		}
		catch(Erange & e)
		{
		    Erange more = Erange(e.get_source(), tools_printf(gettext("In included file %S: "), &buffer) + e.get_message());
		    throw more;
		}
	    }
	    catch(...)
	    {
		if(rec_v != NULL)
		{
		    destroy(rec_c, rec_v);
		    rec_v = NULL;
		    rec_c = 0;
		}
		throw;
	    }

	    if(rec_v != NULL)
	    {
		destroy(rec_c, rec_v);
		rec_v = NULL;
		rec_c = 0;
	    }
	}
	catch(Erange & e)
	{
	    if(e.get_source() != "make_args_from_file")
		throw;
		// else:
		// failed openning the file,
		// nothing to do,
	}
    }

    return retour != syntax;
}


static mask *make_include_exclude_name(const string & x, mask_opt opt)
{
    mask *ret = NULL;

    if(opt.glob_exp)
	ret = new (nothrow) simple_mask(x, opt.case_sensit);
    else
	ret = new (nothrow) regular_mask(x, opt.case_sensit);

    if(ret == NULL)
        throw Ememory("make_include_exclude_name");
    else
        return ret;
}

static mask *make_exclude_path_ordered(const string & x, mask_opt opt)
{
    mask *ret = NULL;
    if(opt.file_listing)
    {
        ret = new (nothrow) mask_list(x, opt.case_sensit, opt.prefix, false);
	if(ret == NULL)
	    throw Ememory("make_exclude_path");
    }
    else // not file listing mask
    {
	if(opt.glob_exp)
	{
	    ou_mask *val = new (nothrow) ou_mask();

	    if(val == NULL)
		throw Ememory("make_exclude_path");

	    val->add_mask(simple_mask((opt.prefix + x).display(), opt.case_sensit));
	    val->add_mask(simple_mask((opt.prefix + x).display() + "/*", opt.case_sensit));
	    ret = val;
	}
	else // regex
	{
  	    ret = new (nothrow) regular_mask(tools_build_regex_for_exclude_mask(opt.prefix.display(), x), opt.case_sensit);

	    if(ret == NULL)
		throw Ememory("make_exclude_path");
	}
    }

    return ret;
}


static mask *make_exclude_path_unordered(const string & x, mask_opt opt)
{
    mask *ret = NULL;

    if(opt.file_listing)
        ret = new (nothrow) mask_list(x, opt.case_sensit, opt.prefix, false);
    else
	if(opt.glob_exp)
	    ret = new (nothrow) simple_mask((opt.prefix + x).display(), opt.case_sensit);
	else
	    ret = new (nothrow) regular_mask(tools_build_regex_for_exclude_mask(opt.prefix.display(), x), opt.case_sensit);
    if(ret == NULL)
        throw Ememory("make_exclude_path");

    return ret;
}

static mask *make_include_path(const string & x, mask_opt opt)
{
    mask *ret = NULL;

    if(opt.file_listing)
        ret = new (nothrow) mask_list(x, opt.case_sensit, opt.prefix, true);
    else
        ret = new (nothrow) simple_path_mask(opt.prefix +x, opt.case_sensit);
    if(ret == NULL)
        throw Ememory("make_include_path");

    return ret;
}

static mask *make_ordered_mask(deque<pre_mask> & listing, mask *(*make_include_mask) (const string & x, mask_opt opt), mask *(*make_exclude_mask)(const string & x, mask_opt opt), const path & prefix)
{
    mask *ret_mask = NULL;
    ou_mask *tmp_ou_mask = NULL;
    et_mask *tmp_et_mask = NULL;
    mask *tmp_mask = NULL;
    mask_opt opt = prefix;

    try
    {
        while(!listing.empty())
        {
            opt.read_from(listing.front());
            if(listing.front().included)
                if(ret_mask == NULL) // first mask
                {
		    ret_mask = (*make_include_mask)(listing.front().mask, opt);
		    if(ret_mask == NULL)
			throw Ememory("make_ordered_mask");
		}
		else // ret_mask != NULL (need to chain to existing masks)
		{
		    if(tmp_ou_mask != NULL)
		    {
  		        tmp_mask = (*make_include_mask)(listing.front().mask, opt);
			tmp_ou_mask->add_mask(*tmp_mask);
			delete tmp_mask;
			tmp_mask = NULL;
		    }
		    else  // need to create ou_mask
		    {
  		        tmp_mask = (*make_include_mask)(listing.front().mask, opt);
			tmp_ou_mask = new (nothrow) ou_mask();
			if(tmp_ou_mask == NULL)
			    throw Ememory("make_ordered_mask");
			tmp_ou_mask->add_mask(*ret_mask);
			tmp_ou_mask->add_mask(*tmp_mask);
			delete tmp_mask;
			tmp_mask = NULL;
			delete ret_mask;
			ret_mask = tmp_ou_mask;
			tmp_et_mask = NULL;
		    }
		}
	    else // exclude mask
		if(ret_mask == NULL)
		{
  		    tmp_mask = (*make_exclude_mask)(listing.front().mask, opt);
		    ret_mask = new (nothrow) not_mask(*tmp_mask);
		    if(ret_mask == NULL)
			throw Ememory("make_ordered_mask");
		    delete tmp_mask;
		    tmp_mask = NULL;
		}
		else // ret_mask != NULL
		{
		    if(tmp_et_mask != NULL)
		    {
 		        tmp_mask = (*make_exclude_mask)(listing.front().mask, opt);
			tmp_et_mask->add_mask(not_mask(*tmp_mask));
			delete tmp_mask;
			tmp_mask = NULL;
		    }
		    else // need to create et_mask
		    {
		        tmp_mask = (*make_exclude_mask)(listing.front().mask, opt);
			tmp_et_mask = new (nothrow) et_mask();
			if(tmp_et_mask == NULL)
			    throw Ememory("make_ordered_mask");
			tmp_et_mask->add_mask(*ret_mask);
			tmp_et_mask->add_mask(not_mask(*tmp_mask));
			delete tmp_mask;
			tmp_mask = NULL;
			delete ret_mask;
			ret_mask = tmp_et_mask;
			tmp_ou_mask = NULL;
		    }
		}
	    listing.pop_front();
	}

	if(ret_mask == NULL)
	{
	    ret_mask = new (nothrow) bool_mask(true);
	    if(ret_mask == NULL)
		throw Ememory("get_args");
	}
    }
    catch(...)
    {
	if(ret_mask != NULL)
	{
	    delete tmp_mask;
	    tmp_mask = NULL;
	}
	if(tmp_ou_mask != NULL && tmp_ou_mask != ret_mask)
	{
	    delete tmp_ou_mask;
	    tmp_ou_mask = NULL;
	}
	if(tmp_et_mask != NULL && tmp_et_mask != ret_mask)
	{
	    delete tmp_et_mask;
	    tmp_et_mask = NULL;
	}
	if(tmp_mask != NULL)
	{
	    delete tmp_mask;
	    tmp_mask = NULL;
	}
	throw;
    }

    return ret_mask;
}

static mask *make_unordered_mask(deque<pre_mask> & listing, mask *(*make_include_mask) (const string & x, mask_opt opt), mask *(*make_exclude_mask)(const string & x, mask_opt opt), const path & prefix)
{
    et_mask *ret_mask = new (nothrow) et_mask();
    ou_mask tmp_include, tmp_exclude;
    mask *tmp_mask = NULL;
    mask_opt opt = prefix;

    if(ret_mask == NULL)
	throw Ememory("make_unordered_mask");

    try
    {
	while(!listing.empty())
	{
	    opt.read_from(listing.front());
	    if(listing.front().included)
	    {
  	        tmp_mask = (*make_include_mask)(listing.front().mask, opt);
		tmp_include.add_mask(*tmp_mask);
		delete tmp_mask;
		tmp_mask = NULL;
	    }
	    else // excluded mask
	    {
 	        tmp_mask = (*make_exclude_mask)(listing.front().mask, opt);
		tmp_exclude.add_mask(*tmp_mask);
		delete tmp_mask;
		tmp_mask = NULL;
	    }
	    listing.pop_front();
	}

	if(tmp_include.size() > 0)
	    ret_mask->add_mask(tmp_include);
	else
	    ret_mask->add_mask(bool_mask(true));
	if(tmp_exclude.size() > 0)
	    ret_mask->add_mask(not_mask(tmp_exclude));
    }
    catch(...)
    {
	delete ret_mask;
	ret_mask = NULL;
	throw;
    }

    return ret_mask;
}

static void split_compression_algo(const char *arg, compression & algo, U_I & level)
{
    if(arg == NULL)
	throw SRC_BUG;
    else
    {
	string working = arg;
	string::iterator it = working.begin();

	while(it != working.end() && *it != ':')
	    it++;

	if(it == working.end()) // no ':' found in string
	{
	    if(!tools_my_atoi(working.c_str(), level))
	    {
		    // argument to -z is not an integer, testing whether this is an algorithm

		try
		{
		    algo = string2compression(working.c_str());
		    level = 9; // argument is a compression algo, level is 9 by default
		}
		catch(Erange & e)
		{
		    throw Erange("split_compression_algo", tools_printf(gettext("%s does not name a compression \"[algorithm][:][level]\" , like for examples \"gzip\", \"lzo\", \"bzip2\", \"lzo:3\", \"gzip:2\", \"8\" or \"1\". Please review the man page about -z option"), working.c_str()));
		}
	    }
	    else // argument is a compression level, algorithm is gzip by default
		algo = gzip;
	}
	else // a ':' has been found and "it" points to it
	{
	    string first_part = string(working.begin(), it);
	    string second_part = string(it+1, working.end());

	    if(first_part != "")
		algo = string2compression(first_part);
	    else
		algo = gzip; // default algorithm

	    if(second_part != "")
	    {
		if(!tools_my_atoi(second_part.c_str(), level) || level > 9 || level < 1)
		    throw Erange("split_compression_algo", gettext("Compression level must be between 1 and 9, included"));
	    }
	    else
		level = 9; // default compression level
	}
    }
}

static fsa_scope string_to_fsa(const string & arg)
{
    fsa_scope ret;
    vector<string> fams = line_tools_split(arg, ',');

    ret.clear();
    if(arg != "none")
    {
	for(vector<string>::iterator it = fams.begin();
	    it != fams.end();
	    ++it)
	{
	    if(*it == "extX"
	       || *it == "ext"
	       || *it == "extx")
		ret.insert(fsaf_linux_extX);
	    else if(*it == "HFS+"
		    || *it == "hfs+")
		ret.insert(fsaf_hfs_plus);
	    else
		throw Erange("string_to_fsa", string(gettext("unknown FSA family: ")) + (*it));
	}
    }

    return ret;
}

static void add_non_options(S_I argc, char * const argv[], vector<string> & non_options)
{
    (void)line_tools_reset_getopt();

#if HAVE_GETOPT_LONG
    while(getopt_long(argc, argv, OPT_STRING, get_long_opt(), NULL) != EOF)
        ;
#else
    while(getopt(argc, argv, OPT_STRING) != EOF)
        ;
#endif

    for(register S_I i = optind ; i < argc ; ++i)
    	if(strcmp(argv[i],"create") == 0
	   || strcmp(argv[i], "extract") == 0
	   || strcmp(argv[i], "listing") == 0
	   || strcmp(argv[i], "list") == 0
	   || strcmp(argv[i], "test") == 0
	   || strcmp(argv[i], "diff") == 0
	   || strcmp(argv[i], "isolate") == 0
	   || strcmp(argv[i], "merge") == 0
	   || strcmp(argv[i], "reference") == 0
	   || strcmp(argv[i], "auxiliary") == 0
	   || strcmp(argv[i], "all") == 0
	   || strcmp(argv[i], "default") == 0)
	    throw Erange("add_non_options", tools_printf(gettext("User target named \"%s\" is not allowed (reserved word for conditional syntax)"), argv[i]));
	else
	    non_options.push_back(argv[i]);

    (void)line_tools_reset_getopt();
}
