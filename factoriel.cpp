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
// $Id: factoriel.cpp,v 1.10 2002/03/18 11:00:54 denis Rel $
//
/*********************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <iostream.h>
#include "infinint.hpp"
#include "deci.hpp"
#include "erreurs.hpp"
#include "test_memory.hpp"

int main(int argc, char *argv[]) throw()
{
    try
    {
	if(argc != 2)
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
    }
    catch(Egeneric & e)
    {
	e.dump();
    }

    infinint *tmp;
    MEM_IN;
    tmp = new infinint(19237);
    delete tmp;
    MEM_OUT;
    MEM_END;
}    
