/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
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

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include "getopt_decision.h"
} // end extern "C"

#include <deque>
#include <string>
#include <new>

#include "dar_suite.hpp"
#include "libdar.hpp"
#include "tools.hpp"
#include "cygwin_adapt.hpp"
#include "no_comment.hpp"
#include "line_tools.hpp"
#include "fichier_local.hpp"

using namespace libdar;

#define DAR_MANAGER_VERSION "1.10.0"


#define ONLY_ONCE "Only one -%c is allowed, ignoring this extra option"
#define MISSING_ARG "Missing argument to -%c"
#define INVALID_ARG "Invalid argument given to -%c (requires integer)"
#define OPT_STRING "C:B:A:lD:b:p:od:ru:f:shVm:vQjw:ie:c@:N;:ka:9:z:x"

enum operation { none_op, create, add, listing, del, chbase, where, options, dar, restore, used, files, stats, moving, interactive, check, batch };

static S_I little_main(shared_ptr<user_interaction> & dialog, S_I argc, char *const argv[], const char **env);
static bool command_line(shell_interaction & dialog,
			 S_I argc, char * const argv[],
                         operation & op,
                         string & base,
                         string & arg,
                         S_I & num,
                         vector<string> & rest,
                         archive_num & num2,
			 infinint & date,
                         bool & verbose,
			 bool & ignore_dat_options,
			 bool & even_when_removed,
			 bool & check_order,
			 compression & algozip,
			 U_I & compression_level,
			 bool & change_compression,
			 bool & detailed_dates,
			 bool recursive); // true if called from op_batch
static void show_usage(shell_interaction & dialog, const char *command);
static void show_version(shell_interaction & dialog, const char *command);

#if HAVE_GETOPT_LONG
static const struct option *get_long_opt();
#endif
static void op_create(shared_ptr<user_interaction> & dialog, const string & base, compression algozip, U_I compr_level, bool info_details);
static void op_add(shared_ptr<user_interaction> & dialog, database *dat, const string &arg, string fake, const infinint & min_digits, bool info_details);
static void op_listing(shell_interaction & dialog, const database *dat, bool info_details);
static void op_del(shared_ptr<user_interaction> & dialog, database *dat, S_I min, archive_num max, bool info_details);
static void op_chbase(shared_ptr<user_interaction> & dialog, database *dat, S_I num, const string & arg, bool info_details);
static void op_where(shared_ptr<user_interaction> & dialog, database *dat, S_I num, const string & arg, bool info_details);
static void op_options(shared_ptr<user_interaction> & dialog, database *dat, const vector<string> & rest, bool info_details);
static void op_dar(shared_ptr<user_interaction> & dialog, database *dat, const string & arg, bool info_details);
static void op_restore(shared_ptr<user_interaction> & dialog,
		       database *dat,
		       const vector<string> & rest,
		       const infinint & date,
		       const string & options_for_dar,
		       bool info_details,
		       bool early_release,
		       bool ignore_dar_options_in_base,
		       bool even_when_removed);
static void op_used(shell_interaction & dialog, const database *dat, S_I num, bool info_details);
static void op_files(shell_interaction & dialog,
		     const database *dat,
		     const string & arg,
		     bool info_details,
		     bool detailed_dates);
static void op_stats(shell_interaction & dialog, const database *dat, bool info_details);
static void op_move(shared_ptr<user_interaction> & dialog, database *dat, S_I src, archive_num dst, bool info_details);
static void op_interactive(shared_ptr<user_interaction> & dialog, database *dat, string base);
static void op_check(shared_ptr<user_interaction> & dialog, const database *dat, bool info_details);
static void op_batch(shared_ptr<user_interaction> & dialog, database *dat, const string & filename, bool info_details);

static database *read_base(shared_ptr<user_interaction> & dialog,
			   const string & base,
			   bool partial,
			   bool partial_read_only,
			   bool check_order);
static void write_base(shared_ptr<user_interaction> & dialog,
		       const string & filename,
		       const database *base,
		       bool overwrite,
		       bool change_compression,
		       compression new_compression,
		       U_I compression_level);
static vector<string> read_vector(shared_ptr<user_interaction> & dialog);
static void finalize(shared_ptr<user_interaction> & dialog,
		     operation op,
		     database *dat,
		     const string & base,
		     bool info_details,
		     bool change_compression,
		     compression new_compression,
		     U_I compression_level);
static void action(shared_ptr<user_interaction> & dialog,
		   operation op,
		   database *dat,
		   const string & arg,
		   S_I num,
		   const vector<string> & rest,
		   archive_num num2,
		   const infinint & date,
		   const string & base,
		   bool info_details,
		   bool early_release,
		   bool ignore_database_options,
		   bool even_when_removed,
		   bool detailed_dates);
static void signed_int_to_archive_num(S_I input, archive_num &num, bool & positive);


int main(S_I argc, char *const argv[], const char **env)
{
    return dar_suite_global(argc,
			    argv,
			    env,
			    OPT_STRING,
#if HAVE_GETOPT_LONG
			    get_long_opt(),
#endif
			    'o',  // stop looking for -j and -Q once -o option is met on command-line
			    &little_main);
}

S_I little_main(shared_ptr<user_interaction> & dialog, S_I argc, char * const argv[], const char **env)
{
    operation op;
    string base;
    string arg;
    S_I num;
    archive_num num2;
    vector<string> rest;
    bool info_details;
    infinint date;
    database *dat = nullptr;
    bool partial_read = false;
    bool partial_read_only = false;
    bool ignore_dat_options;
    bool even_when_removed;
    bool check_order;
    compression algozip;
    U_I compression_level;
    bool change_compression;
    bool detailed_dates;
    shell_interaction *shelli = dynamic_cast<shell_interaction *>(dialog.get());

    if(!dialog)
	throw SRC_BUG;
    if(shelli == nullptr)
	throw SRC_BUG;

    shelli->change_non_interactive_output(cout);

    if(!command_line(*shelli,
		     argc,
		     argv,
		     op,
		     base,
		     arg,
		     num,
		     rest,
		     num2,
		     date,
		     info_details,
		     ignore_dat_options,
		     even_when_removed,
		     check_order,
		     algozip,
		     compression_level,
		     change_compression,
		     detailed_dates,
		     false))
	return EXIT_SYNTAX;

    if(op == none_op)
        throw SRC_BUG;

    switch(op)
    {
    case create:
	break;
    case add:
    case del:
    case restore:
    case used:
    case files:
    case stats:
    case moving:
    case interactive:
    case check:
    case batch:
	partial_read = false;
	break;
    case listing:
	partial_read_only = true;
	if(change_compression)
	    throw Erange("little_main", gettext("Cannot change compression when the requested operation is a read operation"));
	break;
    case chbase:
    case where:
    case options:
    case dar:
	partial_read = true;
	if(change_compression)
	    throw Erange("little_main", gettext("Cannot change compression when the requested operation only modifies the database header"));
	break;
    default:
	throw SRC_BUG;
    }

    if(op == create)
	op_create(dialog, base, algozip, compression_level, info_details);
    else
    {
	if(info_details)
	{
	    if(partial_read)
		dialog->message(gettext("Decompressing and loading database header to memory..."));
	    else
		dialog->message(gettext("Decompressing and loading database to memory..."));
	}
	dat = read_base(dialog, base, partial_read, partial_read_only, check_order);
	try
	{
	    try
	    {
		action(dialog, op, dat, arg, num, rest, num2, date, base, info_details, true, ignore_dat_options, even_when_removed, detailed_dates);
		finalize(dialog, op, dat, base, info_details, change_compression, algozip, compression_level);
	    }
	    catch(Edata & e)
	    {
		dialog->message(string(gettext("Error met while processing operation: ")) + e.get_message());
		finalize(dialog, op, dat, base, info_details, change_compression, algozip, compression_level);
		throw;
	    }
	}
	catch(...)
	{
	    if(dat != nullptr)
		delete dat;
	    throw;
	}
	if(dat != nullptr)
	    delete dat;
    }

    return EXIT_OK;
}

static bool command_line(shell_interaction & dialog,
			 S_I argc, char *const argv[],
                         operation & op,
                         string & base,
			 string & arg,
                         S_I & num,
                         vector<string> & rest,
                         archive_num & num2,
			 infinint & date,
                         bool & verbose,
			 bool & ignore_dat_options,
			 bool & even_when_removed,
			 bool & check_order,
			 compression & algozip,
			 U_I & compression_level,
			 bool & change_compression,
			 bool & detailed_dates,
			 bool recursive)
{
    S_I lu, min;
    U_I max;
    U_I compr_bs; // block compression is not used in dar_manager
    enum { date_not_set, date_set_by_w, date_set_by_9 } date_set = date_not_set;

    base = arg = "";
    num = num2 = 0;
    rest.clear();
    op = none_op;
    verbose = false;
    string chem, filename;
    date = 0;
    ignore_dat_options = false;
    even_when_removed = false;
    check_order = true;
    algozip = compression::gzip;
    compression_level = 9;
    change_compression = false;
    detailed_dates = false;
    string extra = "";

    try
    {

	(void)line_tools_reset_getopt();
#if HAVE_GETOPT_LONG
	while((lu = getopt_long(argc, argv, OPT_STRING, get_long_opt(), nullptr)) != EOF)
#else
	    while((lu = getopt(argc, argv, OPT_STRING)) != EOF)
#endif
	    {
		switch(lu)
		{
		case 'C':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = create;
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    base = optarg;
		    break;
		case 'B':
		    if(recursive)
			throw Erange("command_line", tools_printf(gettext("-B option cannot be given inside a batch file")));
		    if(base != "")
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    base = optarg;
		    break;
		case 'A':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = add;
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    line_tools_split_path_basename(optarg, chem, filename);
		    line_tools_check_basename(dialog, chem, filename, EXTENSION, false);
		    arg = (path(chem).append(filename)).display();
		    break;
		case 'l':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = listing;
		    break;
		case 'D':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = del;
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    try
		    {
			line_tools_read_range(string(optarg), min, max);
		    }
		    catch(Edeci & e)
		    {
			throw Erange("command_line", tools_printf(gettext(INVALID_ARG), char(lu)));
		    }
		    num = min;
		    num2 = max;
		    break;
		case 'b':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = chbase;
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    num = line_tools_str2signed_int(optarg);
		    break;
		case 'p':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = where;
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    num = line_tools_str2signed_int(optarg);
		    break;
		case 'o':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = options;
		    break;
		case 'd':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = dar;
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    arg = optarg;
		    break;
		case 'r':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = restore;
		    break;
		case 'u':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = used;
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    num = line_tools_str2signed_int(optarg);
		    break;
		case 'f':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = files;
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    arg = optarg;
		    break;
		case 's':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = stats;
		    break;
		case 'm':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = moving;
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    num = tools_str2int(optarg);
		    break;
		case 'h':
		    show_usage(dialog, argv[0]);
		    return false;
		case 'V':
		    show_version(dialog, argv[0]);
		    return false;
		case 'v':
		    verbose = true;
		    break;
		case 'Q':
		    break;  // ignore this option already parsed during initialization (dar_suite.cpp)
		case 'w':
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    date = line_tools_convert_date(optarg);
		    date_set = date_set_by_w;
		    break;
		case 'i':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = interactive;
		    break;
		case 'c':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = check;
		    break;
		case 'e':
		    if(extra != "")
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    extra = optarg;
		    break;
		case '@':
		    if(recursive)
			throw Erange("command_line", tools_printf(gettext("Running batch file from a batch file is not allowed")));
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = batch;
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    arg = optarg;
		    break;
		case 'N':
		    ignore_dat_options = true;
		    break;
		case 'k':
		    even_when_removed = true;
		    break;
		case '9':
		    try
		    {
			    // note that the namespace specification is necessary
			    // due to similar existing name in std namespace under
			    // certain OS (FreeBSD 10.0)
			libdar::deci tmp = string(optarg);
			date = tmp.computer();
			date_set = date_set_by_9;
		    }
		    catch(Edeci & e)
		    {
			throw Erange("command_line", tools_printf(gettext("invalid number given to -9 option: %s"), optarg));
		    }
		    break;
		case 'a':
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    if(strcasecmp("i", optarg) == 0 || strcasecmp("ignore-order", optarg) == 0)
			check_order = false;
		    else
			throw Erange("command_line", tools_printf(gettext(INVALID_ARG), char(lu)));
		    break;
		case 'z':
		    if(optarg == nullptr)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));

		    line_tools_split_compression_algo(optarg,
						      1,         // no prefix (used for block anyway)
						      algozip,
						      compression_level,
						      compr_bs); // we ignore this in dar_manager
		    change_compression = true;
		    break;
		case 'x':
		    detailed_dates = true;
		    break;
		case ':':
		    throw Erange("get_args", tools_printf(gettext(MISSING_ARG), char(optopt)));
		case '?':
		    throw Erange("get_args", tools_printf(gettext("Ignoring unknown option -%c"), char(optopt)));
		default:
		    throw Erange("get_args", tools_printf(gettext("Ignoring unknown option -%c"), char(lu)));
		}
		if(lu == 'o' || lu == 'r')
		    break; // stop reading arguments
	    }


	    // related to bug #1598138 on sourceforge
	    // when providing -o'-B dar.dcf' on command line getopt
	    // finds -o option with no argument and keep optind pointing on
	    // the -o'B dar dcf' argument. We must thus tail out the leading -o
	    // from this argument before feeding the 'rest' list variable
	if(optind < argc && strncmp(argv[optind], "-o", 2) == 0)
	{
	    rest.push_back(argv[optind]+2); // argv[optind] is a 'char *' applying '+2' to it skips the 2 leading char '-o'.
	    optind++;
	}

	for(S_I i = optind; i < argc; ++i)
	    rest.push_back(argv[i]);

	    // sanity checks

	if(extra != "" && op != restore)
	{
	    dialog.message(gettext("-e option is only available when using -r option, aborting"));
	    return false;
	}

	switch(date_set)
	{
	case date_not_set:
	    break;
	case date_set_by_w:
	    if(op != restore)
	    {
		dialog.message(gettext("-w option is only valid with -r option, ignoring it"));
		date = 0;
	    }
	    break;
	case date_set_by_9:
	    if(op != add)
	    {
		dialog.message(gettext("-9 option is only valid with -A option, ignoring it"));
		date = 0;
	    }
	    break;
	default:
	    throw SRC_BUG;
	}

	switch(op)
	{
	case none_op:
	    dialog.message(gettext("No action specified, aborting"));
	    return false;
	case create:
	case listing:
	case del:
	case dar:
	case used:
	case files:
	case stats:
	case interactive:
	case check:
	    if(!rest.empty())
		dialog.message(gettext("Ignoring extra arguments on command line"));
	    break;
	case add:
	    if(rest.size() > 1)
		dialog.message(gettext("Ignoring extra arguments on command line"));
	    if(date_set == date_not_set) // --min-digits not set
	    {
		line_tools_check_min_digits(dialog, chem, filename, EXTENSION, date);
		date_set = date_set_by_9;
	    }
	    break;
	case chbase:
	case where:
	    if(rest.size() != 1)
	    {
		dialog.message(gettext("Missing argument to command line, aborting"));
		return false;
	    }
	    arg = rest[0];
	    rest.clear();
	    break;
	case restore:
	    for(unsigned int i = 0; i < rest.size(); ++i)
		if(!path(rest[i]).is_relative())
		    throw Erange("command_line", gettext("Arguments to -r must be relative path (never begin by '/')"));
	    arg = extra;
	    break;
	case options:
	    break;
	case moving:
	    if(rest.size() != 1)
	    {
		dialog.message(gettext("Missing argument to command line, aborting"));
		return false;
	    }
	    num2 = tools_str2int(rest[0]);
	    rest.clear();
	    break;
	case batch:
	    break;
	default:
	    throw SRC_BUG;
	}

	if(base == "" && !recursive)
	{
	    dialog.message(gettext("No database specified, aborting"));
	    return false;
	}
    }
    catch(Erange & e)
    {
        dialog.message(string(gettext("Parse error on command line (or included files): ")) + e.get_message());
        return false;
    }

    return true;
}

static void op_create(shared_ptr<user_interaction> & dialog, const string & base, compression algozip, U_I compr_level, bool info_details)
{
    database dat(dialog); // created empty;

    if(info_details)
    {
        dialog->message(gettext("Creating file..."));
        dialog->message(gettext("Formatting file as an empty database..."));
    }
    write_base(dialog, base, &dat, false, true, algozip, compr_level);
    if(info_details)
        dialog->message(gettext("Database has been successfully created empty."));
}

static void op_add(shared_ptr<user_interaction> & dialog, database *dat, const string &arg, string fake, const infinint & min_digits, bool info_details)
{
    thread_cancellation thr;
    string arch_path, arch_base;
    bool date_order_problem = false;
    archive_options_read read_options;

    if(dat == nullptr)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog->message(gettext("Reading catalogue of the archive to add..."));
    line_tools_split_path_basename(arg, arch_path, arch_base);
    read_options.set_info_details(info_details);
    read_options.set_slice_min_digits(min_digits);
    archive *arch = new (nothrow) archive(dialog, path(arch_path), arch_base, EXTENSION, read_options);
    if(arch == nullptr)
	throw Ememory("dar_manager.cpp:op_add");

    try
    {
	string chemin, b;

	thr.check_self_cancellation();
	if(info_details)
	    dialog->message(gettext("Updating database with catalogue..."));
	if(fake == "")
	    fake = arg;
	line_tools_split_path_basename(fake, chemin, b);
	dat->add_archive(*arch, chemin, b, database_add_options());
	thr.check_self_cancellation();
	if(info_details)
	    dialog->message(gettext("Checking date ordering of files between archives..."));
	date_order_problem = !dat->check_order();
	thr.check_self_cancellation();
    }
    catch(...)
    {
	if(arch != nullptr)
	    delete arch;
	throw;
    }
    delete arch;

    if(date_order_problem)
	throw Edata(gettext("Some files do not follow chronological order when archive index increases withing the database, this can lead dar_manager to restored a wrong version of these files"));
}

static void op_listing(shell_interaction & dialog, const database *dat, bool info_details)
{
    if(dat == nullptr)
	throw SRC_BUG;

    dialog.database_show_contents(*dat);
}

static void op_del(shared_ptr<user_interaction> & dialog, database *dat, S_I min, archive_num max, bool info_details)
{
    thread_cancellation thr;
    database_remove_options opt;
    bool sign_plus;
    archive_num rmin;

    signed_int_to_archive_num(min, rmin, sign_plus);
    opt.set_revert_archive_numbering(!sign_plus);
    if(!sign_plus)
	max = rmin;
    if(dat == nullptr)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog->message(gettext("Removing information from the database..."));
    dat->remove_archive(rmin, max, opt);
    thr.check_self_cancellation();
}

static void op_chbase(shared_ptr<user_interaction> & dialog, database *dat, S_I num, const string & arg, bool info_details)
{
    thread_cancellation thr;
    database_change_basename_options opt;
    bool sign_plus;
    archive_num rnum;

    signed_int_to_archive_num(num, rnum, sign_plus);
    opt.set_revert_archive_numbering(!sign_plus);

    if(dat == nullptr)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog->message(gettext("Changing database header information..."));
    dat->change_name(rnum, arg, opt);
    thr.check_self_cancellation();
}

static void op_where(shared_ptr<user_interaction> & dialog, database *dat, S_I num, const string & arg, bool info_details)
{
    thread_cancellation thr;
    database_change_path_options opt;
    bool sign_plus;
    archive_num rnum;

    signed_int_to_archive_num(num, rnum, sign_plus);
    opt.set_revert_archive_numbering(!sign_plus);

    if(dat == nullptr)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog->message(gettext("Changing database header information..."));
    dat->set_path(rnum, arg, opt);
    thr.check_self_cancellation();
}

static void op_options(shared_ptr<user_interaction> & dialog, database *dat, const vector<string> & rest, bool info_details)
{
    thread_cancellation thr;

    if(dat == nullptr)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog->message(gettext("Changing database header information..."));
    dat->set_options(rest);
    thr.check_self_cancellation();
}

static void op_dar(shared_ptr<user_interaction> & dialog, database *dat, const string & arg, bool info_details)
{
    thread_cancellation thr;

    if(dat == nullptr)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog->message(gettext("Changing database header information..."));
    dat->set_dar_path(arg);
    thr.check_self_cancellation();
}

static void op_restore(shared_ptr<user_interaction> & dialog, database *dat, const vector<string> & rest, const infinint & date, const string & options_for_dar, bool info_details, bool early_release, bool ignore_dar_options_in_base, bool even_when_removed)
{
    thread_cancellation thr;
    vector<string> options;
    database_restore_options dat_opt;

    if(dat == nullptr)
	throw SRC_BUG;

    line_tools_split_in_words(options_for_dar, options);
    thr.check_self_cancellation();
    if(info_details)
	dialog->message(gettext("Looking in archives for requested files, classifying files archive by archive..."));
    dat_opt.set_early_release(early_release);
    dat_opt.set_info_details(info_details);
    dat_opt.set_date(date);
    dat_opt.set_extra_options_for_dar(options);
    dat_opt.set_ignore_dar_options_in_database(ignore_dar_options_in_base);
    dat_opt.set_even_when_removed(even_when_removed);
    dat->restore(rest, dat_opt);
}

static void op_used(shell_interaction & dialog, const database *dat, S_I num, bool info_details)
{
    thread_cancellation thr;
    database_used_options opt;
    bool sign_plus;
    archive_num rnum;

    signed_int_to_archive_num(num, rnum, sign_plus);
    opt.set_revert_archive_numbering(!sign_plus);

    if(dat == nullptr)
	throw SRC_BUG;

    thr.check_self_cancellation();
    dialog.database_show_files(*dat, rnum, opt);
}

static void op_files(shell_interaction & dialog,
		     const database *dat,
		     const string & arg,
		     bool info_details,
		     bool detailed_dates)
{
    thread_cancellation thr;

    if(dat == nullptr)
	throw SRC_BUG;

    thr.check_self_cancellation();
    dialog.set_fully_detailed_datetime(detailed_dates);
    dialog.database_show_version(*dat, arg);
}

static void op_stats(shell_interaction & dialog, const database *dat, bool info_details)
{
    thread_cancellation thr;

    if(dat == nullptr)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog.message(gettext("Computing statistics..."));
    dialog.database_show_statistics(*dat);
}

static void op_move(shared_ptr<user_interaction> & dialog, database *dat, S_I src, archive_num dst, bool info_details)
{
    thread_cancellation thr;
    bool date_order_problem = false;

    if(src <= 0)
	throw Erange("op_move", gettext("Negative number or zero not allowed when moving an archive inside a database"));

    if(dat == nullptr)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog->message(gettext("Changing database information..."));
    dat->set_permutation(src, dst);
    thr.check_self_cancellation();
    if(info_details)
	dialog->message(gettext("Checking date ordering of files between archives..."));
    date_order_problem = !dat->check_order();
    thr.check_self_cancellation();
    if(date_order_problem)
	throw Edata(gettext("Some files do not follow chronological order when archive index increases withing the database, this can lead dar_manager to restored a wrong version of these files"));
}

static void show_usage(shell_interaction & dialog, const char *command)
{
    dialog.printf("usage :\n\n");
    dialog.printf("\tdar_manager [options] -C [<path>/]<database>\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -A [<path>/]<basename> [[<path>/]<archive_basename>]\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -l\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -D <number>\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -b <number> <new_archive_basename>\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -p <number> <path>\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -o [list of options to pass to dar]\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -d [<path to dar command>]\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> [-w <date>] [-e \"<options to dar>\"] -r [list of files to restore]\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -u <number>\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -f file\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -s\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -m <number> <number>\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -i\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -c\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -@ <filename>\n");
    dialog.printf("\tdar_manager -h\n");
    dialog.printf("\tdar_manager -V\n");
    dialog.printf("\n");
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("Commands:\n"));
    dialog.printf(gettext("   -C <database>   creates an empty database\n"));
    dialog.printf(gettext("   -B <database>   specify the database to use (read or modify)\n"));
    dialog.printf(gettext("   -A <archive>    add an archive to the database\n"));
    dialog.printf(gettext("   -l\t\t   gives information about the archive compiled in the database\n"));
    dialog.printf(gettext("   -D <number[-number]> delete an archive from the database\n"));
    dialog.printf(gettext("   -b <number>\t   change the basename to use for the give archive number\n"));
    dialog.printf(gettext("   -p <number>\t   change the path to use for the given archive number\n"));
    dialog.printf(gettext("   -o <options>    specify a list of option to always pass to dar\n"));
    dialog.printf(gettext("   -d <dar's path> specify the path to dar\n"));
    dialog.printf(gettext("   -r <files>\t   restores the given files\n"));
    dialog.printf(gettext("   -w <date>\t   only with -r, restores in state just before the given date\n"));
    dialog.printf(gettext("            \t   date format: [[[year/]month]/day-]hour:minute[:second]\n"));
    dialog.printf(gettext("   -u <number>\t   list the most recent files contained in the given archive\n"));
    dialog.printf(gettext("   -f <file>\t   list the archives where the given file is present\n"));
    dialog.printf(gettext("   -s\t\t   shows the number of most recent file by archive\n"));
    dialog.printf(gettext("   -m <number>\t   move an archive within a given database.\n"));
    dialog.printf(gettext("   -i\t\t   user interactive mode\n"));
    dialog.printf(gettext("   -c\t\t   check database for dates order\n"));
    dialog.printf(gettext("   -@ <filename>   execute on a given database a batch of action as defined by\n"));
    dialog.printf(gettext("\t\t   the provided file.\n"));
    dialog.printf(gettext("   -h\t\t   displays this help information\n"));
    dialog.printf(gettext("   -V\t\t   displays software version\n"));
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("Options:\n"));
    dialog.printf(gettext("   -v\t\t   display more information about what is going on\n"));
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("See man page for more options.\n"));
}

static void show_version(shell_interaction & dialog, const char *command_name)
{
    string name;
    line_tools_extract_basename(command_name, name);
    U_I maj, med, min;

    get_version(maj, med, min);
    dialog.change_non_interactive_output(cout);
    dialog.printf("\n %s version %s, Copyright (C) 2002-2025 Denis Corbin\n", name.c_str(), DAR_MANAGER_VERSION);
    dialog.message(string("   ") + dar_suite_command_line_features() + "\n");
    if(maj > 2)
	dialog.printf(gettext(" Using libdar %u.%u.%u built with compilation time options:\n"), maj, med, min);
    else
	dialog.printf(gettext(" Using libdar %u.%u built with compilation time options:\n"), maj, min);
    line_tools_display_features(dialog);
    dialog.printf("\n");
    dialog.printf(gettext(" compiled the %s with %s version %s\n"), __DATE__, CC_NAT, __VERSION__);
    dialog.printf(gettext(" %s is part of the Disk ARchive suite (Release %s)\n"), name.c_str(), PACKAGE_VERSION);
    dialog.message(tools_printf(gettext(" %s comes with ABSOLUTELY NO WARRANTY; for details\n type `%s -W'."), name.c_str(), "dar")
		   + tools_printf(gettext(" This is free software, and you are welcome\n to redistribute it under certain conditions;"))
		   + tools_printf(gettext(" type `%s -L | more'\n for details.\n\n"), "dar"));
    dialog.printf("");
}

#if HAVE_GETOPT_LONG
static const struct option *get_long_opt()
{
    static const struct option ret[] = {
        {"create", required_argument, nullptr, 'C'},
        {"base", required_argument, nullptr, 'B'},
        {"add", required_argument, nullptr, 'A'},
        {"list", no_argument, nullptr, 'l'},
        {"delete", required_argument, nullptr, 'D'},
        {"basename", required_argument, nullptr, 'b'},
        {"path", required_argument, nullptr, 'p'},
        {"options", no_argument, nullptr, 'o'},
        {"dar", no_argument, nullptr, 'd'},
        {"restore", no_argument, nullptr, 'r'},
        {"used", required_argument, nullptr, 'u'},
        {"file", required_argument, nullptr, 'f'},
        {"stats", no_argument, nullptr, 's'},
        {"help", no_argument, nullptr, 'h'},
        {"version", no_argument, nullptr, 'V'},
        {"verbose", no_argument, nullptr, 'v'},
	{"jog", no_argument, nullptr, 'j'},
	{"when", required_argument, nullptr, 'w'},
	{"interactive", no_argument, nullptr, 'i'},
	{"extra", required_argument, nullptr, 'e'},
	{"check", no_argument, nullptr, 'c'},
 	{"batch", required_argument, nullptr, '@'},
	{"ignore-options-in-base", no_argument, nullptr, 'N'},
	{"min-digits", required_argument, nullptr, '9'},
	{"ignore-when-removed", no_argument, nullptr, 'k'},
	{"alter", required_argument, nullptr, 'a'},
	{"compression", required_argument, nullptr, 'z'},
        { nullptr, 0, nullptr, 0 }
    };

    return ret;
}
#endif

static database *read_base(shared_ptr<user_interaction> & dialog, const string & base, bool partial, bool partial_read_only, bool check_order)
{
    database *ret = nullptr;

    try
    {
	database_open_options dat_opt;
	dat_opt.set_warn_order(check_order);
	dat_opt.set_partial(partial);
	dat_opt.set_partial_read_only(partial_read_only);
        ret = new (nothrow) database(dialog, base, dat_opt);
        if(ret == nullptr)
            throw Ememory("read_base");
    }
    catch(Erange & e)
    {
        string msg = string(gettext("Corrupted database :"))+e.get_message();
        dialog->message(msg);
        throw Edata(msg);
    }

    return ret;
}

static void write_base(shared_ptr<user_interaction> & dialog,
		       const string & filename,
		       const database *base,
		       bool overwrite,
		       bool change_compression,
		       compression algozip,
		       U_I compression_level)
{
    thread_cancellation thr;
    database_dump_options dat_opt;

    dat_opt.set_overwrite(overwrite);
    if(change_compression)
    {
	base->set_compression(algozip);
	base->set_compression_level(compression_level);
    }
    thr.block_delayed_cancellation(true);
    base->dump(filename, dat_opt);
    thr.block_delayed_cancellation(false);
}

static void op_interactive(shared_ptr<user_interaction> & dialog, database *dat, string base)
{
    char choice;
    bool saved = true;
    thread_cancellation thr;
    archive_num num, num2;
    S_I tmp_si;
    bool tmp_sign;
    string input, input2;
    archive *arch = nullptr;
    vector <string> vectinput;
    bool change_compression = false;
    compression zipname;
    U_I compr_level = 9;
    U_I more = 25;
    database_change_basename_options opt_change_name;
    database_change_path_options opt_change_path;
    database_remove_options opt_remove;
    database_used_options opt_used;
    shell_interaction *shelli = dynamic_cast<shell_interaction *>(dialog.get());

    if(dat == nullptr)
	throw SRC_BUG;
    if(shelli == nullptr)
	throw SRC_BUG;

    thr.check_self_cancellation();
    do
    {
	try
	{
	    archive_options_read read_options;

		// diplay choice message

	    shelli->warning_with_more(0);
	    dialog->printf(gettext("\n\n\t Dar Manager Database used [%s] : %S\n"), saved ? gettext("Saved") : gettext("Not Saved"), &base);
	    if(more > 0)
		dialog->printf(gettext("\t Pause each %d line of output\n\n"), more);
	    else
		dialog->printf(gettext("\t No pause in output\n\n"));
	    dialog->printf(gettext(" l : list database contents  \t A : Add an archive\n"));
	    dialog->printf(gettext(" u : list archive contents   \t D : Remove an archive\n"));
	    dialog->printf(gettext(" f : give file localization  \t m : modify archive order\n"));
	    dialog->printf(gettext(" p : modify path of archives \t b : modify basename of archives\n"));
	    dialog->printf(gettext(" d : path to dar             \t o : options to dar\n"));
	    dialog->printf(gettext(" w : write changes to file   \t s : database statistics\n"));
	    dialog->printf(gettext(" c : check date order        \t z : change compression algorithm\n"));
	    dialog->printf(gettext(" a : Save as                 \t n : pause each 'n' line (zero for no pause)\n\n"));
	    dialog->printf(gettext(" q : quit\n\n"));
	    dialog->printf(gettext(" Choice: "));

		// user's choice selection

	    shelli->read_char(choice);
	    thr.check_self_cancellation();
	    dialog->printf("\n\n");

		// performing requested action

	    shelli->warning_with_more(more);
	    switch(choice)
	    {
	    case 'l':
		shelli->database_show_contents(*dat);
		break;
	    case 'u':
		input = dialog->get_string(gettext("Archive number: "), true);
		tmp_si = line_tools_str2signed_int(input);
		signed_int_to_archive_num(tmp_si, num, tmp_sign);
		opt_used.set_revert_archive_numbering(!tmp_sign);
		shelli->database_show_files(*dat, num, opt_used);
		break;
	    case 'f':
		input = dialog->get_string(gettext("File to look for: "), true);
		shelli->database_show_version(*dat, input);
		break;
	    case 'b':
		input = dialog->get_string(gettext("Archive number to modify: "), true);
		tmp_si = line_tools_str2signed_int(input);
		signed_int_to_archive_num(tmp_si, num, tmp_sign);
		opt_change_name.set_revert_archive_numbering(!tmp_sign);
		input = dialog->get_string(tools_printf(gettext("New basename for archive number %d: "), tmp_si), true);
		dat->change_name(num, input, opt_change_name);
		saved = false;
		break;
	    case 'd':
		input = dialog->get_string(gettext("Path to dar (empty string to use the default from PATH variable): "), true);
		dat->set_dar_path(input);
		saved = false;
		break;
	    case 'z':
		input = dialog->get_string(gettext("New compression algorithm to use (none, gzip, bzip2, lzo, xz, zstd, lz4...)? "), true);
		zipname = string2compression(input);
		input = dialog->get_string(gettext("Compression level to use (usually an integer from 1 to 9)? "), true);
		tmp_si = line_tools_str2signed_int(input);
		if(tmp_si < 1)
		    throw Erange("op_interactive", gettext("Compression level must be strictly positive"));
		compr_level = (U_I)tmp_si;
		dialog->message(gettext("Note: the new compression algorithm/level will be used when saving the database"));
		change_compression = true;
		saved = false;
		break;
	    case 'w':
		dialog->message(gettext("Compressing and writing back database to file..."));
		write_base(dialog, base, dat, true, change_compression, zipname, compr_level);
		saved = true;
		break;
	    case 'a':
		input = dialog->get_string(gettext("New database name: "), true);
		dialog->message(gettext("Compressing and writing back database to file..."));
		write_base(dialog, input, dat, false, change_compression, zipname, compr_level);
		base = input;
		saved = true;
		break;
	    case 'A':
		input = dialog->get_string(gettext("Archive basename (or extracted catalogue basename) to add: "), true);
		dialog->message(gettext("Reading catalogue of the archive to add..."));
		line_tools_split_path_basename(input, input, input2);
		read_options.clear();
		read_options.set_info_details(true);
		arch = new (nothrow) archive(dialog, path(input), input2, EXTENSION, read_options);
		if(arch == nullptr)
		    throw Ememory("dar_manager.cpp:op_interactive");
		try
		{
		    dialog->message(gettext("Updating database with catalogue..."));
		    dat->add_archive(*arch, input, input2, database_add_options());
		    thr.check_self_cancellation();
		    dialog->message(gettext("Checking date ordering of files between archives..."));
		    (void)dat->check_order();
		}
		catch(...)
		{
		    delete arch;
		    arch = nullptr;
		    throw;
		}
		delete arch;
		arch = nullptr;
		saved = false;
		break;
	    case 'D':
		input = dialog->get_string(gettext("Archive number to remove: "), true);
		tmp_si = line_tools_str2signed_int(input);
		signed_int_to_archive_num(tmp_si, num, tmp_sign);
		opt_remove.set_revert_archive_numbering(!tmp_sign);
		dialog->pause(tools_printf(gettext("Are you sure to remove archive number %d ?"), tmp_si));
		dialog->message(gettext("Removing information from the database..."));
		dat->remove_archive(num, num, opt_remove);
		saved = false;
		break;
	    case 'm':
		input = dialog->get_string(gettext("Archive number to move: "), true);
		num = tools_str2int(input);
		input = dialog->get_string(gettext("In which position to insert this archive: "), true);
		num2 = tools_str2int(input);
		dat->set_permutation(num, num2);
		thr.check_self_cancellation();
		dialog->message(gettext("Checking date ordering of files between archives..."));
		dat->check_order();
		saved = false;
		break;
	    case 'p':
		input = dialog->get_string(gettext("Archive number who's path to modify: "), true);
		tmp_si = line_tools_str2signed_int(input);
		signed_int_to_archive_num(tmp_si, num, tmp_sign);
		opt_change_path.set_revert_archive_numbering(!tmp_sign);
		input = dialog->get_string(tools_printf(gettext("New path to give to archive number %d: "), tmp_si), true);
		dat->set_path(num, input, opt_change_path);
		saved = false;
		break;
	    case 'o':
		vectinput = read_vector(dialog);
		dat->set_options(vectinput);
		saved = false;
		break;
	    case 's':
		dialog->message(gettext("Computing statistics..."));
		shelli->database_show_statistics(*dat);
		break;
	    case 'n':
		input = dialog->get_string(gettext("How much line to display at once: "), true);
		more = tools_str2int(input);
		break;
	    case 'c':
		dialog->message(gettext("Checking file's dates ordering..."));
		(void)dat->check_order();
		break;
	    case 'q':
		if(!saved)
		{
		    try
		    {
			dialog->pause(gettext("Database not saved, Do you really want to quit ?"));
			dialog->printf(gettext("Continuing the action under process which is to exit... so we exit!"));
		    }
		    catch(Euser_abort & e)
		    {
			choice = ' ';
		    }
		}
		break;
	    default:
		dialog->printf(gettext("Unknown choice\n"));
	    }
	}
	catch(Ethread_cancel & e)
	{
	    bool quit = false;

	    if(!saved)
	    {
		try
		{
		    dialog->pause(gettext("Database not saved, Do you really want to quit ?"));
		    dialog->printf(gettext("Continuing the action under process which is to exit... so we exit!"));
		    quit = true;
		}
		catch(Euser_abort & e)
		{
		    quit = false;
		}
	    }
	    else
		quit = true;
	    if(quit)
		throw;
	    else
	    {
		dialog->printf(gettext("re-enabling all signal handlers and continuing\n"));
		dar_suite_reset_signal_handler();
	    }
	}
	catch(Egeneric & e)
	{
	    string tmp = e.get_message();
	    dialog->message(tools_printf(gettext("Error performing the requested action: %S"), &tmp));
	}
    }
    while(choice != 'q');
    if(arch != nullptr)
	throw SRC_BUG;
}

static void op_check(shared_ptr<user_interaction> & dialog, const database *dat, bool info_details)
{
    thread_cancellation thr;

    if(dat == nullptr)
	throw SRC_BUG;

    if(info_details)
	dialog->message(gettext("Checking date ordering of files between archives..."));
    if(!dat->check_order())
	throw Edata(gettext("Some files do not follow chronological order when archive index increases withing the database, this can lead dar_manager to restored a wrong version of these files"));
    else
	dialog->message(gettext("No problem found"));
    thr.check_self_cancellation();
}

static void op_batch(shared_ptr<user_interaction> & dialog, database *dat, const string & filename, bool info_details)
{
    const string pseudo_cmd = "dar_manager"; // to take the place of the command on the simulated command-line
    string line;
    char tmp;
    bool sub_info_details;
    deque<string> mots;
    argc_argv cmdline;
    operation sub_op;
    string faked_base;
    archive_num num2;
    S_I num;
    vector<string> rest;
    infinint date;
    string arg;
    bool ignore_dat_options;
    bool even_when_removed;
    bool check_order; // not used here
    compression algozip; // not used here neither but at the level of the function that called op_batch
    U_I compression_level; // not used here neither but at the level of the function that called op_batch
    bool change_compression; // not used here neither but at the level of the function that called op_batch
    bool detailed_dates;
    shell_interaction *shelli = dynamic_cast<shell_interaction *>(dialog.get());

    if(shelli == nullptr)
	throw SRC_BUG;

    if(dat == nullptr)
	throw SRC_BUG;

	// openning the batch file

    if(info_details)
	dialog->message(gettext("Opening and reading the batch file..."));

    fichier_local batch_file = fichier_local(filename, false);
    no_comment proper = no_comment(batch_file); // removing the comments from file

	// reading it line by line and executing corresponding action

    try
    {
	do
	{
	    line = "";
	    tmp = 'x'; // something different from '\n' for the end of current loop be properly detected

		// extracting the next line
	    while(proper.read(&tmp,  1) == 1 && tmp != '\n')
		line += tmp;

	    line_tools_split_in_words(line, mots);

	    if(mots.size() == 0)
		continue;

	    if(info_details)
		dialog->printf(gettext("\n\tExecuting batch file line: %S\n "), &line);

	    cmdline.resize(mots.size() + 1); // +1 of pseudo_command
	    cmdline.set_arg(pseudo_cmd, 0);

	    for(U_I i = 0; i < mots.size(); ++i)
		cmdline.set_arg(mots[i], i+1);

	    if(!command_line(*shelli,
			     cmdline.argc(),
			     cmdline.argv(),
			     sub_op,
			     faked_base,
			     arg,
			     num,
			     rest,
			     num2,
			     date,
			     sub_info_details,
			     ignore_dat_options,
			     even_when_removed,
			     check_order,
			     algozip,
			     compression_level,
			     change_compression,
			     detailed_dates,
			     true))
		throw Erange("op_batch", tools_printf(gettext("Syntax error in batch file: %S"), &line));

	    if(sub_op == create)
		throw Erange("op_batch", gettext("Syntax error in batch file: -C option not allowed"));

	    if(sub_op == interactive)
		throw Erange("op_batch", gettext("Syntax error in batch file: -i option not allowed"));

	    action(dialog, sub_op, dat, arg, num, rest, num2, date, faked_base, sub_info_details, false, ignore_dat_options, even_when_removed, detailed_dates);
	}
	while(tmp == '\n');
    }
    catch(Edata & e)
    {
	e.prepend_message(gettext("Aborting batch operation: "));
	throw;
    }
}

static vector<string> read_vector(shared_ptr<user_interaction> & dialog)
{
    vector<string> ret;
    string tmp;

    ret.clear();
    dialog->printf(gettext("Enter each argument line by line, press return at the end\n"));
    dialog->printf(gettext("To terminate enter an empty line\n"));

    do
    {
	tmp = dialog->get_string(" > ", true);
	if(tmp != "")
	    ret.push_back(tmp);
    }
    while(tmp != "");

    return ret;
}

static void finalize(shared_ptr<user_interaction> & dialog,
		     operation op,
		     database *dat,
		     const string & base,
		     bool info_details,
		     bool change_compression,
		     compression new_compression,
		     U_I compression_level)
{
    switch(op)
    {
    case create:
    case listing:
    case restore:
    case used:
    case files:
    case stats:
    case interactive:
    case check:
	break;
    case add:
    case del:
    case chbase:
    case where:
    case options:
    case dar:
    case moving:
    case batch:
	if(info_details)
	    dialog->message(gettext("Compressing and writing back database to file..."));
	write_base(dialog,
		   base,
		   dat,
		   true, // overwriting
		   change_compression,
		   new_compression,
		   compression_level);
	break;
    default:
	throw SRC_BUG;
    }
}

static void action(shared_ptr<user_interaction> & dialog,
		   operation op,
		   database *dat,
		   const string & arg,
		   S_I num,
		   const vector<string> & rest,
		   archive_num num2,
		   const infinint & date,
		   const string & base,
		   bool info_details,
		   bool early_release,
		   bool ignore_database_options,
		   bool even_when_removed,
		   bool detailed_dates)
{
    shell_interaction *shelli = dynamic_cast<shell_interaction *>(dialog.get());

    if(shelli == nullptr)
	throw SRC_BUG;

    switch(op)
    {
    case none_op:
	throw SRC_BUG;
    case create:
	throw SRC_BUG;
    case add:
	op_add(dialog, dat, arg, rest.empty() ? "" : rest[0], date, info_details);
	break;
    case listing:
	op_listing(*shelli, dat, info_details);
	break;
    case del:
	op_del(dialog, dat, num, num2, info_details);
	break;
    case chbase:
	op_chbase(dialog, dat, num, arg, info_details);
	break;
    case where:
	op_where(dialog, dat, num, arg, info_details);
	break;
    case options:
	op_options(dialog, dat, rest, info_details);
	break;
    case dar:
	op_dar(dialog, dat, arg, info_details);
	break;
    case restore:
	op_restore(dialog, dat, rest, date, arg, info_details, early_release, ignore_database_options, even_when_removed);
	break;
    case used:
	op_used(*shelli, dat, num, info_details);
	break;
    case files:
	op_files(*shelli, dat, arg, info_details, detailed_dates);
	break;
    case stats:
	op_stats(*shelli, dat, info_details);
	break;
    case moving:
	op_move(dialog, dat, num, num2, info_details);
	break;
    case interactive:
	op_interactive(dialog, dat, base);
	break;
    case check:
	op_check(dialog, dat, info_details);
	break;
    case batch:
	op_batch(dialog, dat, arg, info_details);
	break;
    default:
	throw SRC_BUG;
    }
}


static void signed_int_to_archive_num(S_I input, archive_num &num, bool & positive)
{
    if(input < 0)
    {
	positive = false;
	input = -input;
    }
    else
	positive = true;

    if(input < 0)
	throw SRC_BUG;
    num = input;
}

