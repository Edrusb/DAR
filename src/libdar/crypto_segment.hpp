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
// to contact the author, see the AUTHOR file
/*********************************************************************/

    /// \file crypto_segment.hpp
    /// \brief defines unit block of information ciphered as once
    /// \ingroup Private
    ///

#ifndef CRYPTO_SEGMENT_HPP
#define CRYPTO_SEGMENT_HPP

#include "../my_config.h"
#include "mem_block.hpp"


namespace libdar
{

	/// \addtogroup Private
	/// @{

    struct crypto_segment
    {
	crypto_segment(U_I crypted_size, U_I clear_size): crypted_data(crypted_size), clear_data(clear_size) {};
	mem_block crypted_data;
	mem_block clear_data;
	infinint block_index;
	void reset() { crypted_data.reset(); clear_data.reset(); block_index = 0; };
    };

	/// @}

} // end of namespace

#endif
