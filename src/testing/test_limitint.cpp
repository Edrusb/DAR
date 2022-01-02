/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
#include <memory>

#include "integers.hpp"
#include "infinint.hpp"
#include "deci.hpp"
#include "user_interaction.hpp"
#include "shell_interaction.hpp"
#include "cygwin_adapt.hpp"
#include "erreurs.hpp"
#include "generic_file.hpp"
#include "fichier_local.hpp"

using namespace libdar;
using namespace std;

static void routine_limitint(shared_ptr<user_interaction> & dialog);
static void routine_real_infinint(shared_ptr<user_interaction> & dialog);

shared_ptr<user_interaction> ui;

int main()
{
    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(!ui)
	cout << "ERREUR !" << endl;

    try
    {
        ui->pause("Esc = generating dump of large real_integer, OK = testing limitint");
        routine_limitint(ui);
    }
    catch(Euser_abort & e)
    {
        routine_real_infinint(ui);
    }
    ui.reset();
}

static void routine_real_infinint(shared_ptr<user_interaction> & dialog)
{
    fichier_local fic = fichier_local(ui, "toto", gf_read_write, 0666, false, true, false);
    infinint r1 = 1;

    r1 <<= 32;
    r1--;

    r1.dump(fic);
    r1++;
    r1.dump(fic);
}

static void routine_limitint(shared_ptr<user_interaction> & dialog)
{
        /////////////////////////////////////////
        // testing construction overflow
        //
        //

    fichier_local fic = fichier_local("toto", false);
    infinint r1 = infinint(fic);
    infinint r2;

    r1 = r1+1 ; // avoid warning of unused variable
    r2.read(fic);

    U_32 twopower32_1 = ~0;
    U_32 twopower31 = 1 << 31;

    infinint a = (U_32)(twopower32_1);
    infinint b = twopower32_1;

    try
    {
        unsigned long long tmp = twopower32_1;
        tmp++;
        infinint c = tmp;
	c = c+1; // avoid warning of unused variable
    }
    catch(Elimitint & e)
    {
        dialog->message(e.get_message());
    }

    try
    {
        infinint c = (long long)(twopower32_1+1);
	c = c+1; // avoid warning of unused variable
    }
    catch(Elimitint & e)
    {
        dialog->message(e.get_message());
    }

        //////////////////////////////////////////////
        // testing addition overflow
        //
        //

    try
    {
        a++;
    }
    catch(Elimitint & e)
    {
        dialog->message(e.get_message());
    }

    infinint c = twopower31;
    infinint d;
    infinint e = c-1;

    try
    {
        d = c+e;
        d = c+c;
    }
    catch(Elimitint & e)
    {
        dialog->message(e.get_message());
    }


        //////////////////////////////////////////
        // testing multiplication overflow
        //
        //

    const unsigned int twopower16 = 65536;

    a = twopower16 - 1;
    b = twopower16;
    c = twopower16 + 1;

    try
    {
        d = a;
        d *= a; // OK
        d = a;
        d *= c; // OK
        d = b;
        d *=b; // NOK
    }
    catch(Elimitint & e)
    {
        dialog->message(e.get_message());
    }

        // testing left shift overflow

    a = 1;

    try
    {
        a <<= 31; // OK
        a <<= 1;  // NOK;
    }
    catch(Elimitint & e)
    {
        dialog->message(e.get_message());
    }
}
