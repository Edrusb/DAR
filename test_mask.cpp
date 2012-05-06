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
// $Id: test_mask.cpp,v 1.9 2002/05/28 20:17:29 denis Rel $
//
/*********************************************************************/

#include <iostream.h>
#include "mask.hpp"

static void display_res(mask *m, string s)
{
    cout << s << " : " << (m->is_covered(s) ? "OUI" : "non") << endl;
}

int main()
{
    simple_mask m1 = string("*.toto");
    simple_mask m2 = string("a?.toto");
    simple_mask m3 = string("a?.toto");
    simple_mask m4 = string("*.toto");

    display_res(&m1, "tutu.toto");
    display_res(&m2, "a1.toto");
    display_res(&m3, "b1.toto");
    display_res(&m4, "toto");
}
