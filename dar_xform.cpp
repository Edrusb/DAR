/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: dar_xform.cpp,v 1.5 2002/10/31 21:02:35 edrusb Rel $
//
/*********************************************************************/
//
#include <unistd.h>
#include <iostream.h>
#include "sar.hpp"
#include "sar_tools.hpp"
#include "user_interaction.hpp"
#include "tools.hpp"
#include "tuyau.hpp"
#include "dar_suite.hpp"
#include "integers.hpp"

#define EXTENSION "dar"
#define DAR_XFORM_VERSION "1.1.0"

static bool command_line(S_I argc, char *argv[], 
			 path * & src_dir, string & src, 
			 path * & dst_dir, string & dst,
			 infinint & first_file_size,
			 infinint & file_size,
			 bool & warn,
			 bool & allow,
			 bool & pause,
			 bool & beep,
			 string & execute_src,
			 string & execute_dst);
static void show_usage(const char *command_name);
static void show_version(const char *command_name);
static S_I sub_main(S_I argc, char *argv[]);

S_I main(S_I argc, char *argv[])
{
    return dar_suite_global(argc, argv, &sub_main);
}

static S_I sub_main(S_I argc, char *argv[])
{
    path *src_dir = NULL;
    path *dst_dir = NULL;
    string src, dst;
    infinint first, size;
    bool warn, allow, pause, beep;
    string execute_src, execute_dst;

    if(command_line(argc, argv, src_dir, src, dst_dir, dst, first, size,
		    warn, allow, pause, beep, execute_src, execute_dst))
    {
	try
	{
	    generic_file *dst_sar = NULL;
	    generic_file *src_sar = NULL;
	    
	    if(dst != "-")
		user_interaction_change_non_interactive_output(&cout);
	    try
	    {
		S_I dst_opt = 
		    (allow ? 0 : SAR_OPT_DONT_ERASE) 
		    | (warn ? SAR_OPT_WARN_OVERWRITE : 0)
		    | (pause ? SAR_OPT_PAUSE : 0);
		user_interaction_set_beep(beep);
		if(src == "-")
		{
		    generic_file *tmp = new tuyau(0, gf_read_only);
		    
		    if(tmp == NULL)
			throw Ememory("main");
		    try
		    {
			src_sar = new trivial_sar(tmp);
			if(src_sar == NULL)
			    delete tmp;
			tmp = NULL;
		    }
		    catch(...)
		    {
			if(tmp != NULL)
			    delete tmp;
			throw;
		    }
		}
		else 
		    src_sar = new sar(src, EXTENSION, SAR_OPT_DEFAULT, *src_dir, execute_src);
		if(src_sar == NULL)
		    throw Ememory("main");
		
		if(size == 0)
		    if(dst == "-")
			dst_sar = sar_tools_open_archive_tuyau(1, gf_write_only);
		    else
			dst_sar = sar_tools_open_archive_fichier((*dst_dir + sar_make_filename(dst, 1, EXTENSION)).display(), allow, warn);
		else
		    dst_sar = new sar(dst, EXTENSION, size, first, dst_opt, *dst_dir, execute_dst);
		if(dst_sar == NULL)
		    throw Ememory("main");

		try
		{
		    src_sar->copy_to(*dst_sar);
		}
		catch(Escript & e)
		{
		    throw;
		}
		catch(Euser_abort & e)
		{
		    throw;
		}
		catch(Ebug & e)
		{
		    throw;
		}
		catch(Egeneric & e)
		{
		    string msg = string("Error transforming the archive :")+e.get_message();
		    user_interaction_warning(msg);
		    throw Edata(msg);
		}
	    }
	    catch(...)
	    {
		if(dst_sar != NULL)
		    delete dst_sar;
		if(src_sar != NULL)
		    delete src_sar;
		throw;
	    }
	    delete src_sar;
	    delete dst_sar;
	}
	catch(...)
	{
	    delete src_dir;
	    delete dst_dir;
	    throw;
	}
	delete src_dir;
	delete dst_dir;
	
	return EXIT_OK;
    }
    else
	return EXIT_SYNTAX;
}

static bool command_line(S_I argc, char *argv[], 
			 path * & src_dir, string & src, 
			 path * & dst_dir, string & dst,
			 infinint & first_file_size,
			 infinint & file_size,
			 bool & warn,
			 bool & allow,
			 bool & pause,
			 bool & beep,
			 string & execute_src,
			 string & execute_dst)
{
    S_I lu;
    src_dir = NULL;
    dst_dir = NULL;
    warn = true;
    allow = true;
    pause = false;
    beep = false;
    first_file_size = 0;
    file_size = 0;
    src = dst = "";
    execute_src = execute_dst = "";

    try
    {
	while((lu = getopt(argc, argv, "s:S:pwnhbVE:F:")) != EOF)
	{
	    switch(lu)
	    {
	    case 's':
		if(file_size != 0)
		    throw Erange("command_line", "only one -s option is allowed");
		if(optarg == NULL)
		    throw Erange("command_line", "missing argument to -s");
		else
		{
		    try
		    {
			file_size = tools_get_extended_size(optarg);
			if(first_file_size == 0)
			    first_file_size = file_size;
		    }
		    catch(Edeci &e)
		    {
			user_interaction_warning("invalid size for option -s");
			return false;
		    }
		}
		break;
	    case 'S':
		if(optarg == NULL)
		    throw Erange("command_line", "missing argument to -S");
		if(first_file_size == 0)
		    first_file_size = tools_get_extended_size(optarg);
		else
		    if(file_size == 0)
			throw Erange("command_line", "only one -S option is allowed");
		    else
			if(file_size == first_file_size)
			{
			    try
			    {
				first_file_size = tools_get_extended_size(optarg);
				if(first_file_size == file_size)
				    user_interaction_warning("specifying -S with the same value as the one given in -s is useless");
			    }
			    catch(Egeneric &e)
			    {
				user_interaction_warning("invalid size for option -S");
				return false;
			    }
			    
			}
			else
			    throw Erange("command_line", "only one -S option is allowed");
		break;
	    case 'p':
		pause = true;
		break;
	    case 'w':
		warn = false;
		break;
	    case 'n':
		allow = false;
		break;
	    case 'b':
		beep = true;
		break;
	    case 'h':
		show_usage(argv[0]);
		return false;
	    case 'V':
		show_version(argv[0]);
		return false;
	    case 'E':
		if(optarg == NULL)
		    throw Erange("get_args", "missing argument to -E");
		if(execute_dst == "")
		    execute_dst = optarg;
		else
		    user_interaction_warning("only one -E option is allowed, ignoring other instances");
		break;
	    case 'F':
		if(optarg == NULL)
		    throw Erange("get_args", "missing argument to -F");
		if(execute_src == "")
		    execute_src = optarg;
		else
		    user_interaction_warning("only one -F option is allowed, ignoring other instances");
		break;
	    case ':':
		throw Erange("command_line", string("missing parameter to option ") + char(optopt));
	    case '?':
		user_interaction_warning(string("ignoring unknown option ") + char(optopt)); 
		break;
	    default:
		throw SRC_BUG;
	    }
	}

	    // reading arguments remain on the command line 
	if(optind + 2 > argc)
	{
	    user_interaction_warning("missing source or destination argument on command line, see -h option for help");
	    return false;
	}
	if(optind + 2 < argc)
	{
	    user_interaction_warning("too many argument on command line, see -h option for help");
	    return false;
	}
	if(argv[optind] != "")
	    tools_split_path_basename(argv[optind], src_dir, src);
	else
	{
	    user_interaction_warning("invalid argument as source archive");
	    return false;
	}
	if(argv[optind+1] != "")
	    tools_split_path_basename(argv[optind+1], dst_dir, dst);
	else
	{
	    user_interaction_warning("invalid argument as destination archive");
	    return false;
	}

	    // sanity checks
	if(dst == "-" && file_size != 0)
	    throw Erange("dar_xform::command_line", "archive on stdout is not compatible with slicing (-s option)");
    }
    catch(...)
    {
	if(src_dir != NULL)
	    delete src_dir;
	if(dst_dir != NULL)
	    delete dst_dir;
	throw;
    }
    return true;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: dar_xform.cpp,v 1.5 2002/10/31 21:02:35 edrusb Rel $";
    dummy_call(id);
}

static void show_usage(const char *command_name)
{
    char *name = tools_extract_basename(command_name);

    try
    {
	ui_printf("usage :\t %s [options] [<path>/]<basename> [<path>/]<basename>\n", name);
	ui_printf("       \t %s -h\n",name);
	ui_printf("       \t %s -V\n",name);
#include "dar_xform.usage"
    }
    catch(...)
    {
	delete name;
	throw;
    }
    delete name;
}

static void show_version(const char *command_name)
{
    char *name = tools_extract_basename(command_name);

    try
    {
	ui_printf("\n %s version %s, Copyright (C) 2002 Denis Corbin\n\n", name, DAR_XFORM_VERSION);
	ui_printf(" %s with %s version %s\n", __DATE__, CC_NAT, __VERSION__);
	ui_printf(" %s is part of the Disk ARchive suite (DAR %s)\n", name,dar_suite_version());
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
