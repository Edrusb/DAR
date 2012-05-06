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
// $Id: testtools.cpp,v 1.8 2002/03/18 11:00:54 denis Rel $
//
/*********************************************************************/

#include <iostream.h>
#include "deci.hpp"
#include "testtools.hpp"

void display(const infinint & x)
{
    deci vu = x;
    cout << vu.human() << endl;
}

void display_read(generic_file & f)
{
    const int size = 10;
    char buffer[size];
    int lu = f.read(buffer, size);
    if(lu < size)
	buffer[lu] = '\0';
    else
	buffer[size-1] = '\0';
    printf("lu = %d : [%s]\n", lu, buffer);
}

void display_back_read(generic_file & f)
{
    const int size = 10;
    char buffer[size];
    int lu = 0;
    while(lu < size && f.read_back(buffer[lu]) == 1 )
	lu++;
    if(lu < size)
	buffer[lu] = '\0';
    else
	buffer[size-1] = '\0';
    printf("lu = %d : [%s]\n", lu, buffer);
}
