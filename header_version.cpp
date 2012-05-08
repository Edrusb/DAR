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
// $Id: header_version.cpp,v 1.6 2003/02/11 22:01:50 edrusb Rel $
//
/*********************************************************************/
//

#pragma implementation

#include "header_version.hpp"
#include "integers.hpp"

void version_copy(version & left, const version & right)
{
    for(S_I i = 0; i < VERSION_SIZE; i++)
	left[i] = right[i];
}

static void dummy_call(char *x)
{
    static char id[]="$Id: header_version.cpp,v 1.6 2003/02/11 22:01:50 edrusb Rel $";
    dummy_call(id);
}

bool version_greater(const version & left, const version & right)
{
    S_I i = 0;

    while(i < VERSION_SIZE && left[i] == right[i])
	i++;
    
    return i < VERSION_SIZE && left[i] > right[i];
}
