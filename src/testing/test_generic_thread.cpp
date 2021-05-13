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

static shared_ptr<shell_interaction> ui(new shell_interaction(cout, cerr, false));

void f1();
void f2(const string & src, const string & dst);


int main(int argc, char *argv[])
{
    U_I maj, med, min;

    get_version(maj, med, min);

    try
    {
//	f1();
	if(argc != 3)
	    cout << "usage: " << argv[0] << " <src file> <dst file>" << endl;
	else
	    f2(argv[1], argv[2]);
    }
    catch(Egeneric & e)
    {
	ui->printf("Exception caught: %S", &(e.get_message()));
	cout << e.dump_str() << endl;
    }
    catch(libthreadar::exception_base & e)
    {
	std::string msg;

	for(unsigned int i = 0; i < e.size(); ++i)
	    msg = e[i] + ": " + msg;
	ui->printf("libthreadar exception: %S", &msg);
    }
    catch(...)
    {
	ui->printf("unknown exception caught");
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
    ui->printf("dst size = %i\n", &tmp);

    t1->skip(0);
    dst.reset();
    t1->read_ahead(1000);
    t1->copy_to(dst);

    t1->skip(0);
    dst.reset();
    t1->read_ahead(1000);
    tmp = t1->get_position();
    ui->printf("t1 posiiton = %i\n", &tmp);
    t1->copy_to(dst);

    delete t1;
    src = new memory_file();
    t1 = new generic_thread(src, 9, 3);
    dst.skip(0);
    dst.copy_to(*t1);
    tmp = t1->get_position();
    ui->printf("t1 position = %i\n", &tmp);

    delete t1;
}

void f2(const string & src, const string & dst)
{
    generic_file *src_f = new fichier_local(src);
    generic_file *dst_f = new fichier_local(ui, dst, gf_write_only, 0600, false, true, false);
    generic_thread t1(dst_f, 10, 20);
    src_f->copy_to(t1);
}
