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

    /// \file gzip_module.hpp
    /// \brief per block encryption using gzip algorithm/library
    /// \ingroup Private
    ///

#ifndef GZIP_MODULE_HPP
#define GZIP_MODULE_HPP

extern "C"
{
}

#include "../my_config.h"

#include "compress_module.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class gzip_module: public compress_module
    {
    public:
	gzip_module(U_I compression_level = 9);
	gzip_module(const gzip_module & ref) = default;
	gzip_module(gzip_module && ref) noexcept = default;
	gzip_module & operator = (const gzip_module & ref) = default;
	gzip_module & operator = (gzip_module && ref) noexcept = default;
	virtual ~gzip_module() noexcept = default;

	    // inherited from compress_module interface

	virtual compression get_algo() const override { return compression::gzip; };

	virtual U_I get_max_compressing_size() const override;

	virtual U_I get_min_size_to_compress(U_I clear_size) const override;

	virtual U_I compress_data(const char *normal,
				   const U_I normal_size,
				   char *zip_buf,
				   U_I zip_buf_size) const override;

	virtual U_I uncompress_data(const char *zip_buf,
				     const U_I zip_buf_size,
				     char *normal,
				     U_I normal_size) const override;


	virtual std::unique_ptr<compress_module> clone() const override;

    private:
	U_I level;

    };
	/// @}

} // end of namespace

#endif
