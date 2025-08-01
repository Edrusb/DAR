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

#if HAVE_STRING_H
# include <string.h>
#endif

#include "getopt_decision.h"
} // end extern "C"

#include <string>
#include <iostream>
#include <new>

#include "tools.hpp"
#include "dar_suite.hpp"
#include "libdar.hpp"
#include "line_tools.hpp"

    // to be removed when dar_slave will be part of libdar
#include "tuyau.hpp"
#include "sar.hpp"
#include "slave_zapette.hpp"

#define ONLY_ONCE "Only one -%c is allowed, ignoring this extra option"
#define OPT_STRING "i:o:hVE:Qj9:"

using namespace libdar;
using namespace std;

#define DAR_SLAVE_VERSION "1.6.1"

static bool command_line(shell_interaction & dialog,
			 S_I argc,
			 char * const argv[],
			 string & chemin,
			 string & filename,
                         string & input_pipe,
			 string & output_pipe,
			 string & execute,
			 infinint & min_digits);
static void show_usage(shell_interaction & dialog, const char *command);
static void show_version(shell_interaction & dialog, const char *command);
static S_I little_main(shared_ptr<user_interaction> & dialog, S_I argc, char * const argv[], const char **env);

int main(S_I argc, char * const argv[], const char **env)
{
    return dar_suite_global(argc,
			    argv,
			    env,
			    OPT_STRING,
#if HAVE_GETOPT_LONG
			    nullptr,
#endif
			    '\0', // should never be met as option, thus early read the whole command-line for -j and -Q options
			    &little_main);
}

static S_I little_main(shared_ptr<user_interaction> & dialog, S_I argc, char * const argv[], const char **env)
{
    string chemin;
    string filename;
    string input_pipe;
    string output_pipe;
    string execute;
    infinint min_digits;
    shell_interaction *ptr = dynamic_cast<shell_interaction *>(dialog.get());

    if(!dialog)
	throw SRC_BUG;
    if(ptr == nullptr)
	throw SRC_BUG;

    if(command_line(*ptr, argc, argv, chemin, filename, input_pipe, output_pipe, execute, min_digits))
    {
	line_tools_check_basename(*dialog, chemin, filename, EXTENSION, false);
	if(min_digits.is_zero())
	    line_tools_check_min_digits(*dialog, chemin, filename, EXTENSION, min_digits);
	libdar::libdar_slave slave(dialog,
				   chemin,
				   filename,
				   EXTENSION,
				   input_pipe.size() == 0,
				   input_pipe.size() == 0 ? "0" : input_pipe,
				   output_pipe.size() == 0,
				   output_pipe.size() == 0 ? "1" : output_pipe,
				   execute,
				   min_digits);
	slave.run();
        return EXIT_OK;
    }
    else
        return EXIT_SYNTAX;
}

static bool command_line(shell_interaction & dialog,
			 S_I argc,
			 char* const argv[],
			 string & chemin,
			 string & filename,
                         string & input_pipe,
			 string & output_pipe,
			 string & execute,
    			 infinint & min_digits)
{
    S_I lu;
    execute = "";
    path *tmp = nullptr;

    if(argc < 1)
    {
        dialog.message(gettext("Cannot read arguments on command line, aborting"));
        return false;
    }

    while((lu = getopt(argc, argv, OPT_STRING)) != EOF)
    {
        switch(lu)
        {
        case 'i':
            if(optarg == nullptr)
                throw Erange("command_line", gettext("Missing argument to -i option"));
            if(input_pipe == "")
                input_pipe = optarg;
            else
                dialog.message(tools_printf(gettext(ONLY_ONCE), char(lu)));
            break;
        case 'o':
            if(optarg == nullptr)
                throw Erange("command_line", gettext("Missing argument to -o option"));
            if(output_pipe == "")
                output_pipe = optarg;
            else
                dialog.message(tools_printf(gettext(ONLY_ONCE), char(lu)));
            break;
        case 'h':
            show_usage(dialog, argv[0]);
            return false;
        case 'V':
            show_version(dialog, argv[0]);
            return false;
        case 'E':
            if(optarg == nullptr)
                throw Erange("command_line", gettext("Missing argument to -E option"));
            if(execute == "")
                execute = optarg;
            else
		execute += string(" ; ") + optarg;
            break;
	case 'Q':
	    break;  // ignore this option already parsed during initialization (dar_suite.cpp)
	case '9':
	    if(optarg == nullptr)
		throw Erange("command_line", tools_printf(gettext("Missing argument to --min-digits"), char(lu)));
	    else
	    {
		infinint tmp2, tmp3;
		line_tools_get_min_digits(optarg, min_digits, tmp2, tmp3);
	    }
	    break;
        case ':':
            throw Erange("command_line", tools_printf(gettext("Missing parameter to option -%c"), char(optopt)));
        case '?':
            throw Erange("command_line", tools_printf(gettext("Ignoring unknown option -%c"), char(optopt)));
        default:
            throw Erange("command_line", tools_printf(gettext("Ignoring unknown option -%c"), char(lu)));
        }
    }

    if(optind + 1 > argc)
    {
        dialog.message(gettext("Missing archive basename, see -h option for help"));
        return false;
    }

    if(optind + 1 < argc)
    {
        dialog.message(gettext("Too many argument on command line, see -h option for help"));
        return false;
    }

    try
    {
	line_tools_split_path_basename(argv[optind], tmp, filename);
	if(tmp == nullptr)
	    throw SRC_BUG;
	chemin = tmp->display();
    }
    catch(...)
    {
	if(tmp != nullptr)
	{
	    delete tmp;
	    tmp = nullptr;
	}
	throw;
    }
    if(tmp != nullptr)
    {
	delete tmp;
	tmp = nullptr;
    }

    return true;
}

static void show_usage(shell_interaction & dialog, const char *command)
{
    string cmd;
    line_tools_extract_basename(command, cmd);
    dialog.change_non_interactive_output(cout);

    dialog.printf("\nusage : \n");
    dialog.printf("  command1 | %s [options] [<path>/]basename | command2\n", cmd.c_str());
    dialog.printf("  %s [options] [-i input_pipe] [-o output_pipe] [<path>/]basename\n", cmd.c_str());
    dialog.printf("  %s -h\n", cmd.c_str());
    dialog.printf("  %s -V\n\n", cmd.c_str());
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("Common options:\n"));
    dialog.printf(gettext("   -i <named pipe> pipe to use instead of std input to read orders from dar\n"));
    dialog.printf(gettext("   -o <named pipe> pipe to use instead of std output to write data to dar\n"));
    dialog.printf(gettext("   -E <string>\t   command line to execute between slices of the archive\n"));
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("See man page for more options.\n"));
}

static void show_version(shell_interaction & dialog, const char *command)
{
    string cmd;
    line_tools_extract_basename(command, cmd);
    U_I maj, med, min;

    get_version(maj, med, min);
    dialog.change_non_interactive_output(cout);
    dialog.printf("\n %s version %s Copyright (C) 2002-2025 Denis Corbin\n\n", cmd.c_str(), DAR_SLAVE_VERSION);
    if(maj > 2)
	dialog.printf(gettext(" Using libdar %u.%u.%u built with compilation time options:\n"), maj, med, min);
    else
	dialog.printf(gettext(" Using libdar %u.%u built with compilation time options:\n"), maj, min);
    line_tools_display_features(dialog);
    dialog.printf("\n");
    dialog.printf(gettext(" compiled the %s with %s version %s\n"), __DATE__, CC_NAT,  __VERSION__);
    dialog.printf(gettext(" %s is part of the Disk ARchive suite (Release %s)\n"), cmd.c_str(), PACKAGE_VERSION);
    dialog.message(tools_printf(gettext(" %s comes with ABSOLUTELY NO WARRANTY;"), cmd.c_str())
		   + tools_printf(gettext(" for details\n type `dar -W'."))
		   + tools_printf(gettext(" This is free software, and you are welcome\n to redistribute it under certain conditions;"))
		   + tools_printf(gettext(" type `dar -L | more'\n for details.\n\n")));
}
