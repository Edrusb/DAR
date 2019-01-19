/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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
#include <iostream>

#include "deci.hpp"

using namespace libdar;
using namespace std;

static void f1();

int main()
{
    f1();
}

static void f1()
{
    libdar::deci d1 = string("00001");
    infinint t = 3;
    libdar::deci d2 = t;
    libdar::deci d3 = infinint(125);
    U_I c;

    cout << d1.human() << endl;
    cout << d2.human() << endl;
    cout << d3.human() << endl;

    c = d1.computer() % 200;
    c = d2.computer() % 200;
    c = d3.computer() % 200;

    c = c+1; // avoid warning of unused variable
}
