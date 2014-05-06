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

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif
} // end extern "C"

#include <iostream>

#include "libdar.hpp"
#include "tronc.hpp"
#include "deci.hpp"
#include "testtools.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"
#include "shell_interaction.hpp"
#include "cygwin_adapt.hpp"
#include "fichier_local.hpp"

using namespace libdar;

void f1();
void f2();

int main()
{
    f1();
    f2();
}

void f1()
{
    U_I maj, med, min;

    try
    {
	get_version(maj, med, min);
	string p = "test/source.txt";
	shell_interaction *ui = new (nothrow) shell_interaction(&cout, &cerr, false);
	if(ui == NULL)
	    cout << "ERREUR !" << endl;

	fichier_local h = fichier_local(*ui, p, gf_read_only, 0777, false, true, false);

	display_read(*ui, h);

	try
	{
	    fichier_local f = fichier_local(*ui, "test/source.txt", gf_read_only, 0777, false, false, false);
	    tronc *t;

	    t = new tronc(&f, 0, infinint(10));
	    t->skip(0);
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;

	    display_read(*ui, *t);
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;

	    display_read(*ui, *t);
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;

	    delete t;
	    t = new tronc(&f, 50, infinint(5));
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;

	    display_read(*ui, *t);
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;

	    delete t;
	    fichier_local g = fichier_local(*ui, "test/destination.txt", gf_read_write, 0666, false, true, false);
	    f.skip(0);
	    f.copy_to(g);
	    t = new tronc(&g, 10, infinint(10));

	    try
	    {
		f.skip(0);
		f.copy_to(*t);
	    }
	    catch(Egeneric & e)
	    {
		cerr << e.dump_str();
	    }

	    t->skip_to_eof();
	    display_back_read(*ui, *t);
	    display_back_read(*ui, *t);
	    g.skip(0);
	    display_read(*ui, g);
	    display_read(*ui, g);

	    t->skip_relative(-5);
	    display_read(*ui, *t);
	    t->skip(3);
	    display_read(*ui, *t);
	    t->skip_relative(2);
	    display_read(*ui, *t);

	    delete t;
	}
	catch(Egeneric &f)
	{
	    cerr << f.dump_str();
	}
	if(ui != NULL)
	    delete ui;
    }
    catch(Egeneric & f)
    {
	cerr << f.dump_str();
    }
}


void f2()
{
    try
    {
	string p = "test/source.txt";
	user_interaction *ui = new (nothrow) shell_interaction(&cout, &cerr, false);
	if(ui == NULL)
	    cout << "ERREUR !" << endl;

	try
	{
	    fichier_local f = fichier_local(*ui, "test/source.txt", gf_read_only, 0666, false, false, false);
	    tronc *t;

	    t = new tronc(&f, 1);
	    t->skip(0);
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;

	    display_read(*ui, *t);
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;

	    display_read(*ui, *t);
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;

	    t->skip_to_eof();
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;

	    display_read(*ui, *t);
	    t->skip_relative(1);
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;

	    t->skip_relative(-3);
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;
	    display_read(*ui, *t);

	    delete t;

		/////// now testing writing mode

	    fichier_local g = fichier_local(*ui, "test/destination.txt", gf_read_write, 0666, false, true, false);
	    g.skip(0);
	    display_read(*ui, g);

	    t = new tronc(&g, 2);

	    f.skip(0);
	    f.copy_to(*t);
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;
	    cout << g.get_position() << endl;

	    g.skip(0);
	    display_read(*ui, g);

	    g.skip(1);
	    display_read(*ui, g);

	    g.skip(2);
	    display_read(*ui, g);

	}
	catch(Egeneric &e)
	{
	    cerr << e.dump_str();
	}
	if(ui != NULL)
	    delete ui;
    }
    catch(Egeneric & f)
    {
	cerr << f.dump_str();
    }
}
