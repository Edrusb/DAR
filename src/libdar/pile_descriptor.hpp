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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file pile_descriptor.hpp
    /// \brief optimization structure to quickly access some commonly used layers of a stack of generic_file
    /// \ingroup Private

#ifndef CAT_PILE_DESCRIPTOR_HPP
#define CAT_PILE_DESCRIPTOR_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "pile.hpp"
#include "escape.hpp"
#include "compressor.hpp"
#include "on_pool.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    struct pile_descriptor: public on_pool
    {
	pile *stack;       //< the stack to read from or write to (should never be equal to nullptr)
	escape *esc;       //< an escape layer in stack (may be nullptr)
	compressor *compr; //< a compressor layer in stack (should never be equal to nullptr)
	pile_descriptor() { stack = nullptr; esc = nullptr; compr = nullptr; };
	pile_descriptor(pile *ptr);
	void check(bool small) const; //< check structure coherence with expected read/write mode (small or normal)
    };
	/// @}

} // end of namespace

#endif
