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

#include "integers.hpp"
#include "infinint.hpp"
#include "deci.hpp"
#include "user_interaction.hpp"
#include "shell_interaction.hpp"
#include "cygwin_adapt.hpp"
#include "erreurs.hpp"
#include "generic_file.hpp"
#include "fichier.hpp"


static void routine_limitint(user_interaction & dialog);
static void routine_real_infinint(user_interaction & dialog);

using namespace libdar;

int main()
{
    user_interaction *ui = shell_interaction_init(&cout, &cerr, false);
    if(ui == NULL)
	cout << "ERREUR !" << endl;

    try
    {
        ui->pause("Esc = generating dump of large real_integer, OK = testing limitint");
        routine_limitint(*ui);
    }
    catch(Euser_abort & e)
    {
        routine_real_infinint(*ui);
    }
    shell_interaction_close();
    if(ui != NULL)
	delete ui;
}

static void routine_real_infinint(user_interaction & dialog)
{
    int fd = ::open("toto", O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0644);

    if(fd >= 0)
    {
        infinint r1 = 1;

        r1 <<= 32;
        r1--;

        r1.dump(dialog, fd);
        r1++;
        r1.dump(dialog, fd);

        close(fd);
    }
}

static void routine_limitint(user_interaction & dialog)
{
        /////////////////////////////////////////
        // testing construction overflow
        //
        //

    int fd = ::open("toto", O_RDONLY | O_BINARY);
    if(fd > 0)
    {
        infinint r1 = infinint(dialog, fd);
        infinint r2;

        try
        {
            fichier tmp = fichier(dialog, fd);
            r2.read(tmp);
	    r2 += r1; // avoid compiler complaining about r1 not used
        }
        catch(Elimitint & e)
        {
            dialog.warning(e.get_message());
        }
    }

    U_32 twopower32_1 = ~0;
    U_32 twopower31 = 1 << 31;

    infinint a = (U_32)(twopower32_1);
    infinint b = twopower32_1;

    try
    {
        unsigned long long tmp = twopower32_1;
        tmp++;
        infinint c = tmp;
	c = c+1; // avoid compiler complaining against unused variable c
    }
    catch(Elimitint & e)
    {
        dialog.warning(e.get_message());
    }

    try
    {
        infinint c = (long long)(twopower32_1+1);
	c = c+1; // avoid compiler complaining against unused variable c
    }
    catch(Elimitint & e)
    {
        dialog.warning(e.get_message());
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
        dialog.warning(e.get_message());
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
        dialog.warning(e.get_message());
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
        dialog.warning(e.get_message());
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
        dialog.warning(e.get_message());
    }
}
