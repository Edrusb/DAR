/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

#include "int_tools.hpp"
#include "erreurs.hpp"

namespace libdar
{
    void int_tools_swap_bytes(unsigned char &a, unsigned char &b)
    {
        unsigned char c = a;
        a = b;
        b = c;
    }

    void int_tools_swap_bytes(unsigned char *a, U_I size)
    {
        if(size <= 1)
            return;
        else
        {
            int_tools_swap_bytes(a[0], a[size-1]);
            int_tools_swap_bytes(a+1, size-2); // terminal recursivity
        }
    }

    void int_tools_expand_byte(unsigned char a, int_tools_bitfield &bit)
    {
        unsigned char mask = 0x80;

        for(S_I i = 0; i < 8; ++i)
        {
            bit[i] = (a & mask) >> (7 - i);
            mask >>= 1;
        }
    }

    void int_tools_contract_byte(const int_tools_bitfield &b, unsigned char & a)
    {
        a = 0;

        for(S_I i = 0; i < 8; ++i)
        {
            a <<= 1;
            if(b[i] > 1)
                throw Erange("infinint.cpp : contract_byte", gettext("a binary digit is either 0 or 1"));
            a += b[i];
        }
    }

} // end of namespace
