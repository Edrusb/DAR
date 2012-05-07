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
// $Id: factoriel.cpp,v 1.10 2002/12/08 20:03:07 edrusb Rel $
//
/*********************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <errno.h>
#include "infinint.hpp"
#include "deci.hpp"
#include "erreurs.hpp"
#include "test_memory.hpp"
#include "generic_file.hpp"
#include "integers.hpp"

S_I main(S_I argc, char *argv[]) throw()
{
    MEM_BEGIN;
    MEM_IN;
    try
    {
	if(argc != 2 && argc != 3)
	    exit(1);
	
	string s = argv[1];
	deci f = s;
	infinint max = f.computer();
	infinint i = 2;
	infinint p = 1;
	
	while(i <= max)
	    p *= i++;

	cout << "calcul finished, now computing the decimal representation ... " << endl;
	f = deci(p);
	cout << f.human() << endl;
	if(argc == 3)
	{
	    S_I fd = open(argv[2], O_RDWR|O_CREAT|O_TRUNC, 0644);
	    if(fd < 0)
		cout << "cannot open file for test ! " << strerror(errno) << endl;
	    else
	    {
		fichier fic = fd;
		infinint cp;

		p.dump(fic);
		fic.skip(0);
		cp.read_from_file(fic);
		cout << "read from file: " << cp << endl;
	    }
	}
    }
    catch(Egeneric & e)
    {
	e.dump();
    }

    infinint *tmp;
    {
	MEM_IN;
	tmp = new infinint(19237);
	delete tmp;
	MEM_OUT;
    }
    MEM_OUT; // matches the MEM_IN at the beginning of main
    MEM_END;
}    

static void dummy_call(char *x)
{
    static char id[]="$Id: factoriel.cpp,v 1.10 2002/12/08 20:03:07 edrusb Rel $";
    dummy_call(id);
}
