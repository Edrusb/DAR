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
// $Id: test_deci.cpp,v 1.8 2002/05/28 20:17:29 denis Rel $
//
/*********************************************************************/

#include "deci.hpp"
#include <iostream.h>
#include "test_memory.hpp"

static void f1();

int main()
{
    MEM_IN;
    f1();
    MEM_OUT;
    MEM_END;
}

static void f1()
{
    deci d1 = string("00001");
    infinint t = 3;
    deci d2 = t;
    deci d3 = 125;
    unsigned int c;
    
    cout << d1.human() << endl;
    cout << d2.human() << endl;
    cout << d3.human() << endl;

    c = d1.computer() % 200;
    c = d2.computer() % 200;
    c = d3.computer() % 200;
}


