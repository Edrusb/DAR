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

    /// \file lz4_module.hpp
    /// \brief per block encryption using LZ4 algorithm/library
    /// \ingroup Private
    ///
    /// we use libfar cryptography infrastructure
    /// (tronconneuse/parallel_tronconneuse/crypto_module) because
    /// it makes sense for a per block compression (suitable for
    /// multithreading) by opposition to streamed compression per
    /// saved file. Each file will be compressed by segment,
    /// resulting in non optimum compression ratio but speed gain.
    /// the larger the block size is, the better closer the
    /// compression ratio will be to the optimum

#ifndef LZ4_MODULE_HPP
#define LZ4_MODULE_HPP

extern "C"
{
#if HAVE_LZ4_H
#include <lz4.h>
#endif
}

#include "../my_config.h"
#include <string>
#include <memory>

#include "compress_module.hpp"
#include "infinint.hpp"
#include "integers.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class lz4_module: public compress_module
    {
    public:
	lz4_module(U_I compression_level);
	lz4_module(const lz4_module & ref);
	lz4_module(lz4_module && ref) noexcept;
	lz4_module & operator = (const lz4_module & ref);
	lz4_module & operator = (lz4_module && ref) noexcept;
	virtual ~lz4_module() noexcept;

	    // inherited from compress_module interface

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
	U_I acceleration;
	char *state;

    };
	/// @}

} // end of namespace

#endif
