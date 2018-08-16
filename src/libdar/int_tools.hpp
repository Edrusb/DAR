/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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

    /// \file int_tools.hpp
    /// \brief elementary operation for infinint integers
    /// \ingroup Private
    /// \note API included module due to dependencies

#ifndef INT_TOOLS_HPP
#define INT_TOOLS_HPP

#include "../my_config.h"

#include "integers.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    using int_tools_bitfield = unsigned char[8];

    extern void int_tools_swap_bytes(unsigned char &a, unsigned char &b);
    extern void int_tools_swap_bytes(unsigned char *a, U_I size);
    extern void int_tools_expand_byte(unsigned char a, int_tools_bitfield &bit);
    extern void int_tools_contract_byte(const int_tools_bitfield &b, unsigned char & a);

        // integer (agregates) manipulations
        // argument must be a regular interger (a bit field).
    template <class T> extern T int_tools_rotate_right_one_bit(T v)
    {
        bool retenue = (v & 1) != 0;

        v >>= 1;
        if(retenue)
            v |= T(1) << (sizeof(v)*8 - 1);

        return v;
    }

    template <class T> extern T int_tools_maxof_agregate(T unused) { unused = 0; unused = ~unused; unused = unused > 0 ? unused : ~int_tools_rotate_right_one_bit(T(1)); return unused; }

    template <class B> static B int_tools_higher_power_of_2(B val)
    {
        B i = 0;

        while((val >> i) > 1)
            i++;

        return i;
    }

	/// @}

}

#endif
