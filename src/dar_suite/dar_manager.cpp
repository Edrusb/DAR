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
// $Id: dar_manager.cpp,v 1.18.2.2 2003/12/20 23:05:34 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_GETOPT_H
#include <getopt.h>
#endif
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

using namespace libdar;

#define DAR_MANAGER_VERSION "1.2.1"

enum operation { none_op, create, add, listing, del, chbase, where, options, dar, restore, used, files, stats, move };

static S_I little_main(S_I argc, char *argv[], const char **env);
static bool command_line(S_I argc, char *argv[],
                         operation & op,
                         string & base,
                         string & arg,
                         archive_num & num,
                         vector<string> & rest,
                         archive_num & num2,
                         bool & verbose);
static void show_usage(const char *command);
static void show_version(const char *command);
#if HAVE_GETOPT_H
static const struct option *get_long_opt();
#endif
static void op_create(const string & base, bool info_details);
static void op_add(const string & base, const string &arg, string fake, bool info_details);
static void op_listing(const string & base, bool info_details);
static void op_del(const string & base, archive_num num, bool info_details);
static void op_chbase(const string & base, archive_num num, const string & arg, bool info_details);
static void op_where(const string & base, archive_num num, const string & arg, bool info_details);
static void op_options(const string & base, const vector<string> & rest, bool info_details);
static void op_dar(const string & base, const string & arg, bool info_details);
static void op_restore(const string & base, const vector<string> & rest, bool info_details);
static void op_used(const string & base, archive_num num, bool info_details);
static void op_files(const string & base, const string & arg, bool info_details);
static void op_stats(const string & base, bool info_details);
static void op_move(const string & base, archive_num src, archive_num dst, bool info_details);

static database *read_base(const string & base, bool partial);
static void write_base(const string & filename, const database *base, bool overwrite);

int main(S_I argc, char *argv[], const char **env)
{
    return dar_suite_global(argc, argv, env, &little_main);
}

S_I little_main(S_I argc, char *argv[], const char **env)
{
    operation op;
    string base;
    string arg;
    archive_num num, num2;
    vector<string> rest;
    bool info_details;

    shell_interaction_change_non_interactive_output(&cout);
    if(!command_line(argc, argv, op, base, arg, num, rest, num2, info_details))
        return EXIT_SYNTAX;
    MEM_IN;
    switch(op)
    {
    case none_op:
        throw SRC_BUG;
    case create:
        op_create(base, info_details);
        break;
    case add:
        op_add(base, arg, rest.size() > 0 ? rest[0] : "", info_details);
        break;
    case listing:
        op_listing(base, info_details);
        break;
    case del:
        op_del(base, num, info_details);
        break;
    case chbase:
        op_chbase(base, num, arg, info_details);
        break;
    case where:
        op_where(base, num, arg, info_details);
        break;
    case options:
        op_options(base, rest, info_details);
        break;
    case dar:
        op_dar(base, arg, info_details);
        break;
    case restore:
        op_restore(base, rest, info_details);
        break;
    case used:
        op_used(base, num, info_details);
        break;
    case files:
        op_files(base, arg, info_details);
        break;
    case stats:
        op_stats(base, info_details);
        break;
    case move:
        op_move(base, num, num2, info_details);
        break;
    default:
        throw SRC_BUG;
    }
    MEM_OUT;
    return EXIT_OK;
}

static bool command_line(S_I argc, char *argv[],
                         operation & op,
                         string & base,
                         string & arg,
                         archive_num & num,
                         vector<string> & rest,
                         archive_num & num2,
                         bool & verbose)
{
    S_I lu;

    base = arg = "";
    num = num2 = 0;
    rest.clear();
    op = none_op;
    verbose = false;
    string chem, filename;

#if HAVE_GETOPT_H
    while((lu = getopt_long(argc, argv, "C:B:A:lD:b:p:od:ru:f:shVm:v", get_long_opt(), NULL)) != EOF)
#else
	while((lu = getopt(argc, argv, "C:B:A:lD:b:p:od:ru:f:shVm:v")) != EOF)
#endif
	{
	    switch(lu)
	    {
	    case 'C':
		if(op != none_op)
		    throw Erange("command_line", "only one action is allowed at a time");
		op = create;
		if(optarg == NULL)
		    throw Erange("command_line", "missing argument to -C");
		base = optarg;
		break;
	    case 'B':
		if(base != "")
		    throw Erange("command_line", "only one -B option on command line is allowed");
		if(optarg == NULL)
		    throw Erange("command_line", "missing argument to -B");
		base = optarg;
		break;
	    case 'A':
		if(op != none_op)
		    throw Erange("command_line", "only one action is allowed at a time");
		op = add;
		if(optarg == NULL)
		    throw Erange("command_line", "missing argument to -A");
		tools_split_path_basename(optarg, chem, filename);
		tools_check_basename(chem, filename, EXTENSION);
		arg = (path(chem)+filename).display();
		break;
	    case 'l':
		if(op != none_op)
		    throw Erange("command_line", "only one action is allowed at a time");
		op = listing;
		break;
	    case 'D':
		if(op != none_op)
		    throw Erange("command_line", "only one action is allowed at a time");
		op = del;
		if(optarg == NULL)
		    throw Erange("command_line", "missing argument to -D");
		num = atoi(optarg);
		break;
	    case 'b':
		if(op != none_op)
		    throw Erange("command_line", "only one action is allowed at a time");
		op = chbase;
		if(optarg == NULL)
		    throw Erange("command_line", "missing argument to -b");
		num = atoi(optarg);
		break;
	    case 'p':
		if(op != none_op)
		    throw Erange("command_line", "only one action is allowed at a time");
		op = where;
		if(optarg == NULL)
		    throw Erange("command_line", "missing argument to -p");
		num = atoi(optarg);
		break;
	    case 'o':
		if(op != none_op)
		    throw Erange("command_line", "only one action is allowed at a time");
		op = options;
		break;
	    case 'd':
		if(op != none_op)
		    throw Erange("command_line", "only one action is allowed at a time");
		op = dar;
		if(optarg == NULL)
		    throw Erange("command_line", "missing argument to -d");
		arg = optarg;
		break;
	    case 'r':
		if(op != none_op)
		    throw Erange("command_line", "only one action is allowed at a time");
		op = restore;
		break;
	    case 'u':
		if(op != none_op)
		    throw Erange("command_line", "only one action is allowed at a time");
		op = used;
		if(optarg == NULL)
		    throw Erange("command_line", "missing argument to -u");
		num = atoi(optarg);
		break;
	    case 'f':
		if(op != none_op)
		    throw Erange("command_line", "only one action is allowed at a time");
		op = files;
		if(optarg == NULL)
		    throw Erange("command_line", "missing argument to -p");
		arg = optarg;
		break;
	    case 's':
		if(op != none_op)
		    throw Erange("command_line", "only one action is allowed at a time");
		op = stats;
		break;
	    case 'm':
		if(op != none_op)
		    throw Erange("command_line", "only one action is allowed at a time");
		op = move;
		if(optarg == NULL)
		    throw Erange("command_line", "missing argument to -m");
		num = atoi(optarg);
		break;
	    case 'h':
		show_usage(argv[0]);
		return false;
	    case 'V':
		show_version(argv[0]);
		return false;
	    case 'v':
		verbose = true;
		break;
	    case '?':
		user_interaction_warning(string("ignoring unknown option ") + char(optopt));
		break;
	    default:
		user_interaction_warning(string("ignoring unknown option ") + char(lu));
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
        user_interaction_warning("no action specified, aborting");
        return false;
    case create:
    case listing:
    case del:
    case dar:
    case used:
    case files:
    case stats:
        if(rest.size() > 0)
            user_interaction_warning("ignoring extra arguments on command line");
        break;
    case add:
        if(rest.size() > 1)
            user_interaction_warning("ignoring extra arguments on command line");
        break;
    case chbase:
    case where:
        if(rest.size() != 1)
        {
            user_interaction_warning("missing argument to command line, aborting");
            return false;
        }
        arg = rest[0];
        rest.clear();
        break;
    case restore:
        for(unsigned int i = 0; i < rest.size(); i++)
            if(!path(rest[i]).is_relative())
                throw Erange("command_line", "arguments to -r must be relative path (never begin by '/')");
        break;
    case options:
        break;
    case move:
        if(rest.size() != 1)
        {
            user_interaction_warning("missing argument to command line, aborting");
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
        user_interaction_warning("no database specified, aborting");
        return false;
    }

    return true;
}


static void dummy_call(char *x)
{
    static char id[]="$Id: dar_manager.cpp,v 1.18.2.2 2003/12/20 23:05:34 edrusb Rel $";
    dummy_call(id);
}

static void op_create(const string & base, bool info_details)
{
    database dat; // created empty;

    if(info_details)
    {
        user_interaction_warning("Creating file...");
        user_interaction_warning("Formating file as an empty database...");
    }
    write_base(base, &dat, false);
    if(info_details)
        user_interaction_warning("Database has been successfully created empty.");
}

static void op_add(const string & base, const string &arg, string fake, bool info_details)
{
    if(info_details)
        user_interaction_warning("Uncompressing and loading database in memory...");
    database *dat = read_base(base, false);

    try
    {
        if(info_details)
            user_interaction_warning("Reading catalogue of the archive to add...");
        catalogue *catal = macro_tools_get_catalogue_from(arg, EXTENSION, crypto_none, "");

        try
        {
            string chemin, b;

            if(info_details)
                user_interaction_warning("Updating database with catalogue...");
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
            user_interaction_warning("Compressing and writing back database to file...");
        write_base(base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;
}

static void op_listing(const string & base, bool info_details)
{
    if(info_details)
        user_interaction_warning("Uncompressing and loading database header in memory...");
    database *dat = read_base(base, true);

    try
    {
        dat->show_contents();
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;
}

static void op_del(const string & base, archive_num num, bool info_details)
{
    if(info_details)
        user_interaction_warning("Uncompressing and loading database in memory...");
    database *dat = read_base(base, false);

    try
    {
        if(info_details)
            user_interaction_warning("Removing informations from the archive...");
        dat->remove_archive(num);
        if(info_details)
            user_interaction_warning("Compressing and writing back database to file...");
        write_base(base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;
}

static void op_chbase(const string & base, archive_num num, const string & arg, bool info_details)
{
    if(info_details)
        user_interaction_warning("Uncompressing and loading database header in memory...");
    database *dat = read_base(base, true);

    try
    {
        if(info_details)
            user_interaction_warning("Changing database header information...");
        dat->change_name(num, arg);
        if(info_details)
            user_interaction_warning("Compressing and writing back database header to file...");
        write_base(base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;

}

static void op_where(const string & base, archive_num num, const string & arg, bool info_details)
{
    if(info_details)
        user_interaction_warning("Uncompressing and loading database header in memory...");
    database *dat = read_base(base, true);

    try
    {
        if(info_details)
            user_interaction_warning("Changing database header information...");
        dat->set_path(num, arg);
        if(info_details)
            user_interaction_warning("Compressing and writing back database header to file...");
        write_base(base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;

}

static void op_options(const string & base, const vector<string> & rest, bool info_details)
{
    if(info_details)
        user_interaction_warning("Uncompressing and loading database header in memory...");
    database *dat = read_base(base, true);

    try
    {
        if(info_details)
            user_interaction_warning("Changing database header information...");
        dat->set_options(rest);
        if(info_details)
            user_interaction_warning("Compressing and writing back database header to file...");
        write_base(base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;

}

static void op_dar(const string & base, const string & arg, bool info_details)
{
    if(info_details)
        user_interaction_warning("Uncompressing and loading database header in memory...");
    database *dat = read_base(base, true);

    try
    {
        if(info_details)
            user_interaction_warning("Changing database header information...");
        dat->set_dar_path(arg);
        if(info_details)
            user_interaction_warning("Compressing and writing back database header to file...");
        write_base(base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;

}

static void op_restore(const string & base, const vector<string> & rest, bool info_details)
{
    if(info_details)
        user_interaction_warning("Uncompressing and loading database in memory...");
    database *dat = read_base(base, false);

    try
    {
        if(info_details)
            user_interaction_warning("Looking for archives of most recent versions, sorting files by archive to use for restoration...");
        dat->restore(rest, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;
}

static void op_used(const string & base, archive_num num, bool info_details)
{
    if(info_details)
        user_interaction_warning("Uncompressing and loading database in memory...");
    database *dat = read_base(base, false);

    try
    {
        dat->show_files(num);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;

}

static void op_files(const string & base, const string & arg, bool info_details)
{
    if(info_details)
        user_interaction_warning("Uncompressing and loading database in memory...");
    database *dat = read_base(base, false);

    try
    {
        dat->show_version(arg);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;

}

static void op_stats(const string & base, bool info_details)
{
    if(info_details)
        user_interaction_warning("Uncompressing and loading database in memory...");
    database *dat = read_base(base, false);

    try
    {
        if(info_details)
            user_interaction_warning("Computing statistics...");
        dat->show_most_recent_stats();
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;
}

static void op_move(const string & base, archive_num src, archive_num dst, bool info_details)
{
    if(info_details)
        user_interaction_warning("Uncompressing and loading database in memory...");
    database *dat = read_base(base, false);

    try
    {
        if(info_details)
            user_interaction_warning("changing database information...");
        dat->set_permutation(src, dst);
        if(info_details)
            user_interaction_warning("Compressing and writing back database header to file...");
        write_base(base, dat, true);
    }
    catch(...)
    {
        delete dat;
        throw;
    }
    delete dat;
}

static void show_usage(const char *command)
{
    ui_printf("usage :\n\n");
    ui_printf("\tdar_manager [-v] -C [<path>/]<database>\n");
    ui_printf("\tdar_manager [-v] -B [<path>/]<database> -A [<path>/]<basename> [[<path>/]<archive_basename>]\n");
    ui_printf("\tdar_manager [-v] -B [<path>/]<database> -l\n");
    ui_printf("\tdar_manager [-v] -B [<path>/]<database> -D <number>\n");
    ui_printf("\tdar_manager [-v] -B [<path>/]<database> -b <number> <new_archive_basename>\n");
    ui_printf("\tdar_manager [-v] -B [<path>/]<database> -p <number> <path>\n");
    ui_printf("\tdar_manager [-v] -B [<path>/]<database> -o [list of options to pass to dar]\n");
    ui_printf("\tdar_manager [-v] -B [<path>/]<database> -d [<path to dar command>]\n");
    ui_printf("\tdar_manager [-v] -B [<path>/]<database> -r [list of files to restore]\n");
    ui_printf("\tdar_manager [-v] -B [<path>/]<database> -u <number>\n");
    ui_printf("\tdar_manager [-v] -B [<path>/]<database> -f file\n");
    ui_printf("\tdar_manager [-v] -B [<path>/]<database> -s\n");
    ui_printf("\tdar_manager [-v] -B [<path>/]<database> -m <number> <number>\n");
    ui_printf("\tdar_manager -h\n");
    ui_printf("\tdar_manager -V\n");
    ui_printf("\n");
#include "dar_manager.usage"
}

static void show_version(const char *command_name)
{
    char *name = tools_extract_basename(command_name);
    U_I maj, min, bits;
    bool ea, largefile, nodump, special_alloc;

    get_version(maj, min);
    get_compile_time_features(ea, largefile, nodump, special_alloc, bits);
    try
    {
        ui_printf("\n %s version %s, Copyright (C) 2002-2052 Denis Corbin\n\n", name, DAR_MANAGER_VERSION);
        ui_printf(" Using libdar %u.%u built with compilation time options:\n", maj, min);
        tools_display_features(ea, largefile, nodump, special_alloc, bits);
        ui_printf("\n");
        ui_printf(" %s with %s version %s\n", __DATE__, CC_NAT, __VERSION__);
        ui_printf(" %s is part of the Disk ARchive suite (Release %s)\n", name, PACKAGE_VERSION);
        ui_printf(" %s comes with ABSOLUTELY NO WARRANTY; for details\n", name);
        ui_printf(" type `%s -W'.  This is free software, and you are welcome\n", "dar");
        ui_printf(" to redistribute it under certain conditions; type `%s -L | more'\n", "dar");
        ui_printf(" for details.\n\n");
    }
    catch(...)
    {
        delete name;
        throw;
    }
    delete name;
}

#if HAVE_GETOPT_H
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
        { NULL, 0, NULL, 0 }
    };

    return ret;
}
#endif

static database *read_base(const string & base, bool partial)
{
    generic_file *f = database_header_open(base);
    database *ret = NULL;

    try
    {
        ret = new database(*f, partial);
        if(ret == NULL)
            throw Ememory("read_base");
    }
    catch(Erange & e)
    {
        string msg = string("Corrupted database. ")+e.get_message();
        if(f != NULL)
            delete f;
        user_interaction_warning(msg);
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

static void write_base(const string & filename, const database *base, bool overwrite)
{
    generic_file *dat = database_header_create(filename, overwrite);

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
