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

#include "infinint.hpp"
#include "integers.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class lz4_module: public crypto_module
    {
    public:
	lz4_module(U_I compression_level, U_I block_size) {};
	lz4_module(const lz4_module & ref) = default;
	lz4_module(lz4_module && ref) noexcept = default;
	lz4_module & operator = (const lz4_module & ref) = default;
	lz4_module & operator = (lz4_module && ref) noexcept = default;
	virtual ~lz4_module() noexcept = default;

	virtual U_32 encrypted_block_size_for(U_32 clear_block_size) override { return LZ4_compressBound(clear_block_size); };
	virtual U_32 clear_block_allocated_size_for(U_32 clear_block_size) override { return clear_block_size; };

	virtual U_32 encrypt_data(const infinint & block_num,
				  const char *clear_buf,
				  const U_32 clear_size,
				  const U_32 clear_allocated,
				  char *crypt_buf,
				  U_32 crypt_size) override;

	virtual U_32 decrypt_data(const infinint & block_num,
				  const char *crypt_buf,
				  const U_32 crypt_size,
				  char *clear_buf,
				  U_32 clear_size) override;


	virtual std::unique_ptr<crypto_module> clone() const override { return std::make_unique<lz4_module>(*this); };
    };
	/// @}

} // end of namespace

#endif
