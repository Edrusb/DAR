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
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include "getopt_decision.h"
} // end extern "C"

#include <iostream>
#include <new>

#include "dar_suite.hpp"
#include "libdar.hpp"
#include "line_tools.hpp"
    // includes directives that should be removed once dar_xform will be integrated into libdar
#include "trivial_sar.hpp"
#include "sar.hpp"
#include "macro_tools.hpp"
#include "libdar.hpp"
    //  ---

using namespace libdar;
using namespace std;

#define DAR_XFORM_VERSION "1.7.0"

#define OPT_STRING  "s:S:p::wnhbVE:F:a::Qj^:3:9:"

static bool command_line(shell_interaction & dialog,
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

static void show_usage(shell_interaction & dialog, const char *command_name);
static void show_version(shell_interaction & dialog, const char *command_name);
static S_I sub_main(shared_ptr<user_interaction> & dialog, S_I argc, char *const argv[], const char **env);

int main(S_I argc, char *const argv[], const char **env)
{
    return dar_suite_global(argc,
			    argv,
			    env,
			    OPT_STRING,
#if HAVE_GETOPT_LONG
			    nullptr,
#endif
			    '\0', // should never be met as option, thus early read the whole command-line for -j and -Q options
			    &sub_main);
}

static S_I sub_main(shared_ptr<user_interaction> & dialog, S_I argc, char * const argv[], const char **env)
{
    path *src_dir = nullptr;
    path *dst_dir = nullptr;
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
    shell_interaction *ptr = dynamic_cast<shell_interaction *>(dialog.get());
    std::unique_ptr<libdar_xform> xform;

    if(!dialog)
	throw SRC_BUG;
    if(ptr == nullptr)
	throw SRC_BUG;

    try
    {
	if(command_line(*ptr, argc, argv, src_dir, src, dst_dir, dst, first, size,
			warn, allow, pause, beep, execute_src, execute_dst, slice_perm, slice_user, slice_group, hash,
			src_min_digits, dst_min_digits))
	{
	    if(dst != "-")
		ptr->change_non_interactive_output(cout);
	    ptr->set_beep(beep);


	    if(src == "-")
		xform.reset(new (nothrow) libdar_xform(dialog, 0));
	    else
		xform.reset(new (nothrow) libdar_xform(dialog,
						       src_dir->display(),
						       src,
						       EXTENSION,
						       src_min_digits,
						       execute_src));
	    if(!xform)
		throw Ememory("dar_xform");

	    if(dst == "-")
		xform->xform_to(1, execute_dst);
	    else
		xform->xform_to(dst_dir->display(),
				dst,
				EXTENSION,
				allow,
				warn,
				pause,
				first,
				size,
				slice_perm,
				slice_user,
				slice_group,
				hash,
				dst_min_digits,
				execute_dst);

	    ret = EXIT_OK;
	}
	else
	    ret = EXIT_SYNTAX;
    }
    catch(...)
    {
	if(src_dir != nullptr)
	    delete src_dir;
	if(dst_dir != nullptr)
	    delete dst_dir;
	throw;
    }
    if(src_dir != nullptr)
	delete src_dir;
    if(dst_dir != nullptr)
	delete dst_dir;
    return ret;
}

static bool command_line(shell_interaction & dialog, S_I argc, char * const argv[],
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
    src_dir = nullptr;
    dst_dir = nullptr;
    warn = true;
    allow = true;
    pause = 0;
    beep = false;
    first_file_size = 0;
    file_size = 0;
    src = dst = "";
    execute_src = execute_dst = "";
    U_I suffix_base = LINE_TOOLS_BIN_SUFFIX;
    slice_perm = "";
    slice_user = "";
    slice_group = "";
    hash = hash_algo::none;
    src_min_digits = 0;
    dst_min_digits = 0;

    try
    {
        while((lu = getopt(argc, argv, OPT_STRING)) != EOF)
        {
            switch(lu)
            {
            case 's':
                if(!file_size.is_zero())
                    throw Erange("command_line", gettext("Only one -s option is allowed"));
                if(optarg == nullptr)
                    throw Erange("command_line", gettext("Missing argument to -s"));
                else
                {
                    try
                    {
                        file_size = tools_get_extended_size(optarg, suffix_base);
                        if(first_file_size.is_zero())
                            first_file_size = file_size;
                    }
                    catch(Edeci &e)
                    {
                        dialog.message(gettext("Invalid size for option -s"));
                        return false;
                    }
                }
                break;
            case 'S':
                if(optarg == nullptr)
                    throw Erange("command_line", gettext("Missing argument to -S"));
                if(first_file_size.is_zero())
                    first_file_size = tools_get_extended_size(optarg, suffix_base);
                else
                    if(file_size.is_zero())
                        throw Erange("command_line", gettext("Only one -S option is allowed"));
                    else
                        if(file_size == first_file_size)
                        {
                            try
                            {
                                first_file_size = tools_get_extended_size(optarg, suffix_base);
                                if(first_file_size == file_size)
                                    dialog.message(gettext("Giving -S option the same value as the one given to -s is useless"));
                            }
                            catch(Egeneric &e)
                            {
                                dialog.message(gettext("Invalid size for option -S"));
                                return false;
                            }

                        }
                        else
                            throw Erange("command_line", gettext("Only one -S option is allowed"));
                break;
            case 'p':
		if(optarg != nullptr)
		{
			// note that the namespace specification is necessary
			// due to similar existing name in std namespace under
			// certain OS (FreeBSD 10.0)
		    libdar::deci conv = string(optarg);
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
                if(optarg == nullptr)
                    throw Erange("command_line", gettext("Missing argument to -E"));
                if(execute_dst == "")
                    execute_dst = optarg;
                else
		    execute_dst += string(" ; ") + optarg;
                break;
            case 'F':
                if(optarg == nullptr)
                    throw Erange("command_line", gettext("Missing argument to -F"));
                if(execute_src == "")
                    execute_src = optarg;
                else
		    execute_src += string(" ; ") + optarg;
                break;
	    case 'a':
		if(optarg == nullptr)
		    throw Erange("command_line", gettext("-a option requires an argument"));
		if(strcasecmp("SI-unit", optarg) == 0 || strcasecmp("SI", optarg) == 0 || strcasecmp("SI-units", optarg) == 0)
		    suffix_base = LINE_TOOLS_SI_SUFFIX;
		else
		    if(strcasecmp("binary-unit", optarg) == 0 || strcasecmp("binary", optarg) == 0 || strcasecmp("binary-units", optarg) == 0)
			suffix_base = LINE_TOOLS_BIN_SUFFIX;
		    else
			throw Erange("command_line", string(gettext("Unknown parameter given to -a option: ")) + optarg);
		break;
	    case 'Q':
		break;  // ignore this option already parsed during initialization (dar_suite.cpp)
	    case '^':
		if(optarg == nullptr)
		    throw Erange("command_line", tools_printf(gettext("Missing argument to -^"), char(lu)));
		line_tools_slice_ownership(string(optarg), slice_perm, slice_user, slice_group);
		break;
	    case '3':
		if(optarg == nullptr)
		    throw Erange("command_line", tools_printf(gettext("Missing argument to --hash"), char(lu)));
		if(strcasecmp(optarg, "md5") == 0)
		    hash = hash_algo::md5;
		else
		    if(strcasecmp(optarg, "sha1") == 0)
			hash = hash_algo::sha1;
		    else
			throw Erange("command_line", string(gettext("Unknown parameter given to --hash option: ")) + optarg);
		break;
	    case '9':
		if(optarg == nullptr)
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
                throw Erange("command_line", tools_printf(gettext("Ignoring unknown option -%c"), char(optopt)));
            default:
                throw SRC_BUG;
            }
        }

            // reading arguments remain on the command line
        if(optind + 2 > argc)
        {
            dialog.message(gettext("Missing source or destination argument on command line, see -h option for help"));
            return false;
        }
        if(optind + 2 < argc)
        {
            dialog.message(gettext("Too many argument on command line, see -h option for help"));
            return false;
        }
        if(string(argv[optind]) != string(""))
	{
            line_tools_split_path_basename(argv[optind], src_dir, src);
	    line_tools_check_basename(dialog, *src_dir, src, EXTENSION, false);
	    if(src_min_digits.is_zero())
		line_tools_check_min_digits(dialog, *src_dir, src, EXTENSION, src_min_digits);
	}
        else
        {
            dialog.message(gettext("Invalid argument as source archive"));
            return false;
        }
        if(string(argv[optind+1]) != string(""))
	{
            line_tools_split_path_basename(argv[optind+1], dst_dir, dst);
	    line_tools_check_basename(dialog, *dst_dir, dst, EXTENSION, true);
	}
        else
        {
            dialog.message(gettext("Invalid argument as destination archive"));
            return false;
        }

            // sanity checks
        if(dst == "-" && !file_size.is_zero())
            throw Erange("dar_xform::command_line", gettext("Archive on stdout is not compatible with slicing (-s option)"));
    }
    catch(...)
    {
        if(src_dir != nullptr)
            delete src_dir;
	src_dir = nullptr;
        if(dst_dir != nullptr)
            delete dst_dir;
	dst_dir = nullptr;
        throw;
    }
    return true;
}

static void show_usage(shell_interaction & dialog, const char *command_name)
{
    string name;
    line_tools_extract_basename(command_name, name);
    dialog.change_non_interactive_output(cout);

    dialog.printf("usage :\t %s [options] [<path>/]<basename> [<path>/]<basename>\n", name.c_str());
    dialog.printf("       \t %s -h\n",name.c_str());
    dialog.printf("       \t %s -V\n",name.c_str());
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("\t\t   the first non options argument is the archive to read\n"));
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("\t\t   the second non option argument is the archive to create\n"));
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("Common options:\n"));
    dialog.printf(gettext("   -h\t\t   displays this help information\n"));
    dialog.printf(gettext("   -V\t\t   displays version information\n"));
    dialog.printf(gettext("   -s <integer>    split the archive in several files of size <integer>\n"));
    dialog.printf(gettext("   -S <integer>    first file size\n"));
    dialog.printf(gettext("   -p\t\t   pauses before writing to a new file\n"));
    dialog.printf(gettext("   -n\t\t   don't overwrite files\n"));
    dialog.printf(gettext("   -w\t\t   don't warn before overwriting files\n"));
    dialog.printf(gettext("   -b\t\t   ring the terminal bell when user action is required\n"));
    dialog.printf(gettext("   -E <string>\t   command to execute between slices of destination archive\n"));
    dialog.printf(gettext("   -F <string>\t   command to execute between slice of source archive\n"));
    dialog.printf(gettext("   -aSI \t   slice size suffixes k, M, T, G, etc. are power of 10\n"));
    dialog.printf(gettext("   -abinary\t   slice size suffixes k, M, T, G, etc. are power of 2\n"));
    dialog.printf(gettext("   -^ <string>\t   permission[:user[:group]] of created slices\n"));
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

    dialog.printf("\n %s version %s, Copyright (C) 2002-2025 Denis Corbin\n\n", name.c_str(), DAR_XFORM_VERSION);
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
}
