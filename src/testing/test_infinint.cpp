/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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

extern "C"
{
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"

#include <iostream>

#include "libdar.hpp"
#include "integers.hpp"
#include "infinint.hpp"
#include "deci.hpp"
#include "cygwin_adapt.hpp"
#include "macro_tools.hpp"
#include "shell_interaction.hpp"
#include "generic_file.hpp"
#include "fichier_local.hpp"

using namespace libdar;
using namespace std;

static void routine1();
static void routine2();

static shared_ptr<user_interaction>ui;

int main()
{
    U_I maj, med, min;

    get_version(maj, med, min);
    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(ui == nullptr)
	cout << "ERREUR !" << endl;
    routine1();
    routine2();
    ui.reset();
}

static void routine1()
{
    infinint f1 = 123;
    infinint f2 = f1;
    infinint f3 = 0;

    libdar::deci d1 = f1;
    libdar::deci d2 = f2;
    libdar::deci d3 = f3;

    ui->message(d1.human() + " " + d2.human() + " " + d3.human());

    fichier_local *fic = new (nothrow) fichier_local(ui, "toto", gf_write_only, 0600, false, true, false);
    if(fic == nullptr)
	throw Ememory("routine1");
    f1.dump(*fic);
    delete fic;
    fic = nullptr;

    fic = new (nothrow) fichier_local("toto", false);
    if(fic == nullptr)
	throw Ememory("routine1");

    f3 = infinint(*fic);
    d3 = libdar::deci(f3);
    ui->message(d3.human());
    delete fic;
    fic = nullptr;

    f1 += 3;
    d1 = libdar::deci(f1);
    ui->message(d1.human());

    f1 -= 2;
    d1 = libdar::deci(f1);
    ui->message(d1.human());

    f1 *= 10;
    d1 = libdar::deci(f1);
    ui->message(d1.human());

    f2 = f1;
    f1 /= 3;
    d1 = libdar::deci(f1);
    ui->message(d1.human());

    f2 %= 3;
    d2 = libdar::deci(f2);
    ui->message(d2.human());

    f2 >>= 12;
    d2 = libdar::deci(f2);
    ui->message(d2.human());

    f1 = 4;
    f2 >>= f1;
    d2 = libdar::deci(f2);
    ui->message(d2.human());

    f1 = 4+12;
    f2 = f3;
    cout << f3 << endl;
    cout << f1 << endl;
    f3 <<= f1;
    cout << f3 << endl;
    cout << (123 << 16) << endl;
    f2 <<= 4+12;
    d2 = libdar::deci(f2);
    ui->message(d2.human());
    d3 = libdar::deci(f3);
    ui->message(d3.human());

    try
    {
	f1 = 1024;
	f2 = 2048;
	f2 -= f1;
	cout << f2 << endl;
	f1 = 4;
	f2 = 1;
	f1 <<= f2;
	f3 = 8;
	cout << f1 << endl;
	cout << f3 << endl;
	f3 -= f1;
	cout << f3 << endl;
	f1 = 4;
	f1 <<= (U_I)1;
	cout << f1 << endl;
	f1 = 1000;
	f2 = 1;
	f1 <<= f2;
	cout << f1 << endl;
	f1 = 1000;
	f1 <<= (U_32)1;
	cout << f1 << endl;
    }
    catch(Egeneric & e)
    {
	cerr << e.dump_str();
    }

    f1 = 21;
    f2 = 1;
    try
    {
        for(f3 = 2; f3 <= f1; f3++)
        {
            d1 = libdar::deci(f1);
            d2 = libdar::deci(f2);
            d3 = libdar::deci(f3);
            ui->message(d1.human() + " " + d2.human() + " " + d3.human());
            f2 *= f3;
            d2 = libdar::deci(f2);
            ui->message(d2.human());
        }
    }
    catch(Elimitint & e)
    {
        ui->message(e.get_message());
    }
    d2 = libdar::deci(f2);
    d1 = libdar::deci(f1);
    ui->message(string("factoriel(") + d1.human() + ") = " + d2.human());
}

static void routine2()
{
    ui->message(libdar::deci(infinint(2).power((U_I)0)).human());
    ui->message(libdar::deci(infinint(2).power(infinint(0))).human());
    ui->message(libdar::deci(infinint(2).power((U_I)1)).human());
    ui->message(libdar::deci(infinint(2).power(infinint(1))).human());
    ui->message(libdar::deci(infinint(2).power((U_I)2)).human());
    ui->message(libdar::deci(infinint(2).power(infinint(2))).human());
}
