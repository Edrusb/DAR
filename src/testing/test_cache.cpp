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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_STDIO_H
#include <stdio.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

}

#include "libdar.hpp"
#include "cache.hpp"
#include "shell_interaction.hpp"
#include "erreurs.hpp"
#include "generic_file.hpp"
#include "shell_interaction.hpp"
#include "cygwin_adapt.hpp"
#include "fichier_local.hpp"

using namespace libdar;
using namespace std;

void f1();
void f2();

static user_interaction *ui = nullptr;

int main()
{
    U_I maj, med, min;

    get_version(maj, med, min);
    ui = new (nothrow) shell_interaction(&cout, &cerr, false);
    if(ui == nullptr)
	cout << "ERREUR !" << endl;
    try
    {
	f1();
	f2();
    }
    catch(Ebug & e)
    {
	ui->warning(e.dump_str());
    }
    catch(Egeneric & e)
    {
	ui->warning(string("Aborting on exception: ") + e.get_message());
    }
    if(ui != nullptr)
	delete ui;
}


void f1()
{
    fichier_local f = fichier_local(*ui, "toto", gf_read_only, 0, false, false, false);
    cache c = cache(f,
		    false,
		    20);
    char buffer[200];

    c.read(buffer, 3);
    c.read(buffer+3, 22);

    c.skip(0);
    c.read(buffer, 5);
    c.skip(10);
    c.read(buffer, 5);
    c.skip(20);
    c.read(buffer, 30);
    c.read(buffer+30, 5);
    c.read(buffer+35, 50);
}

void f2()
{
    int fd = open("titi", O_RDWR|O_TRUNC|O_CREAT|O_BINARY, 0666);
    if(fd < 0)
    {
	printf("%s\n", strerror(errno));
	return;
    }
    fichier_local g = fichier_local(*ui, "titi", gf_read_write, 0666, false, true, false);
    cache c = cache(g,
		    false,
		    20);
    const char *buf = "coucou les amis";
    c.write(buf, strlen(buf));
    c.write(" ", 1);
    c.write("!", 1);
    c.write(" ", 1);
    c.write(buf, strlen(buf));
    c.skip(0);
    c.write("C", 1);
    char buffer[100];
    c.read(buffer, 3);
    c.skip(0);
    c.read(buffer, 99);
    buffer[99] = '\0';
    c.skip_to_eof();
    c.write("*",1);

    c.truncate(5);
    c.skip(3);
    c.read(buffer, 2);
    c.write(buf+5,strlen(buf)-5);
    if(!c.skippable(generic_file::skip_backward, 2))
	throw SRC_BUG;
    c.truncate(10);
    c.skip(8);
    c.read(buffer, 2);
    c.write(buf+10, strlen(buf)-10);
    c.write(" !!", 3);
    if(!c.skippable(generic_file::skip_backward, 2))
	throw SRC_BUG;
    c.truncate(15);
    c.skip(13);
    c.read(buffer, 2);
    c.write(" !?!A<20", 8);
    c.write("!2345*");
    c.skip(20);
    c.read(buffer,2);
    c.write("coucou");
    if(!c.skippable(generic_file::skip_backward, 10))
	throw SRC_BUG;
    c.truncate(c.get_position() - 12);
    c.read(buffer, 2);
    c.write("bye", 3);
}



