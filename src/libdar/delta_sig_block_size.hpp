/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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

    /// \file delta_sig_block_size.hpp
    /// \brief structure used to define how to select block size for delta signature
    /// \ingroup API

#ifndef DELTA_SIG_BLOCK_SIZE_HPP
#define DELTA_SIG_BLOCK_SIZE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "infinint.hpp"


namespace libdar
{
	/// defines how to calculate delta signature block size based of file size to delta sign

	/// \note the global formula is block_size = multiplicator/divisor*fs_function(filesize)
	/// \note of course, divisor cannot be null


    struct delta_sig_block_size
    {
	    /// defines the function to use to derivate block size from file size
	enum fs_function_t
	{
	    fixed,      ///< block size is independant from file size
	    linear,     ///< block size if proportionnal to file size
	    log2,       ///< block size is proportional to log2(file size)
	    square2,    ///< block size is proportional to filesize^2
	    square3     ///< block size if proportional to filesize^3
	};

	fs_function_t fs_function; //< the function to use to calculate the signature block len
	infinint multiplier; ///< function dependently used multiplier
	infinint divisor;    ///< function dependently used divisor
	U_I min_block_len;   ///< calculated block len will never be lower than that
	U_I max_block_len;   ///< calculated block len will never be higer than that except if this field is set to zero (disabling this ceiling check)

	    // definitions to help using the struct

	delta_sig_block_size() { reset(); }; ///< set the structure to defaults value
	delta_sig_block_size(const delta_sig_block_size & ref) = default;
	delta_sig_block_size(delta_sig_block_size && ref) noexcept = default;
	delta_sig_block_size & operator = (const delta_sig_block_size & ref) = default;
	delta_sig_block_size & operator = (delta_sig_block_size && ref) noexcept = default;
	~delta_sig_block_size() = default;

	bool operator == (const delta_sig_block_size & ref) const;

	    /// reset to default value
	void reset();

	    /// whether structure has default values
	bool equals_default() { return (*this) == delta_sig_block_size(); };

	    /// check the sanity of the provided values
	void check() const;

	    /// calculate the value of the block size given the file size

	    /// \param[in] filesize is the size of the file which delta signature to be calculated
	    /// \return the block len to use for delta signature
	U_I calculate(const infinint & filesize) const;

    };


} // end of namespace

#endif
