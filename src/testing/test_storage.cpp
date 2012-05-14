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
// $Id: test_storage.cpp,v 1.7 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"
#include <iostream>

#include "storage.hpp"
#include "infinint.hpp"
#include "erreurs.hpp"
#include "test_memory.hpp"
#include "integers.hpp"
#include "test_memory.hpp"

using namespace libdar;
using namespace std;

void f1();
void f2();

void affiche(infinint x)
{
    U_32 l = 0;
    x.unstack(l);
    do {
        cout <<  "+" << (U_I)l ;
        l = 0;
        x.unstack(l);
    } while( l > 0);
    cout << endl;
}

void affiche(const storage & ref)
{
    storage::iterator it = ref.begin();
    while(it != ref.end())
        cout << *(it++);
    cout << endl;
}

int main(S_I argc, char *argv[])
{
    MEM_BEGIN;
    MEM_IN;
    f1();
    MEM_OUT;
    f2();
    MEM_OUT;
    MEM_END;
}

void f1()
{
    try
    {
        storage st1(10), st2(12);
        storage *test;
        infinint u;
        
        test = new storage(st1);
        delete test;
        
        u = 10;
        affiche(u);
        
        test = new storage(u);
        u = test->size();
        affiche(u);
        
        if(*test < st1)
            cout << "vrai" << endl;
        else
            cout << "faux" << endl;
        
        if(*test == st2)
            cout << "vrai" << endl;
        else
            cout << "faux" << endl;
        
        
        if(*test == st1)
            cout << "vrai" << endl;
        else
            cout << "faux" << endl;
        
        if(st2 < *test)
            cout << "vrai" << endl;
        else
            cout << "faux" << endl;

        unsigned char b = 'a' + 3;
        cout << b << endl;

        for(S_I i = 0; infinint(i) < st1.size(); i++)
            st1[i] = 'a' + i;
        affiche(st1);

        storage::iterator it = st1.begin();
        
        while(it != st1.end())
            cout << *(it++);
        cout << endl;
        
        it = st1.rbegin();
        while(it != st1.rend())
            cout << *(it--);
        cout << endl;
        
        const storage cst = st1;
        cout << cst[3] << endl;

        st2 = st1;
        st1.clear();
        affiche(st1);
        affiche(st2);

        it = st2.rbegin();
        affiche(it.get_position());
        it = st2.begin();
        st2.write(it, (unsigned char *)"coucou les amis il fait chaud il fait beau, les mouches pettent et les cailloux fleurissent", st2.size() % 100);
        affiche(st2);
        char buffer[100];

        it = ++(st2.begin());
        st2.read(it, (unsigned char *)buffer, st2.size() % 100);
        
        delete test;

        it = st2.begin() + 3;
        affiche(it.get_position());

        st2.insert_null_bytes_at_iterator(it, 5);
        affiche(st2);

        it = st2.rbegin() - 5;
        st2.remove_bytes_at_iterator(it, 10);
        affiche(st2);
        it = st2.rbegin() - 5;
        st2.insert_bytes_at_iterator(it,(unsigned char *)buffer, st2.size() % 100);
        affiche(st2);
    }
    catch(Egeneric &r)
    {
        cout << "exception connue attrappee" << endl;
    }
    catch(...)
    {
        cout << "exception NON connue attrappee" << endl;
    }
}

void f2()
{
    infinint u = 1;
    infinint s, size;
    S_I i;

    for(i = 2; i < 10; i++)
        u *= i;

    affiche(u);

    storage x = u;
    storage::iterator it;

    it = x.begin();
    i = 0;
    while(it != x.end())
        *(it++) = (unsigned char)('A' + (i++ % 70));
    
    affiche(x);
    size = x.size();
    u = size / 2;
    it.skip_to(x, u);
    
    x.remove_bytes_at_iterator(it, 2000);
    
    u = x.size();
    affiche(u);
    affiche(x);
}
