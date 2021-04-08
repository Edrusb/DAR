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


    /// \file compress_module.hpp
    /// \brief provides abstracted interface of per-block compression/decompression
    /// \ingroup Private

#ifndef COMPRESS_MODULE_HPP
#define COMPRESS_MODULE_HPP

#include "../my_config.h"
#include "integers.hpp"
#include "compression.hpp"
#include <memory>

namespace libdar
{
	/// \addtogroup Private
	/// @{


    class compress_module
    {
    public:
	compress_module() {};
	compress_module(const compress_module & ref) = default;
	compress_module(compress_module && ref) noexcept = default;
	compress_module & operator = (const compress_module & ref) = default;
	compress_module & operator = (compress_module && ref) noexcept = default;
	virtual ~compress_module() = default;

	    /// return the compression algorithm used by the module
	virtual compression get_algo() const = 0;

	    /// returns the maximum size of data to be compressed as a single block
	virtual U_I get_max_compressing_size() const = 0;

	    /// minimal buffer size to compress clear_size of data for compression to be guaranteed to succeed

	    /// \param[in] clear_size is the size of the data buffer to compress
	    /// \return the minimal size of the destination buffer for compression to be garanteed to succeed
	virtual U_I get_min_size_to_compress(U_I clear_size) const = 0;

	    /// compress a block of data

	    /// \param[in] normal points to the first byte of the data block to compress
	    /// \param[in] normal_size is the number of bytes found at normal address to be compressed
	    /// \param[in] zip_buf is where to put the resulting compressed data
	    /// \param[in] zip_buf_size is the allocated bytes at zip_buf address
	    /// \return the number of bytes effectively used in zip_buf by the compressed data
	    /// \note in case of error a Egeneric based exception should be thrown
	virtual U_I compress_data(const char *normal,
				   const U_I normal_size,
				   char *zip_buf,
				   U_I zip_buf_size) const = 0;

	    /// uncompress a block of data

	    /// \param[in] zip_buf points to the first byte of the compressed data block
	    /// \param[in] zip_buf_size is the number of bytes of the compressed dat starting at zip_buf
	    /// \param[in] normal is the place where to write the uncompressed data
	    /// \param[in] nomral_size is the allocated bytes starting at normal address
	    /// \return the number of bytes written at normal address that constitute the uncompressed data
	    /// \note it is expected that the implementation would throw an libdar::Edata() exception
	    /// on error, like corrupted compressed data submitted for decompression
	virtual U_I uncompress_data(const char *zip_buf,
				     const U_I zip_buf_size,
				     char *normal,
				     U_I normal_size) const = 0;

	    /// used to duplicate a inherited class of compress_module when it is pointed by a compress_module pointer

	    /// \note this call should provide a pointer to a valid object or throw an exception (Ememory, ...)
	virtual std::unique_ptr<compress_module> clone() const = 0;

    };

	/// @}

} // end of namespace

#endif
