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


    /// \file compress_module.hpp
    /// \brief provides abstracted interface of per-block compression/decompression
    /// \ingroup Private

#ifndef COMPRESS_MODULE_HPP
#define COMPRESS_MODULE_HPP

#include "../my_config.h"
#include "integers.hpp"

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

	    /// compress a block of data

	    /// \param[in] normal points to the first byte of the data block to compress
	    /// \param[in] normal_size is the number of bytes found at normal address to be compressed
	    /// \param[in] zip_buf is where to put the resulting compressed data
	    /// \param[in] zip_buf_size is the allocated bytes at zip_buf address
	    /// \return the number of bytes effectively used in zip_buf by the compressed data
	virtual U_32 compress_data(const char *normal,
				   const U_32 normal_size,
				   char *zip_buf,
				   U_32 zip_buf_size) = 0;

	    /// uncompress a block of data

	    /// \param[in] zip_buf points to the first byte of the compressed data block
	    /// \param[in] zip_buf_size is the number of bytes of the compressed dat starting at zip_buf
	    /// \param[in] normal is the place where to write the uncompressed data
	    /// \param[in] nomral_size is the allocated bytes starting at normal address
	    /// \return the number of bytes written at normal address that constitute the uncompressed data
	    /// \note it is expected that the implementation would throw an libdar::Edata() exception
	    /// on error, like corrupted compressed data submitted for decompression
	virtual U_32 uncompress_data(const char *zip_buf,
				     const U_32 zip_buf_size,
				     char *normal,
				     U_32 normal_size) = 0;

	    /// used to duplicate a inherited class of compress_module when it is pointed by a compress_module pointer
	virtual std::unique_ptr<compress_module> clone() const = 0;

    };

	/// @}

} // end of namespace

#endif
