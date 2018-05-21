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
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
}

#include <iostream>

#include "libdar.hpp"
#include "erreurs.hpp"
#include "shell_interaction.hpp"
#include "deci.hpp"
#include "mask_list.hpp"
#include "tools.hpp"

using namespace libdar;
using namespace std;

void f1(const shared_ptr<user_interaction> & dialog, const char *filename);

int main(int argc, char *argv[])
{
    shared_ptr<user_interaction> dialog(new (nothrow) shell_interaction(cout, cerr, false));
    U_I maj, med, min;

    get_version(maj, med, min);
    if(!dialog)
	cout << "ERREUR !" << endl;

    try
    {
	if(argc != 2)
	    throw Erange("mask_list", tools_printf("usage: %s <filename>\n", argv[0]));
	f1(dialog, argv[1]);
    }
    catch(Egeneric & e)
    {
	cout << "exception caught : " + e.get_message() << endl;
    }
    catch(...)
    {
	cout << "unknown exception caught" << endl;
    }
    dialog.reset();
}

void f1(const shared_ptr<user_interaction> & dialog, const char *filename)
{
    mask_list m = mask_list(filename, true, "/toto/tutu", true);
    string tester;
    U_I count = 10;

    cout << "taille : " << m.size() << endl;

    while(--count)
    {
	cin >> tester;
	cout << (m.is_covered(tester) ? string("COVERED") : string("not covered")) << endl;
    }

    m = mask_list(filename, false, "/toto/tutu", false);
    count = 10;
    while(--count)
    {
	cin >> tester;
	cout << (m.is_covered(tester) ? string("COVERED") : string("not covered")) << endl;
    }
}
