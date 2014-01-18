/*********************************************************************/
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
#if HAVE_STRING_H
#include <string.h>
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

#include <vector>
#include <string>
#include <new>

#include "dar_suite.hpp"
#include "macro_tools.hpp"
#include "data_tree.hpp"
#include "database.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"
#include "libdar.hpp"
#include "shell_interaction.hpp"
#include "tools.hpp"
#include "thread_cancellation.hpp"
#include "archive.hpp"
#include "string_file.hpp"
#include "cygwin_adapt.hpp"
#include "no_comment.hpp"
#include "line_tools.hpp"
#include "fichier.hpp"
#include "deci.hpp"

using namespace libdar;

#define DAR_MANAGER_VERSION "1.7.6"


#define ONLY_ONCE "Only one -%c is allowed, ignoring this extra option"
#define MISSING_ARG "Missing argument to -%c"
#define INVALID_ARG "Invalid argument given to -%c (requires integer)"
#define OPT_STRING "C:B:A:lD:b:p:od:ru:f:shVm:vQjw:ie:c@:N;:ka:"

enum operation { none_op, create, add, listing, del, chbase, where, options, dar, restore, used, files, stats, moving, interactive, check, batch };

static S_I little_main(user_interaction & dialog, S_I argc, char *const argv[], const char **env);
static bool command_line(user_interaction & dialog,
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
			 bool recursive); // true if called from op_batch
static void show_usage(user_interaction & dialog, const char *command);
static void show_version(user_interaction & dialog, const char *command);

#if HAVE_GETOPT_LONG
static const struct option *get_long_opt();
#endif
static void op_create(user_interaction & dialog, const string & base, bool info_details);
static void op_add(user_interaction & dialog, database *dat, const string &arg, string fake, const infinint & min_digits, bool info_details);
static void op_listing(user_interaction & dialog, const database *dat, bool info_details);
static void op_del(user_interaction & dialog, database *dat, S_I min, archive_num max, bool info_details);
static void op_chbase(user_interaction & dialog, database *dat, S_I num, const string & arg, bool info_details);
static void op_where(user_interaction & dialog, database *dat, S_I num, const string & arg, bool info_details);
static void op_options(user_interaction & dialog, database *dat, const vector<string> & rest, bool info_details);
static void op_dar(user_interaction & dialog, database *dat, const string & arg, bool info_details);
static void op_restore(user_interaction & dialog,
		       database *dat,
		       const vector<string> & rest,
		       const infinint & date,
		       const string & options_for_dar,
		       bool info_details,
		       bool early_release,
		       bool ignore_dar_options_in_base,
		       bool even_when_removed);
static void op_used(user_interaction & dialog, const database *dat, S_I num, bool info_details);
static void op_files(user_interaction & dialog, const database *dat, const string & arg, bool info_details);
static void op_stats(user_interaction & dialog, const database *dat, bool info_details);
static void op_move(user_interaction & dialog, database *dat, S_I src, archive_num dst, bool info_details);
static void op_interactive(user_interaction & dialog, database *dat, string base);
static void op_check(user_interaction & dialog, const database *dat, bool info_details);
static void op_batch(user_interaction & dialog, database *dat, const string & filename, bool info_details);

static database *read_base(user_interaction & dialog,
			   const string & base,
			   bool partial,
			   bool partial_read_only,
			   bool check_order);
static void write_base(user_interaction & dialog, const string & filename, const database *base, bool overwrite);
static vector<string> read_vector(user_interaction & dialog);
static void finalize(user_interaction & dialog, operation op, database *dat, const string & base, bool info_details);
static void action(user_interaction & dialog,
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
		   bool even_when_removed);
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
			    &little_main);
}

S_I little_main(user_interaction & dialog, S_I argc, char * const argv[], const char **env)
{
    operation op;
    string base;
    string arg;
    S_I num;
    archive_num num2;
    vector<string> rest;
    bool info_details;
    infinint date;
    database *dat = NULL;
    bool partial_read = false;
    bool partial_read_only = false;
    bool ignore_dat_options;
    bool even_when_removed;
    bool check_order;

    shell_interaction_change_non_interactive_output(&cout);

    if(!command_line(dialog, argc, argv, op, base, arg, num, rest, num2, date, info_details, ignore_dat_options, even_when_removed, check_order, false))
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
	break;
    case chbase:
    case where:
    case options:
    case dar:
	partial_read = true;
	break;
    default:
	throw SRC_BUG;
    }

    if(op == create)
	op_create(dialog, base, info_details);
    else
    {
	if(info_details)
	{
	    if(partial_read)
		dialog.warning(gettext("Decompressing and loading database header to memory..."));
	    else
		dialog.warning(gettext("Decompressing and loading database to memory..."));
	}
	dat = read_base(dialog, base, partial_read, partial_read_only, check_order);
	try
	{
	    try
	    {
		action(dialog, op, dat, arg, num, rest, num2, date, base, info_details, true, ignore_dat_options, even_when_removed);
		finalize(dialog, op, dat, base, info_details);
	    }
	    catch(Edata & e)
	    {
		dialog.warning(string(gettext("Error met while processing operation: ")) + e.get_message());
		finalize(dialog, op, dat, base, info_details);
		throw;
	    }
	}
	catch(...)
	{
	    if(dat != NULL)
		delete dat;
	    throw;
	}
	if(dat != NULL)
	    delete dat;
    }

    return EXIT_OK;
}

static bool command_line(user_interaction & dialog,
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
			 bool recursive)
{
    S_I lu, min;
    U_I max;

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
    string extra = "";

    try
    {

	(void)line_tools_reset_getopt();
#if HAVE_GETOPT_LONG
	while((lu = getopt_long(argc, argv, OPT_STRING, get_long_opt(), NULL)) != EOF)
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
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    base = optarg;
		    break;
		case 'B':
		    if(recursive)
			throw Erange("command_line", tools_printf(gettext("-B option cannot be given inside a batch file")));
		    if(base != "")
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    base = optarg;
		    break;
		case 'A':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = add;
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    tools_split_path_basename(optarg, chem, filename);
		    tools_check_basename(dialog, chem, filename, EXTENSION);
		    arg = (path(chem)+filename).display();
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
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    try
		    {
			tools_read_range(string(optarg), min, max);
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
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    num = tools_str2signed_int(optarg);
		    break;
		case 'p':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = where;
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    num = tools_str2signed_int(optarg);
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
		    if(optarg == NULL)
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
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    num = tools_str2signed_int(optarg);
		    break;
		case 'f':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = files;
		    if(optarg == NULL)
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
		    if(optarg == NULL)
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
		case 'j':
		    break;  // ignore this option already parsed during initialization (dar_suite.cpp)
		case 'w':
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    date = tools_convert_date(optarg);
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
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    extra = optarg;
		    break;
		case '@':
		    if(recursive)
			throw Erange("command_line", tools_printf(gettext("Running batch file from a batch file is not allowed")));
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = batch;
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    arg = optarg;
		    break;
		case 'N':
		    ignore_dat_options = true;
		    break;
		case 'k':
		    even_when_removed = true;
		    break;
		case ';':
		    if(op != add)
			dialog.warning(gettext("-; option is only valid after -A option, ignoring it"));
		    else
		    {
			try
			{
			    deci tmp = string(optarg);
			    date = tmp.computer();
			}
			catch(Edeci & e)
			{
			    throw Erange("command_line", tools_printf(gettext("invalid number give to -; option: %s"), optarg));
			}
		    }
		    break;
		case 'a':
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    if(strcasecmp("i", optarg) == 0 || strcasecmp("ignore-order", optarg) == 0)
			check_order = false;
		    else
			throw Erange("command_line", tools_printf(gettext(INVALID_ARG), char(lu)));
		    break;
		case '?':
		    dialog.warning(tools_printf(gettext("Ignoring unknown option -%c"), char(optopt)));
		    break;
		default:
		    dialog.warning(tools_printf(gettext("Ignoring unknown option -%c"), char(lu)));
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
	    dialog.warning(gettext("-e option is only available when using -r option, aborting"));
	    return false;
	}

	switch(op)
	{
	case none_op:
	    dialog.warning(gettext("No action specified, aborting"));
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
		dialog.warning(gettext("Ignoring extra arguments on command line"));
	    break;
	case add:
	    if(rest.size() > 1)
		dialog.warning(gettext("Ignoring extra arguments on command line"));
	    break;
	case chbase:
	case where:
	    if(rest.size() != 1)
	    {
		dialog.warning(gettext("Missing argument to command line, aborting"));
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
		dialog.warning(gettext("Missing argument to command line, aborting"));
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
	    dialog.warning(gettext("No database specified, aborting"));
	    return false;
	}
    }
    catch(Erange & e)
    {
        dialog.warning(string(gettext("Parse error on command line (or included files): ")) + e.get_message());
        return false;
    }

    return true;
}

static void op_create(user_interaction & dialog, const string & base, bool info_details)
{
    database dat; // created empty;

    if(info_details)
    {
        dialog.warning(gettext("Creating file..."));
        dialog.warning(gettext("Formatting file as an empty database..."));
    }
    write_base(dialog, base, &dat, false);
    if(info_details)
        dialog.warning(gettext("Database has been successfully created empty."));
}

static void op_add(user_interaction & dialog, database *dat, const string &arg, string fake, const infinint & min_digits, bool info_details)
{
    thread_cancellation thr;
    string arch_path, arch_base;
    bool date_order_problem = false;
    archive_options_read read_options;

    if(dat == NULL)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog.warning(gettext("Reading catalogue of the archive to add..."));
    tools_split_path_basename(arg, arch_path, arch_base);
    read_options.set_info_details(info_details);
    read_options.set_slice_min_digits(min_digits);
    archive *arch = new (nothrow) archive(dialog, path(arch_path), arch_base, EXTENSION, read_options);
    if(arch == NULL)
	throw Ememory("dar_manager.cpp:op_add");

    try
    {
	string chemin, b;

	thr.check_self_cancellation();
	if(info_details)
	    dialog.warning(gettext("Updating database with catalogue..."));
	if(fake == "")
	    fake = arg;
	tools_split_path_basename(fake, chemin, b);
	dat->add_archive(*arch, chemin, b, database_add_options());
	thr.check_self_cancellation();
	if(info_details)
	    dialog.warning(gettext("Checking date ordering of files between archive..."));
	date_order_problem = dat->check_order(dialog);
	thr.check_self_cancellation();

    }
    catch(...)
    {
	if(arch != NULL)
	    delete arch;
	throw;
    }
    delete arch;

    if(!date_order_problem)
	throw Edata(gettext("Some files do not follow chronological order when archive index increases withing the database, this can lead dar_manager to restored a wrong version of these files"));
}

static void op_listing(user_interaction & dialog, const database *dat, bool info_details)
{
    if(dat == NULL)
	throw SRC_BUG;

    dat->show_contents(dialog);
}

static void op_del(user_interaction & dialog, database *dat, S_I min, archive_num max, bool info_details)
{
    thread_cancellation thr;
    database_remove_options opt;
    bool sign_plus;
    archive_num rmin;

    signed_int_to_archive_num(min, rmin, sign_plus);
    opt.set_revert_archive_numbering(!sign_plus);
    if(!sign_plus)
	max = rmin;
    if(dat == NULL)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog.warning(gettext("Removing information from the database..."));
    dat->remove_archive(rmin, max, opt);
    thr.check_self_cancellation();
}

static void op_chbase(user_interaction & dialog, database *dat, S_I num, const string & arg, bool info_details)
{
    thread_cancellation thr;
    database_change_basename_options opt;
    bool sign_plus;
    archive_num rnum;

    signed_int_to_archive_num(num, rnum, sign_plus);
    opt.set_revert_archive_numbering(!sign_plus);

    if(dat == NULL)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog.warning(gettext("Changing database header information..."));
    dat->change_name(rnum, arg, opt);
    thr.check_self_cancellation();
}

static void op_where(user_interaction & dialog, database *dat, S_I num, const string & arg, bool info_details)
{
    thread_cancellation thr;
    database_change_path_options opt;
    bool sign_plus;
    archive_num rnum;

    signed_int_to_archive_num(num, rnum, sign_plus);
    opt.set_revert_archive_numbering(!sign_plus);

    if(dat == NULL)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog.warning(gettext("Changing database header information..."));
    dat->set_path(rnum, arg, opt);
    thr.check_self_cancellation();
}

static void op_options(user_interaction & dialog, database *dat, const vector<string> & rest, bool info_details)
{
    thread_cancellation thr;

    if(dat == NULL)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog.warning(gettext("Changing database header information..."));
    dat->set_options(rest);
    thr.check_self_cancellation();
}

static void op_dar(user_interaction & dialog, database *dat, const string & arg, bool info_details)
{
    thread_cancellation thr;

    if(dat == NULL)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog.warning(gettext("Changing database header information..."));
    dat->set_dar_path(arg);
    thr.check_self_cancellation();
}

static void op_restore(user_interaction & dialog, database *dat, const vector<string> & rest, const infinint & date, const string & options_for_dar, bool info_details, bool early_release, bool ignore_dar_options_in_base, bool even_when_removed)
{
    thread_cancellation thr;
    string_file strfile = string_file(options_for_dar);
    vector <string> options = tools_split_in_words(strfile);

    database_restore_options dat_opt;

    if(dat == NULL)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog.warning(gettext("Looking in archives for requested files, classifying files archive by archive..."));
    dat_opt.set_early_release(early_release);
    dat_opt.set_info_details(info_details);
    dat_opt.set_date(date);
    dat_opt.set_extra_options_for_dar(options);
    dat_opt.set_ignore_dar_options_in_database(ignore_dar_options_in_base);
    dat_opt.set_even_when_removed(even_when_removed);
    dat->restore(dialog, rest, dat_opt);
}

static void op_used(user_interaction & dialog, const database *dat, S_I num, bool info_details)
{
    thread_cancellation thr;
    database_used_options opt;
    bool sign_plus;
    archive_num rnum;

    signed_int_to_archive_num(num, rnum, sign_plus);
    opt.set_revert_archive_numbering(!sign_plus);

    if(dat == NULL)
	throw SRC_BUG;

    thr.check_self_cancellation();
    dat->show_files(dialog, rnum, opt);
}

static void op_files(user_interaction & dialog, const database *dat, const string & arg, bool info_details)
{
    thread_cancellation thr;

    if(dat == NULL)
	throw SRC_BUG;

    thr.check_self_cancellation();
    dat->show_version(dialog, arg);
}

static void op_stats(user_interaction & dialog, const database *dat, bool info_details)
{
    thread_cancellation thr;

    if(dat == NULL)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog.warning(gettext("Computing statistics..."));
    dat->show_most_recent_stats(dialog);
}

static void op_move(user_interaction & dialog, database *dat, S_I src, archive_num dst, bool info_details)
{
    thread_cancellation thr;
    bool date_order_problem = false;

    if(src <= 0)
	throw Erange("op_move", gettext("Negative number or zero not allowed when moving an archive inside a database"));

    if(dat == NULL)
	throw SRC_BUG;

    thr.check_self_cancellation();
    if(info_details)
	dialog.warning(gettext("Changing database information..."));
    dat->set_permutation(src, dst);
    thr.check_self_cancellation();
    if(info_details)
	dialog.warning(gettext("Checking date ordering of files between archive..."));
    date_order_problem = dat->check_order(dialog);
    thr.check_self_cancellation();
    if(date_order_problem)
	throw Edata(gettext("Some files do not follow chronological order when archive index increases withing the database, this can lead dar_manager to restored a wrong version of these files"));
}

static void show_usage(user_interaction & dialog, const char *command)
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
#include "dar_manager.usage"
}

static void show_version(user_interaction & dialog, const char *command_name)
{
    string name;
    tools_extract_basename(command_name, name);
    U_I maj, med, min;

    get_version(maj, med, min);
    shell_interaction_change_non_interactive_output(&cout);
    dialog.printf("\n %s version %s, Copyright (C) 2002-2052 Denis Corbin\n", name.c_str(), DAR_MANAGER_VERSION);
    dialog.warning(string("   ") + dar_suite_command_line_features() + "\n");
    if(maj > 2)
	dialog.printf(gettext(" Using libdar %u.%u.%u built with compilation time options:\n"), maj, med, min);
    else
	dialog.printf(gettext(" Using libdar %u.%u built with compilation time options:\n"), maj, min);
    tools_display_features(dialog);
    dialog.printf("\n");
    dialog.printf(gettext(" compiled the %s with %s version %s\n"), __DATE__, CC_NAT, __VERSION__);
    dialog.printf(gettext(" %s is part of the Disk ARchive suite (Release %s)\n"), name.c_str(), PACKAGE_VERSION);
    dialog.warning(tools_printf(gettext(" %s comes with ABSOLUTELY NO WARRANTY; for details\n type `%s -W'."), name.c_str(), "dar")
		   + tools_printf(gettext(" This is free software, and you are welcome\n to redistribute it under certain conditions;"))
		   + tools_printf(gettext(" type `%s -L | more'\n for details.\n\n"), "dar"));
    dialog.printf("");
}

#if HAVE_GETOPT_LONG
static const struct option *get_long_opt()
{
    static const struct option ret[] = {
        {"create", required_argument, NULL, 'C'},
        {"base", required_argument, NULL, 'B'},
        {"add", required_argument, NULL, 'A'},
        {"list", no_argument, NULL, 'l'},
        {"delete", required_argument, NULL, 'D'},
        {"basename", required_argument, NULL, 'b'},
        {"path", required_argument, NULL, 'p'},
        {"options", no_argument, NULL, 'o'},
        {"dar", no_argument, NULL, 'd'},
        {"restore", no_argument, NULL, 'r'},
        {"used", required_argument, NULL, 'u'},
        {"file", required_argument, NULL, 'f'},
        {"stats", no_argument, NULL, 's'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'V'},
        {"verbose", no_argument, NULL, 'v'},
	{"jog", no_argument, NULL, 'j'},
	{"when", required_argument, NULL, 'w'},
	{"interactive", no_argument, NULL, 'i'},
	{"extra", required_argument, NULL, 'e'},
	{"check", no_argument, NULL, 'c'},
 	{"batch", required_argument, NULL, '@'},
	{"ignore-options-in-base", no_argument, NULL, 'N'},
	{"min-digits", required_argument, NULL, ';'},
	{"ignore-when-removed", no_argument, NULL, 'k'},
	{"alter", required_argument, NULL, 'a'},
        { NULL, 0, NULL, 0 }
    };

    return ret;
}
#endif

static database *read_base(user_interaction & dialog, const string & base, bool partial, bool partial_read_only, bool check_order)
{
    database *ret = NULL;

    try
    {
	database_open_options dat_opt;
	dat_opt.set_warn_order(check_order);
	dat_opt.set_partial(partial);
	dat_opt.set_partial_read_only(partial_read_only);
        ret = new (nothrow) database(dialog, base, dat_opt);
        if(ret == NULL)
            throw Ememory("read_base");
    }
    catch(Erange & e)
    {
        string msg = string(gettext("Corrupted database :"))+e.get_message();
        dialog.warning(msg);
        throw Edata(msg);
    }

    return ret;
}

static void write_base(user_interaction & dialog, const string & filename, const database *base, bool overwrite)
{
    thread_cancellation thr;
    database_dump_options dat_opt;

    dat_opt.set_overwrite(overwrite);
    thr.block_delayed_cancellation(true);
    base->dump(dialog, filename, dat_opt);
    thr.block_delayed_cancellation(false);
}

static void op_interactive(user_interaction & dialog, database *dat, string base)
{
    char choice;
    bool saved = true;
    thread_cancellation thr;
    archive_num num, num2;
    S_I tmp_si;
    bool tmp_sign;
    string input, input2;
    archive *arch = NULL;
    vector <string> vectinput;
    U_I more = 25;
    database_change_basename_options opt_change_name;
    database_change_path_options opt_change_path;
    database_remove_options opt_remove;
    database_used_options opt_used;

    if(dat == NULL)
	throw SRC_BUG;

    thr.check_self_cancellation();
    do
    {
	try
	{
	    archive_options_read read_options;

		// diplay choice message

	    dialog.warning_with_more(0);
	    dialog.printf(gettext("\n\n\t Dar Manager Database used [%s] : %S\n"), saved ? gettext("Saved") : gettext("Not Saved"), &base);
	    if(more > 0)
		dialog.printf(gettext("\t Pause each %d line of output\n\n"), more);
	    else
		dialog.printf(gettext("\t No pause in output\n\n"));
	    dialog.printf(gettext(" l : list database contents  \t A : Add an archive\n"));
	    dialog.printf(gettext(" u : list archive contents   \t D : Remove an archive\n"));
	    dialog.printf(gettext(" f : give file localization  \t m : modify archive order\n"));
	    dialog.printf(gettext(" p : modify path of archives \t b : modify basename of archives\n"));
	    dialog.printf(gettext(" d : path to dar             \t o : options to dar\n"));
	    dialog.printf(gettext(" w : write changes to file   \t s : database statistics\n"));
	    dialog.printf(gettext(" a : Save as                 \t n : pause each 'n' line (zero for no pause)\n"));
	    dialog.printf(gettext(" c : check date order\n\n"));
	    dialog.printf(gettext(" q : quit\n\n"));
	    dialog.printf(gettext(" Choice: "));

		// user's choice selection

	    shell_interaction_read_char(choice);
	    thr.check_self_cancellation();
	    dialog.printf("\n\n");

		// performing requested action

	    dialog.warning_with_more(more);
	    switch(choice)
	    {
	    case 'l':
		dat->show_contents(dialog);
		break;
	    case 'u':
		input = dialog.get_string(gettext("Archive number: "), true);
		tmp_si = tools_str2signed_int(input);
		signed_int_to_archive_num(tmp_si, num, tmp_sign);
		opt_used.set_revert_archive_numbering(!tmp_sign);
		dat->show_files(dialog, num, opt_used);
		break;
	    case 'f':
		input = dialog.get_string(gettext("File to look for: "), true);
		dat->show_version(dialog, input);
		break;
	    case 'b':
		input = dialog.get_string(gettext("Archive number to modify: "), true);
		tmp_si = tools_str2signed_int(input);
		signed_int_to_archive_num(tmp_si, num, tmp_sign);
		opt_change_name.set_revert_archive_numbering(!tmp_sign);
		input = dialog.get_string(tools_printf(gettext("New basename for archive number %d: "), tmp_si), true);
		dat->change_name(num, input, opt_change_name);
		saved = false;
		break;
	    case 'd':
		input = dialog.get_string(gettext("Path to dar (empty string to use the default from PATH variable): "), true);
		dat->set_dar_path(input);
		saved = false;
		break;
	    case 'w':
		dialog.warning(gettext("Compressing and writing back database to file..."));
		write_base(dialog, base, dat, true);
		saved = true;
		break;
	    case 'a':
		input = dialog.get_string(gettext("New database name: "), true);
		dialog.warning(gettext("Compressing and writing back database to file..."));
		write_base(dialog, input, dat, false);
		base = input;
		saved = true;
		break;
	    case 'A':
		input = dialog.get_string(gettext("Archive basename (or extracted catalogue basename) to add: "), true);
		dialog.warning(gettext("Reading catalogue of the archive to add..."));
		tools_split_path_basename(input, input, input2);
		read_options.clear();
		read_options.set_info_details(true);
		arch = new (nothrow) archive(dialog, path(input), input2, EXTENSION, read_options);
		if(arch == NULL)
		    throw Ememory("dar_manager.cpp:op_interactive");
		try
		{
		    dialog.warning(gettext("Updating database with catalogue..."));
		    dat->add_archive(*arch, input, input2, database_add_options());
		    thr.check_self_cancellation();
		    dialog.warning(gettext("Checking date ordering of files between archive..."));
		    (void)dat->check_order(dialog);
		}
		catch(...)
		{
		    delete arch;
		    arch = NULL;
		    throw;
		}
		delete arch;
		arch = NULL;
		saved = false;
		break;
	    case 'D':
		input = dialog.get_string(gettext("Archive number to remove: "), true);
		tmp_si = tools_str2signed_int(input);
		signed_int_to_archive_num(tmp_si, num, tmp_sign);
		opt_remove.set_revert_archive_numbering(!tmp_sign);
		dialog.pause(tools_printf(gettext("Are you sure to remove archive number %d ?"), tmp_si));
		dialog.warning(gettext("Removing information from the database..."));
		dat->remove_archive(num, num, opt_remove);
		saved = false;
		break;
	    case 'm':
		input = dialog.get_string(gettext("Archive number to move: "), true);
		num = tools_str2int(input);
		input = dialog.get_string(gettext("In which position to insert this archive: "), true);
		num2 = tools_str2int(input);
		dat->set_permutation(num, num2);
		thr.check_self_cancellation();
		dialog.warning(gettext("Checking date ordering of files between archive..."));
		dat->check_order(dialog);
		saved = false;
		break;
	    case 'p':
		input = dialog.get_string(gettext("Archive number who's path to modify: "), true);
		tmp_si = tools_str2signed_int(input);
		signed_int_to_archive_num(tmp_si, num, tmp_sign);
		opt_change_path.set_revert_archive_numbering(!tmp_sign);
		input = dialog.get_string(tools_printf(gettext("New path to give to archive number %d: "), tmp_si), true);
		dat->set_path(num, input, opt_change_path);
		saved = false;
		break;
	    case 'o':
		vectinput = read_vector(dialog);
		dat->set_options(vectinput);
		saved = false;
		break;
	    case 's':
		dialog.warning(gettext("Computing statistics..."));
		dat->show_most_recent_stats(dialog);
		break;
	    case 'n':
		input = dialog.get_string(gettext("How much line to display at once: "), true);
		more = tools_str2int(input);
		break;
	    case 'c':
		dialog.warning(gettext("Checking file's dates ordering..."));
		(void)dat->check_order(dialog);
		break;
	    case 'q':
		if(!saved)
		{
		    try
		    {
			dialog.pause(gettext("Database not saved, Do you really want to quit ?"));
			dialog.printf(gettext("Continuing the action under process which is to exit... so we exit!"));
		    }
		    catch(Euser_abort & e)
		    {
			choice = ' ';
		    }
		}
		break;
	    default:
		dialog.printf(gettext("Unknown choice\n"));
	    }
	}
	catch(Ethread_cancel & e)
	{
	    bool quit = false;

	    if(!saved)
	    {
		try
		{
		    dialog.pause(gettext("Database not saved, Do you really want to quit ?"));
		    dialog.printf(gettext("Continuing the action under process which is to exit... so we exit!"));
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
		dialog.printf(gettext("re-enabling all signal handlers and continuing\n"));
		dar_suite_reset_signal_handler();
	    }
	}
	catch(Egeneric & e)
	{
	    string tmp = e.get_message();
	    dialog.warning(tools_printf(gettext("Error performing the requested action: %S"), &tmp));
	}
    }
    while(choice != 'q');
    if(arch != NULL)
	throw SRC_BUG;
}

static void op_check(user_interaction & dialog, const database *dat, bool info_details)
{
    thread_cancellation thr;

    if(dat == NULL)
	throw SRC_BUG;

    if(info_details)
	dialog.warning(gettext("Checking date ordering of files between archive..."));
    if(!dat->check_order(dialog))
	throw Edata(gettext("Some files do not follow chronological order when archive index increases withing the database, this can lead dar_manager to restored a wrong version of these files"));
    else
	dialog.warning(gettext("No problem found"));
    thr.check_self_cancellation();
}

static void op_batch(user_interaction & dialog, database *dat, const string & filename, bool info_details)
{
    const string pseudo_cmd = "dar_manager"; // to take the place of the command on the simulated command-line
    string line;
    char tmp;
    bool sub_info_details;
    vector<string> mots;
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

    if(dat == NULL)
	throw SRC_BUG;

	// openning the batch file

    if(info_details)
	dialog.warning(gettext("Opening and reading the batch file..."));
    int fd = -1;
    fd = open(filename.c_str(), O_RDONLY|O_BINARY);

    if(fd < 0)
	throw Erange("op_batch", tools_printf(gettext("Cannot open file %S : %s"), &filename, strerror(errno)));

    fichier batch_file = fichier(dialog, fd); // the object batch_file will close fd upon destructor call.

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

	    string_file line_file = string_file(line);
	    mots = tools_split_in_words(line_file);

	    if(mots.size() == 0)
		continue;

	    if(info_details)
		dialog.printf(gettext("\n\tExecuting batch file line: %S\n "), &line);

	    cmdline.resize(mots.size() + 1); // +1 of pseudo_command
	    cmdline.set_arg(pseudo_cmd, 0);

	    for(register U_I i = 0; i < mots.size(); ++i)
		cmdline.set_arg(mots[i], i+1);

	    if(!command_line(dialog, cmdline.argc(), cmdline.argv(), sub_op, faked_base, arg, num, rest, num2, date, sub_info_details, ignore_dat_options, even_when_removed, check_order, true))
		throw Erange("op_batch", tools_printf(gettext("Syntax error in batch file: %S"), &line));

	    if(sub_op == create)
		throw Erange("op_batch", gettext("Syntax error in batch file: -C option not allowed"));

	    if(sub_op == interactive)
		throw Erange("op_batch", gettext("Syntax error in batch file: -i option not allowed"));

	    action(dialog, sub_op, dat, arg, num, rest, num2, date, faked_base, sub_info_details, false, ignore_dat_options, even_when_removed);
	}
	while(tmp == '\n');
    }
    catch(Edata & e)
    {
	e.prepend_message(gettext("Aborting batch operation: "));
	throw;
    }
}

static vector<string> read_vector(user_interaction & dialog)
{
    vector<string> ret;
    string tmp;

    ret.clear();
    dialog.printf(gettext("Enter each argument line by line, press return at the end\n"));
    dialog.printf(gettext("To terminate enter an empty line\n"));

    do
    {
	tmp = dialog.get_string(" > ", true);
	if(tmp != "")
	    ret.push_back(tmp);
    }
    while(tmp != "");

    return ret;
}

static void finalize(user_interaction & dialog, operation op, database *dat, const string & base, bool info_details)
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
	    dialog.warning(gettext("Compressing and writing back database to file..."));
	write_base(dialog, base, dat, true);
	break;
    default:
	throw SRC_BUG;
    }
}

static void action(user_interaction & dialog,
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
		   bool even_when_removed)
{
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
	op_listing(dialog, dat, info_details);
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
	op_used(dialog, dat, num, info_details);
	break;
    case files:
	op_files(dialog, dat, arg, info_details);
	break;
    case stats:
	op_stats(dialog, dat, info_details);
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
    if(input >= ARCHIVE_NUM_MAX)
	throw Erange("signed_int_to_archive_name", tools_printf(gettext("Absolute value too high for an archive number: %d"), input));
    else
	num = input;
}

