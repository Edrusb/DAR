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
// $Id: test_compressor.cpp,v 1.6.4.1 2003/12/20 23:05:35 edrusb Rel $
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
} // end extern "C"

#include "compressor.hpp"
#include "integers.hpp"
#include "test_memory.hpp"
#include "cygwin_adapt.hpp"

using namespace libdar;

static void f1();

int main()
{
    MEM_BEGIN;
    MEM_IN;
    f1();
    MEM_OUT;
    MEM_END;
}

static void f1()
{
    infinint pos2, pos3;

    try
    {
        fichier src1 = fichier("toto", gf_read_only);
        fichier src2 = fichier("toto", gf_read_only);
        fichier src3 = fichier("toto", gf_read_only);
        fichier dst1 = open("tutu.none", O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0644);
        fichier dst2 = open("tutu.gz",O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0644);
        fichier dst3 = open("tutu.bz",O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0644);

        compressor c1 = compressor(none, dst1);
        compressor c2 = compressor(gzip, dst2);
        compressor c3 = compressor(bzip2, dst3);

        src1.copy_to(c1);
        src2.copy_to(c2);
        src3.copy_to(c3);

            // ajout d'un deuxieme block de donnees indentiques
        c2.flush_write();
        pos2 = c2.get_position();
        src2.skip(0);
        src2.copy_to(c2);

            // ajout d'un deuxieme block de donnees indentiques
        c3.flush_write();
        pos3 = c3.get_position();
        src3.skip(0);
        src3.copy_to(c3);

            // alteration du premier block de donnees compresses
        c2.flush_write(); // to be sure all data is written to file
        dst2.skip(pos2 / 2);
        dst2.write("A", 1);

            // alteration du premier block de donnees compresses
        c3.flush_write(); // to be sure all data is written to file
        dst3.skip(pos3 / 2);
        dst3.write("A", 1);

    }
    catch(Egeneric & e)
    {
        e.dump();
    }

    try
    {
        fichier src1 = fichier("tutu.none", gf_read_only);
        fichier src2 = fichier("tutu.gz", gf_read_only);
        fichier src3 = fichier("tutu.bz", gf_read_only);
        fichier dst1 = open("tutu.none.bak", O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0644);
        fichier dst2 = open("tutu.gz.bak", O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0644);
        fichier dst3 = open("tutu.bz.bak", O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0644);

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
            e.dump();
            c2.skip(pos2);
        }

        try
        {
            c3.copy_to(dst3);
        }
        catch(Erange &e)
        {
            e.dump();
            c3.skip(pos3);
        }

        c2.flush_read();
        c2.copy_to(dst2);
        c3.flush_read();
        c3.copy_to(dst3);
    }
    catch(Egeneric & e)
    {
        e.dump();
    }
    unlink("tutu.none");
    unlink("tutu.gz");
    unlink("tutu.none.bak");
    unlink("tutu.gz.bak");
    unlink("tutu.bz");
    unlink("tutu.bz.bak");
}
