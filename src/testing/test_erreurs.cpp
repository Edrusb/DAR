/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2026 Denis Corbin
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

#include "erreurs.hpp"
#include "integers.hpp"

using namespace libdar;
using namespace std;

void f1(S_I i);
void f2(S_I i, S_I j);

void f1(S_I i)
{
    if(i < 0)
	throw Erange("f1", "i < 0");
    if(i == 0)
	throw Edeci("f1", "i == 0");
}

void f2(S_I i, S_I j)
{
    if(j > 0)
	f2(i, j-1);
    else
	f1(i);
}

void f3()
{
    Ememory *x;
    Ebug y = SRC_BUG;
    string s;

    x = new Ememory();
    delete x;
}

void f4()
{
    Erange *y;
    Erange x = Erange("essai", "coucou");
    Edeci dec = Edeci("f4", "essai");

    x.prepend_message("par ici: ");
    x.prepend_message("par ila: ");
    cerr << dec.get_message() << endl;

    y = new Erange(x);
    cerr << y->get_message() << endl;
    cerr << y->get_message() << endl;
    delete y;
}

void f5(U_I loop)
{
    if(--loop > 0)
	f5(loop);
    else
	throw SRC_BUG;
}

int main()
{
    f4();
    f4();
    f4();
    try
    {
        f3();
        f2(3, 3);
        f2(-3, 3);
    }
    catch(Egeneric & e)
    {
        cerr << e.get_message() << endl;
    }

    try
    {
        f2(0, 10);
    }
    catch(Egeneric & e)
    {
        cerr << e.get_message() << endl;
    }

    try
    {
	f5(200);
    }
    catch(Egeneric & e)
    {
	cerr << e.get_message() << endl;
    }
}
