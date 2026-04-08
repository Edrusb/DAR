/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2026 Denis Corbin
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

#include <string>
#include <memory>

#include "sparse_file.hpp"
#include "memory_file.hpp"
#include "null_file.hpp"
#include "user_interaction.hpp"
#include "libdar.hpp"

using namespace libdar;
using namespace std;

static shared_ptr<user_interaction> ui;

static void f1();

int main()
{
    U_I major, medium, minor;
    get_version(major, medium, minor);
    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(!ui)
	cout << "ERREUR !" << endl;
    f1();
    ui.reset();
}

void display_buffer(char* buffer, U_I size)
{
    string ret = "buffer value = [";

    for(U_I i = 0 ; i < size ; ++i)
    {
	if(buffer[i] == 0)
	    ret += "\0";
	else
	    ret += string(1, buffer[i]);
	if(i < size - 1)
	    ret += "|";
    }
    ret += "]";
    ui->printf(ret.c_str());
}

void display_generic(generic_file & f)
{
    static unsigned int bufsize = 1000;
    char buffer[bufsize];
    infinint pos = f.get_position();
    U_I lu = f.read(buffer, bufsize);
    f.skip(pos);

    display_buffer(buffer, lu);
    ui->printf("   position %i\n", &pos);
}


static void check(sparse_file & sparse)
{
    display_generic(sparse);
    sparse.skip(0);
    display_generic(sparse);
    sparse.skip_relative(5);
    display_generic(sparse);
    sparse.skip_relative(-2);
    display_generic(sparse);
    sparse.skip(10);
    display_generic(sparse);
    sparse.skip(30);
    display_generic(sparse);
    sparse.skip(5);
    display_generic(sparse);
    sparse.skip(40);
    display_generic(sparse);
    sparse.skip(30);
    display_generic(sparse);
}


static void f1()
{
    string zeroed = string(20, '\0');
    string one = "hello world";
    string two = "salut tout le monde";
    memory_file src, dst;
    sparse_file sp1(&dst, 15);
    sparse_file sp2(&dst, 15);
    infinint tmp;

	// without hole

    src.write(one.c_str(), one.size());
    src.write(two.c_str(), two.size());
    src.skip(0);
    display_generic(src);

    src.copy_to(sp1);

    dst.skip(0);
    dst.change_mode(gf_read_only);
    display_generic(dst);

    sp2 = sparse_file(&dst, 15);
    check(sp2);

	// with hole

    src.reset();
    src.write(one.c_str(), one.size());
    src.write(zeroed.c_str(), zeroed.size());
    src.write(two.c_str(), two.size());
    src.skip(0);
    display_generic(src);

    dst.reset();
    dst.change_mode(gf_write_only);
    sp1 = sparse_file(&dst, 15);
    src.copy_to(sp1);

    dst.skip(0);
    dst.change_mode(gf_read_only);
    display_generic(dst);

    sp2 = sparse_file(&dst, 15);
    check(sp2);
}

