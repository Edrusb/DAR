/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
} // end extern "C"

#include <memory>
#include "libdar.hpp"
#include "compressor.hpp"
#include "integers.hpp"
#include "cygwin_adapt.hpp"
#include "shell_interaction.hpp"
#include "fichier_local.hpp"

using namespace libdar;
using namespace std;

static shared_ptr<user_interaction> ui;
static void f1();
static void f2();

int main()
{
    U_I maj, med, min;

    get_version(maj, med, min);

    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(!ui)
	cout << "ERREUR !" << endl;
    f1();
    f2();
    ui.reset();
}

static void f1()
{
    infinint pos2, pos3;

    try
    {
        fichier_local src1 = fichier_local(ui, "toto", gf_read_only, 0666, false, false, false);
        fichier_local src2 = fichier_local(ui, "toto", gf_read_only, 0666, false, false, false);
        fichier_local src3 = fichier_local(ui, "toto", gf_read_only, 0666, false, false, false);
        fichier_local dst1 = fichier_local(ui, "tutu.none", gf_write_only, 0666, false, true, false);
	fichier_local dst2 = fichier_local(ui, "tutu.gz", gf_write_only, 0666, false, true, false);
	fichier_local dst3 = fichier_local(ui, "tutu.bz", gf_write_only, 0666, false, true, false);

        compressor c1(compression::none, dst1);
        compressor c2(compression::gzip, dst2);
        compressor c3(compression::bzip2, dst3);

        src1.copy_to(c1);
        src2.copy_to(c2);
        src3.copy_to(c3);

	    // ajout d'un deuxieme block de donnees indentiques
	c2.sync_write();
	pos2 = c2.get_position();
	src2.skip(0);
	src2.copy_to(c2);

	    // ajout d'un deuxieme block de donnees indentiques
        c3.sync_write();
        pos3 = c3.get_position();
        src3.skip(0);
        src3.copy_to(c3);

            // alteration du premier block de donnees compresses
	c2.sync_write(); // to be sure all data is written to file
	dst2.skip(pos2 / 2);
	dst2.write("A", 1);

	    // alteration du premier block de donnees compresses
	c3.sync_write(); // to be sure all data is written to file
	dst3.skip(pos3 / 2);
	dst3.write("A", 1);
    }
    catch(Egeneric & e)
    {
        cerr << e.dump_str();
    }

    try
    {
        fichier_local src1 = fichier_local(ui, "tutu.none", gf_read_only, 0666, false, false, false);
        fichier_local src2 = fichier_local(ui, "tutu.gz", gf_read_only, 0666, false, false, false);
        fichier_local src3 = fichier_local(ui, "tutu.bz", gf_read_only, 0666, false, false, false);
        fichier_local dst1 = fichier_local(ui, "tutu.none.bak", gf_write_only, 0666, false, true, false);
        fichier_local dst2 = fichier_local(ui, "tutu.gz.bak", gf_write_only, 0666, false, true, false);
        fichier_local dst3 = fichier_local(ui, "tutu.bz.bak", gf_write_only, 0666, false, true, false);

        compressor c1(compression::none, src1);
        compressor c2(compression::gzip, src2);
        compressor c3(compression::bzip2, src3);

        c1.copy_to(dst1);

        try
        {
            c2.copy_to(dst2);
        }
        catch(Erange &e)
        {
            cerr << e.dump_str();
            c2.skip(pos2);
	    dst2.skip(0);
	    c2.copy_to(dst2);
        }

        try
        {
            c3.copy_to(dst3);
        }
        catch(Erange &e)
        {
            cerr << e.dump_str();
            c3.skip(pos3);
	    dst3.skip(0);
	    c3.copy_to(dst3);
        }
    }
    catch(Egeneric & e)
    {
        cerr << e.dump_str();
    }
    unlink("tutu.none");
    unlink("tutu.gz");
    unlink("tutu.none.bak");
    unlink("tutu.gz.bak");
    unlink("tutu.bz");
    unlink("tutu.bz.bak");
}


static void f2()
{
    try
    {
	string clearfile = "toto.clear";
	string zipfile = "toto.zstd";
	string unzipfile1 = "toto.unzipped1";
	string unzipfile2 = "toto.unzipped2";
	infinint pos;

	if(true)
	{
	    fichier_local src = fichier_local(ui, clearfile, gf_read_only, 0666, false, false, false);
	    fichier_local dst = fichier_local(ui, zipfile, gf_write_only, 0666, false, true, false);

	    compressor cz(compression::zstd, dst, 22);

	    src.copy_to(cz);
	    cz.sync_write();
	    pos = cz.get_position();
	    src.skip(0);
	    src.copy_to(cz);
	}

	if(true)
	{
	    fichier_local src = fichier_local(ui, zipfile, gf_read_only, 0666, false, false, false);
	    fichier_local dst1 = fichier_local(ui, unzipfile1, gf_write_only, 0666, false, true, false);
	    fichier_local dst2 = fichier_local(ui, unzipfile2, gf_write_only, 0666, false, true, false);

	    compressor cz(compression::zstd, src);

	    cz.copy_to(dst1);
	    cz.flush_read();
	    cz.skip(pos);
	    cz.copy_to(dst2);
	}
    }
    catch(Egeneric &e)
    {
	cerr << e.dump_str();
    }
}
