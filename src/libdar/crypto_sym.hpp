//*********************************************************************/
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

    /// \file crypto_sym.hpp
    /// \brief class crypto_sym for symetrical cipherings
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

#include "crypto_module.hpp"
#include "secu_string.hpp"
#include "crypto.hpp"
#include "archive_aux.hpp"
#include "archive_version.hpp"


namespace libdar
{

	/// \addtogroup Private
	/// @{

    inline bool crypto_min_ver_libgcrypt_no_bug()
    {
#if CRYPTO_AVAILABLE
	return gcry_check_version(MIN_VERSION_GCRYPT_HASH_BUG);
#else
	return true;
#endif
    }

	/// symetrical strong encryption, interface to grypt library

    class crypto_sym : public crypto_module
    {
    public:
	crypto_sym(const secu_string & password,
		   const archive_version & reading_ver,
		   crypto_algo algo,
		   const std::string & salt, //< not used is use_pkcs5 below is not set
		   const infinint & iteration_count, //< not used if use_pkcs5 is not set
		   hash_algo kdf_hash,       //< not used if use_pkcs5 is not set
		   bool use_pkcs5);          //< must be set to true when password is human defined to add a key derivation
	crypto_sym(const crypto_sym & ref) { copy_from(ref); };
	crypto_sym(crypto_sym && ref) noexcept { try { move_from(std::move(ref)); } catch(...) {} };
	crypto_sym & operator = (const crypto_sym & ref) { detruit(); copy_from(ref); return *this; };
	crypto_sym & operator = (crypto_sym && ref) noexcept { try { detruit(); move_from(std::move(ref)); } catch(...) {} return *this; };
	virtual ~crypto_sym() noexcept { try { detruit(); } catch(...) {} };

	    // inherited from class crypto_module

	virtual U_32 encrypted_block_size_for(U_32 clear_block_size) override;
	virtual U_32 clear_block_allocated_size_for(U_32 clear_block_size) override;
	virtual U_32 encrypt_data(const infinint & block_num,
				  const char *clear_buf,
				  const U_32 clear_size,
				  const U_32 clear_allocated,
				  char *crypt_buf, U_32 crypt_size) override;
	virtual U_32 decrypt_data(const infinint & block_num,
				  const char *crypt_buf,
				  const U_32 crypt_size,
				  char *clear_buf,
				  U_32 clear_size) override;
	virtual std::unique_ptr<crypto_module> clone() const override { return std::make_unique<crypto_sym>(*this); };


	    /// returns the max key length in octets for the given algorithm
	static size_t max_key_len(crypto_algo algo);

	    /// returns the max key length in octets to use to compute a key from a user provided password
	static size_t max_key_len_libdar(crypto_algo algo);

	    /// check whether the given password is reported as strong in regard to the given cipher
	static bool is_a_strong_password(crypto_algo algo, const secu_string & password);

	    /// generates a random salt of given size
	static std::string generate_salt(U_I size);

    private:

#if CRYPTO_AVAILABLE
	archive_version reading_ver;    ///< the currently followed archive format
	crypto_algo algo;               ///< algo ID in libgcrypt
	secu_string hashed_password;    ///< pkcs5 hashed password or provided password if pkcs5 is not needed
	secu_string essiv_password;     ///< password for essiv
	gcry_cipher_hd_t main_clef;  ///< used to encrypt/decrypt the data
	gcry_cipher_hd_t essiv_clef; ///< used to build the Initialization Vector
	size_t algo_block_size;      ///< the block size of the algorithm (main key)
	unsigned char *ivec;         ///< algo_block_size allocated in secure memory to be used as Initial Vector for main_clef

	void init_hashed_password(const secu_string & password,
				  bool use_pkcs5,
				  const std::string & salt,
				  infinint iteration_count,
				  hash_algo kdf_hash,
				  crypto_algo algo);
	void init_essiv_password(const secu_string & key,
				 unsigned int IV_hashing);
	void init_main_clef(const secu_string & password, ///< key password
			    crypto_algo algo              ///< only use when use_pkcs5
	    );
	void init_essiv_clef(const secu_string & essiv_password,
			     U_I IV_cipher,
			     U_I main_cipher_algo_block_size);
	void init_algo_block_size(crypto_algo algo);
	void init_ivec(crypto_algo algo, size_t algo_block_size);

	void detruit();
	void copy_from(const crypto_sym & ref);
	void move_from(crypto_sym && ref);

	    /// creates a blowfish key using as key a SHA1 of the given string (no IV assigned)

	    /// \note such key is intended to be used to generate IV for the main key

	static void get_IV_cipher_and_hashing(const archive_version & ver, U_I main_cipher, U_I & cipher, U_I & hashing);

	    /// Fills up a new initial vector based on a reference and and a encryption key

	    /// \param[in] ref is the reference to base the IV on
	    /// \param[in] ivec is the address where to drop down the new IV
	    /// \param[in] size is the amount of data allocated at ivec address
	    /// \param[in] IVkey is the key used to generate the IV data
	    /// \note the IV is created by encrypting a hash of ref with IVkey
	static void make_ivec(const infinint & ref,
			      unsigned char *ivec,
			      U_I size,
			      const gcry_cipher_hd_t & IVkey);

	    /// Create a hash string of requested length from a given password and interation cout
	static secu_string pkcs5_pass2key(const secu_string & password,         ///< human provided password
					  const std::string & salt,             ///< salt string
					  U_I iteration_count,                  ///< number of time to shake the melange
					  U_I hash_gcrypt,                      ///< hashing fonction used for key derivation (SHA1 historically)
					  U_I output_length);                   ///< length of the string to return

	    /// converts libdar crypto algo designation to index used by libgcrypt
	static U_I get_algo_id(crypto_algo algo);

#ifdef LIBDAR_NO_OPTIMIZATION
	static bool self_tested;
	static void self_test(void);
#endif

#else
	void detruit() { throw Ecompilation(gettext("Strong encryption support (libgcrypt)")); };
	void copy_from(const crypto_sym & ref) { throw Ecompilation(gettext("Strong encryption support (libgcrypt)")); };
	void move_from(crypto_sym && ref) { throw Ecompilation(gettext("Strong encryption support (libgcrypt)")); };
#endif
    };

	/// @}

} // end of namespace

#endif
