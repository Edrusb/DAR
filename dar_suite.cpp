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
// $Id: dar_suite.cpp,v 1.12 2002/06/26 22:20:20 denis Rel $
//
/*********************************************************************/
//
#include <iostream>
#include "dar_suite.hpp"
#include "user_interaction.hpp"
#include "erreurs.hpp"
#include "test_memory.hpp"

const char *application_version = "1.1.0";

const char *dar_suite_version()
{
    return application_version;
}

int dar_suite_global(int argc, char *argv[], int (*call)(int, char *[]))
{
    MEM_BEGIN;
    MEM_IN;
    int ret = EXIT_OK;

    try
    {
	user_interaction_init(0, &cerr, &cerr);
	ret = (*call)(argc, argv);
    }
    catch(Efeature &e)
    {
	user_interaction_warning(string("NOT YET IMPLEMENTED FEATURE has been used : ") + e.get_message());
	user_interaction_warning("please check documentation or upgrade your software if available");
	ret = EXIT_SYNTAX;
    }
    catch(Ehardware & e)
    {
	user_interaction_warning(string("SEEMS TO BE A HARDWARE PROBLEM : ")+e.get_message());
	user_interaction_warning("please check your hardware");
	ret = EXIT_ERROR;
    }
    catch(Ememory & e)
    {
	user_interaction_warning("lack of memory to achieve the operation, aborting operation");
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
	user_interaction_warning(string("aborting program. User refused to continue while asking : ") + e.get_message());
	ret = EXIT_USER_ABORT;
    }
    catch(Edata & e)
    {
	    // no output just the exit code is set
	ret = EXIT_DATA_ERROR;
    }
    catch(Egeneric & e)
    {
	e.dump();
	user_interaction_warning("INTERNAL ERROR, PLEASE REPORT THE PREVIOUS OUTPUT TO MAINTAINER");
	ret = EXIT_BUG;
    }
    user_interaction_close();
    MEM_OUT;
    MEM_END;
    return ret;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: dar_suite.cpp,v 1.12 2002/06/26 22:20:20 denis Rel $";
    dummy_call(id);
}
