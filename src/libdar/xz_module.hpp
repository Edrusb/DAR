/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

    /// \file xz_module.hpp
    /// \brief per block encryption using xz algorithm/library
    /// \ingroup Private
    ///

#ifndef XZ_MODULE_HPP
#define XZ_MODULE_HPP

extern "C"
{
#if HAVE_LZMA_H
#include <lzma.h>
#endif
}

#include "../my_config.h"

#include "compress_module.hpp"
#include "infinint.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class xz_module: public compress_module
    {
    public:
	xz_module(U_I compression_level = 9);
	xz_module(const xz_module & ref) { setup(ref.level); };
	xz_module(xz_module && ref) noexcept = default;
	xz_module & operator = (const xz_module & ref) { end_process(); setup(ref.level); return *this; };
	xz_module & operator = (xz_module && ref) noexcept = default;
	virtual ~xz_module() { end_process(); };

	    // inherited from compress_module interface

	virtual compression get_algo() const override { return compression::xz; };

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
#if LIBLZMA_AVAILABLE
	mutable lzma_stream lzma_str;
#endif

	void setup(U_I compression_level);
	void init_decompr() const;
	void init_compr() const;
	void end_process() const;
    };

	/// @}

} // end of namespace

#endif
