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
// $Id: erreurs.cpp,v 1.14 2004/09/13 13:07:51 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C" {
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
} // end extern "C"

#include <iostream>
#include "erreurs.hpp"
#include "infinint.hpp"
#include "deci.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{
    static bool initialized = false;
	// does not avoid thread safe library.
	// if two thread act at the same time on this
	// value, this would have the same result
	// as if only one had acted on it.
	// subsequent action are read-only (once initialized is true).

    static void init();
    static void inattendue();
    static void notcatched();

    Egeneric::Egeneric(const string &source, const string &message)
    {
        if(!initialized)
            init();
        pile.push_front(niveau(source, message));
    }

    void Egeneric::dump() const
    {
        list<niveau> & tmp = const_cast< list<niveau> & >(pile);
        list<niveau>::iterator it;

        it = tmp.begin();

        cerr << "---- exception type = ["  << exceptionID() << "] ----------" << endl;
        cerr << "[source]" << endl;
        while(it != tmp.end())
        {
            cerr << '\t' << it->lieu << " : " << it->objet << endl;
            it++;
        }
        cerr << "[most outside call]" << endl;
        cerr << "-----------------------------------" << endl << endl;
    }

    Ebug::Ebug(const string & file, S_I line) : Egeneric(tools_printf(gettext("File %S line %d"), &file, line), gettext("it seems to be a bug here")) {}

    void Ebug::stack(const string & passage, const string & file, const string & line)
    {
        Egeneric::stack(passage, tools_printf(gettext("in file %S line %S"), &file, &line));
    }

    static void init()
    {
        set_unexpected(inattendue);
        set_terminate(notcatched);
        initialized = true;
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: erreurs.cpp,v 1.14 2004/09/13 13:07:51 edrusb Rel $";
        dummy_call(id);
    }

    static void inattendue()
    {
        cerr << "###############################################" << endl;
        cerr << gettext("#   UNEXPECTED EXCEPTION,                     #") << endl;
        cerr << gettext("#                         E X I T I N G !     #") << endl;
        cerr << "#                                             #" << endl;
        cerr << "###############################################" << endl;
        cerr << tools_printf(gettext(" THANKS TO REPORT THE PREVIOUS OUTPUT TO MAINTAINER\n GIVING A DESCRIPTION OF THE CIRCUMPSTANCES.")) << endl;
	cerr << tools_printf(gettext(" IF POSSIBLE TRY TO PRODUCE THIS ERROR, A\n SCENARIO THAT CAN REPRODUCE IT WOULD HELP MUCH\n IN SOLVING THIS PROBLEM.                THANKS")) << endl;
        exit(3); // this was exit code for bugs at the time this code was part of dar
	    // now it is part of libdar, while exit code stay defined in typical command line code (dar_suite software)
    }

    static void notcatched()
    {
        cerr << "###############################################" << endl;
        cerr << gettext("#   NOT CAUGHT EXCEPTION,                     #") << endl;
        cerr << gettext("#                         E X I T I N G !     #") << endl;
        cerr << "#                                             #" << endl;
        cerr << "###############################################" << endl;
        cerr << tools_printf(gettext(" THANKS TO REPORT THE PREVIOUS OUTPUT TO MAINTAINER\n GIVING A DESCRIPTION OF THE CIRCUMPSTANCES.")) << endl;
	cerr << tools_printf(gettext(" IF POSSIBLE TRY TO PRODUCE THIS ERROR, A\n SCENARIO THAT CAN REPRODUCE IT WOULD HELP MUCH\n IN SOLVING THIS PROBLEM.                THANKS")) <<endl;
	exit(3); // this was exit code for bugs at the time this code was part of dar
	    // now it is part of libdar, while exit code stay defined in typical command line code (dar_suite software)
    }

} // end of namespace
