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
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#if MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#if !defined(makedev) && defined(mkdev)
#define makedev(a,b) mkdev((a),(b))
#endif
#else
#if MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#endif
#endif
} // end extern "C"

#include <string.h>
#include <iostream>
#include <memory>

#include "libdar.hpp"
#include "filesystem_backup.hpp"
#include "filesystem_restore.hpp"
#include "filesystem_diff.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"
#include "shell_interaction.hpp"
#include "cygwin_adapt.hpp"
#include "label.hpp"
#include "fichier_local.hpp"
#include "tuyau.hpp"

using namespace libdar;
using namespace std;

static shared_ptr<user_interaction> ui;

static void build();
static void test();
static void del();
static void re_test();

static catalogue *cat;

int main()
{
    U_I maj, med, min;
    label data_name;

    get_version(maj, med, min);
    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(!ui)
	cout << "ERREUR !" << endl;
    cat = new catalogue(ui, datetime(120), data_name);
    build();
    test();
    {
        re_test();
        del();
    }
    delete cat;
    ui.reset();
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
    fd = ::open("arbo/sub/fichier", O_WRONLY|O_CREAT|O_BINARY, 0777);
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
    cat_entree *p;
    infinint root_fs_device;
    infinint errors, skipped_dump;
    bool_mask all = true;
    fsa_scope sc;

    filesystem_backup fs(ui, path("arbo"), true, bool_mask(true), false, false, false,false, root_fs_device, false, sc);

    while(fs.read(p, errors, skipped_dump))
    {
        cat_file *f = dynamic_cast<cat_file *>(p);
        cat->add(p);
        if(f != nullptr)
        {
            generic_file *entree = f->get_data(cat_file::normal, nullptr, 0, nullptr);

            try
            {
                crc *val = nullptr;
		infinint crc_size = 1;

                tuyau sortie = tuyau(ui, dup(1));
                entree->copy_to(sortie, crc_size, val);
		if(val == nullptr)
		    throw SRC_BUG;
		try
		{
		    f->set_crc(*val);
		}
		catch(...)
		{
		    delete val;
		    throw;
		}
		delete val;
            }
            catch(...)
            {
                delete entree;
                throw;
            }
            delete entree;
        }
    }
//  cat->listing(*ui, false, all, all, false, false, false, "");
}

static void re_test()
{
    try
    {
	const cat_entree *e;
	cat_detruit det1 = cat_detruit("lien", 'l' | 0x80, datetime(129));
	cat_detruit det2 = cat_detruit("dev1", 'd', datetime(192));
	path where("algi");
	bool_mask all = true;
	crit_constant_action todo =  crit_constant_action(data_preserve, EA_preserve);
	fsa_scope sc;
	filesystem_restore fs(ui, where, true, true, all, comparison_fields::all, true, false, &todo, false, sc);
	bool hasbeencreated, ea_restored, hard_link, fsa_restored;
	libdar::filesystem_restore::action_done_for_data  data_restored;

	cat->reset_read();

	while(cat->read(e))
 	    fs.write(e, data_restored, ea_restored, hasbeencreated, hard_link, fsa_restored);

	fs.reset_write();
	fs.write(&det1, data_restored, ea_restored, hasbeencreated, hard_link, fsa_restored);
	fs.write(&det2, data_restored, ea_restored, hasbeencreated, hard_link, fsa_restored);
    }
    catch(Egeneric & e)
    {
	cerr << e.dump_str();
    }
}
