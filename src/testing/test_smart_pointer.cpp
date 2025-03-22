/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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

extern "C"
{
} // end extern "C"

#include <iostream>

#include "smart_pointer.hpp"
#include "pile_descriptor.hpp"

using namespace libdar;
using namespace std;

void f1();
void f2();
void f3();

int main()
{
    f1();
    f2();
    f3();
}


void f1()
{
    int *u = new (nothrow) int();
    *u = 18;

    smart_pointer<int> sptr1(u);
    smart_pointer<int> sptr2 = sptr1;

    cout << "sptr1 = " << *sptr1 << endl;
    cout << "sptr2 = " << *sptr2 << endl;

    *u = 19;
    cout << "sptr1 = " << *sptr1 << endl;
    cout << "sptr2 = " << *sptr2 << endl;
}

void f2()
{
    smart_pointer<pile_descriptor> spdesc(new (nothrow) pile_descriptor());
    if(spdesc.is_null())
	throw Ememory("f2");

    cout << "sizeof(smart_pointer<pile_descriptor>) = " << sizeof(spdesc) << endl;
    cout << "sizeof(pile_descriptor) = " << sizeof(pile_descriptor) << endl;
    cout << "sizeof(pile_descriptor *) = " << sizeof(pile_descriptor *) << endl;
}

void f3()
{
    struct example
    {
	int x;
	int y;
    };

    example *ptr = new example();
    ptr->x = 5;
    ptr->y = 6;

    smart_pointer<example> eptr(ptr);

    cout << "x = " << eptr->x << endl;
    cout << "y = " << eptr->y << endl;

    eptr->x = 7;
    cout << "x = " << eptr->x << endl;
    cout << "y = " << eptr->y << endl;

}
