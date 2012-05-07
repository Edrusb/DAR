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
// $Id: test_erreurs.cpp,v 1.10 2002/12/08 20:03:07 edrusb Rel $
//
/*********************************************************************/

#include <iostream>
#include "erreurs.hpp"
#include "integers.hpp"

void f1(S_I i);
void f2(S_I i, S_I j) throw(Erange);

void status()
{
    cout << "nombre total d'exceptions : " << Egeneric::total() << "   dont  " << Egeneric::alive() << " vivante(s) " << " et " << Egeneric::zombies() << " zombie(s) " << endl;
}

void f1(S_I i) 
{
    try
    {
	if(i < 0)
	    throw Erange("f1", "i < 0");
	if(i == 0)
	    throw Edeci("f1", "i == 0");
    }
    catch(Egeneric & e)
    {
	e.stack("f1", "essai");
	throw;
    }
}

void f2(S_I i, S_I j) throw(Erange)
{
    try
    {
	if(j > 0)
	    f2(i, j-1);
	else
	    f1(i);
    }
    catch(Erange & e)
    {
	e.stack("f2", "calling f1");
	throw;
    }
    catch(Egeneric & e)
    {
	e.stack("f2", "unexpected");
	throw;
    }
}

void f3()
{
    try
    {
	Ememory *x;
	Ebug y = SRC_BUG;
	string s;
	
	status();
	x = new Ememory("f3");
	status();
	Egeneric::display_last_destroyed();
	delete x;
	status();
    }
    catch(Egeneric & e)
    {
	e.stack("f3", "");
	throw;
    }
}

void f4()
{
    Erange *y;
    Erange x = Erange("essai", "coucou");

    status();
    Egeneric::display_last_destroyed();
    x.stack("ajout", "par ici");
    x.stack("crotte", "par ila");
    
    y = new Erange(x);
    y->dump();
    y->dump();
    status();
    Egeneric::display_last_destroyed();
    delete y;
    status();
    Egeneric::display_last_destroyed();
}

S_I main()
{
    status();
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
	e.dump();
	status();
    }
    status();
    
    try
    {
	f2(0, 10);
    }
    catch(Egeneric & e)
    {
	e.dump();
    }

    status();
    Egeneric::display_alive();
    Egeneric::display_last_destroyed();
    status();
    Egeneric::clear_last_destroyed();
    status();
}
