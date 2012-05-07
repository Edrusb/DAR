/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: test_filesystem.cpp,v 1.10 2002/12/08 20:03:07 edrusb Rel $
//
/*********************************************************************/

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <iostream>
#include "filesystem.hpp"
#include "user_interaction.hpp"
#include "test_memory.hpp"
#include "integers.hpp"

static void build();
static void test();
static void del();
static void re_test();

static catalogue *cat;

S_I main()
{
    MEM_IN;
    user_interaction_init(&cout, &cerr);
    cat = new catalogue();
    MEM_OUT;
    build();
    MEM_OUT;
    test();
    MEM_OUT;
    {
	MEM_IN;
	re_test();
	MEM_OUT;
	del();
	MEM_OUT;
    }
    delete cat;
    MEM_OUT;
    filesystem_freemem();
    user_interaction_close();
    MEM_OUT;
    MEM_END;
}

static void build()
{
    S_I fd;
    char phrase[] = "bonjour les amis il fait chaud il fait beau !";
    struct sockaddr_un name;
    name.sun_family = AF_UNIX;
    strcpy(name.sun_path, "arbo/sub/prise");

    mkdir("arbo", 0777);
    mknod("arbo/dev1", 0777 | S_IFCHR, makedev(20, 30));
    mkdir("arbo/sub", 0777);
    fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(fd >= 0)
    {
	bind(fd, (const sockaddr *)&name, sizeof(name));
	close(fd);
    }
    mknod("arbo/sub/tube", 0777 | S_IFIFO, 0);
    fd = open("arbo/sub/fichier", O_WRONLY|O_CREAT, 0777);
    if(fd >= 0)
    {
	write(fd, phrase, strlen(phrase));
	close(fd);
    }
    mknod("arbo/dev2", 0777 | S_IFBLK, makedev(20, 30));
    symlink("/yoyo/crotte", "arbo/lien");
}

static void del()
{
    unlink("arbo/sub/fichier");
    unlink("arbo/sub/tube");
    unlink("arbo/sub/prise");
    rmdir("arbo/sub");
    unlink("arbo/dev1");
    unlink("arbo/dev2");
    unlink("arbo/lien");
    rmdir("arbo");
}

static void test()
{
    entree *p;
    
    filesystem_set_root(path("arbo"), false, true, true, false, false);
    filesystem_reset_read();
    while(filesystem_read(p))
    {
	file *f = dynamic_cast<file *>(p);
	cat->add(p);
	if(f != NULL)
	{
	    generic_file *entree = f->get_data();

	    try
	    {
		fichier sortie = dup(1);
		entree->copy_to(sortie);
	    }
	    catch(...)
	    {
		delete entree;
		throw;
	    }
	    delete entree;
	}
    }
    cat->listing(cout);
}

static void re_test()
{
    const entree *e;
    detruit det1 = detruit("lien", 'l' | 0x80);
    detruit det2 = detruit("dev1", 'd');

    cat->reset_read();
    filesystem_set_root("algi", true, true, true, false, false);
    filesystem_reset_write();
    
    
    while(cat->read(e))
	filesystem_write(e);
    
    filesystem_reset_write();
    filesystem_write(&det1);
    filesystem_write(&det2);
}
