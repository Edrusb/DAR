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

#if HAVE_STRINGS_H
#include <strings.h>
#endif

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

#include "getopt_decision.h"
} // end extern "C"

#include <string>
#include <iostream>
#include <new>

#include "user_interaction.hpp"
#include "zapette.hpp"
#include "sar.hpp"
#include "path.hpp"
#include "tuyau.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "dar_suite.hpp"
#include "integers.hpp"
#include "libdar.hpp"
#include "shell_interaction.hpp"
#include "line_tools.hpp"
#include "entrepot.hpp"

#define ONLY_ONCE "Only one -%c is allowed, ignoring this extra option"

using namespace libdar;
using namespace std;

#define DAR_SLAVE_VERSION "1.4.4"

static bool command_line(user_interaction & dialog,
			 S_I argc, char * const argv[], path * &chemin, string & filename,
                         string &input_pipe, string &output_pipe, string & execute,
			 infinint & min_digits);
static void show_usage(user_interaction & dialog, const char *command);
static void show_version(user_interaction & dialog, const char *command);
static S_I little_main(user_interaction & dialog, S_I argc, char * const argv[], const char **env);

int main(S_I argc, char * const argv[], const char **env)
{
    return dar_suite_global(argc, argv, env, &little_main);
}

static S_I little_main(user_interaction & dialog, S_I argc, char * const argv[], const char **env)
{
    path *chemin = NULL;
    string filename;
    string input_pipe;
    string output_pipe;
    string execute;
    infinint min_digits;

    if(command_line(dialog, argc, argv, chemin, filename, input_pipe, output_pipe, execute, min_digits))
    {
        tuyau *input = NULL;
        tuyau *output = NULL;
        sar *source = NULL;
	entrepot_local entrep = entrepot_local("", "", "", false);

	if(chemin == NULL)
	    throw SRC_BUG;

	entrep.set_location(*chemin);
        try
        {
	    source = new (nothrow) sar(dialog, filename, EXTENSION, entrep, true, min_digits, false, execute);
            if(source == NULL)
                throw Ememory("little_main");

            tools_open_pipes(dialog, input_pipe, output_pipe, input, output);

            slave_zapette zap = slave_zapette(input, output, source);
            input = output = NULL; // now managed by zap;
            source = NULL;  // now managed by zap;

            try
            {
                zap.action();
            }
            catch(Erange &e)
            {
                dialog.warning(e.get_message());
                throw Edata(e.get_message());
            }
        }
        catch(...)
        {
            delete chemin;
            if(input != NULL)
                delete input;
            if(output != NULL)
                delete output;
            if(source != NULL)
                delete source;
            throw;
        }
        delete chemin;
        if(input != NULL)
            delete input;
        if(output != NULL)
            delete output;
        if(source != NULL)
            delete source;

        return EXIT_OK;
    }
    else
        return EXIT_SYNTAX;
}

static bool command_line(user_interaction & dialog,
			 S_I argc,char * const argv[], path * &chemin, string & filename,
                         string &input_pipe, string &output_pipe, string & execute,
    			 infinint & min_digits)
{
    S_I lu;
    execute = "";

    if(argc < 1)
    {
        dialog.warning(gettext("Cannot read arguments on command line, aborting"));
        return false;
    }

    while((lu = getopt(argc, argv, "i:o:hVE:Qj")) != EOF)
    {
        switch(lu)
        {
        case 'i':
            if(optarg == NULL)
                throw Erange("command_line", gettext("Missing argument to -i option"));
            if(input_pipe == "")
                input_pipe = optarg;
            else
                dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
            break;
        case 'o':
            if(optarg == NULL)
                throw Erange("command_line", gettext("Missing argument to -o option"));
            if(output_pipe == "")
                output_pipe = optarg;
            else
                dialog.warning(tools_printf(gettext(ONLY_ONCE), char(lu)));
            break;
        case 'h':
            show_usage(dialog, argv[0]);
            return false;
        case 'V':
            show_version(dialog, argv[0]);
            return false;
        case 'E':
            if(optarg == NULL)
                throw Erange("command_line", gettext("Missing argument to -E option"));
            if(execute == "")
                execute = optarg;
            else
		execute += string(" ; ") + optarg;
            break;
	case 'Q':
	case 'j':
	    break;  // ignore this option already parsed during initialization (dar_suite.cpp)
	case ';':
	    if(optarg == NULL)
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
            dialog.warning(tools_printf(gettext("Ignoring unknown option -%c"), char(optopt)));
            break;
        default:
            dialog.warning(tools_printf(gettext("Ignoring unknown option -%c"), char(lu)));
        }
    }

    if(optind + 1 > argc)
    {
        dialog.warning(gettext("Missing archive basename, see -h option for help"));
        return false;
    }

    if(optind + 1 < argc)
    {
        dialog.warning(gettext("Too many argument on command line, see -h option for help"));
        return false;
    }

    tools_split_path_basename(argv[optind], chemin, filename);
    tools_check_basename(dialog, *chemin, filename, EXTENSION);
    return true;
}

static void show_usage(user_interaction & dialog, const char *command)
{
    string cmd;
    tools_extract_basename(command, cmd);
    shell_interaction_change_non_interactive_output(&cout);

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
    dialog.printf(gettext("   -j\t\t   ask user what to do when memory is exhausted\n"));
    dialog.printf(gettext("\n"));
    dialog.printf(gettext("See man page for more options.\n"));
}

static void show_version(user_interaction & dialog, const char *command)
{
    string cmd;
    tools_extract_basename(command, cmd);
    U_I maj, med, min;

    get_version(maj, med, min);
    shell_interaction_change_non_interactive_output(&cout);
    dialog.printf("\n %s version %s Copyright (C) 2002-2052 Denis Corbin\n\n", cmd.c_str(), DAR_SLAVE_VERSION);
    if(maj > 2)
	dialog.printf(gettext(" Using libdar %u.%u.%u built with compilation time options:\n"), maj, med, min);
    else
	dialog.printf(gettext(" Using libdar %u.%u built with compilation time options:\n"), maj, min);
    tools_display_features(dialog);
    dialog.printf("\n");
    dialog.printf(gettext(" compiled the %s with %s version %s\n"), __DATE__, CC_NAT,  __VERSION__);
    dialog.printf(gettext(" %s is part of the Disk ARchive suite (Release %s)\n"), cmd.c_str(), PACKAGE_VERSION);
    dialog.warning(tools_printf(gettext(" %s comes with ABSOLUTELY NO WARRANTY;"), cmd.c_str())
		   + tools_printf(gettext(" for details\n type `dar -W'."))
		   + tools_printf(gettext(" This is free software, and you are welcome\n to redistribute it under certain conditions;"))
		   + tools_printf(gettext(" type `dar -L | more'\n for details.\n\n")));
}
