//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2052 Denis Corbin
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file crypto.hpp
    /// \brief the crypto algoritm definition
    /// \ingroup Private

#ifndef CRYPTO_SYM_HPP
#define CRYPTO_SYM_HPP

extern "C"
{
#if HAVE_GCRYPT_H
#ifndef GCRYPT_NO_DEPRECATED
#define GCRYPT_NO_DEPRECATED
#endif
#include <gcrypt.h>
#endif
}

#include "../my_config.h"
#include <string>

#include "tronconneuse.hpp"
#include "secu_string.hpp"
#include "crypto.hpp"

namespace libdar
{

	/// \ingroup Private
	/// @}

    inline bool crypto_min_ver_libgcrypt_no_bug()
    {
#if CRYPTO_AVAILABLE
	return gcry_check_version(MIN_VERSION_GCRYPT_HASH_BUG);
#else
	return true;
#endif
    }

	/// inherited class from tronconneuse class
	/// \ingroup Private
    class crypto_sym : public tronconneuse
    {
    public:
	crypto_sym(U_32 block_size,
		   const secu_string & password,
		   generic_file & encrypted_side,
		   bool no_initial_shift,
		   const archive_version & reading_ver,
		   crypto_algo algo);
	~crypto_sym() { detruit(); };

    protected:
	U_32 encrypted_block_size_for(U_32 clear_block_size);
	U_32 clear_block_allocated_size_for(U_32 clear_block_size);
	U_32 encrypt_data(const infinint & block_num,
			  const char *clear_buf, const U_32 clear_size, const U_32 clear_allocated,
			  char *crypt_buf, U_32 crypt_size);
	U_32 decrypt_data(const infinint & block_num,
			  const char *crypt_buf, const U_32 crypt_size,
			  char *clear_buf, U_32 clear_size);

    private:
#if CRYPTO_AVAILABLE
	gcry_cipher_hd_t clef;       //< used to encrypt/decrypt the data
	gcry_cipher_hd_t essiv_clef; //< used to build the Initialization Vector
#endif
	size_t algo_block_size;         //< the block size of the algorithm
	unsigned char *ivec;            //< algo_block_size allocated in secure memory to be used as Initial Vector
	U_I algo_id;                    //< algo ID in libgcrypt
	archive_version reading_version;

	secu_string pkcs5_pass2key(const secu_string & password,         //< human provided password
				   const std::string & salt,             //< salt string
				   U_I iteration_count,                  //< number of time to shake the melange
				   U_I output_length);                   //< length of the string to return
	void dar_set_essiv(const secu_string & key);                     //< assign essiv from the given (hash) string
	void make_ivec(const infinint & ref, unsigned char *ivec, U_I size);
	void self_test(void);
	void detruit();
    };

	/// @}

} // end of namespace

#endif
