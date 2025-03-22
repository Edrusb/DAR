/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "libdar.hpp"
#include "shell_interaction.hpp"
#include "fichier_local.hpp"
#include "cache.hpp"
#include "memory_file.hpp"
#include "entrepot_local.hpp"
#include "sar.hpp"
#include "trivial_sar.hpp"
#include "tools.hpp"
#include <string>

using namespace libdar;
using namespace std;

static shared_ptr<user_interaction> ui;
static void f1();
static void f2();
static void f3();
static void f4();
static void f5();

int main()
{
    U_I maj, med, min;

    get_version(maj, med, min);

    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(!ui)
	cout << "ERREUR !" << endl;
    f1();
    f2();
    f3();
    f4();
    f5();
}

static void f1()
{
    fichier_local fic(ui, "toto.txt", gf_read_write, 0644, false, true, false);
    string message = "bonjour les amis, il fait beau, il fait chaud, le soleil brille et les cailloux fleurissent";

    fic.write(message.c_str(), message.size());
    fic.truncate(message.size()/2);
    cout << "file size = " << deci(fic.get_position()).human() << endl;
    fic.write(message.c_str(), message.size());
    fic.skip(message.size()/3);
    fic.truncate(message.size()/2);
    fic.write(message.c_str(), message.size());
    fic.truncate(message.size()*2); // after EOF
    fic.write("!", 1);
    fic.truncate(0);

    cache over(fic, false, 10);

    over.write(message.c_str(), message.size());
    over.truncate(message.size()/2);
    cout << "file size = " << deci(over.get_position()).human() << endl;
    over.write(message.c_str(), message.size());
    over.skip(0);  // to flush data pending for writing
    over.write(message.c_str(), message.size());
    over.truncate(message.size() - 5);
    over.write("!!!", 3);
    over.skip(0);
    over.truncate(message.size()/2);
    over.write("HELLO", 5);
    over.sync_write();

    over.truncate(0);
    over.write("123456789", 9);
    over.truncate(5);
    over.write("ABCDEF", 6);
}

static void f2()
{
    memory_file mem;
    string message = "0123456";
    char buffer[100];
    U_I lu = 0;

//    mem.skip_to_eof();
    cout << "file size = " << deci(mem.get_position()).human() << endl;
    mem.write(message.c_str(), message.size());
    cout << "file size = " << deci(mem.get_position()).human() << endl;
    mem.truncate(5);
    cout << "file size = " << deci(mem.get_position()).human() << endl;
    mem.skip_to_eof();
    cout << "file size = " << deci(mem.get_position()).human() << endl;
    mem.skip(0);
    lu = mem.read(buffer, 99);
    buffer[lu] = '\0';
}

static void f3()
{
    storage store(deci("5000000000").computer()); // 5 GByte of RAM
    cout << "storage size = " << deci(store.size()).human() << endl;
    store.truncate(10);
    cout << "storage size = " << deci(store.size()).human() << endl;
}

static void f4()
{
    string message = "0123456789ABCDEF";
    char buffer[100];
    U_I lu = 0;
    shared_ptr<entrepot> entrep(new entrepot_local("", "", false));
    entrep->set_location(tools_getcwd());
    label lab;
    lab.generate_internal_filename();
    sar puzzle(ui,
	       gf_read_write,
	       "toto",
	       "test",
	       60,    // slice size
	       60,    // first slice size
	       false,  // warn overwrite
	       true,  // allow overwrite
	       0,     // pause
	       entrep,
	       lab,
	       lab,
	       false, // force permission
	       0777,  // permission
	       hash_algo::none,
	       2,     // min_digits
	       false,
	       "");

    puzzle.write(message.c_str(), message.size());
    puzzle.skip(3);
    puzzle.write("T", 1);
    puzzle.skip_to_eof();
    puzzle.write("G", 1);
    puzzle.skip(0);
    lu = puzzle.read(buffer, 99);
    buffer[lu] = '\0';
    cout << buffer << endl;
    cout << lu << endl;
    puzzle.truncate(5);
    puzzle.write(message.c_str(), message.size());

    trivial_sar triv(ui,
		     gf_read_write,
		     "tutu",
		     "test",
		     *entrep,
		     lab,
		     lab,
		     "",
		     true,
		     false,
		     false,
		     0777,
		     hash_algo::none,
		     2,
		     false);

    triv.write(message.c_str(), message.size());
    triv.skip(0);
    lu = triv.read(buffer, 99);
    buffer[lu] = '\0';
    cout << buffer << endl;
    cout << lu << endl;
    triv.truncate(5);
    triv.write("coucou", 6);

}

static void f5()
{
}
