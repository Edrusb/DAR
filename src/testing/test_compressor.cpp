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
} // end extern "C"

#include "libdar.hpp"
#include "compressor.hpp"
#include "integers.hpp"
#include "cygwin_adapt.hpp"
#include "shell_interaction.hpp"
#include "fichier_local.hpp"

using namespace libdar;

static user_interaction *ui = nullptr;
static void f1();

int main()
{
    U_I maj, med, min;

    get_version(maj, med, min);

    ui = new (nothrow) shell_interaction(&cout, &cerr, false);
    if(ui == nullptr)
	cout << "ERREUR !" << endl;
    f1();
    if(ui != nullptr)
	delete ui;
}

static void f1()
{
    infinint pos2, pos3;

    try
    {
        fichier_local src1 = fichier_local(*ui, "toto", gf_read_only, 0666, false, false, false);
        fichier_local src2 = fichier_local(*ui, "toto", gf_read_only, 0666, false, false, false);
        fichier_local src3 = fichier_local(*ui, "toto", gf_read_only, 0666, false, false, false);
        fichier_local dst1 = fichier_local(*ui, "tutu.none", gf_write_only, 0666, false, true, false);
	fichier_local dst2 = fichier_local(*ui, "tutu.gz", gf_write_only, 0666, false, true, false);
	fichier_local dst3 = fichier_local(*ui, "tutu.bz", gf_write_only, 0666, false, true, false);

        compressor c1 = compressor(none, dst1);
        compressor c2 = compressor(gzip, dst2);
        compressor c3 = compressor(bzip2, dst3);

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
        fichier_local src1 = fichier_local(*ui, "tutu.none", gf_read_only, 0666, false, false, false);
        fichier_local src2 = fichier_local(*ui, "tutu.gz", gf_read_only, 0666, false, false, false);
        fichier_local src3 = fichier_local(*ui, "tutu.bz", gf_read_only, 0666, false, false, false);
        fichier_local dst1 = fichier_local(*ui, "tutu.none.bak", gf_write_only, 0666, false, true, false);
        fichier_local dst2 = fichier_local(*ui, "tutu.gz.bak", gf_write_only, 0666, false, true, false);
        fichier_local dst3 = fichier_local(*ui, "tutu.bz.bak", gf_write_only, 0666, false, true, false);

        compressor c1 = compressor(none, src1);
        compressor c2 = compressor(gzip, src2);
        compressor c3 = compressor(bzip2, src3);

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
