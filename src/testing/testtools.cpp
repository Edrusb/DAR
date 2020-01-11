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

#include <iostream>

#include "deci.hpp"
#include "tools.hpp"
#include "testtools.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"

using namespace libdar;
using namespace std;

void display(const infinint & x)
{
    libdar::deci vu = x;
    cout << vu.human() << endl;
}

void display_read(user_interaction & dialog, generic_file & f)
{
    const S_I size = 10;
    char buffer[size];
    S_I lu = f.read(buffer, size);
    if(lu < size)
        buffer[lu] = '\0';
    else
        buffer[size-1] = '\0';
    dialog.message(tools_printf("lu = %d : [%s]\n", lu, buffer));
}

void display_back_read(user_interaction & dialog, generic_file & f)
{
    const S_I size = 10;
    char buffer[size];
    S_I lu = 0;
    while(lu < size && f.read_back(buffer[lu]) == 1 )
        lu++;
    if(lu < size)
        buffer[lu] = '\0';
    else
        buffer[size-1] = '\0';
    dialog.message(tools_printf("lu = %d : [%s]\n", lu, buffer));
}
