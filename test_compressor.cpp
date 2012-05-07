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
// $Id: test_compressor.cpp,v 1.9 2002/10/31 21:02:36 edrusb Rel $
//
/*********************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "compressor.hpp"
#include "test_memory.hpp"
#include "integers.hpp"

static void f1();

S_I main()
{
    MEM_IN;
    f1();
    MEM_OUT;
    MEM_END;
}

static void f1()
{
    infinint pos;

    try
    {
	fichier src1 = fichier("toto", gf_read_only);
	fichier src2 = fichier("toto", gf_read_only);
	fichier dst1 = open("tutu.none", O_WRONLY|O_CREAT|O_TRUNC, 0644);
	fichier dst2 = open("tutu.gz", O_WRONLY|O_CREAT|O_TRUNC, 0644);
	
	compressor c1 = compressor(none, dst1);
	compressor c2 = compressor(gzip, dst2);
	
	src1.copy_to(c1);
	src2.copy_to(c2);

	    // ajout d'un deuxieme block de donnees indentiques
	c2.flush_write();
	pos = c2.get_position();
	src2.skip(0);
	src2.copy_to(c2);

	    // alteration du premier block de donnees compresses
	c2.flush_write(); // to be sure all data is written to file
//	dst2.skip(pos / 2);
//	dst2.write("A", 1);
    }
    catch(Egeneric & e)
    {
	e.dump();
    }

    try
    {
	fichier src1 = fichier("tutu.none", gf_read_only);
	fichier src2 = fichier("tutu.gz", gf_read_only);
	fichier dst1 = open("tutu.none.bak", O_WRONLY|O_CREAT|O_TRUNC, 0644);
	fichier dst2 = open("tutu.gz.bak", O_WRONLY|O_CREAT|O_TRUNC, 0644);
	
	compressor c1 = compressor(none, src1);
	compressor c2 = compressor(gzip, src2);
	
	c1.copy_to(dst1);
	try {
	    c2.copy_to(dst2);
	}
	catch(Erange &e)
	{
	    e.dump();
	    c2.skip(pos);
	}
	c2.flush_read();
	c2.copy_to(dst2);
    }
    catch(Egeneric & e)
    {
	e.dump();
    }
    unlink("tutu.none");
    unlink("tutu.gz");
    unlink("tutu.none.bak");
    unlink("tutu.gz.bak");
}

