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
// $Id: dar_slave.cpp,v 1.13.2.3 2004/03/10 21:18:20 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"

extern "C"
{
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

using namespace libdar;
using namespace std;

#define DAR_SLAVE_VERSION "1.2.1"

static bool command_line(S_I argc, char *argv[], path * &chemin, string & filename,
                         string &input_pipe, string &output_pipe, string & execute);
static void show_usage(const char *command);
static void show_version(const char *command);
static S_I little_main(S_I argc, char *argv[], const char **env);

int main(S_I argc, char *argv[], const char **env)
{
    return dar_suite_global(argc, argv, env, &little_main);
}

static S_I little_main(S_I argc, char *argv[], const char **env)
{
    path *chemin = NULL;
    string filename;
    string input_pipe;
    string output_pipe;
    string execute;

    if(command_line(argc, argv, chemin, filename, input_pipe, output_pipe, execute))
    {
        tuyau *input = NULL;
        tuyau *output = NULL;
        sar *source = NULL;
        try
        {
            source = new sar(filename, EXTENSION, SAR_OPT_DONT_ERASE, *chemin, execute);
            if(source == NULL)
                throw Ememory("little_main");

            tools_open_pipes(input_pipe, output_pipe, input, output);

            slave_zapette zap = slave_zapette(input, output, source);
            input = output = NULL; // now managed by zap;
            source = NULL;  // now managed by zap;

            try
            {
                zap.action();
            }
            catch(Erange &e)
            {
                user_interaction_warning(e.get_message());
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

static bool command_line(S_I argc,char *argv[], path * &chemin, string & filename,
                         string &input_pipe, string &output_pipe, string & execute)
{
    S_I lu;
    execute = "";

    if(argc < 1)
    {
        user_interaction_warning("cannot read arguments on command line, aborting");
        return false;
    }

    while((lu = getopt(argc, argv, "i:o:hVE:")) != EOF)
    {
        switch(lu)
        {
        case 'i':
            if(optarg == NULL)
                throw Erange("get_args", "missing argument to -i");
            if(input_pipe == "")
                input_pipe = optarg;
            else
                user_interaction_warning("only one -i option is allowed, considering the first one");
            break;
        case 'o':
            if(optarg == NULL)
                throw Erange("get_args", "missing argument to -o");
            if(output_pipe == "")
                output_pipe = optarg;
            else
                user_interaction_warning("only one -o option is allowed, considering the first one");
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
            if(execute == "")
                execute = optarg;
            else
		execute += string(" ; ") + optarg;
            break;
        case ':':
            throw Erange("get_args", string("missing parameter to option ") + char(optopt));
        case '?':
            user_interaction_warning(string("ignoring unknown option ") + char(optopt));
            break;
        default:
            user_interaction_warning(string("ignoring unknown option ") + char(lu));
        }
    }

    if(optind + 1 > argc)
    {
        user_interaction_warning("missing archive basename, see -h option for help");
        return false;
    }

    if(optind + 1 < argc)
    {
        user_interaction_warning("too many argument on command line, see -h option for help");
        return false;
    }

    tools_split_path_basename(argv[optind], chemin, filename);
    return true;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: dar_slave.cpp,v 1.13.2.3 2004/03/10 21:18:20 edrusb Rel $";
    dummy_call(id);
}

static void show_usage(const char *command)
{
    char *cmd = tools_extract_basename(command);
    shell_interaction_change_non_interactive_output(&cout);

    try
    {
        ui_printf("\nusage : \n");
        ui_printf("  command1 | %s [options] [<path>/]basename | command2\n", cmd);
        ui_printf("  %s [options] [-i input_pipe] [-o output_pipe] [<path>/]basename\n", cmd);
        ui_printf("  %s -h\n", cmd);
        ui_printf("  %s -V\n\n", cmd);
#include "dar_slave.usage"
    }
    catch(...)
    {
        delete cmd;
        throw;
    }
    delete cmd;
}

static void show_version(const char *command)
{
    char *cmd = tools_extract_basename(command);
    U_I maj, min, bits;
    bool ea, largefile, nodump, fastbadalloc;

    get_version(maj, min);
    get_compile_time_features(ea, largefile, nodump, fastbadalloc, bits);

    try
    {
        ui_printf("\n %s version %s Copyright (C) 2002-2052 Denis Corbin\n\n", cmd, DAR_SLAVE_VERSION);
        ui_printf(" Using libdar %u.%u built with compilation time options:\n", maj, min);
        tools_display_features(ea, largefile, nodump, fastbadalloc, bits);
        ui_printf("\n");
        ui_printf(" %s with %s version %s\n", __DATE__, CC_NAT,  __VERSION__);
        ui_printf(" %s is part of the Disk ARchive suite (Release %s)\n", cmd, PACKAGE_VERSION);
        ui_printf(" %s comes with ABSOLUTELY NO WARRANTY; for details\n", cmd);
        ui_printf(" type `dar -W'.  This is free software, and you are welcome\n");
        ui_printf(" to redistribute it under certain conditions; type `dar -L | more'\n");
        ui_printf(" for details.\n\n");
    }
    catch(...)
    {
        delete cmd;
        throw;
    }
    delete cmd;
}
