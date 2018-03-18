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
#include <memory>

#include "sar.hpp"
#include "deci.hpp"
#include "testtools.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"
#include "shell_interaction.hpp"
#include "libdar.hpp"
#include "fichier_local.hpp"
#include "entrepot_local.hpp"

using namespace libdar;
using namespace std;

static shared_ptr<user_interaction> ui;

static label data_name;
static label internal_name;

static void f1();
static void f2();
static void f3();
static void f4();

int main()
{
    U_I major, medium, minor;
    get_version(major, medium, minor);
    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(!ui)
	cout << "ERREUR !" << endl;
    data_name.clear();
    internal_name.generate_internal_filename();
    f1();
    f2();
    f3();
    f4();
    ui.reset();
}

static void f1()
{
    entrepot_local where = entrepot_local("", "", false);
    where.set_location("./test");
    try
    {
	sar sar1(ui, gf_write_only, "destination", "txt", 100, 110, true, false, 0, where, internal_name, data_name, false, 0, hash_none, false, 0);
        fichier_local src = fichier_local(ui, "./test/source.txt", gf_read_only, 0666, false, false, false);
        src.copy_to(sar1);
    }
    catch(Egeneric &e)
    {
        cerr << e.dump_str();
    }
}

static void f2()
{
    entrepot_local where = entrepot_local("", "", false);
    where.set_location("./test");
    try
    {
        sar sar2(ui, "destination", "txt", where, 0, false);
        fichier_local dst = fichier_local(ui, "./test/destination.txt", gf_write_only, 0777, false, true, false);

        sar2.copy_to(dst);
    }
    catch(Egeneric &e)
    {
        cerr << e.dump_str();
    }
}

static void f3()
{
    entrepot_local where = entrepot_local("", "", false);
    where.set_location("./test");

    try
    {
        sar sar3(ui, "destination", "txt", where, 0, false);
        fichier_local src = fichier_local(ui, "./test/source.txt", gf_read_only, 0666, false, false, false);

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
        cerr << e.dump_str();
    }
}

void f4()
{
    entrepot_local where = entrepot_local("", "", false);
    where.set_location("./test");

    try
    {
        sar sar2(ui, "destination", "txt", where, 0, true, "echo SLICE %n");

	display(sar2.get_position());
	display_back_read(*ui, sar2);
	sar2.skip_relative(-1);
	display(sar2.get_position());
	sar2.skip(0);
	display_read(*ui, sar2);
    }
    catch(Egeneric & e)
    {
	cerr << e.dump_str();
    }

}
