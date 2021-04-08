/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"
#include <iostream>

#include "libdar.hpp"

using namespace libdar;
using namespace std;

static void f1();

int main()
{
    U_I major, medium, minor;
    get_version(major, medium, minor);
    f1();
}

static void f1()
{
    range r1(1, 5);
    range r2(7, 10);

    cout << r1.display() << " " << r2.display() << endl;

    range r3 = r1 + r2;
    range r4 = r2 + r1;

    cout << r3.display() << " " << r4.display() << endl;

    range r5(6,6);

    r3 += r5;

    cout << r5.display() << endl;
}

