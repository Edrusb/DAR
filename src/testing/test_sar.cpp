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
#include <iostream>

#include "sar.hpp"
#include "deci.hpp"
#include "testtools.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"
#include "shell_interaction.hpp"
#include "libdar.hpp"
#include "fichier.hpp"

using namespace libdar;

static user_interaction *ui = NULL;

static label data_name;

static void f1();
static void f2();
static void f3();
static void f4();

int main()
{
    U_I major, medium, minor;
    get_version(major, medium, minor);
    ui = shell_interaction_init(&cout, &cerr, false);
    if(ui == NULL)
	cout << "ERREUR !" << endl;
    data_name.clear();
    f1();
    f2();
    f3();
    f4();
    shell_interaction_close();
    if(ui != NULL)
	delete ui;
}

static void f1()
{
    try
    {
        sar sar1 = sar(*ui, "destination", "txt", 100, 110, true, false, 0, path("./test"), data_name, "", "", "", hash_none, false, 0);
        fichier src = fichier(*ui, "./test/source.txt", gf_read_only, tools_octal2int("0777"), false);
        src.copy_to(sar1);
    }
    catch(Egeneric &e)
    {
        e.dump();
    }
}

static void f2()
{
    try
    {
        sar sar2 = sar(*ui, "destination", "txt", path("./test"), 0, false);
        fichier dst = fichier(*ui, "./test/destination.txt", gf_write_only, tools_octal2int("0777"), false);

        sar2.copy_to(dst);
    }
    catch(Egeneric &e)
    {
        e.dump();
    }
}

static void f3()
{
    try
    {
        sar sar3 = sar(*ui, "destination", "txt", path("./test"), 0, false);
        fichier src = fichier(*ui, "./test/source.txt", gf_read_only, tools_octal2int("0777"), false);

        display(sar3.get_position());
        display(src.get_position());

        sar3.skip(210);
        src.skip(210);
        display(sar3.get_position());
        display(src.get_position());
        display_read(*ui, sar3);
        display_read(*ui, src);
        display(sar3.get_position());
        display(src.get_position());

        sar3.skip_to_eof();
        src.skip_to_eof();
        display(sar3.get_position());
        display(src.get_position());
        display_read(*ui, sar3);
        display_read(*ui, src);
        display_back_read(*ui, sar3);
        display_back_read(*ui, src);

        display(sar3.get_position());
        display(src.get_position());
        sar3.skip_relative(-20);
        src.skip_relative(-20);
        display(sar3.get_position());
        display(src.get_position());
        display_read(*ui, sar3);
        display_read(*ui, src);
        display_back_read(*ui, sar3);
        display_back_read(*ui, src);
        display(sar3.get_position());
        display(src.get_position());

        sar3.skip(3);
        src.skip(3);
        display_back_read(*ui, sar3);
        display_back_read(*ui, src);
        display(sar3.get_position());
        display(src.get_position());
        display_read(*ui, sar3);
        display_read(*ui, src);
        display(sar3.get_position());
        display(src.get_position());

        sar3.skip(0);
        src.skip(0);
        display_back_read(*ui, sar3);
        display_back_read(*ui, src);
        display_read(*ui, sar3);
        display_read(*ui, src);
    }
    catch(Egeneric & e)
    {
        e.dump();
    }
}

void f4()
{
    try
    {
        sar sar2 = sar(*ui, "destination", "txt", path("./test"), 0, true, "echo SLICE %n");

	display(sar2.get_position());
	display_back_read(*ui, sar2);
	sar2.skip_relative(-1);
	display(sar2.get_position());
	sar2.skip(0);
	display_read(*ui, sar2);
    }
    catch(Egeneric & e)
    {
	e.dump();
    }

}
