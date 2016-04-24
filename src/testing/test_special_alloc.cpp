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

#include "libdar.hpp"
#include "user_interaction.hpp"
#include "on_pool.hpp"

#include <iostream>
#include <new>

using namespace libdar;
using namespace std;

class example : public on_pool
{
public:
    example() { val = 0; };
    example(int x): val(x) {};

    int get_val() const { return val; };
    void set_val(int x) { val = x; };

    memory_pool *my_get_pool() { return get_pool(); };

private:
    int val;
};

void f1();

int main()
{
    U_I maj, med, min;
    get_version(maj, med, min);


    try
    {
	f1();
    }
    catch(Egeneric & e)
    {
	cerr << e.dump_str();
    }
    catch(...)
    {
	cout << "unknown exception caught" << endl;
    }

    return 0;
}

void f1()
{
    memory_pool mem;
    example a = 1;
    example *ptr1 = new (nothrow) example;
    example *ptr2 = new (&mem) example;

    try
    {
	if(ptr1 == nullptr || ptr2 == nullptr)
	    throw Ememory("f1");

	cout << a.get_val() << endl;
	cout << ptr1->get_val() << endl;
	cout << ptr2->get_val() << endl;

	if(a.my_get_pool() != nullptr)
	    cout << "Strange!" << endl;
	if(ptr1->my_get_pool() != nullptr)
	    cout << "Very strange!" << endl;
	if(ptr2->my_get_pool() != &mem)
	    cout << "Extremely strange!" << endl;
    }

    catch(...)
    {
	if(ptr1 != nullptr)
	    delete ptr1;
	if(ptr2 != nullptr)
	    delete ptr2;
	throw;
    }

    if(ptr1 != nullptr)
	delete ptr1;
    if(ptr2 != nullptr)
	delete ptr2;

    mem.garbage_collect();

    if(! mem.is_empty())
	cout << mem.dump() << endl;
    else
    {
	cout << mem.dump() << endl;
	cout << "ALL memory has been released" << endl;
    }
}
