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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: command_line.cpp,v 1.76.2.8 2009/05/09 21:15:56 edrusb Rel $
//
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

#include "deci.hpp"
#include "command_line.hpp"
#include "user_interaction.hpp"
#include "tools.hpp"
#include "dar.hpp"
#include "dar_suite.hpp"
#include "etage.hpp"
#include "integers.hpp"
#include "no_comment.hpp"
#include "config_file.hpp"
#include "shell_interaction.hpp"
#include "dar.hpp"
#include "libdar.hpp"
#include "cygwin_adapt.hpp"
#include "mask_list.hpp"

#define OPT_STRING "c:A:x:d:t:l:v::z::y::nw::p::kR:s:S:X:I:P:bhLWDru:U:VC:i:o:OT::E:F:K:J:Y:Z:B:fm:NH::a::eQG:Mg:j#:*:,[:]:+:@:$:~:%:q"

#define ONLY_ONCE "Only one -%c is allowed, ignoring this extra option"
#define MISSING_ARG "Missing argument to -%c"
#define INVALID_ARG "Invalid argument for -%c option"
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

    // return a newly allocated memory (to be deleted by the caller)
static void show_license(user_interaction & dialog);
static void show_warranty(user_interaction & dialog);
static void show_version(user_interaction & dialog, const char *command_name);
static void usage(user_interaction & dialog, const char *command_name);
#if HAVE_GETOPT_LONG
static const struct option *get_long_opt();
#endif
static S_I reset_getopt(); // return the old position of parsing (next argument to parse)

static bool get_args_recursive(user_interaction & dialog,
                               vector<string> & inclusions,
                               S_I argc,
                               char *argv[],
                               operation &op,
                               path * &fs_root,
                               path * &sauv_root,
                               path * &ref_root,
                               infinint &file_size,
                               infinint &first_file_size,
                               deque<pre_mask> & name_include_exclude,
                               deque<pre_mask> & path_include_exclude,
                               string & filename,
                               string *& ref_filename,
                               bool &allow_over,
                               bool &warn_over,
                               bool &info_details,
                               compression &algo,
                               U_I & compression_level,
                               bool & detruire,
                               infinint & pause,
                               bool & beep,
                               bool & make_empty_dir,
                               bool & only_more_recent,
                               deque<pre_mask> & ea_include_exclude,
                               string & input_pipe,
                               string &output_pipe,
                               inode::comparison_fields & what_to_check,
                               string & execute,
                               string & execute_ref,
                               string & pass,
                               string & pass_ref,
                               deque<pre_mask> & compr_include_exclude,
                               bool & flat,
                               infinint & min_compr_size,
                               bool & readconfig,
                               bool & nodump,
                               infinint & hourshift,
                               bool & warn_remove_no_match,
                               string & alteration,
                               bool & empty,
                               U_I & suffix_base,
                               path * & on_fly_root,
                               string * & on_fly_filename,
                               bool & alter_atime,
                               bool & same_fs,
                               bool & snapshot,
                               bool & cache_directory_tagging,
                               U_32 crypto_size,
                               U_32 crypto_size_ref,
                               bool & ordered_filters,
                               bool & case_sensit,
			       bool & ea_erase,
			       bool & display_skipped,
			       archive::listformat & list_mode,
			       path * & aux_root,
			       string * & aux_filename,
			       string & aux_pass,
			       string & aux_execute,
			       U_32 & aux_crypto_size,
			       bool & glob_mode,
			       bool & keep_compressed,
			       bool & fixed_date_mode,
			       infinint & fixed_date,
			       bool & quiet);

static void make_args_from_file(user_interaction & dialog,
                                operation op, const char *filename, S_I & argc,
                                char **&argv, bool info_details);
static void destroy(S_I argc, char **argv);
static void skip_getopt(S_I argc, char *argv[], S_I next_to_read);
static bool update_with_config_files(user_interaction & dialog,
                                     const char * home,
                                     operation &op,
                                     path * &fs_root,
                                     path * &sauv_root,
                                     path * &ref_root,
                                     infinint &file_size,
                                     infinint &first_file_size,
                                     deque<pre_mask> & name_include_exclude,
                                     deque<pre_mask> & path_include_exclude,
                                     string & filename,
                                     string *& ref_filename,
                                     bool &allow_over,
                                     bool &warn_over,
                                     bool &info_details,
                                     compression &algo,
                                     U_I & compression_level,
                                     bool & detruire,
                                     infinint & pause,
                                     bool & beep,
                                     bool & make_empty_dir,
                                     bool & only_more_recent,
				     deque<pre_mask> & ea_include_exclude,
                                     string & input_pipe,
                                     string &output_pipe,
                                     inode::comparison_fields & what_to_check,
                                     string & execute,
                                     string & execute_ref,
                                     string & pass,
                                     string & pass_ref,
                                     deque<pre_mask> & compr_include_exclude,
                                     bool & flat,
                                     infinint & min_compr_size,
                                     bool & nodump,
                                     infinint & hourshift,
                                     bool & warn_remove_no_match,
                                     string & alteration,
                                     bool & empty,
                                     U_I & suffix_base,
                                     path * & on_fly_root,
                                     string * & on_fly_filename,
                                     bool & alter_atime,
                                     bool & same_fs,
                                     bool & snapshot,
                                     bool & cache_directory_tagging,
                                     U_32 crypto_size,
                                     U_32 crypto_size_ref,
                                     bool & ordered_filters,
                                     bool & case_sensit,
				     bool & ea_erase,
				     bool & display_skipped,
				     archive::listformat & list_mode,
				     path * & aux_root,
				     string * & aux_filename,
				     string & aux_pass,
				     string & aux_execute,
				     U_32 & aux_crypto_size,
				     bool & glob_mode,
				     bool & keep_compressed,
				     bool & fixed_date_mode,
				     infinint & fixed_date,
				     bool & quiet);

static mask *make_include_exclude_name(const string & x, mask_opt opt);
static mask *make_exclude_path_ordered(const string & x, mask_opt opt);
static mask *make_exclude_path_unordered(const string & x, mask_opt opt);
static mask *make_include_path(const string & x, mask_opt opt);
static mask *make_ordered_mask(deque<pre_mask> & listing, mask *(*make_include_mask) (const string & x, mask_opt opt), mask *(*make_exclude_mask)(const string & x, mask_opt opt), const path & prefix);
static mask *make_unordered_mask(deque<pre_mask> & listing, mask *(*make_include_mask) (const string & x, mask_opt opt), mask *(*make_exclude_mask)(const string & x, mask_opt opt), const path & prefix);

// #define DEBOGGAGE
#ifdef DEBOGGAGE
static void show_args(S_I argc, char *argv[]);
#endif

bool get_args(user_interaction & dialog,
              const char *home, S_I argc, char *argv[],
              operation &op, path * &fs_root, path * &sauv_root, path * &ref_root,
              infinint &file_size, infinint &first_file_size,
              mask *&selection, mask *&subtree,
              string &filename, string *&ref_filename,
              bool &allow_over, bool &warn_over, bool &info_details,
              compression &algo, U_I & compression_level,
              bool & detruire,
	      infinint & pause,
	      bool & beep,
	      bool & make_empty_dir,
              bool & only_more_recent,
	      mask * & ea_mask,
              string & input_pipe, string &output_pipe,
              inode::comparison_fields & what_to_check,
              string & execute, string & execute_ref,
              string & pass, string & pass_ref,
              mask *& compress_mask,
              bool & flat,
              infinint & min_compr_size,
              bool & nodump,
              infinint & hourshift,
              bool & warn_remove_no_match,
              string & alteration,
              bool & empty,
              path * & on_fly_root,
              string * & on_fly_filename,
              bool & alter_atime,
              bool & same_fs,
              bool & snapshot,
              bool & cache_directory_tagging,
              U_32 & crypto_size,
              U_32 & crypto_size_ref,
	      bool & ea_erase,
	      bool & display_skipped,
	      archive::listformat & list_mode,
	      path * & aux_root,
	      string * & aux_filename,
	      string & aux_pass,
	      string & aux_execute,
	      U_32 & aux_crypto_size,
	      bool & keep_compressed,
	      infinint & fixed_date,
	      bool & quiet)
{
    op = noop;
    fs_root = NULL;
    sauv_root = NULL;
    ref_root = NULL;
    selection = NULL;
    subtree = NULL;
    ref_filename = NULL;
    ea_mask = NULL;
    file_size = 0;
    first_file_size = 0;
    filename = "";
    allow_over = true;
    warn_over = true;
    info_details = false;
    algo = none;
    compression_level = 9;
    detruire = true;
    pause = 0;
    beep = false;
    make_empty_dir = false;
    only_more_recent = false;
    input_pipe = "";
    output_pipe = "";
    what_to_check = inode::cf_all;
    execute = "";
    execute_ref = "";
    pass = "";
    pass_ref = "";
    compress_mask = NULL;
    deque<pre_mask> name_include_exclude;
    deque<pre_mask> path_include_exclude;
    deque<pre_mask> compr_include_exclude;
    deque<pre_mask> ea_include_exclude;
    vector<string> inclusions;
    flat = false;
    min_compr_size = min_compr_size_default;
    nodump = false;
    hourshift = 0;
    warn_remove_no_match = true;
    alteration = "";
    empty = false;
    on_fly_root = NULL;
    on_fly_filename = NULL;
    alter_atime = false;
    same_fs = false;
    snapshot = false;
    cache_directory_tagging = false;
    crypto_size = DEFAULT_CRYPTO_SIZE;
    crypto_size_ref = DEFAULT_CRYPTO_SIZE;
    ea_erase = false;
    display_skipped = false;
    list_mode = archive::normal;
    aux_root = NULL;
    aux_filename = NULL;
    aux_pass = "";
    aux_execute = "";
    aux_crypto_size = DEFAULT_CRYPTO_SIZE;
    keep_compressed = false;
    fixed_date = 0;
    quiet = false;

    bool readconfig = true;
    U_I suffix_base = TOOLS_BIN_SUFFIX;
    bool ordered_filters = false;
    bool case_sensit = true;
    string cmd = path(argv[0]).basename();
    bool glob_mode = true; // defaults to glob expressions
    bool fixed_date_mode = false;


    try
    {
        try
        {
            opterr = 0;


            if(!get_args_recursive(dialog,
                                   inclusions, argc, argv,
                                   op, fs_root,
                                   sauv_root, ref_root,
                                   file_size, first_file_size,
                                   name_include_exclude,
                                   path_include_exclude,
                                   filename, ref_filename,
                                   allow_over, warn_over, info_details,
                                   algo, compression_level, detruire,
				   pause,
                                   beep, make_empty_dir,
                                   only_more_recent,
				   ea_include_exclude,
                                   input_pipe, output_pipe,
                                   what_to_check,
                                   execute,
                                   execute_ref,
                                   pass, pass_ref,
                                   compr_include_exclude,
                                   flat, min_compr_size, readconfig, nodump,
                                   hourshift, warn_remove_no_match,
                                   alteration, empty, suffix_base,
                                   on_fly_root,
                                   on_fly_filename,
                                   alter_atime,
                                   same_fs,
                                   snapshot,
                                   cache_directory_tagging,
                                   crypto_size,
                                   crypto_size_ref,
                                   ordered_filters,
                                   case_sensit,
				   ea_erase,
				   display_skipped,
				   list_mode,
				   aux_root,
				   aux_filename,
				   aux_pass,
				   aux_execute,
				   aux_crypto_size,
				   glob_mode,
				   keep_compressed,
				   fixed_date_mode,
				   fixed_date,
				   quiet))
                return false;

                // checking and updating options with configuration file if any
            if(readconfig)
                if(! update_with_config_files(dialog,
                                              home,
                                              op, fs_root,
                                              sauv_root, ref_root,
                                              file_size, first_file_size,
                                              name_include_exclude,
                                              path_include_exclude,
                                              filename, ref_filename,
                                              allow_over, warn_over, info_details,
                                              algo,
					      compression_level,
					      detruire,
					      pause,
                                              beep, make_empty_dir,
                                              only_more_recent,
					      ea_include_exclude,
                                              input_pipe, output_pipe,
                                              what_to_check,
                                              execute,
                                              execute_ref,
                                              pass, pass_ref,
                                              compr_include_exclude,
                                              flat, min_compr_size, nodump,
                                              hourshift,
                                              warn_remove_no_match,
                                              alteration,
                                              empty,
                                              suffix_base,
                                              on_fly_root,
                                              on_fly_filename,
                                              alter_atime,
                                              same_fs,
                                              snapshot,
                                              cache_directory_tagging,
                                              crypto_size,
                                              crypto_size_ref,
                                              ordered_filters,
                                              case_sensit,
					      ea_erase,
					      display_skipped,
					      list_mode,
					      aux_root,
					      aux_filename,
					      aux_pass,
					      aux_execute,
					      aux_crypto_size,
					      glob_mode,
					      keep_compressed,
					      fixed_date_mode,
					      fixed_date,
					      quiet))
                    return false;

                // some sanity checks

            if(filename == "" || sauv_root == NULL || op == noop)
                throw Erange("get_args", tools_printf(gettext("Missing -c -x -d -t -l -C -+ option, see `%S -h' for help"), &cmd));
            if(filename == "-" && file_size != 0)
                throw Erange("get_args", gettext("Slicing (-s option), is not compatible with archive on standard output (\"-\" as filename)"));
            if(filename != "-" && (op != create && op != isolate && op != merging))
                if(sauv_root == NULL)
                    throw SRC_BUG;
            if(filename != "-")
                tools_check_basename(dialog, *sauv_root, filename, EXTENSION);
	    if(op == merging && aux_filename != NULL)
	    {
		if(aux_root == NULL)
		    throw SRC_BUG;
		else
		    tools_check_basename(dialog, *aux_root, *aux_filename, EXTENSION);
	    }

            if(fs_root == NULL)
                fs_root = new path(".");
	    if(fixed_date_mode && op != create)
		throw Erange("get_args", gettext("-af option is only available with -c"));
            if(ref_filename != NULL && op != create && op != isolate && op != merging)
                dialog.warning(gettext("-A option is only useful with -c, -C or -+ options"));
            if(op == isolate && ref_filename == NULL)
                throw Erange("get_args", gettext("with -C option, -A option is mandatory"));
	    if(op == merging && ref_filename == NULL)
                throw Erange("get_args", gettext("with -+ option, -A option is mandatory"));
            if(op != extract && !warn_remove_no_match)
                dialog.warning(gettext("-wa is only useful with -x option"));
            if(filename == "-" && ref_filename != NULL && *ref_filename == "-"
               && output_pipe == "")
                throw Erange("get_args", gettext("-o is mandatory when using \"-A -\" with \"-c -\" \"-C -\" or \"-+ -\""));
            if(ref_filename != NULL && *ref_filename != "-")
	    {
                if(ref_root == NULL)
                    throw SRC_BUG;
                else
                    tools_check_basename(dialog, *ref_root, *ref_filename, EXTENSION);
	    }

            if(algo != none && op != create && op != isolate && op != merging)
                dialog.warning(gettext("-z or -y need only to be used with -c -C or -+ options"));
            if(first_file_size != 0 && file_size == 0)
                throw Erange("get_args", gettext("-S option requires the use of -s"));
            if(what_to_check != inode::cf_all && (op == isolate || (op == create && ref_root == NULL) || op == test || op == listing || op == merging))
                dialog.warning(gettext("ignoring -O option, as it is useless in this situation"));
            if(getuid() != 0 && op == extract && what_to_check == inode::cf_all) // uid == 0 for root
            {
		string name;
                tools_extract_basename(argv[0], name);

		what_to_check = inode::cf_ignore_owner;
		string msg = tools_printf(gettext("File ownership will not be restored as %s is not run as root. to avoid this message use -O option"), name.c_str());
		dialog.pause(msg);
            }
            if(execute != "" && file_size == 0 && (op == create || op == isolate || op == merging))
                dialog.warning(gettext("-E is not possible (and useless) without slicing (-s option), -E will be ignored"));
            if(execute_ref != "" && ref_filename == NULL)
                dialog.warning(gettext("-F is only useful with -A option, for the archive of reference"));
            if(pass_ref != "" && ref_filename == NULL)
	    {
                dialog.warning(gettext("-J is only useful with -A option, for the archive of reference"));
	    }
            if(flat && op != extract)
                dialog.warning(gettext("-f in only available with -x option, ignoring"));
            if(min_compr_size != min_compr_size_default && op != create && op != merging)
                dialog.warning(gettext("-m is only useful with -c"));
            if(hourshift > 0)
	    {
                if(op == create)
                {
                    if(ref_filename == NULL)
                        dialog.warning(gettext("-H is only useful with -A option when making a backup"));
                }
                else
                    if(op == extract)
                    {
                        if(!only_more_recent)
                            dialog.warning(gettext("-H is only useful with -r option when extracting"));
                    }
                    else
                        dialog.warning(gettext("-H is only useful with -c or -x"));
	    }

            if(alteration != "" && op != listing)
                dialog.warning(gettext("-as is only available with -l, ignoring -as option"));
            if(empty && op != create && op != extract && op != merging)
                dialog.warning(gettext("-e is only useful with -x, -c or -+ options"));
            if(op != create && op != merging && (on_fly_root != NULL || on_fly_filename != NULL))
                throw Erange("get_args", gettext("-G option is only available with -c or -+ options"));
            if((on_fly_root != NULL) ^ (on_fly_filename != NULL))
                throw SRC_BUG;
            if(alter_atime && op != create && op != diff)
                dialog.warning(gettext("-aa is only useful with -c or -d"));
            if(same_fs && op != create)
                dialog.warning(gettext("-M is only useful with -c"));
            if(snapshot && op != create)
                dialog.warning(gettext("The snapshot backup (-A +) is only available with -c option, ignoring"));
            if(cache_directory_tagging && op != create)
                dialog.warning(gettext("The Cache Directory Tagging Standard is only useful while performing a backup, ignoring it here"));
	    if(op == listing && path_include_exclude.size() != 0)
		dialog.warning(gettext("Warning, the following options -[ , -], -P and -g are not used with -l (listing) operation"));

	    if((aux_root != NULL || aux_filename != NULL) && op != merging)
		throw Erange("get_args", gettext("-@ is only available with -+ option"));
	    if(aux_pass != "" && op != merging)
		throw Erange("get_args", gettext("-$ is only available with -+ option"));
	    if(aux_execute != "" && op != merging)
		throw Erange("get_args", gettext("-~ is only available with -+ option"));
	    if(aux_crypto_size != DEFAULT_CRYPTO_SIZE && op != merging)
		throw Erange("get_args", tools_printf(gettext("-%% is only available with -+ option")));

	    if(aux_pass != "" && aux_filename == NULL)
		dialog.warning(gettext("-$ is only useful with -@ option, for the auxiliary archive of reference"));
	    if(aux_crypto_size != DEFAULT_CRYPTO_SIZE && aux_filename == NULL)
		dialog.printf(gettext("-%% is only useful with -@ option, for the auxiliary archive of reference"));
	    if(aux_execute != "" && aux_filename == NULL)
		dialog.warning(gettext("-~ is only useful with -@ option, for the auxiliary archive of reference"));
	    if(keep_compressed && op != merging)
	    {
		dialog.warning(gettext("-ak is only available while merging (operation -+), ignoring -ak"));
		keep_compressed = false;
	    }

	    if(algo != none && op == merging && keep_compressed)
		dialog.warning(gettext("Compression option (-z or -y) is useless and ignored when using -ak option"));


                //////////////////////
                // generating masks
                // for filenames
                //
            if(ordered_filters)
                selection = make_ordered_mask(name_include_exclude, &make_include_exclude_name, &make_include_exclude_name,
                                              tools_relative2absolute_path(*fs_root, tools_getcwd()));
            else // unordered filters
                selection = make_unordered_mask(name_include_exclude, &make_include_exclude_name, &make_include_exclude_name,
                                                tools_relative2absolute_path(*fs_root, tools_getcwd()));


                /////////////////////////
                // generating masks for
                // directory tree
                //

            if(ordered_filters)
                subtree = make_ordered_mask(path_include_exclude, &make_include_path, &make_exclude_path_ordered,
                                            op != test && op != merging ? tools_relative2absolute_path(*fs_root, tools_getcwd()) : "<ROOT>");
            else // unordered filters
                subtree = make_unordered_mask(path_include_exclude, &make_include_path, &make_exclude_path_unordered,
                                              op != test && op != merging ? tools_relative2absolute_path(*fs_root, tools_getcwd()) : "<ROOT>");


                ////////////////////////////////
                // generating mask for
                // compression selected files
                //
            if(algo == none)
            {
                if(!compr_include_exclude.empty())
                    dialog.warning(gettext("-Y and -Z are only useful with compression (-z or -y option for example), ignoring any -Y and -Z option"));
                if(min_compr_size != min_compr_size_default)
                    dialog.warning(gettext("-m is only useful with compression (-z or -y option for example), ignoring -m"));
            }

            if(algo != none)
                if(ordered_filters)
                    compress_mask = make_ordered_mask(compr_include_exclude, &make_include_exclude_name, & make_include_exclude_name,
                                                      tools_relative2absolute_path(*fs_root, tools_getcwd()));
                else
                    compress_mask = make_unordered_mask(compr_include_exclude, &make_include_exclude_name, & make_include_exclude_name,
                                                        tools_relative2absolute_path(*fs_root, tools_getcwd()));
            else
            {
                compress_mask = new bool_mask(true);
                if(compress_mask == NULL)
                    throw Ememory("get_args");
            }

	        ////////////////////////////////
		// generating mask for EA
		//
		//

	    if(ordered_filters)
		ea_mask = make_ordered_mask(ea_include_exclude, &make_include_exclude_name, &make_include_exclude_name,
					    tools_relative2absolute_path(*fs_root, tools_getcwd()));
	    else // unordered filters
		ea_mask = make_unordered_mask(ea_include_exclude, &make_include_exclude_name, &make_include_exclude_name,
					      tools_relative2absolute_path(*fs_root, tools_getcwd()));

        }
        catch(...)
        {
            if(fs_root != NULL)
            {
                delete fs_root;
                fs_root = NULL;
            }
            if(sauv_root != NULL)
            {
                delete sauv_root;
                sauv_root = NULL;
            }
            if(selection != NULL)
            {
                delete selection;
                selection = NULL;
            }
            if(subtree != NULL)
            {
                delete subtree;
                subtree = NULL;
            }
            if(ref_root != NULL)
            {
                delete ref_root;
                ref_root = NULL;
            }
            if(ref_filename != NULL)
            {
                delete ref_filename;
                ref_filename = NULL;
            }
            if(compress_mask != NULL)
            {
                delete compress_mask;
                compress_mask = NULL;
            }
            if(on_fly_root != NULL)
            {
                delete on_fly_root;
                on_fly_root = NULL;
            }
            if(on_fly_filename != NULL)
            {
                delete on_fly_filename;
                on_fly_filename = NULL;
            }
	    if(ea_mask != NULL)
	    {
		delete ea_mask;
		ea_mask = NULL;
	    }
            throw;
        }
    }
    catch(Erange & e)
    {
        dialog.warning(string(gettext("Parse error on command line (or included files): ")) + e.get_message());
        return false;
    }
    return true;
}

static bool get_args_recursive(user_interaction & dialog,
                               vector<string> & inclusions,
                               S_I argc,
                               char *argv[],
                               operation &op,
                               path * &fs_root,
                               path * &sauv_root,
                               path * &ref_root,
                               infinint &file_size,
                               infinint &first_file_size,
                               deque<pre_mask> & name_include_exclude,
                               deque<pre_mask> & path_include_exclude,
                               string & filename,
                               string *& ref_filename,
                               bool &allow_over,
                               bool &warn_over,
                               bool &info_details,
                               compression &algo,
                               U_I & compression_level,
                               bool & detruire,
                               infinint & pause,
                               bool & beep,
                               bool & make_empty_dir,
                               bool & only_more_recent,
			       deque<pre_mask> & ea_include_exclude,
                               string & input_pipe,
                               string &output_pipe,
                               inode::comparison_fields & what_to_check,
                               string & execute,
                               string & execute_ref,
                               string & pass,
                               string & pass_ref,
                               deque<pre_mask> & compr_include_exclude,
                               bool & flat,
                               infinint & min_compr_size,
                               bool & readconfig,
                               bool & nodump,
                               infinint & hourshift,
                               bool & warn_remove_no_match,
                               string & alteration,
                               bool & empty,
                               U_I & suffix_base,
                               path * & on_fly_root,
                               string * & on_fly_filename,
                               bool & alter_atime,
                               bool & same_fs,
                               bool & snapshot,
                               bool & cache_directory_tagging,
                               U_32 crypto_size,
                               U_32 crypto_size_ref,
                               bool & ordered_filters,
                               bool & case_sensit,
			       bool & ea_erase,
			       bool & display_skipped,
			       archive::listformat & list_mode,
			       path * & aux_root,
			       string * & aux_filename,
			       string & aux_pass,
			       string & aux_execute,
			       U_32 & aux_crypto_size,
			       bool & glob_mode,
			       bool & keep_compressed,
			       bool & fixed_date_mode,
			       infinint & fixed_date,
			       bool & quiet)
{
    S_I lu;
    S_I rec_c;
    char **rec_v = NULL;
    pre_mask tmp_pre_mask;
    U_I tmp;

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
                if(filename != "" || sauv_root != NULL)
                    throw Erange("get_args", gettext(" Only one option of -c -d -t -l -C -x or -+ is allowed"));
                if(string(optarg) != string(""))
                    tools_split_path_basename(optarg, sauv_root, filename);
                else
                    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
                switch(lu)
                {
                case 'c':
                    op = create;
                    break;
                case 'x':
                    op = extract;
                    break;
                case 'd':
                    op = diff;
                    break;
                case 't':
                    op = test;
                    break;
                case 'l':
                    op = listing;
                    break;
                case 'C':
                    op = isolate;
                    break;
		case '+':
		    op = merging;
		    break;
                default:
                    throw SRC_BUG;
                }
                break;
            case 'A':
                if(ref_filename != NULL || ref_root != NULL || snapshot || fixed_date != 0)
                    throw Erange("get_args", gettext("Only one -A option is allowed"));
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(strcmp("", optarg) == 0)
                    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
		if(fixed_date_mode)
		{
		    try
		    {
			try
			{
				// trying to read a simple integer
			    deci tmp = string(optarg);
			    fixed_date = tmp.computer();
			}
			catch(Edeci & e)
			{
				// fallback to human readable string

			    fixed_date = tools_convert_date(optarg);
			}
		    }
		    catch(Egeneric & e)
		    {
			throw Erange("get_args", string(gettext("Error while parsing -A argument as a date: ")+ e.get_message()));
		    }
		}
		else
		    if(strcmp("+", optarg) == 0)
			snapshot = true;
		    else
		    {
			ref_filename = new string();
			if(ref_filename == NULL)
			    throw Ememory("get_args");
			try
			{
			    tools_split_path_basename(optarg, ref_root, *ref_filename);
			}
			catch(...)
			{
			    delete ref_filename;
			    throw;
			}
		    }
                break;
            case 'v':
		if(optarg == NULL)
		    info_details = true;
		else
		    if (strcasecmp("skipped", optarg) == 0 || strcasecmp("s", optarg) == 0)
			display_skipped = true;
		    else
			throw Erange("command_line.cpp:get_args_recursive", tools_printf(gettext(INVALID_ARG), char(lu)));
                break;
            case 'z':
                if(algo == none)
                    algo = gzip;
                else
                    throw Erange("get_args", gettext("Choose either -z or -y not both"));
                if(optarg != NULL)
                    if(! tools_my_atoi(optarg, compression_level) || compression_level > 9 || compression_level < 1)
                        throw Erange("get_args", gettext("Compression level must be between 1 and 9, included"));
                break;
            case 'y':
                if(algo == none)
                    algo = bzip2;
                else
                    throw Erange("get_args", gettext("Choose either -z or -y not both"));
                if(optarg != NULL)
                    if(! tools_my_atoi(optarg, compression_level) || compression_level > 9 || compression_level < 1)
                        throw Erange("get_args", gettext("Compression level must be between 1 and 9, included"));
                break;
            case 'n':
                allow_over = false;
                if(!warn_over)
                {
                    dialog.warning(gettext("-w option is useless with -n"));
                    warn_over = false;
                }
                break;
            case 'w':
                warn_over = false;
                if(optarg != NULL)
                {
                    if(strcmp(optarg, "a") == 0 || strcmp(optarg, "all") == 0)
                        warn_remove_no_match = false;
                    else
                        if(strcmp(optarg, "d") != 0 && strcmp(optarg, "default") != 0)
                            throw Erange("get_args", string(gettext("Unknown argument given to -w: ")) + optarg);
                        // else this is the default -w
                }
                break;
            case 'p':
		if(optarg != NULL)
		{
		    deci conv = string(optarg);
		    pause = conv.computer();
		}
		else
		    pause = 1;
                break;
            case 'k':
                detruire = false;
                break;
            case 'R':
                if(fs_root != NULL)
                    throw Erange("get_args", gettext("Only one -R option is allowed"));
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                else
                    fs_root = new path(optarg);
                if(fs_root == NULL)
                    throw Ememory("get_args");
                break;
            case 's':
                if(file_size != 0)
                    throw Erange("get_args", gettext("Only one -s option is allowed"));
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                else
                {
                    try
                    {
                        file_size = tools_get_extended_size(optarg, suffix_base);
                        if(first_file_size == 0)
                            first_file_size = file_size;
                    }
                    catch(Edeci &e)
                    {
                        dialog.warning(tools_printf(gettext(INVALID_SIZE), char(lu)));
                        return false;
                    }
                }
                break;
            case 'S':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(first_file_size == 0)
                    first_file_size = tools_get_extended_size(optarg, suffix_base);
                else
                    if(file_size == 0)
                        throw Erange("get_args", gettext("Only one -S option is allowed"));
                    else
                        if(file_size == first_file_size)
                        {
                            try
                            {
                                first_file_size = tools_get_extended_size(optarg, suffix_base);
                                if(first_file_size == file_size)
                                    dialog.warning(gettext("Giving to -S option the same value as the one given to -s option is useless"));
                            }
                            catch(Egeneric &e)
                            {
                                dialog.warning(tools_printf(gettext(INVALID_SIZE), char(lu)));
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
                tmp_pre_mask.case_sensit = case_sensit;
                tmp_pre_mask.included = false;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = glob_mode;
                name_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'I':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                tmp_pre_mask.file_listing = false;
                tmp_pre_mask.case_sensit = case_sensit;
                tmp_pre_mask.included = true;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = glob_mode;
                name_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'P':
            case ']':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                tmp_pre_mask.file_listing = lu == ']';
                tmp_pre_mask.case_sensit = case_sensit;
                tmp_pre_mask.included = false;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = glob_mode;
                path_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'g':
            case '[':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                tmp_pre_mask.file_listing = lu == '[';
                tmp_pre_mask.case_sensit = case_sensit;
                tmp_pre_mask.included = true;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = glob_mode;
                path_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'b':
                beep = true;
                break;
            case 'h':
                usage(dialog, argv[0]);
                return false;
            case 'L':
                show_license(dialog);
                return false;
            case 'W':
                show_warranty(dialog);
                return false;
            case 'D':
                if(make_empty_dir)
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    make_empty_dir = true;
                break;
            case 'r':
                if(!allow_over)
                    dialog.warning(gettext("-r is useless with -n"));
                if(only_more_recent)
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    only_more_recent = true;
                break;
            case 'u':
            case 'U':
		if(optarg == NULL)
		    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		tmp_pre_mask.file_listing = false;
		tmp_pre_mask.case_sensit = case_sensit;
		tmp_pre_mask.included = lu == 'U';
		tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = glob_mode;
		ea_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'V':
                show_version(dialog, argv[0]);
                return false;
            case ':':
                throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(optopt)));
            case 'i':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(input_pipe == "")
                    input_pipe = optarg;
                else
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                break;
            case 'o':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(output_pipe == "")
                    output_pipe = optarg;
                else
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                break;
            case 'O':
                if(what_to_check != inode::cf_all)
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
		    if(optarg == NULL)
			what_to_check = inode::cf_ignore_owner;
		    else
			if(strcasecmp(optarg, "ignore-owner") == 0)
			    what_to_check = inode::cf_ignore_owner;
			else
			    if(strcasecmp(optarg, "mtime") == 0)
				what_to_check = inode::cf_mtime;
			    else
				if(strcasecmp(optarg, "inode-type") == 0)
				    what_to_check = inode::cf_inode_type;
				else
				    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));

                break;
            case 'T':
                if(optarg == NULL)
                    list_mode = archive::tree;
		else
		    if(strcasecmp("normal", optarg) == 0)
                	list_mode = archive::normal;
		    else
			if (strcasecmp("tree", optarg) == 0)
			    list_mode = archive::tree;
			else
			    if (strcasecmp("xml", optarg) == 0)
				list_mode = archive::xml;
			    else
				throw Erange("command_line.cpp:get_args_recursive", tools_printf(gettext(INVALID_ARG), char(lu)));
                break;
            case 'E':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(execute == "")
                    execute = optarg;
                else
                    execute += string(" ; ") + optarg;
                break;
            case 'F':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(execute_ref == "")
                    execute_ref = optarg;
                else
                    execute_ref += string(" ; ") + optarg;
                break;
            case 'J':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(pass_ref == "")
                    pass_ref = optarg;
                else
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                break;
            case 'K':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(pass == "")
                    pass = optarg;
                else
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                break;
            case 'Y':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                tmp_pre_mask.file_listing = false;
                tmp_pre_mask.case_sensit = case_sensit;
                tmp_pre_mask.included = true;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = glob_mode;
                compr_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'Z':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                tmp_pre_mask.file_listing = false;
                tmp_pre_mask.case_sensit = case_sensit;
                tmp_pre_mask.included = false;
                tmp_pre_mask.mask = string(optarg);
		tmp_pre_mask.glob_exp = glob_mode;
                compr_include_exclude.push_back(tmp_pre_mask);
                break;
            case 'B':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(find(inclusions.begin(), inclusions.end(), string(optarg)) != inclusions.end())
                    throw Erange("geta_args", tools_printf(gettext("File inclusion loop detected. The file %s includes itself directly or through other files (-B option)"), optarg));
                else
                {
                    bool ret;
                    make_args_from_file(dialog, op, optarg, rec_c, rec_v, info_details);
#ifdef DEBOGGAGE
                    show_args(rec_c, rec_v);
#endif
                    S_I optind_mem = reset_getopt(); // save the external variable to use recursivity (see getopt)
                        // reset getopt module

                    try
                    {
                        inclusions.push_back(optarg);
                        ret = get_args_recursive(dialog,
                                                 inclusions, rec_c, rec_v, op, fs_root, sauv_root, ref_root,
                                                 file_size, first_file_size,
                                                 name_include_exclude,
                                                 path_include_exclude,
                                                 filename, ref_filename,
                                                 allow_over, warn_over, info_details,
                                                 algo,
						 compression_level,
						 detruire,
						 pause,
                                                 beep, make_empty_dir,
                                                 only_more_recent,
						 ea_include_exclude,
                                                 input_pipe, output_pipe,
                                                 what_to_check,
                                                 execute,
                                                 execute_ref,
                                                 pass, pass_ref,
                                                 compr_include_exclude,
                                                 flat,
                                                 min_compr_size,
                                                 readconfig, nodump,
                                                 hourshift,
                                                 warn_remove_no_match,
                                                 alteration,
                                                 empty,
                                                 suffix_base,
                                                 on_fly_root,
                                                 on_fly_filename,
                                                 alter_atime,
                                                 same_fs,
                                                 snapshot,
                                                 cache_directory_tagging,
                                                 crypto_size,
                                                 crypto_size_ref,
                                                 ordered_filters,
                                                 case_sensit,
						 ea_erase,
						 display_skipped,
						 list_mode,
						 aux_root,
						 aux_filename,
						 aux_pass,
						 aux_execute,
						 aux_crypto_size,
						 glob_mode,
						 keep_compressed,
						 fixed_date_mode,
						 fixed_date,
						 quiet);
                        inclusions.pop_back();
                    }
                    catch(...)
                    {
                        destroy(rec_c, rec_v);
                        skip_getopt(argc, argv, optind_mem);
                        throw;
                    }
                    destroy(rec_c, rec_v);
                    skip_getopt(argc, argv, optind_mem); // restores getopt after recursion

                    if(!ret)
                        return false;
                }
                break;
            case 'f':
                if(flat)
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    flat = true;
                break;
            case 'm':
                if(min_compr_size != min_compr_size_default)
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                {
                    if(optarg == NULL)
                        throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                    min_compr_size = tools_get_extended_size(optarg, suffix_base);
                    if(min_compr_size == min_compr_size_default)
                        dialog.warning(tools_printf(gettext("%d is the default value for -m, no need to specify it on command line, ignoring"), min_compr_size_default));
                    break;
                }
            case 'N':
                if(!readconfig)
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    readconfig = false;
                break;
            case ' ':
#ifdef LIBDAR_NODUMP_FEATURE
                if(nodump)
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    nodump = true;
                break;
#else
                throw Ecompilation(gettext("--nodump feature has not been activated at compilation time, it is thus not available"));
#endif
            case 'H':
                if(optarg == NULL)
                    hourshift = 1;
                else
                {
                    try
                    {
                        hourshift = deci(string(optarg)).computer();
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
                    suffix_base = TOOLS_SI_SUFFIX;
                else
                    if(strcasecmp("binary-unit", optarg) == 0 || strcasecmp("binary", optarg) == 0 || strcasecmp("binary-units", optarg) == 0)
                        suffix_base = TOOLS_BIN_SUFFIX;
                    else
                        if(strcasecmp("atime", optarg) == 0 || strcasecmp("a", optarg) == 0)
                        {
                            if(alter_atime)
                                dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                            alter_atime = true;
                        }
                        else
                            if(strcasecmp("ctime", optarg) == 0 || strcasecmp("c", optarg) == 0)
                            {
                                alter_atime = false;
                            }
                            else
                                if(strcasecmp("m", optarg) == 0 || strcasecmp("mask", optarg) == 0)
                                {
                                    if(ordered_filters)
                                        dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                                    else
                                        ordered_filters = true;
                                }
                                else
                                    if(strcasecmp("n", optarg) == 0 || strcasecmp("no-case", optarg) == 0 || strcasecmp("no_case", optarg) == 0)
                                        case_sensit = false;
                                    else
                                        if(strcasecmp("case", optarg) == 0)
                                            case_sensit = true;
                                        else
                                            if(strcasecmp("s", optarg) == 0 || strcasecmp("saved", optarg) == 0)
                                            {
                                                if(alteration != "")
                                                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                                                else
                                                    alteration = optarg;
                                            }
                                            else
						if(strcasecmp("e", optarg) == 0 || strcasecmp("erase_ea", optarg) == 0)
						{
						    if(ea_erase)
							dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
						    else
							ea_erase = true;
						}
						else
						    if(strcasecmp("g", optarg) == 0 || strcasecmp("glob", optarg) == 0)
							glob_mode = true;
						    else
							if(strcasecmp("r", optarg) == 0 || strcasecmp("regex", optarg) == 0)
							    glob_mode = false;
							else
							    if(strcasecmp("k", optarg) == 0 || strcasecmp("keep-compressed", optarg) == 0)
							    {
								if(keep_compressed)
								    dialog.warning(gettext("-ak option need not be specified more than once, ignoring extra -ak options"));
								keep_compressed = true;
							    }
							    else
								if(strcasecmp("f", optarg) == 0 || strcasecmp("fixed-date", optarg) == 0)
								{
								    if(ref_filename != NULL || ref_root != NULL || snapshot)
									throw Erange("get_args", gettext("-af must be present before -A option not after!"));
								    if(fixed_date_mode)
									dialog.warning(gettext("-af option need not be specified more than once, ignoring extra -af options"));
								    fixed_date_mode = true;
								}
								else
								    throw Erange("command_line.cpp:get_args_recursive", tools_printf(gettext("Unknown argument given to -a : %s"), optarg));
                break;
            case 'e':
                if(empty)
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    empty = true;
                break;
            case 'Q':
            case 'j':
                break;  // ignore this option already parsed during initialization (dar_suite.cpp)
            case 'G':
                if(on_fly_filename != NULL || on_fly_root != NULL)
                    throw Erange("get_args", tools_printf(gettext(ONLY_ONCE), char(lu)));
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(strcmp("", optarg) == 0)
                    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
                if(strcmp("-", optarg) == 0)
                    throw Erange("get_args", gettext("\"-\" not allowed with -G option"));
                on_fly_filename = new string();
                if(on_fly_filename == NULL)
                    throw Ememory("get_args");
                try
                {
                    tools_split_path_basename(optarg, on_fly_root, *on_fly_filename);
                }
                catch(...)
                {
                    delete on_fly_filename;
                    throw;
                }
                break;
            case 'M':
                if(same_fs)
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    same_fs = true;
                break;
            case '#':
                if(! tools_my_atoi(optarg, tmp))
                    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
                else
                    crypto_size = (U_32)tmp;
                break;
            case '*':
                if(! tools_my_atoi(optarg, tmp))
                    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
                else
                    crypto_size_ref = (U_32)tmp;
                break;
            case ',':
                if(cache_directory_tagging)
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                else
                    cache_directory_tagging = true;
                break;
	    case '@':
		if(aux_filename != NULL || aux_root != NULL)
		    throw Erange("get_args", gettext("Only one -@ option is allowed"));
		if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
		if(strcmp("", optarg) == 0)
                    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
		else
		{
		    aux_filename = new string();
		    if(aux_filename == NULL)
			throw Ememory("get_args");
		    try
		    {
			tools_split_path_basename(optarg, aux_root, *aux_filename);
		    }
		    catch(...)
		    {
			delete aux_filename;
			throw;
		    }
		}
		break;
	    case '~':
		if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(aux_execute == "")
                    aux_execute = optarg;
                else
                    aux_execute += string(" ; ") + optarg;
		break;
	    case '$':
                if(optarg == NULL)
                    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(lu)));
                if(aux_pass == "")
                    aux_pass = optarg;
                else
                    dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
                break;
	    case '%':
                if(! tools_my_atoi(optarg, tmp))
                    throw Erange("get_args", tools_printf(gettext(INVALID_ARG), char(lu)));
                else
                    aux_crypto_size = (U_32)tmp;
                break;
	    case 'q':
		quiet = true;
		break;
            case '?':
                dialog.warning(tools_printf(gettext("Ignoring unknown option -%c"),char(optopt)));
                break;
            default:
                dialog.warning(tools_printf(gettext("Ignoring unknown option -%c"),char(lu)));
            }
        }

    if(optind < argc)
        throw Erange("get_args_recursive", tools_printf(gettext("Unknown argument : %s"), argv[optind]));

    return true;
}

static void usage(user_interaction & dialog, const char *command_name)
{
    string name;
    tools_extract_basename(command_name, name);
    shell_interaction_change_non_interactive_output(&cout);

    dialog.printf(gettext("usage: %s [ -c | -x | -d | -t | -l | -C | -+ ] [<path>/]<basename> [options...]\n"), name.c_str());
    dialog.printf("       %s -h\n", name.c_str());
    dialog.printf("       %s -V\n", name.c_str());
#include "dar.usage"
}

static void dummy_call(char *x)
{
    static char id[]="$Id: command_line.cpp,v 1.76.2.8 2009/05/09 21:15:56 edrusb Rel $";
    dummy_call(id);
}

static void show_warranty(user_interaction & dialog)
{
    shell_interaction_change_non_interactive_output(&cout);
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

static void show_license(user_interaction & dialog)
{
    shell_interaction_change_non_interactive_output(&cout);
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

static void show_version(user_interaction & dialog, const char *command_name)
{
    string name;
    tools_extract_basename(command_name, name);
    U_I maj, med, min, bits;
    bool ea, largefile, nodump, special_alloc, thread, libz, libbz2, libcrypto, new_blowfish;

    get_version(maj, min);
    if(maj > 2)
        get_version(maj, med, min);
    else
        med = 0;
    get_compile_time_features(ea, largefile, nodump, special_alloc, bits, thread, libz, libbz2, libcrypto, new_blowfish);
    shell_interaction_change_non_interactive_output(&cout);
    dialog.warning(tools_printf("\n %s version %s, Copyright (C) 2002-2052 Denis Corbin\n",  name.c_str(), ::dar_version())
		   + "   " + dar_suite_command_line_features()
		   + "\n"
		   + (maj > 2 ? tools_printf(gettext(" Using libdar %u.%u.%u built with compilation time options:"), maj, med, min)
		      : tools_printf(gettext(" Using libdar %u.%u built with compilation time options:"), maj, min)));
    tools_display_features(dialog, ea, largefile, nodump, special_alloc, bits, thread, libz, libbz2, libcrypto, new_blowfish);
    dialog.printf("\n");
    dialog.warning(tools_printf(gettext(" compiled the %s with %s version %s\n"), __DATE__, CC_NAT, __VERSION__)
		   + tools_printf(gettext(" %s is part of the Disk ARchive suite (Release %s)\n"), name.c_str(), PACKAGE_VERSION)
		   + tools_printf(gettext(" %s comes with ABSOLUTELY NO WARRANTY; for details\n type `%s -W'."), name.c_str(), name.c_str())
		   + tools_printf(gettext(" This is free software, and you are welcome\n to redistribute it under certain conditions;"))
		   + tools_printf(gettext(" type `%s -L | more'\n for details.\n\n"), name.c_str()));
}

#if HAVE_GETOPT_LONG
static const struct option *get_long_opt()
{
    static const struct option ret[] = {
        {"beep", no_argument, NULL, 'b' },
        {"create", required_argument, NULL, 'c'},
        {"diff", required_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {"input", required_argument, NULL, 'i'},
        {"no-deleted", no_argument, NULL, 'k'},
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
        {"bzip2", optional_argument, NULL, 'y'},
        {"gzip", optional_argument, NULL, 'z'},
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
        {"key", required_argument, NULL, 'K'},
        {"key-ref", required_argument, NULL, 'J'},
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
        {"on-fly-isolate", required_argument, NULL, 'G'},
        {"no-mount-points", no_argument, NULL, 'M'},
        {"go-into", required_argument, NULL, 'g'},
        {"jog", no_argument, NULL, 'j'},
        {"crypto-block", required_argument, NULL, '#'},
        {"crypto-block-ref", required_argument, NULL, '*'},
        {"cache-directory-tagging", no_argument, NULL, ','},
        {"include-from-file", required_argument, NULL, '['},
        {"exclude-from-file", required_argument, NULL, ']'},
	{"merge", required_argument, NULL, '+'},
	{"aux-ref", required_argument, NULL, '@'},
	{"aux-key", required_argument, NULL, '$'},
	{"aux-execute", required_argument, NULL, '~'},
	{"aux-crypto-block", required_argument, NULL, '%'},
	{"quiet", no_argument, NULL, 'q'},
        { NULL, 0, NULL, 0 }
    };

    return ret;
}
#endif

static void make_args_from_file(user_interaction & dialog, operation op, const char *filename, S_I & argc, char **&argv, bool info_details)
{
    vector <string> cibles;
    argv = NULL;
    argc = 0;

    S_I fd = ::open(filename, O_RDONLY|O_BINARY);
    if(fd < 0)
        throw Erange("make_args_from_file", tools_printf(gettext("Cannot open file %s : %s"), filename, strerror(errno)));

    fichier conf = fichier(dialog, fd); // the object conf will close fd

        ////////
        // removing the comments from file
        //
    no_comment sousconf = no_comment(dialog, conf);

        ////////
        // defining the conditional syntax targets
        // that will be considered in the file
    cibles.clear();
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
    config_file surconf = config_file(dialog, cibles, sousconf);

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
        argv = new char *[argc];
        if(argv == NULL)
            throw Ememory("make_args_from_file");
	for(S_I i = 0; i < argc; ++i)
	    argv[i] = NULL;

            // adding a fake "dar" word as first argument (= argv[0])
            //
	char *pseudo_command = new char[strlen(command)+1];
	if(pseudo_command == NULL)
	    throw Ememory("make_args_from_file");
        strncpy(pseudo_command, command, strlen(command));
        pseudo_command[strlen(command)] = '\0';
	argv[0] = pseudo_command;
	pseudo_command = NULL;

        if(info_details)
            dialog.printf(gettext("Arguments read from %s :"), filename);
        for(U_I i = 0; i < mots.size(); ++i)
        {
            argv[i+1] = tools_str2charptr(mots[i]); // mots[i] goes to argv[i+1] !
            if(info_details && i > 0)
                dialog.printf(" \"%s\"", argv[i]);
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
        }
	argv = NULL;
	argc = 0;
	if(pseudo_command != NULL)
	    delete[] pseudo_command;
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

static void skip_getopt(S_I argc, char *argv[], S_I next_to_read)
{
    (void)reset_getopt();
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


static bool update_with_config_files(user_interaction & dialog,
                                     const char * home,
                                     operation &op,
                                     path * &fs_root,
                                     path * &sauv_root,
                                     path * &ref_root,
                                     infinint &file_size,
                                     infinint &first_file_size,
                                     deque<pre_mask> & name_include_exclude,
                                     deque<pre_mask> & path_include_exclude,
                                     string & filename,
                                     string *& ref_filename,
                                     bool &allow_over,
                                     bool &warn_over,
                                     bool &info_details,
                                     compression &algo,
                                     U_I & compression_level,
                                     bool & detruire,
                                     infinint & pause,
                                     bool & beep,
                                     bool & make_empty_dir,
                                     bool & only_more_recent,
				     deque<pre_mask> & ea_include_exclude,
                                     string & input_pipe,
                                     string &output_pipe,
                                     inode::comparison_fields & what_to_check,
                                     string & execute,
                                     string & execute_ref,
                                     string & pass,
                                     string & pass_ref,
                                     deque<pre_mask> & compr_include_exclude,
                                     bool & flat,
                                     infinint & min_compr_size,
                                     bool & nodump,
                                     infinint & hourshift,
                                     bool & warn_remove_no_match,
                                     string & alteration,
                                     bool & empty,
                                     U_I & suffix_base,
                                     path * & on_fly_root,
                                     string * & on_fly_filename,
                                     bool & alter_atime,
                                     bool & same_fs,
                                     bool & snapshot,
                                     bool & cache_directory_tagging,
                                     U_32 crypto_size,
                                     U_32 crypto_size_ref,
                                     bool & ordered_filters,
                                     bool & case_sensit,
				     bool & ea_erase,
				     bool & display_skipped,
				     archive::listformat & list_mode,
				     path * & aux_root,
				     string * & aux_filename,
				     string & aux_pass,
				     string & aux_execute,
				     U_32 & aux_crypto_size,
				     bool & glob_mode,
				     bool & keep_compressed,
				     bool & fixed_date_mode,
				     infinint & fixed_date,
				     bool & quiet)
{
    const unsigned int len = strlen(home);
    const unsigned int delta = 20;
    char *buffer = new char[len+delta+1];
    enum { syntax, ok, unknown } retour = unknown;

    if(buffer == NULL)
        throw Ememory("command_line.cpp:update_with_config_files");

    try
    {
            // 20 bytes is enough for
            // "/.darrc" and "/etc/darrc"
        S_I rec_c = 0;
        char **rec_v = NULL;
        bool btmp = true;


            // trying to open $HOME/.darrc
        strncpy(buffer, home, len+1); // +1 because of final '\0'
        strncat(buffer, "/.darrc", delta);

        try
        {
            make_args_from_file(dialog, op, buffer, rec_c, rec_v, info_details);
            try
            {
                vector<string> inclusions;
                (void)reset_getopt();

                if(! get_args_recursive(dialog,
                                        inclusions,
                                        rec_c, rec_v, op, fs_root,
                                        sauv_root, ref_root,
                                        file_size, first_file_size,
                                        name_include_exclude,
                                        path_include_exclude,
                                        filename, ref_filename,
                                        allow_over, warn_over,
                                        info_details,
                                        algo, compression_level,
                                        detruire,
					pause,
                                        beep, make_empty_dir,
                                        only_more_recent,
					ea_include_exclude,
                                        input_pipe, output_pipe,
                                        what_to_check,
                                        execute,
                                        execute_ref,
                                        pass, pass_ref,
                                        compr_include_exclude,
                                        flat,
                                        min_compr_size,
                                        btmp, nodump,
                                        hourshift,
                                        warn_remove_no_match,
                                        alteration,
                                        empty,
                                        suffix_base,
                                        on_fly_root,
                                        on_fly_filename,
                                        alter_atime,
                                        same_fs,
                                        snapshot,
                                        cache_directory_tagging,
                                        crypto_size,
                                        crypto_size_ref,
                                        ordered_filters,
                                        case_sensit,
					ea_erase,
					display_skipped,
					list_mode,
					aux_root,
					aux_filename,
					aux_pass,
					aux_execute,
					aux_crypto_size,
					glob_mode,
					keep_compressed,
					fixed_date_mode,
					fixed_date,
					quiet))
                    retour = syntax;
                else
                    retour = ok;
            }
            catch(...)
            {
                if(rec_v != NULL)
                    destroy(rec_c, rec_v);
                throw;
            }
            if(rec_v != NULL)
                destroy(rec_c, rec_v);
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
                // trying to open /etc/darrc
            strncpy(buffer, "/etc/darrc", len+delta);

            try
            {
                make_args_from_file(dialog, op, buffer, rec_c, rec_v, info_details);

                try
                {
                    vector<string> inclusions;
                    dialog.warning(string(gettext("Reading config file: "))+buffer);
                    (void)reset_getopt(); // reset getopt call

                    if(! get_args_recursive(dialog,
                                            inclusions,
                                            rec_c, rec_v, op, fs_root,
                                            sauv_root, ref_root,
                                            file_size, first_file_size,
                                            name_include_exclude,
                                            path_include_exclude,
                                            filename, ref_filename,
                                            allow_over, warn_over,
                                            info_details,
                                            algo, compression_level,
                                            detruire,
					    pause,
                                            beep, make_empty_dir,
                                            only_more_recent,
					    ea_include_exclude,
                                            input_pipe, output_pipe,
                                            what_to_check,
                                            execute,
                                            execute_ref,
                                            pass, pass_ref,
                                            compr_include_exclude,
                                            flat,
                                            min_compr_size,
                                            btmp,
                                            nodump,
                                            hourshift,
                                            warn_remove_no_match,
                                            alteration,
                                            empty,
                                            suffix_base,
                                            on_fly_root,
                                            on_fly_filename,
                                            alter_atime,
                                            same_fs,
                                            snapshot,
                                            cache_directory_tagging,
                                            crypto_size,
                                            crypto_size_ref,
                                            ordered_filters,
                                            case_sensit,
					    ea_erase,
					    display_skipped,
					    list_mode,
					    aux_root,
					    aux_filename,
					    aux_pass,
					    aux_execute,
					    aux_crypto_size,
					    glob_mode,
					    keep_compressed,
					    fixed_date_mode,
					    fixed_date,
					    quiet))
                        retour = syntax;
                    else
                        retour = ok;
                }
                catch(...)
                {
                    if(rec_v != NULL)
                        destroy(rec_c, rec_v);
                    throw;
                }
                if(rec_v != NULL)
                    destroy(rec_c, rec_v);
            }
            catch(Erange & e)
            {
                    // nothing to do
            }
        }
    }
    catch(...)
    {
        delete [] buffer;
        throw;
    }
    delete [] buffer;

    return retour != syntax;
}

static S_I reset_getopt()
{
    S_I ret = optind;

#if HAVE_OPTRESET
    optreset = 1;
    optind = 1;
#else
    optind = 0;
#endif

    return ret;
}


static mask *make_include_exclude_name(const string & x, mask_opt opt)
{
    mask *ret = NULL;

    if(opt.glob_exp)
	ret = new simple_mask(x, opt.case_sensit);
    else
	ret = new regular_mask(x, opt.case_sensit);

    if(ret == NULL)
        throw Ememory("make_include_exclude_name");
    else
        return ret;
}

static mask *make_exclude_path_ordered(const string & x, mask_opt opt)
{
    mask *ret = NULL;
    if(opt.file_listing)
        ret = new mask_list(x, opt.case_sensit, opt.prefix, false);
    else // not file listing mask
    {
	if(opt.glob_exp)
	{
	    ou_mask *val = new ou_mask();

	    if(val == NULL)
		throw Ememory("make_exclude_path");

	    val->add_mask(simple_mask((opt.prefix + x).display(), opt.case_sensit));
	    val->add_mask(simple_mask((opt.prefix + x).display() + "/*", opt.case_sensit));
	    ret = val;
	}
	else // regex
	{
  	    ret = new regular_mask(tools_build_regex_for_exclude_mask(opt.prefix.display(), x), opt.case_sensit);

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
        ret = new mask_list(x, opt.case_sensit, opt.prefix, false);
    else
	if(opt.glob_exp)
	    ret = new simple_mask((opt.prefix + x).display(), opt.case_sensit);
	else
	    ret = new regular_mask(tools_build_regex_for_exclude_mask(opt.prefix.display(), x), opt.case_sensit);
    if(ret == NULL)
        throw Ememory("make_exclude_path");

    return ret;
}

static mask *make_include_path(const string & x, mask_opt opt)
{
    mask *ret = NULL;

    if(opt.file_listing)
        ret = new mask_list(x, opt.case_sensit, opt.prefix, true);
    else
        ret = new simple_path_mask((opt.prefix +x).display(), opt.case_sensit);
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
			tmp_ou_mask = new ou_mask();
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
		    ret_mask = new not_mask(*tmp_mask);
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
			tmp_et_mask = new et_mask();
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
	    ret_mask = new bool_mask(true);
	    if(ret_mask == NULL)
		throw Ememory("get_args");
	}
    }
    catch(...)
    {
	if(ret_mask != NULL)
	    delete tmp_mask;
	if(tmp_ou_mask != NULL && tmp_ou_mask != ret_mask)
	    delete tmp_ou_mask;
	if(tmp_et_mask != NULL && tmp_et_mask != ret_mask)
	    delete tmp_et_mask;
	if(tmp_mask != NULL)
	    delete tmp_mask;
	throw;
    }

    return ret_mask;
}

static mask *make_unordered_mask(deque<pre_mask> & listing, mask *(*make_include_mask) (const string & x, mask_opt opt), mask *(*make_exclude_mask)(const string & x, mask_opt opt), const path & prefix)
{
    et_mask *ret_mask = new et_mask();
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
	throw;
    }

    return ret_mask;
}
