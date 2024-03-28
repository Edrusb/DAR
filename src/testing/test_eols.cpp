/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif
}

#include "../libdar/eols.hpp"

#include <deque>
#include <string>

using namespace libdar;
using namespace std;

void f1();

int main()
{
    deque<string> liste;
    U_I seq_length;
    U_I over;
    bool x;

    liste.push_back("totu");
    liste.push_back("to");
    liste.push_back("ota");
    liste.push_back("oto");

    eols ob(liste);

    ob.reset_detection();

    x = ob.eol_reached('t', seq_length, over);
    x = ob.eol_reached('o', seq_length, over);
    x = ob.eol_reached('t', seq_length, over);
    x = ob.eol_reached('a', seq_length, over);

    x = ob.eol_reached('a', seq_length, over);
}
