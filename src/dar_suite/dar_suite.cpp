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
// $Id: dar_suite.cpp,v 1.8.4.2 2004/01/14 13:21:57 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"
#include <iostream>
#include "integers.hpp"
#include "dar_suite.hpp"
#include "user_interaction.hpp"
#include "shell_interaction.hpp"
#include "erreurs.hpp"
#include "test_memory.hpp"
#include "libdar.hpp"

using namespace libdar;

int dar_suite_global(int argc, char *argv[], const char **env, int (*call)(int, char *[], const char **env))
{
    MEM_BEGIN;
    MEM_IN;
    int ret = EXIT_OK;

    try
    {
        U_I min, maj;

        shell_interaction_init(&cerr, &cerr);
        get_version(maj, min);
        if(maj != LIBDAR_COMPILE_TIME_MAJOR)
        {
            user_interaction_warning(string("We have linked with an incompatible version of libdar. Expecting ")+deci(LIBDAR_COMPILE_TIME_MAJOR).human()+".x but having "+deci(maj).human()+"."+deci(min).human()+" version linked with");
            ret = EXIT_ERROR;
        }
        else
            ret = (*call)(argc, argv, env);
    }
    catch(Efeature &e)
    {
        user_interaction_warning(string("NOT YET IMPLEMENTED FEATURE has been used: ") + e.get_message());
        user_interaction_warning("please check documentation or upgrade your software if available");
        ret = EXIT_SYNTAX;
    }
    catch(Ehardware & e)
    {
        user_interaction_warning(string("SEEMS TO BE A HARDWARE PROBLEM: ")+e.get_message());
        user_interaction_warning("please check your hardware");
        ret = EXIT_ERROR;
    }
    catch(Ememory & e)
    {
        user_interaction_warning("Lack of memory to achieve the operation, aborting operation");
        ret = EXIT_ERROR;
    }
    catch(Erange & e)
    {
        user_interaction_warning("FATAL error, aborting operation");
        user_interaction_warning(e.get_message());
        ret = EXIT_ERROR;
    }
    catch(Euser_abort & e)
    {
        user_interaction_warning(string("Aborting program. User refused to continue while asking: ") + e.get_message());
        ret = EXIT_USER_ABORT;
    }
    catch(Edata & e)
    {
            // no output just the exit code is set
        ret = EXIT_DATA_ERROR;
    }
    catch(Escript & e)
    {
        user_interaction_warning(string("Aborting program. An error occured concerning user command execution: ") + e.get_message());
        ret = EXIT_SCRIPT_ERROR;
    }
    catch(Elibcall & e)
    {
        user_interaction_warning(string("Aborting program. An error occured while calling libdar: ") + e.get_message());
        ret = EXIT_LIBDAR;
    }
    catch(Elimitint & e)
    {
        user_interaction_warning(string("Aborting program. ") + e.get_message());
        ret = EXIT_LIMITINT;
    }
    catch(Egeneric & e)
    {
        e.dump();
        user_interaction_warning("INTERNAL ERROR, PLEASE REPORT THE PREVIOUS OUTPUT TO MAINTAINER");
        ret = EXIT_BUG;
    }
    catch(...)
    {
	Ebug x = SRC_BUG;
	x.dump();
	user_interaction_warning("CAUGHT A NON (LIB)DAR EXCEPTION");
        user_interaction_warning("INTERNAL ERROR, PLEASE REPORT THE PREVIOUS OUTPUT TO MAINTAINER");
    }

    try
    {
        shell_interaction_close();
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
    static char id[]="$Id: dar_suite.cpp,v 1.8.4.2 2004/01/14 13:21:57 edrusb Rel $";
    dummy_call(id);
}
