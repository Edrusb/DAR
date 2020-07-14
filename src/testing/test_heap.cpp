/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif


}

#include "libdar.hpp"
#include "../libdar/heap.hpp"

using namespace libdar;
using namespace std;

void f1();

static shared_ptr<user_interaction>ui;

int main()
{
    U_I maj, med, min;

    get_version(maj, med, min);
    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(!ui)
	cout << "ERREUR !" << endl;
    try
    {
	f1();
    }
    catch(Ebug & e)
    {
	ui->message(e.dump_str());
    }
    catch(Egeneric & e)
    {
	ui->message(string("Aborting on exception: ") + e.get_message());
    }
    ui.reset();
}


void f1()
{
    heap<U_I, U_I> montas(10, 5);
    unique_ptr<U_I> ptr;

    cout << montas.get_size() << endl;
    ptr = montas.get();
    cout << montas.get_size() << endl;
    cout << *ptr << endl;
    *ptr = 6;
    montas.put(move(ptr));
    cout << bool(ptr) << endl;
    ptr = montas.get();
    cout << bool(ptr) << endl;
    cout << *ptr << endl;
}



