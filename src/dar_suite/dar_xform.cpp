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
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#include "getopt_decision.h"
} // end extern "C"

#include <iostream>
#include "sar.hpp"
#include "sar_tools.hpp"
#include "user_interaction.hpp"
#include "tools.hpp"
#include "tuyau.hpp"
#include "dar_suite.hpp"
#include "integers.hpp"
#include "shell_interaction.hpp"
#include "libdar.hpp"
#include "thread_cancellation.hpp"
#include "header.hpp"
#include "line_tools.hpp"
#include "entrepot.hpp"

using namespace libdar;

#define DAR_XFORM_VERSION "1.5.2"

static bool command_line(user_interaction & dialog,
			 S_I argc, char * const argv[],
                         path * & src_dir, string & src,
                         path * & dst_dir, string & dst,
                         infinint & first_file_size,
                         infinint & file_size,
                         bool & warn,
                         bool & allow,
                         infinint & pause,
                         bool & beep,
                         string & execute_src,
                         string & execute_dst,
			 string & slice_perm,
			 string & slice_user,
			 string & slice_group,
			 hash_algo & hash,
			 infinint & src_min_digits,
			 infinint & dst_min_digits);

static void show_usage(user_interaction & dialog, const char *command_name);
static void show_version(user_interaction & dialog, const char *command_name);
static S_I sub_main(user_interaction & dialog, S_I argc, char *const argv[], const char **env);

int main(S_I argc, char *const argv[], const char **env)
{
    return dar_suite_global(argc, argv, env, &sub_main);
}

static S_I sub_main(user_interaction & dialog, S_I argc, char * const argv[], const char **env)
{
    path *src_dir = NULL;
    path *dst_dir = NULL;
    string src, dst;
    infinint first, size;
    bool warn, allow, beep;
    infinint pause;
    string execute_src, execute_dst;
    thread_cancellation thr;
    string slice_perm;
    string slice_user;
    string slice_group;
    hash_algo hash;
    infinint src_min_digits;
    infinint dst_min_digits;
    S_I ret = EXIT_OK;

    try
    {
	if(command_line(dialog, argc, argv, src_dir, src, dst_dir, dst, first, size,
			warn, allow, pause, beep, execute_src, execute_dst, slice_perm, slice_user, slice_group, hash,
			src_min_digits, dst_min_digits))
	{
	    generic_file *dst_sar = NULL;
	    generic_file *src_sar = NULL;
	    label data_name;
	    entrepot_local entrep = entrepot_local(slice_perm, slice_user, slice_group, false);

	    data_name.clear();
	    try
	    {
		if(dst != "-")
		{
		    shell_interaction_change_non_interactive_output(&cout);
		    tools_avoid_slice_overwriting_regex(dialog,  dst_dir->display(), string("^")+dst+"\\.[0-9]+\\."+EXTENSION+"(\\.(md5|sha1))?$", false, allow, warn, false);
		}

		thr.check_self_cancellation();

		shell_interaction_set_beep(beep);
		if(src == "-")
		{
		    trivial_sar *tmp_sar = new trivial_sar(dialog, src, false);
		    if(tmp_sar == NULL)
			throw Ememory("sub_main");

		    src_sar = tmp_sar;
		    if(src_sar != NULL)
			data_name = tmp_sar->get_data_name();
		    else
			throw SRC_BUG;
		}
		else 	// source not from a pipe
		{
		    if(src_dir != NULL)
			entrep.set_location(src_dir->display());
		    else
			throw SRC_BUG;

		    sar *tmp_sar = new sar(dialog, src, EXTENSION, entrep, false, src_min_digits, false, execute_src);
		    if(tmp_sar == NULL)
			throw Ememory("main");
		    else
			tmp_sar->set_info_status(CONTEXT_OP);
		    src_sar = tmp_sar;
		    if(src_sar != NULL)
			data_name = tmp_sar->get_data_name();
		    else
			throw SRC_BUG;
		}

		if(size == 0)
		    if(dst == "-")
			dst_sar = sar_tools_open_archive_tuyau(dialog, 1, gf_write_only, data_name, execute_dst);
		    else
		    {
			if(dst_dir != NULL)
			    entrep.set_location(dst_dir->display());
			else
			    throw SRC_BUG;
			dst_sar = new trivial_sar(dialog, dst, EXTENSION, entrep, data_name, execute_dst, allow, warn, hash, dst_min_digits);
		    }
		else
		{
		    if(dst_dir != NULL)
			entrep.set_location(dst_dir->display());
		    else
			throw SRC_BUG;
		    dst_sar = new sar(dialog, dst, EXTENSION, size, first, warn, allow, pause, entrep, data_name, hash, dst_min_digits, execute_dst);
		}
		if(dst_sar == NULL)
		    throw Ememory("main");

		thr.check_self_cancellation();
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
		catch(Ethread_cancel & e)
		{
		    throw;
		}
		catch(Egeneric & e)
		{
		    string msg = string(gettext("Error transforming the archive :"))+e.get_message();
		    dialog.warning(msg);
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
	    if(src_sar != NULL)
		delete src_sar;
	    if(dst_sar != NULL)
		delete dst_sar;

	    ret = EXIT_OK;
	}
	else
	    ret = EXIT_SYNTAX;
    }
    catch(...)
    {
	if(src_dir != NULL)
	    delete src_dir;
	if(dst_dir != NULL)
	    delete dst_dir;
	throw;
    }
    if(src_dir != NULL)
	delete src_dir;
    if(dst_dir != NULL)
	delete dst_dir;
    return ret;
}

static bool command_line(user_interaction & dialog, S_I argc, char * const argv[],
                         path * & src_dir, string & src,
                         path * & dst_dir, string & dst,
                         infinint & first_file_size,
                         infinint & file_size,
                         bool & warn,
                         bool & allow,
                         infinint & pause,
                         bool & beep,
                         string & execute_src,
                         string & execute_dst,
			 string & slice_perm,
			 string & slice_user,
			 string & slice_group,
			 hash_algo & hash,
			 infinint & src_min_digits,
			 infinint & dst_min_digits)
{
    S_I lu;
    src_dir = NULL;
    dst_dir = NULL;
    warn = true;
    allow = true;
    pause = 0;
    beep = false;
    first_file_size = 0;
    file_size = 0;
    src = dst = "";
    execute_src = execute_dst = "";
    U_I suffix_base = TOOLS_BIN_SUFFIX;
    slice_perm = "";
    slice_user = "";
    slice_group = "";
    hash = hash_none;
    src_min_digits = 0;
    dst_min_digits = 0;

    try
    {
        while((lu = getopt(argc, argv, "s:S:p::wnhbVE:F:a::Qj^:3:;:")) != EOF)
        {
            switch(lu)
            {
            case 's':
                if(file_size != 0)
                    throw Erange("command_line", gettext("Only one -s option is allowed"));
                if(optarg == NULL)
                    throw Erange("command_line", gettext("Missing argument to -s"));
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
                        dialog.warning(gettext("Invalid size for option -s"));
                        return false;
                    }
                }
                break;
            case 'S':
                if(optarg == NULL)
                    throw Erange("command_line", gettext("Missing argument to -S"));
                if(first_file_size == 0)
                    first_file_size = tools_get_extended_size(optarg, suffix_base);
                else
                    if(file_size == 0)
                        throw Erange("command_line", gettext("Only one -S option is allowed"));
                    else
                        if(file_size == first_file_size)
                        {
                            try
                            {
                                first_file_size = tools_get_extended_size(optarg, suffix_base);
                                if(first_file_size == file_size)
                                    dialog.warning(gettext("Giving -S option the same value as the one given to -s is useless"));
                            }
                            catch(Egeneric &e)
                            {
                                dialog.warning(gettext("Invalid size for option -S"));
                                return false;
                            }

                        }
                        else
                            throw Erange("command_line", gettext("Only one -S option is allowed"));
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
                show_usage(dialog, argv[0]);
                return false;
            case 'V':
                show_version(dialog, argv[0]);
                return false;
            case 'E':
                if(optarg == NULL)
                    throw Erange("command_line", gettext("Missing argument to -E"));
                if(execute_dst == "")
                    execute_dst = optarg;
                else
		    execute_dst += string(" ; ") + optarg;
                break;
            case 'F':
                if(optarg == NULL)
                    throw Erange("command_line", gettext("Missing argument to -F"));
                if(execute_src == "")
                    execute_src = optarg;
                else
		    execute_src += string(" ; ") + optarg;
                break;
	    case 'a':
		if(optarg == NULL)
		    throw Erange("command_line", gettext("-a option requires an argument"));
		if(strcasecmp("SI-unit", optarg) == 0 || strcasecmp("SI", optarg) == 0 || strcasecmp("SI-units", optarg) == 0)
		    suffix_base = TOOLS_SI_SUFFIX;
		else
		    if(strcasecmp("binary-unit", optarg) == 0 || strcasecmp("binary", optarg) == 0 || strcasecmp("binary-units", optarg) == 0)
			suffix_base = TOOLS_BIN_SUFFIX;
		    else
			throw Erange("command_line", string(gettext("Unknown parameter given to -a option: ")) + optarg);
		break;
	    case 'Q':
	    case 'j':
		break;  // ignore this option already parsed during initialization (dar_suite.cpp)
	    case '^':
		if(optarg == NULL)
		    throw Erange("command_line", tools_printf(gettext("Missing argument to -^"), char(lu)));
		line_tools_slice_ownership(string(optarg), slice_perm, slice_user, slice_group);
		break;
	    case '3':
		if(optarg == NULL)
		    throw Erange("command_line", tools_printf(gettext("Missing argument to --hash"), char(lu)));
		if(strcasecmp(optarg, "md5") == 0)
		    hash = hash_md5;
		else
		    if(strcasecmp(optarg, "sha1") == 0)
			hash = hash_sha1;
		    else
			throw Erange("command_line", string(gettext("Unknown parameter given to --hash option: ")) + optarg);
		break;
	    case ';':
		if(optarg == NULL)
		    throw Erange("command_line", tools_printf(gettext("Missing argument to --min-digits"), char(lu)));
		else
		{
		    infinint tmp2;
		    line_tools_get_min_digits(optarg, src_min_digits, dst_min_digits, tmp2);
		}
		break;
            case ':':
                throw Erange("command_line", tools_printf(gettext("Missing parameter to option -%c"), char(optopt)));
            case '?':
                dialog.warning(tools_printf(gettext("Ignoring unknown option -%c"), char(optopt)));
                break;
            default:
                throw SRC_BUG;
            }
        }

            // reading arguments remain on the command line
        if(optind + 2 > argc)
        {
            dialog.warning(gettext("Missing source or destination argument on command line, see -h option for help"));
            return false;
        }
        if(optind + 2 < argc)
        {
            dialog.warning(gettext("Too many argument on command line, see -h option for help"));
            return false;
        }
        if(string(argv[optind]) != string(""))
	{
            tools_split_path_basename(argv[optind], src_dir, src);
	    tools_check_basename(dialog, *src_dir, src, EXTENSION);
	}
        else
        {
            dialog.warning(gettext("Invalid argument as source archive"));
            return false;
        }
        if(string(argv[optind+1]) != string(""))
            tools_split_path_basename(argv[optind+1], dst_dir, dst);
        else
        {
            dialog.warning(gettext("Invalid argument as destination archive"));
            return false;
        }

            // sanity checks
        if(dst == "-" && file_size != 0)
            throw Erange("dar_xform::command_line", gettext("Archive on stdout is not compatible with slicing (-s option)"));
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

static void show_usage(user_interaction & dialog, const char *command_name)
{
    string name;
    tools_extract_basename(command_name, name);
    shell_interaction_change_non_interactive_output(&cout);

    dialog.printf("usage :\t %s [options] [<path>/]<basename> [<path>/]<basename>\n", name.c_str());
    dialog.printf("       \t %s -h\n",name.c_str());
    dialog.printf("       \t %s -V\n",name.c_str());
#include "dar_xform.usage"
}

static void show_version(user_interaction & dialog, const char *command_name)
{
    string name;
    tools_extract_basename(command_name, name);
    U_I maj, med, min;

    get_version(maj, med, min);
    shell_interaction_change_non_interactive_output(&cout);

    dialog.printf("\n %s version %s, Copyright (C) 2002-2052 Denis Corbin\n\n", name.c_str(), DAR_XFORM_VERSION);
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
}
