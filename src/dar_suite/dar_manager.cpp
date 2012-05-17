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
// $Id: dar_manager.cpp,v 1.36.2.4 2005/03/13 20:07:48 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#include "getopt_decision.h"
} // end extern "C"

#include <vector>
#include <string>
#include "dar_suite.hpp"
#include "database_header.hpp"
#include "macro_tools.hpp"
#include "data_tree.hpp"
#include "database.hpp"
#include "test_memory.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"
#include "libdar.hpp"
#include "shell_interaction.hpp"
#include "tools.hpp"

using namespace libdar;

#define DAR_MANAGER_VERSION "1.3.1"


#define ONLY_ONCE "Only one -%c is allowed, ignoring this extra option"
#define MISSING_ARG "Missing argument to -%c"
#define INVALID_ARG "Invalid argument given to -%c (requires integer)"

enum operation { none_op, create, add, listing, del, chbase, where, options, dar, restore, used, files, stats, move };

static S_I little_main(user_interaction & dialog, S_I argc, char *argv[], const char **env);
static bool command_line(user_interaction & dialog,
			 S_I argc, char *argv[],
                         operation & op,
                         string & base,
                         string & arg,
                         archive_num & num,
                         vector<string> & rest,
                         archive_num & num2,
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
static void op_restore(user_interaction & dialog, const string & base, const vector<string> & rest, bool info_details);
static void op_used(user_interaction & dialog, const string & base, archive_num num, bool info_details);
static void op_files(user_interaction & dialog, const string & base, const string & arg, bool info_details);
static void op_stats(user_interaction & dialog, const string & base, bool info_details);
static void op_move(user_interaction & dialog, const string & base, archive_num src, archive_num dst, bool info_details);

static database *read_base(user_interaction & dialog, const string & base, bool partial);
static void write_base(user_interaction & dialog, const string & filename, const database *base, bool overwrite);

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

    shell_interaction_change_non_interactive_output(&cout);
    if(!command_line(dialog, argc, argv, op, base, arg, num, rest, num2, info_details))
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
        op_add(dialog, base, arg, rest.size() > 0 ? rest[0] : "", info_details);
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
        op_restore(dialog, base, rest, info_details);
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

#if HAVE_GETOPT_LONG
    while((lu = getopt_long(argc, argv, "C:B:A:lD:b:p:od:ru:f:shVm:vQj", get_long_opt(), NULL)) != EOF)
#else
	while((lu = getopt(argc, argv, "C:B:A:lD:b:p:od:ru:f:shVm:vQj")) != EOF)
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
	    case '?':
		dialog.warning(tools_printf(gettext("Ignoring unknown option -%c"), char(optopt)));
		break;
	    default:
		dialog.warning(tools_printf(gettext("Ignoring unknown option -%c"), char(lu)));
	    }
	    if(lu == 'o' || lu == 'r')
		break; // stop reading arguments
	}

    for(S_I i = optind; i < argc; i++)
        rest.push_back(argv[i]);

        // sanity checks

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
        if(rest.size() > 0)
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
        for(unsigned int i = 0; i < rest.size(); i++)
            if(!path(rest[i]).is_relative())
                throw Erange("command_line", gettext("Arguments to -r must be relative path (never begin by '/')"));
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

    return true;
}


static void dummy_call(char *x)
{
    static char id[]="$Id: dar_manager.cpp,v 1.36.2.4 2005/03/13 20:07:48 edrusb Rel $";
    dummy_call(id);
}

static void op_create(user_interaction & dialog, const string & base, bool info_details)
{
    database dat; // created empty;

    if(info_details)
    {
        dialog.warning(gettext("Creating file..."));
        dialog.warning(gettext("Formating file as an empty database..."));
    }
    write_base(dialog, base, &dat, false);
    if(info_details)
        dialog.warning(gettext("Database has been successfully created empty."));
}

static void op_add(user_interaction & dialog, const string & base, const string &arg, string fake, bool info_details)
{
    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database in memory..."));
    database *dat = read_base(dialog, base, false);

    try
    {
        if(info_details)
            dialog.warning(gettext("Reading catalogue of the archive to add..."));
        catalogue *catal = macro_tools_get_catalogue_from(dialog, arg, EXTENSION, crypto_none, "", 0);

        try
        {
            string chemin, b;

            if(info_details)
                dialog.warning(gettext("Updating database with catalogue..."));
            if(fake == "")
                fake = arg;
            tools_split_path_basename(fake, chemin, b);
            dat->add_archive(*catal, chemin, b);
        }
        catch(...)
        {
            if(catal != NULL)
                delete catal;
            throw;
        }
        delete catal;

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
        dialog.warning(gettext("Uncompressing and loading database header in memory..."));
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
    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database in memory..."));
    database *dat = read_base(dialog, base, false);

    try
    {
        if(info_details)
            dialog.warning(gettext("Removing informations from the archive..."));
        dat->remove_archive(min, max);
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
    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database header in memory..."));
    database *dat = read_base(dialog, base, true);

    try
    {
        if(info_details)
            dialog.warning(gettext("Changing database header information..."));
        dat->change_name(num, arg);
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
    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database header in memory..."));
    database *dat = read_base(dialog, base, true);

    try
    {
        if(info_details)
            dialog.warning(gettext("Changing database header information..."));
        dat->set_path(num, arg);
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
    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database header in memory..."));
    database *dat = read_base(dialog, base, true);

    try
    {
        if(info_details)
            dialog.warning(gettext("Changing database header information..."));
        dat->set_options(rest);
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
    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database header in memory..."));
    database *dat = read_base(dialog, base, true);

    try
    {
        if(info_details)
            dialog.warning(gettext("Changing database header information..."));
        dat->set_dar_path(arg);
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

static void op_restore(user_interaction & dialog, const string & base, const vector<string> & rest, bool info_details)
{
    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database in memory..."));
    database *dat = read_base(dialog, base, false);

    try
    {
        if(info_details)
            dialog.warning(gettext("Looking for archives of most recent versions, sorting files by archive to use for restoration..."));
        dat->restore(dialog, rest, true);
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
    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database in memory..."));
    database *dat = read_base(dialog, base, false);

    try
    {
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
    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database in memory..."));
    database *dat = read_base(dialog, base, false);

    try
    {
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
    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database in memory..."));
    database *dat = read_base(dialog, base, false);

    try
    {
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
    if(info_details)
        dialog.warning(gettext("Uncompressing and loading database in memory..."));
    database *dat = read_base(dialog, base, false);

    try
    {
        if(info_details)
            dialog.warning(gettext("Changing database information..."));
        dat->set_permutation(src, dst);
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
    dialog.printf("\tdar_manager [options] -B [<path>/]<database> -r [list of files to restore]\n");
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
    char *name = tools_extract_basename(command_name);
    U_I maj, med, min, bits;
    bool ea, largefile, nodump, special_alloc, thread, libz, libbz2, libcrypto;

    get_version(maj, min);
    if(maj > 2)
	get_version(maj, med, min);
    else
	med = 0;
    get_compile_time_features(ea, largefile, nodump, special_alloc, bits, thread, libz, libbz2, libcrypto);
    shell_interaction_change_non_interactive_output(&cout);
    try
    {
        dialog.printf("\n %s version %s, Copyright (C) 2002-2052 Denis Corbin\n", name, DAR_MANAGER_VERSION);
	dialog.warning(string("   ") + dar_suite_command_line_features() + "\n");
	if(maj > 2)
	    dialog.printf(gettext(" Using libdar %u.%u.%u built with compilation time options:\n"), maj, med, min);
	else
	    dialog.printf(gettext(" Using libdar %u.%u built with compilation time options:\n"), maj, min);
        tools_display_features(dialog, ea, largefile, nodump, special_alloc, bits, thread, libz, libbz2, libcrypto);
        dialog.printf("\n");
        dialog.printf(gettext(" compiled the %s with %s version %s\n"), __DATE__, CC_NAT, __VERSION__);
        dialog.printf(gettext(" %s is part of the Disk ARchive suite (Release %s)\n"), name, PACKAGE_VERSION);
        dialog.warning(tools_printf(gettext(" %s comes with ABSOLUTELY NO WARRANTY; for details\n type `%s -W'."), name, "dar")
		       + tools_printf(gettext(" This is free software, and you are welcome\n to redistribute it under certain conditions;"))
		       + tools_printf(gettext(" type `%s -L | more'\n for details.\n\n"), "dar"));
        dialog.printf("");
    }
    catch(...)
    {
        delete [] name;
        throw;
    }
    delete [] name;
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
        { NULL, 0, NULL, 0 }
    };

    return ret;
}
#endif

static database *read_base(user_interaction & dialog, const string & base, bool partial)
{
    generic_file *f = database_header_open(dialog, base);
    database *ret = NULL;

    try
    {
        ret = new database(*f, partial);
        if(ret == NULL)
            throw Ememory("read_base");
    }
    catch(Erange & e)
    {
        string msg = string(gettext("Corrupted database. "))+e.get_message();
        if(f != NULL)
            delete f;
        dialog.warning(msg);
        throw Edata(msg);
    }
    catch(...)
    {
	if(f != NULL)
            delete f;
        throw;
    }
    if(f != NULL)
        delete f;
    return ret;
}

static void write_base(user_interaction & dialog, const string & filename, const database *base, bool overwrite)
{
    generic_file *dat = database_header_create(dialog, filename, overwrite);

    try
    {
        base->dump(*dat);
    }
    catch(...)
    {
        delete dat;
        throw;
    }

    delete dat;
}
