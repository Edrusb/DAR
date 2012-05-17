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
// $Id: dar_suite.cpp,v 1.23.2.2 2005/03/05 16:43:13 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"
#include <iostream>
#include <locale.h>
#include <new>
#include "integers.hpp"
#include "dar_suite.hpp"
#include "shell_interaction.hpp"
#include "erreurs.hpp"
#include "test_memory.hpp"
#include "libdar.hpp"

#define GENERAL_REPORT(msg) 	if(ui != NULL)\
	                            ui->warning(msg);\
	                        else\
                                    cerr << msg << endl;



using namespace libdar;

static void jogger();

static user_interaction *ui = NULL;


int dar_suite_global(int argc, char *argv[], const char **env, int (*call)(user_interaction & dialog, int, char *[], const char **env))
{
    MEM_BEGIN;
    MEM_IN;
    int ret = EXIT_OK;

#ifdef ENABLE_NLS
	// gettext settings
    try
    {
	if(DAR_LOCALEDIR != "")
	    if(bindtextdomain(PACKAGE, DAR_LOCALEDIR) == NULL)
		throw Erange("", "Cannot open the translated messages directory, native language support will not work");
	if(setlocale(LC_MESSAGES, "") == NULL || setlocale(LC_CTYPE, "") == NULL)
	    throw Erange("", "Cannot set locale category, native language support will not work");
	if(textdomain(PACKAGE) == NULL)
	    throw Erange("", "Cannot find dar's catalogue, native language support will not work");
    }
    catch(Erange & e)
    {
	cerr << e.get_message() << endl;
    }
#endif

    try
    {
	U_I min, med, maj;
	bool silent = tools_look_for("-Q", argc, argv);
	bool jog = tools_look_for("-j", argc, argv) || tools_look_for("--jog", argc, argv);
	ui = shell_interaction_init(&cerr, &cerr, silent);

	if(jog)
	    std::set_new_handler(&jogger);

	get_version(maj, min); // mandatory first call to libdar
	if(maj > 2)
	    get_version(maj, med, min);
	else
	    med = 0;
	if(maj != LIBDAR_COMPILE_TIME_MAJOR)
	{
	    GENERAL_REPORT(tools_printf(gettext("We have linked with an incompatible version of libdar. Expecting version %d.%d.x but having linked with version %d.%d.%d"), LIBDAR_COMPILE_TIME_MAJOR, LIBDAR_COMPILE_TIME_MEDIUM, maj, med, min));
	    ret = EXIT_ERROR;
	}
	else
	    ret = (*call)(*ui, argc, argv, env);
    }
    catch(Efeature &e)
    {
	GENERAL_REPORT(string(gettext("NOT YET IMPLEMENTED FEATURE has been used: ")) + e.get_message());
	GENERAL_REPORT(string(gettext("Please check documentation or upgrade your software if available")));
	ret = EXIT_SYNTAX;
    }
    catch(Ehardware & e)
    {
	GENERAL_REPORT(string(gettext("SEEMS TO BE A HARDWARE PROBLEM: "))+e.get_message());
	GENERAL_REPORT(string(gettext("Please check your hardware")));
	ret = EXIT_ERROR;
    }
    catch(Ememory & e)
    {
	GENERAL_REPORT(string(gettext("Lack of memory to achieve the operation, aborting operation")));
	ret = EXIT_ERROR;
    }
    catch(std::bad_alloc & e)
    {
        GENERAL_REPORT(string(gettext("Lack of memory to achieve the operation, aborting operation")));
        ret = EXIT_ERROR;
    }
    catch(Erange & e)
    {
	GENERAL_REPORT(string(gettext("FATAL error, aborting operation")));
	GENERAL_REPORT(e.get_message());
	ret = EXIT_ERROR;
    }
    catch(Euser_abort & e)
    {
	GENERAL_REPORT(string(gettext("Aborting program. User refused to continue while asking: ")) + e.get_message());
	ret = EXIT_USER_ABORT;
    }
    catch(Edata & e)
    {
	    // no output just the exit code is set
	ret = EXIT_DATA_ERROR;
    }
    catch(Escript & e)
    {
	GENERAL_REPORT(string(gettext("Aborting program. An error occured concerning user command execution: ")) + e.get_message());
	ret = EXIT_SCRIPT_ERROR;
    }
    catch(Elibcall & e)
    {
	GENERAL_REPORT(string(gettext("Aborting program. An error occured while calling libdar: ")) + e.get_message());
	ret = EXIT_LIBDAR;
    }
    catch(Einfinint & e)
    {
	GENERAL_REPORT(string(gettext("Aborting program. ")) + e.get_message());
	ret = EXIT_BUG;
    }
    catch(Elimitint & e)
    {
	GENERAL_REPORT(string(gettext("Aborting program. ")) + e.get_message());
	ret = EXIT_LIMITINT;
    }
    catch(Ecompilation & e)
    {
	GENERAL_REPORT(string(gettext("Aborting program. The requested operation needs a feature that has been disabled at compilation time: ")) + e.get_message());
	ret = EXIT_COMPILATION;
    }
    catch(Egeneric & e)
    {
	e.dump();
	GENERAL_REPORT(string(gettext("INTERNAL ERROR, PLEASE REPORT THE PREVIOUS OUTPUT TO MAINTAINER")));
	ret = EXIT_BUG;
    }
    catch(...)
    {
	Ebug x = SRC_BUG;
	x.dump();
	GENERAL_REPORT(string(gettext("CAUGHT A NON (LIB)DAR EXCEPTION")));
	GENERAL_REPORT(string(gettext("INTERNAL ERROR, PLEASE REPORT THE PREVIOUS OUTPUT TO MAINTAINER")));
	ret = EXIT_BUG;
    }

	// restoring terminal settings
    try
    {
	shell_interaction_close();
	if(ui != NULL)
	    delete ui;
    }
    catch(...)
    {
	ret = EXIT_UNKNOWN_ERROR;
    }

    MEM_OUT;
    MEM_END;
    return ret;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: dar_suite.cpp,v 1.23.2.2 2005/03/05 16:43:13 edrusb Rel $";
    dummy_call(id);
}


static void jogger()
{
    if(ui == NULL)
	throw Ememory("jogger");
    ui->pause(gettext("No more (virtual) memory available, you have the opportunity to stop unncessary applications to free up some memory. Can we continue now ?"));
}

string dar_suite_command_line_features()
{
#if HAVE_GETOPT_LONG
    const char *long_opt = gettext("YES");
#else
    const char *long_opt = gettext("NO");
#endif

    return tools_printf(gettext("Long options support       : %s\n"), long_opt);
}
