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
// to contact the author : dar.linux@free.fr
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif

#include "getopt_decision.h"
} // end extern "C"

#include <vector>
#include <string>
#include "dar_suite.hpp"
#include "macro_tools.hpp"
#include "data_tree.hpp"
#include "database.hpp"
#include "test_memory.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"
#include "libdar.hpp"
#include "shell_interaction.hpp"
#include "tools.hpp"
#include "thread_cancellation.hpp"
#include "archive.hpp"
#include "string_file.hpp"

using namespace libdar;

#define DAR_MANAGER_VERSION "1.4.3"


#define ONLY_ONCE "Only one -%c is allowed, ignoring this extra option"
#define MISSING_ARG "Missing argument to -%c"
#define INVALID_ARG "Invalid argument given to -%c (requires integer)"

enum operation { none_op, create, add, listing, del, chbase, where, options, dar, restore, used, files, stats, move, interactive };

static S_I little_main(user_interaction & dialog, S_I argc, char *argv[], const char **env);
static bool command_line(user_interaction & dialog,
			 S_I argc, char *argv[],
                         operation & op,
                         string & base,
                         string & arg,
                         archive_num & num,
                         vector<string> & rest,
                         archive_num & num2,
			 infinint & date,
                         bool & verbose);
static void show_usage(user_interaction & dialog, const char *command);
static void show_version(user_interaction & dialog, const char *command);
#if HAVE_GETOPT_LONG
static const struct option *get_long_opt();
#endif
static void op_create(user_interaction & dialog, const string & base, bool info_details);
static void op_add(user_interaction & dialog, const string & base, const string &arg, string fake, bool info_details);
static void op_listing(user_interaction & dialog, const string & base, bool info_details);
static void op_del(user_interaction & dialog, const string & base, archive_num min, archive_num max, bool info_details);
static void op_chbase(user_interaction & dialog, const string & base, archive_num num, const string & arg, bool info_details);
static void op_where(user_interaction & dialog, const string & base, archive_num num, const string & arg, bool info_details);
static void op_options(user_interaction & dialog, const string & base, const vector<string> & rest, bool info_details);
static void op_dar(user_interaction & dialog, const string & base, const string & arg, bool info_details);
static void op_restore(user_interaction & dialog, const string & base, const vector<string> & rest, const infinint & date, const string & options_for_dar, bool info_details);
static void op_used(user_interaction & dialog, const string & base, archive_num num, bool info_details);
static void op_files(user_interaction & dialog, const string & base, const string & arg, bool info_details);
static void op_stats(user_interaction & dialog, const string & base, bool info_details);
static void op_move(user_interaction & dialog, const string & base, archive_num src, archive_num dst, bool info_details);
static void op_interactive(user_interaction & dialog, string base);

static database *read_base(user_interaction & dialog, const string & base, bool partial);
static void write_base(user_interaction & dialog, const string & filename, const database *base, bool overwrite);
static vector<string> read_vector(user_interaction & dialog);

int main(S_I argc, char *argv[], const char **env)
{
    return dar_suite_global(argc, argv, env, &little_main);
}

S_I little_main(user_interaction & dialog, S_I argc, char *argv[], const char **env)
{
    operation op;
    string base;
    string arg;
    archive_num num, num2;
    vector<string> rest;
    bool info_details;
    infinint date;

    shell_interaction_change_non_interactive_output(&cout);

    if(!command_line(dialog, argc, argv, op, base, arg, num, rest, num2, date, info_details))
	return EXIT_SYNTAX;
    MEM_IN;
    switch(op)
    {
    case none_op:
        throw SRC_BUG;
    case create:
        op_create(dialog, base, info_details);
        break;
    case add:
        op_add(dialog, base, arg, rest.empty() ? "" : rest[0], info_details);
        break;
    case listing:
        op_listing(dialog, base, info_details);
        break;
    case del:
        op_del(dialog, base, num, num2, info_details);
        break;
    case chbase:
        op_chbase(dialog, base, num, arg, info_details);
        break;
    case where:
        op_where(dialog, base, num, arg, info_details);
        break;
    case options:
        op_options(dialog, base, rest, info_details);
        break;
    case dar:
        op_dar(dialog, base, arg, info_details);
        break;
    case restore:
        op_restore(dialog, base, rest, date, arg, info_details);
        break;
    case used:
        op_used(dialog, base, num, info_details);
        break;
    case files:
        op_files(dialog, base, arg, info_details);
        break;
    case stats:
        op_stats(dialog, base, info_details);
        break;
    case move:
        op_move(dialog, base, num, num2, info_details);
        break;
    case interactive:
	op_interactive(dialog, base);
	break;
    default:
        throw SRC_BUG;
    }
    MEM_OUT;
    return EXIT_OK;
}

static bool command_line(user_interaction & dialog,
			 S_I argc, char *argv[],
                         operation & op,
                         string & base,
			 string & arg,
                         archive_num & num,
                         vector<string> & rest,
                         archive_num & num2,
			 infinint & date,
                         bool & verbose)
{
    S_I lu;
    U_I min, max;

    base = arg = "";
    num = num2 = 0;
    rest.clear();
    op = none_op;
    verbose = false;
    string chem, filename;
    date = 0;
    string extra = "";

    try
    {
#if HAVE_GETOPT_LONG
	while((lu = getopt_long(argc, argv, "C:B:A:lD:b:p:od:ru:f:shVm:vQjw:ie:", get_long_opt(), NULL)) != EOF)
#else
	    while((lu = getopt(argc, argv, "C:B:A:lD:b:p:od:ru:f:shVm:vQjw:ie:")) != EOF)
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
		    num = atoi(optarg);
		    break;
		case 'p':
		    if(op != none_op)
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    op = where;
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    num = atoi(optarg);
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
		    num = atoi(optarg);
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
		    op = move;
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    num = atoi(optarg);
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
		case 'e':
		    if(extra != "")
			throw Erange("command_line", tools_printf(gettext(ONLY_ONCE), char(lu)));
		    if(optarg == NULL)
			throw Erange("command_line", tools_printf(gettext(MISSING_ARG), char(lu)));
		    extra = optarg;
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
	case move:
	    if(rest.size() != 1)
	    {
		dialog.warning(gettext("Missing argument to command line, aborting"));
		return false;
	    }
	    num2 = tools_str2int(rest[0]);
	    rest.clear();
	    break;
	default:
	    throw SRC_BUG;
	}

	if(base == "")
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

static void op_add(user_interaction & dialog, const string & base, const string &arg, string fake, bool info_details)
{
    thread_cancellation thr;
    string arch_path, arch_base;

    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database to memory..."));
    database *dat = read_base(dialog, base, false);


    try
    {
	thr.check_self_cancellation();
	if(info_details)
            dialog.warning(gettext("Reading catalogue of the archive to add..."));
	tools_split_path_basename(arg, arch_path, arch_base);
        archive *arch = new archive(dialog, path(arch_path), arch_base, EXTENSION, crypto_none, "", 0, "", "", "", info_details);
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
            dat->add_archive(*arch, chemin, b);
	    thr.check_self_cancellation();
        }
        catch(...)
        {
            if(arch != NULL)
                delete arch;
            throw;
        }
        delete arch;

        if(info_details)
            dialog.warning(gettext("Compressing and writing back database to file..."));
        write_base(dialog, base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;
}

static void op_listing(user_interaction & dialog, const string & base, bool info_details)
{
    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database header to memory..."));
    database *dat = read_base(dialog, base, true);

    try
    {
        dat->show_contents(dialog);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;
}

static void op_del(user_interaction & dialog, const string & base, archive_num min, archive_num max, bool info_details)
{
    thread_cancellation thr;

    if(info_details)        dialog.warning(gettext("Uncompressing and loading database to memory..."));
    database *dat = read_base(dialog, base, false);

    try
    {
	thr.check_self_cancellation();
        if(info_details)
            dialog.warning(gettext("Removing informations from the archive..."));
        dat->remove_archive(min, max);
	thr.check_self_cancellation();
	if(info_details)
            dialog.warning(gettext("Compressing and writing back database to file..."));
        write_base(dialog, base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;
}

static void op_chbase(user_interaction & dialog, const string & base, archive_num num, const string & arg, bool info_details)
{
    thread_cancellation thr;

    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database header to memory..."));
    database *dat = read_base(dialog, base, true);

    try
    {
	thr.check_self_cancellation();
        if(info_details)
            dialog.warning(gettext("Changing database header information..."));
        dat->change_name(num, arg);
	thr.check_self_cancellation();
        if(info_details)
            dialog.warning(gettext("Compressing and writing back database header to file..."));
        write_base(dialog, base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;

}

static void op_where(user_interaction & dialog, const string & base, archive_num num, const string & arg, bool info_details)
{
    thread_cancellation thr;

    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database header to memory..."));
    database *dat = read_base(dialog, base, true);

    try
    {
	thr.check_self_cancellation();
        if(info_details)
            dialog.warning(gettext("Changing database header information..."));
        dat->set_path(num, arg);
	thr.check_self_cancellation();
	if(info_details)
            dialog.warning(gettext("Compressing and writing back database header to file..."));
        write_base(dialog, base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;

}

static void op_options(user_interaction & dialog, const string & base, const vector<string> & rest, bool info_details)
{
    thread_cancellation thr;

    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database header to memory..."));
    database *dat = read_base(dialog, base, true);

    try
    {
	thr.check_self_cancellation();
        if(info_details)
            dialog.warning(gettext("Changing database header information..."));
        dat->set_options(rest);
	thr.check_self_cancellation();
	if(info_details)
            dialog.warning(gettext("Compressing and writing back database header to file..."));
        write_base(dialog, base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;

}

static void op_dar(user_interaction & dialog, const string & base, const string & arg, bool info_details)
{
    thread_cancellation thr;

    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database header to memory..."));
    database *dat = read_base(dialog, base, true);

    try
    {
	thr.check_self_cancellation();
        if(info_details)
            dialog.warning(gettext("Changing database header information..."));
        dat->set_dar_path(arg);
	thr.check_self_cancellation();
	if(info_details)
            dialog.warning(gettext("Compressing and writing back database header to file..."));
        write_base(dialog, base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;

}

static void op_restore(user_interaction & dialog, const string & base, const vector<string> & rest, const infinint & date, const string & options_for_dar, bool info_details)
{
    thread_cancellation thr;
    string_file strfile = string_file(dialog, options_for_dar);
    vector <string> options = tools_split_in_words(strfile);

    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database to memory..."));
    database *dat = read_base(dialog, base, false);

    try
    {
	thr.check_self_cancellation();
        if(info_details)
            dialog.warning(gettext("Looking in archives for requested files, classifying files archive by archive..."));
        dat->restore(dialog, rest, true, options, date);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;
}

static void op_used(user_interaction & dialog, const string & base, archive_num num, bool info_details)
{
    thread_cancellation thr;

    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database to memory..."));
    database *dat = read_base(dialog, base, false);

    try
    {
	thr.check_self_cancellation();
        dat->show_files(dialog, num);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;

}

static void op_files(user_interaction & dialog, const string & base, const string & arg, bool info_details)
{
    thread_cancellation thr;

    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database to memory..."));
    database *dat = read_base(dialog, base, false);

    try
    {
	thr.check_self_cancellation();
        dat->show_version(dialog, arg);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;

}

static void op_stats(user_interaction & dialog, const string & base, bool info_details)
{
    thread_cancellation thr;

    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database to memory..."));
    database *dat = read_base(dialog, base, false);

    try
    {
	thr.check_self_cancellation();
        if(info_details)
            dialog.warning(gettext("Computing statistics..."));
        dat->show_most_recent_stats(dialog);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;
}

static void op_move(user_interaction & dialog, const string & base, archive_num src, archive_num dst, bool info_details)
{
    thread_cancellation thr;

    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database to memory..."));
    database *dat = read_base(dialog, base, false);

    try
    {
	thr.check_self_cancellation();
        if(info_details)
            dialog.warning(gettext("Changing database information..."));
        dat->set_permutation(src, dst);
	thr.check_self_cancellation();
	if(info_details)
            dialog.warning(gettext("Compressing and writing back database header to file..."));
        write_base(dialog, base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;
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
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> [-w <date>] -r [list of files to restore]\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -u <number>\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -f file\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -s\n");
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -m <number> <number>\n");
    dialog.printf("\tdar_manager -h\n");
    dialog.printf("\tdar_manager -V\n");
    dialog.printf("\n");
#include "dar_manager.usage"
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
    dialog.printf("\n %s version %s, Copyright (C) 2002-2052 Denis Corbin\n", name.c_str(), DAR_MANAGER_VERSION);
    dialog.warning(string("   ") + dar_suite_command_line_features() + "\n");
    if(maj > 2)
	dialog.printf(gettext(" Using libdar %u.%u.%u built with compilation time options:\n"), maj, med, min);
    else
	dialog.printf(gettext(" Using libdar %u.%u built with compilation time options:\n"), maj, min);
    tools_display_features(dialog, ea, largefile, nodump, special_alloc, bits, thread, libz, libbz2, libcrypto, new_blowfish);
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
        { NULL, 0, NULL, 0 }
    };

    return ret;
}
#endif

static database *read_base(user_interaction & dialog, const string & base, bool partial)
{
    database *ret = NULL;

    try
    {
        ret = new database(dialog, base, partial);
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

    thr.block_delayed_cancellation(true);
    base->dump(dialog, filename, overwrite);
    thr.block_delayed_cancellation(false);
}

static void op_interactive(user_interaction & dialog, string base)
{
    char choice;
    bool saved = true;
    database *dat = NULL;
    thread_cancellation thr;
    archive_num num, num2;
    string input, input2;
    archive *arch = NULL;
    vector <string> vectinput;
    U_I more = 25;

    dialog.warning(gettext("Uncompressing and loading database to memory..."));
    dat = read_base(dialog, base, false);
    thr.check_self_cancellation();

    try
    {
	do
	{
	    try
	    {

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
		dialog.printf(gettext(" a : Save as                 \t n : pause each 'n' line (zero for no pause)\n\n"));
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
		    num = tools_str2int(input);
		    dat->show_files(dialog, num);
		    break;
		case 'f':
		    input = dialog.get_string(gettext("File to look for: "), true);
		    dat->show_version(dialog, input);
		    break;
		case 'b':
		    input = dialog.get_string(gettext("Archive number to modify: "), true);
		    num = tools_str2int(input);
		    input = dialog.get_string(tools_printf(gettext("New basename for archive number %d: "), num), true);
		    dat->change_name(num, input);
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
		    arch = new archive(dialog, path(input), input2, EXTENSION, crypto_none, "", 0, "", "", "", true);
		    if(arch == NULL)
			throw Ememory("dar_manager.cpp:op_interactive");
		    try
		    {
			dialog.warning(gettext("Updating database with catalogue..."));
			dat->add_archive(*arch, input, input2);
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
		    num = tools_str2int(input);
		    dialog.warning(gettext("Removing informations from the archive..."));
		    dat->remove_archive(num, num);
		    saved = false;
		    break;
		case 'm':
		    input = dialog.get_string(gettext("Archive number to move: "), true);
		    num = tools_str2int(input);
		    input = dialog.get_string(gettext("In which position to insert this archive: "), true);
		    num2 = tools_str2int(input);
		    dat->set_permutation(num, num2);
		    saved = false;
		    break;
		case 'p':
		    input = dialog.get_string(gettext("Archive number who's path to modify: "), true);
		    num = tools_str2int(input);
		    input = dialog.get_string(tools_printf(gettext("New path to give to archive number %d: "), num), true);
		    dat->set_path(num, input);
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
    }
    catch(...)
    {
	delete dat;
	throw;
    }
    delete dat;
    if(arch != NULL)
	throw SRC_BUG;
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
