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
#include "shell_interaction.hpp"
#include "fichier_local.hpp"
#include "generic_thread.hpp"
#include "memory_file.hpp"

extern "C"
{
}


using namespace std;
using namespace libdar;

static shell_interaction ui = shell_interaction(&cout, &cerr, false);

void f1();


int main(int argc, char *argv[])
{
    U_I maj, med, min;

    get_version(maj, med, min);

    try
    {
	f1();
    }
    catch(Egeneric & e)
    {
	ui.printf("Exception caught: %S", &(e.get_message()));
	cout << e.dump_str() << endl;
    }
    catch(libthreadar::exception_base & e)
    {
	std::string msg;

	for(unsigned int i = 0; i < e.size(); ++i)
		msg = e[i] + ": " + msg;
	ui.printf("libthreadar exception: %S", &msg);
    }
    catch(...)
    {
	ui.printf("unknown exception caught");
    }

    return 0;
}

void f1()
{
    generic_file *src = new fichier_local("toto.txt");
    memory_file dst;
    generic_thread *t1 = new generic_thread(src, 9, 3);
    infinint tmp;

    t1->copy_to(dst);
    tmp = dst.size();
    ui.printf("dst size = %i\n", &tmp);

    t1->skip(0);
    dst.reset();
    t1->read_ahead(1000);
    t1->copy_to(dst);

    t1->skip(0);
    dst.reset();
    t1->read_ahead(1000);
    tmp = t1->get_position();
    ui.printf("t1 posiiton = %i\n", &tmp);
    t1->copy_to(dst);

    delete t1;
    src = new memory_file();
    t1 = new generic_thread(src, 9, 3);
    dst.copy_to(*t1);
    tmp = t1->get_position();
    ui.printf("t1 position = %i\n", &tmp);

    delete t1;
}
