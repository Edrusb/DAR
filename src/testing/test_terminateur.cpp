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
// $Id: test_terminateur.cpp,v 1.7.4.2 2004/04/20 09:27:02 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
} // end extern "C"

#include <iostream>

#include "terminateur.hpp"
#include "generic_file.hpp"
#include "deci.hpp"
#include "test_memory.hpp"
#include "integers.hpp"
#include "cygwin_adapt.hpp"

using namespace libdar;
using namespace std;

static void f1();

int main()
{
    MEM_BEGIN;
    MEM_IN;
    f1();
    MEM_OUT;
    MEM_END;
}

static void f1()
{
    fichier toto = ::open("toto", O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0644);
    terminateur term;

    infinint grand = 1;

    for(S_I i=2;i<30;i++)
        grand *= i;

    deci conv = grand;
    cout << conv.human() << endl;
    term.set_catalogue_start(grand);
    term.dump(toto);
    toto.skip(0);
    term.read_catalogue(toto);
    conv = term.get_catalogue_start();
    cout << conv.human() << endl;
}
