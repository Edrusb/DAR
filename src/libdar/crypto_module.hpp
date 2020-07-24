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

    /// \file crypto_module.hpp
    /// \brief per block cryptography implementation
    /// \ingroup Private
    ///
    /// used for strong encryption.

#ifndef CRYPTO_MODULE_HPP
#define CRYPTO_MODULE_HPP

#include "../my_config.h"
#include <string>

#include "infinint.hpp"
#include "integers.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class crypto_module
    {
    public:
	crypto_module();
	crypto_module(const crypto_module & ref) = default;
	crypto_module(crypto_module && ref) noexcept = default;
	crypto_module & operator = (const crypto_module & ref) = default;
	crypto_module & operator = (crypto_module && ref) noexcept = default;
	virtual ~crypto_module() = default;

	virtual U_32 encrypted_block_size_for(U_32 clear_block_size) = 0;
	virtual U_32 clear_block_allocated_size_for(U_32 clear_block_size) = 0;
	virtual U_32 encrypt_data(const infinint & block_num,
				  const char *clear_buf,
				  const U_32 clear_size,
				  const U_32 clear_allocated,
				  char *crypt_buf, U_32 crypt_size) = 0;
	virtual U_32 decrypt_data(const infinint & block_num,
				  const char *crypt_buf,
				  const U_32 crypt_size,
				  char *clear_buf,
				  U_32 clear_size) = 0;

	virtual std::unique_ptr<crypto_module> clone() const = 0;
    };
	/// @}

} // end of namespace

#endif
