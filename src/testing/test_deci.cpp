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

#include "../my_config.h"
#include <iostream>

#include "test_memory.hpp"
#include "deci.hpp"

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
    deci d1 = string("00001");
    infinint t = 3;
    deci d2 = t;
    deci d3 = infinint(125);
    U_I c;

    cout << d1.human() << endl;
    cout << d2.human() << endl;
    cout << d3.human() << endl;

    c = d1.computer() % 200;
    c = d2.computer() % 200;
    c = d3.computer() % 200;
}
