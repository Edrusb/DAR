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
// $Id: test_infinint.cpp,v 1.16 2005/09/11 19:01:16 edrusb Rel $
//
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
#include "test_memory.hpp"
#include "integers.hpp"
#include "infinint.hpp"
#include "deci.hpp"
#include "cygwin_adapt.hpp"
#include "macro_tools.hpp"
#include "shell_interaction.hpp"
#include "generic_file.hpp"

using namespace libdar;
using namespace std;

static void routine1();
static void routine2();

static user_interaction *ui = NULL;

int main()
{
    MEM_BEGIN;
    MEM_IN;
    U_I maj, med, min;

    get_version(maj, med, min);
    ui = shell_interaction_init(&cout, &cerr, false);
    if(ui == NULL)
	cout << "ERREUR !" << endl;
    routine1();
    routine2();
    shell_interaction_close();
    if(ui != NULL)
	delete ui;
    MEM_OUT;
    MEM_END;
}

static void routine1()
{
    infinint f1 = 123;
    infinint f2 = f1;
    infinint f3 = 0;

    deci d1 = f1;
    deci d2 = f2;
    deci d3 = f3;

    ui->warning(d1.human() + " " + d2.human() + " " + d3.human());

    S_I fd = ::open("toto", O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0644);
    if(fd >= 0)
    {
        f1.dump(*ui, fd);
        close(fd);
        fd = ::open("toto", O_RDONLY|O_BINARY);
        if(fd >= 0)
        {
            f3 = infinint(*ui, &fd, NULL);
            d3 = deci(f3);
            ui->warning(d3.human());
        }
        close(fd);
        fd = -1;
    }

    f1 += 3;
    d1 = deci(f1);
    ui->warning(d1.human());

    f1 -= 2;
    d1 = deci(f1);
    ui->warning(d1.human());

    f1 *= 10;
    d1 = deci(f1);
    ui->warning(d1.human());

    f2 = f1;
    f1 /= 3;
    d1 = deci(f1);
    ui->warning(d1.human());

    f2 %= 3;
    d2 = deci(f2);
    ui->warning(d2.human());

    f2 >>= 12;
    d2 = deci(f2);
    ui->warning(d2.human());

    f1 = 4;
    f2 >>= f1;
    d2 = deci(f2);
    ui->warning(d2.human());

    f1 = 4+12;
    f2 = f3;
    cout << f3 << endl;
    cout << f1 << endl;
    f3 <<= f1;
    cout << f3 << endl;
    cout << (123 << 16) << endl;
    f2 <<= 4+12;
    d2 = deci(f2);
    ui->warning(d2.human());
    d3 = deci(f3);
    ui->warning(d3.human());

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
	e.dump();
    }

    f1 = 21;
    f2 = 1;
    try
    {
        for(f3 = 2; f3 <= f1; f3++)
        {
            d1 = deci(f1);
            d2 = deci(f2);
            d3 = deci(f3);
            ui->warning(d1.human() + " " + d2.human() + " " + d3.human());
            f2 *= f3;
            d2 = deci(f2);
            ui->warning(d2.human());
        }
    }
    catch(Elimitint & e)
    {
        ui->warning(e.get_message());
    }
    d2 = deci(f2);
    d1 = deci(f1);
    ui->warning(string("factoriel(") + d1.human() + ") = " + d2.human());
}

static void routine2()
{
    ui->warning(deci(infinint(2).power((U_I)0)).human());
    ui->warning(deci(infinint(2).power(infinint(0))).human());
    ui->warning(deci(infinint(2).power((U_I)1)).human());
    ui->warning(deci(infinint(2).power(infinint(1))).human());
    ui->warning(deci(infinint(2).power((U_I)2)).human());
    ui->warning(deci(infinint(2).power(infinint(2))).human());
}
