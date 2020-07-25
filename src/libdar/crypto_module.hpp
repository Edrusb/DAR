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
#include <memory>

#include "infinint.hpp"
#include "integers.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class crypto_module
    {
    public:
	crypto_module() {};
	crypto_module(const crypto_module & ref) = default;
	crypto_module(crypto_module && ref) noexcept = default;
	crypto_module & operator = (const crypto_module & ref) = default;
	crypto_module & operator = (crypto_module && ref) noexcept = default;
	virtual ~crypto_module() noexcept = default;

		    /// defines the size necessary to encrypt a given amount of clear data

	    /// \param[in] clear_block_size is the size of the clear block to encrypt.
	    /// \return the size of the memory to allocate to receive corresponding encrypted data.
	    /// \note this implies that encryption algorithm must always generate a fixed size encrypted block of data for
	    /// a given fixed size block of data. However, the size of the encrypted block of data may differ from
	    /// the size of the clear block of data
	virtual U_32 encrypted_block_size_for(U_32 clear_block_size) = 0;

	    /// it may be necessary by the inherited class have few more bytes allocated after the clear data given for encryption

	    /// \param[in] clear_block_size is the size in byte of the clear data that will be asked to encrypt.
	    /// \return the requested allocated buffer size (at least the size of the clear data).
	    /// \note when giving clear buffer of data of size "clear_block_size" some inherited class may requested
	    /// that a bit more of data must be allocated.
	    /// this is to avoid copying data when the algorithm needs to add some data after the
	    /// clear data before encryption.
	virtual U_32 clear_block_allocated_size_for(U_32 clear_block_size) = 0;

	    /// this method encrypts the clear data given

	    /// \param block_num is the number of the block to which correspond the given data, This is an informational field for inherited classes.
	    /// \param[in] clear_buf points to the first byte of clear data to encrypt.
	    /// \param[in] clear_size is the length in byte of data to encrypt.
	    /// \param[in] clear_allocated is the size of the allocated memory (modifiable bytes) in clear_buf: clear_block_allocated_size_for(clear_size)
	    /// \param[in,out] crypt_buf is the area where to put corresponding encrypted data.
	    /// \param[in] crypt_size is the allocated memory size for crypt_buf: encrypted_block_size_for(clear_size)
	    /// \return is the amount of data put in crypt_buf (<= crypt_size).
	    /// \note it must respect that : returned value = encrypted_block_size_for(clear_size argument)
	virtual U_32 encrypt_data(const infinint & block_num,
				  const char *clear_buf,
				  const U_32 clear_size,
				  const U_32 clear_allocated,
				  char *crypt_buf,
				  U_32 crypt_size) = 0;

	    /// this method decyphers data

	    /// \param[in] block_num block number of the data to decrypt.
	    /// \param[in] crypt_buf pointer to the first byte of encrypted data.
	    /// \param[in] crypt_size size of encrypted data to decrypt.
	    /// \param[in,out] clear_buf pointer where to put clear data.
	    /// \param[in] clear_size allocated size of clear_buf.
	    /// \return is the amount of data put in clear_buf (<= clear_size)
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
