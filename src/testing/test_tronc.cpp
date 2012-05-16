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
// $Id: test_tronc.cpp,v 1.11.2.1 2005/02/02 10:51:36 edrusb Rel $
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

#if HAVE_ERRNO_H
#include <errno.h>
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

using namespace libdar;

int main()
{
    U_I maj, med, min;

    try
    {
	get_version(maj, med, min);
	path p = "test/source.txt";
	user_interaction *ui = shell_interaction_init(&cout, &cerr, false);
	fichier h = fichier(*ui, p, gf_read_only);

	display_read(*ui, h);

	try
	{
	    fichier f = fichier(*ui, "test/source.txt", gf_read_only);
	    tronc *t;

	    t = new tronc(*ui, &f, 0, 10);
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
	    t = new tronc(*ui, &f, 50, 5);
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;

	    display_read(*ui, *t);
	    cout << t->get_position() << endl;
	    cout << f.get_position() << endl;

	    delete t;
	    S_I fd = ::open("test/destination.txt", O_RDWR|O_CREAT|O_TRUNC|O_BINARY, 0666);
	    if(fd < 0)
		ui->warning(strerror(errno));
	    fichier g = fichier(*ui, fd);
	    f.skip(0);
	    f.copy_to(g);
	    t = new tronc(*ui, &g, 10, 10);

	    try
	    {
		f.skip(0);
		f.copy_to(*t);
	    }
	    catch(Egeneric & e)
	    {
		e.dump();
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
	    f.dump();
	}
	if(ui != NULL)
	    delete ui;
    }
    catch(Egeneric & f)
    {
	f.dump();
    }
}
