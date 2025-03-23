/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#if HAVE_ARGON2_H
#include <argon2.h>
#endif
}

#include "crypto_sym.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "elastic.hpp"

using namespace std;

namespace libdar
{

    constexpr const U_I MAX_RETRY_IF_WEAK_PASSWORD = 5;

    crypto_sym::crypto_sym(const secu_string & password,
			   const archive_version & reading_version,
			   crypto_algo xalgo,
			   const std::string & salt,
			   const infinint & iteration_count,
			   hash_algo kdf_hash,
			   bool use_pkcs5)
    {
#if CRYPTO_AVAILABLE
	U_I algo_id;
	S_I retry = use_pkcs5? MAX_RETRY_IF_WEAK_PASSWORD: 0;

	main_clef = nullptr;
	essiv_clef = nullptr;
	ivec = nullptr;

	try
	{
	    reading_ver = reading_version;
	    algo = xalgo;
	    algo_id = get_algo_id(algo);

	    if(reading_ver <= 5 && algo == crypto_algo::blowfish)
		throw Erange("crypto_sym::crypto_sym", gettext("Current implementation of blowfish encryption is not compatible with old (weak) implementation, use dar-2.3.x software or later (or other software based on libdar-4.4.x or greater) to read this archive"));

	    if(kdf_hash == hash_algo::none && use_pkcs5)
		throw Erange("crypto_sym::crypto_sym", gettext("cannot use 'none' as hashing algorithm for key derivation function"));


		// checking for algorithm availability in libgcrypt

	    gcry_error_t err = gcry_cipher_algo_info(algo_id, GCRYCTL_TEST_ALGO, nullptr, nullptr);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::crypto_sym",tools_printf(gettext("Cyphering algorithm not available in libgcrypt: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	    do
	    {
		    // generate a salt if not provided and pkcs5 is needed

		if(salt.empty() && use_pkcs5 && reading_version >= 10)
		    sel = generate_salt(max_key_len(xalgo));
		else
		    sel = salt;

		    // building the hashed_password

		init_hashed_password(password,
				     use_pkcs5,
				     sel,
				     iteration_count,
				     kdf_hash,
				     algo);

	    }
	    while(! is_a_strong_password(algo, hashed_password) && --retry >= 0);

	    if(retry < 0)
		throw Erange("crypto_sym::crypto_sym", tools_printf(gettext("Failed to obtain a strong hashed password after %d retries with pkcs5 and different salt values, aborting"), MAX_RETRY_IF_WEAK_PASSWORD));


		// building the main key for this object

	    init_main_clef(hashed_password,
			   algo);

		// building the Initial Vector structure that will be
		// used with the main key

	    init_algo_block_size(algo);
	    init_ivec(algo, algo_block_size);

	    U_I IV_cipher;
	    U_I IV_hashing;

	    get_IV_cipher_and_hashing(reading_ver, algo_id, IV_cipher, IV_hashing);

		// making an hash of the provided password into the digest variable
	    init_essiv_password(hashed_password, IV_hashing);

		// building the auxilliary key that will be used
		// to derive the IV value from the block number
	    init_essiv_clef(essiv_password, IV_cipher, algo_block_size);

#ifdef LIBDAR_NO_OPTIMIZATION
	    self_test();
#endif
	}
	catch(...)
	{
	    detruit();
	    throw;
	}
#else
	throw Ecompilation(gettext("Strong encryption support (libgcrypt)"));
#endif
    }

    U_32 crypto_sym::encrypted_block_size_for(U_32 clear_block_size)
    {
#if CRYPTO_AVAILABLE

	return ((clear_block_size / algo_block_size) + 1) * algo_block_size;
	    // round to the upper "algo_block_size" byte block of data.
	    // and add an additional "algo_block_size" block if no rounding is necessary.
	    // (we need some place to add the elastic buffer at the end of the block)

#else
	throw Ecompilation(gettext("Strong encryption support (libgcrypt)"));
#endif
    }

    U_32 crypto_sym::clear_block_allocated_size_for(U_32 clear_block_size)
    {
	return encrypted_block_size_for(clear_block_size);
    }

    U_32 crypto_sym::encrypt_data(const infinint & block_num,
				  const char *clear_buf,
				  const U_32 clear_size,
				  const U_32 clear_allocated,
				  char *crypt_buf,
				  U_32 crypt_size)
    {
#if CRYPTO_AVAILABLE
	U_32 size_to_fill = encrypted_block_size_for(clear_size);

	    // sanity checks
	    //
	if(crypt_size < size_to_fill)
	    throw SRC_BUG; // not enough room to write encrypted data
	if(clear_allocated < size_to_fill)  // note : clear_block_size_for() returns the same as encrypted_block_size_for()
	    throw SRC_BUG; // not large enough allocated memory in clear buffer to add padding
	    //
	    // end of sanity checks

	if(clear_size < size_to_fill)
	{
	    elastic stic = elastic(size_to_fill - clear_size);
	    gcry_error_t err;

	    stic.dump((unsigned char *)(const_cast<char *>(clear_buf + clear_size)), (U_32)(clear_allocated - clear_size));
	    err = gcry_cipher_reset(main_clef);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::encrypt_data",tools_printf(gettext("Error while resetting encryption key for a new block: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	    make_ivec(block_num, ivec, algo_block_size, essiv_clef);
	    err = gcry_cipher_setiv(main_clef, (const void *)ivec, algo_block_size);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::encrypt_data",tools_printf(gettext("Error while setting IV for current block: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	    err = gcry_cipher_encrypt(main_clef, (unsigned char *)crypt_buf, size_to_fill, (const unsigned char *)clear_buf, size_to_fill);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::encrypt_data",tools_printf(gettext("Error while cyphering data: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	    return size_to_fill;
	}
	else
	    throw SRC_BUG;
#else
	throw Ecompilation(gettext("Strong encryption support (libgcrypt)"));
#endif
    }

    U_32 crypto_sym::decrypt_data(const infinint & block_num,
				  const char *crypt_buf,
				  const U_32 crypt_size,
				  char *clear_buf,
				  U_32 clear_size)
    {
#if CRYPTO_AVAILABLE
	gcry_error_t err;

	if(crypt_size == 0)
	    return 0; // nothing to decipher

	make_ivec(block_num, ivec, algo_block_size, essiv_clef);
	err = gcry_cipher_setiv(main_clef, (const void *)ivec, algo_block_size);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::decrypt_data",tools_printf(gettext("Error while setting IV for current block: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	err = gcry_cipher_decrypt(main_clef, (unsigned char *)clear_buf, clear_size, (const unsigned char *)crypt_buf, crypt_size);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::decrypt_data",tools_printf(gettext("Error while decyphering data: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	elastic stoc = elastic((unsigned char *)clear_buf, crypt_size, elastic_backward, reading_ver);
	if(stoc.get_size() > crypt_size)
	    throw Erange("crypto_sym::decrypt_data",gettext("Data corruption may have occurred, cannot decrypt data"));
	return crypt_size - stoc.get_size();
	    // this is crypt_size to be used here as the clear data has the same length
	    // gcry_cipher_decrypt does not provide any mean to know the amount of clear bytes produced

#else
	throw Ecompilation(gettext("Strong encryption support (libgcrypt)"));
#endif
    }

    std::unique_ptr<crypto_module> crypto_sym::clone() const
    {
	try
	{
	    return std::make_unique<crypto_sym>(*this);
	}
	catch(bad_alloc &)
	{
	    throw Ememory("crypto_sym::clone");
	}
    }

    size_t crypto_sym::max_key_len(crypto_algo algo)
    {
#if CRYPTO_AVAILABLE
	size_t key_len;
	U_I algo_id = get_algo_id(algo);
	gcry_error_t err;

	    // checking for algorithm availability
	err = gcry_cipher_algo_info(algo_id, GCRYCTL_TEST_ALGO, nullptr, nullptr);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::max_key_len",tools_printf(gettext("Cyphering algorithm not available in libgcrypt: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	    // obtaining the maximum key length
	key_len = gcry_cipher_get_algo_keylen(algo_id);
	if(key_len == 0)
	    throw Erange("crypto_sym::max_key_len",gettext("Failed retrieving from libgcrypt the maximum key length"));

	return key_len;
#else
	throw Ecompilation("Strong encryption support (libgcrypt)");
#endif

    }

    size_t crypto_sym::max_key_len_libdar(crypto_algo algo)
    {
#if CRYPTO_AVAILABLE
	size_t key_len = max_key_len(algo);

	if(algo == crypto_algo::blowfish)
	    key_len = 56; // for historical reasons

	return key_len;
#else
	throw Ecompilation("Strong encryption support (libgcrypt)");
#endif
    }

    bool crypto_sym::is_a_strong_password(crypto_algo algo, const secu_string & password)
    {
#if CRYPTO_AVAILABLE
	bool ret = true;
	gcry_error_t err;
	gcry_cipher_hd_t main_clef;
	U_I algo_id = get_algo_id(algo);

	err = gcry_cipher_open(&main_clef, algo_id, GCRY_CIPHER_MODE_CBC, GCRY_CIPHER_SECURE);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::is_a_strong_password",tools_printf(gettext("Error while opening libgcrypt key handle to check password strength: %s/%s"),
									 gcry_strsource(err),
									 gcry_strerror(err)));

	try
	{
	    err = gcry_cipher_setkey(main_clef, (const void *)password.c_str(), password.get_size());
	    if(err != GPG_ERR_NO_ERROR)
	    {
		if(gcry_err_code(err) == GPG_ERR_WEAK_KEY)
		    ret = false;
		else
		    throw Erange("crypto_sym::is_a_strong_password",tools_printf(gettext("Error while assigning key to libgcrypt key handle to check password strength: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	    }
	}
	catch(...)
	{
	    gcry_cipher_close(main_clef);
	    throw;
	}
	gcry_cipher_close(main_clef);

	return ret;
#else
	throw Ecompilation("Strong encryption support (libgcrypt)");
#endif
    }

#if CRYPTO_AVAILABLE
    string crypto_sym::generate_salt(U_I size)
    {
	string ret;
	unsigned char* buffer = new (nothrow) unsigned char[size];

	if(buffer == nullptr)
	    throw Ememory("crypto_sym::generate_salt");

	try
	{
	    gcry_create_nonce(buffer, size);
	    ret.assign((const char *)buffer, size);
	    delete [] buffer;
	    buffer = nullptr;
	}
	catch(...)
	{
	    if(buffer != nullptr)
	    {
		delete [] buffer;
		buffer = nullptr;
	    }
	    throw;
	}

	return ret;
    }

    void crypto_sym::init_hashed_password(const secu_string & password,
					  bool use_pkcs5,
					  const std::string & salt,
					  infinint iteration_count,
					  hash_algo kdf_hash,
					  crypto_algo algo)
    {
	if(use_pkcs5)
	{
	    U_I it = 0;

	    iteration_count.unstack(it);
	    if(!iteration_count.is_zero())
		throw Erange("crypto_sym::init_hashed_password", gettext("Too large value give for key derivation interation count"));

	    switch(kdf_hash)
	    {
	    case hash_algo::none:
		throw SRC_BUG;
	    case hash_algo::md5:
	    case hash_algo::sha1:
	    case hash_algo::sha512:
		hashed_password = pkcs5_pass2key(password, salt, it, hash_algo_to_gcrypt_hash(kdf_hash), max_key_len_libdar(algo));
		break;
	    case hash_algo::argon2:
		hashed_password = argon2_pass2key(password, salt, it, max_key_len_libdar(algo));
		break;
	    default:
		throw SRC_BUG;
	    }
	}
	else
	    hashed_password = password;
    }

    void crypto_sym::init_essiv_password(const secu_string & key,
					 unsigned int IV_hashing)
    {
	U_I digest_len = gcry_md_get_algo_dlen(IV_hashing);

	if(digest_len == 0)
	    throw SRC_BUG;
	essiv_password.resize(digest_len);
	essiv_password.expand_string_size_to(digest_len);

	    // making a hash of the provided password into the digest variable

	gcry_md_hash_buffer(IV_hashing, essiv_password.get_array(), (const void *)key.c_str(), key.get_size());
    }

    void crypto_sym::init_main_clef(const secu_string & password,
				    crypto_algo algo)
    {
	try
	{
	    gcry_error_t err;


		// key handle initialization

	    err = gcry_cipher_open(&main_clef, get_algo_id(algo), GCRY_CIPHER_MODE_CBC, GCRY_CIPHER_SECURE);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::init_main_clef",tools_printf(gettext("Error while opening libgcrypt key handle: %s/%s"),
								   gcry_strsource(err),
								   gcry_strerror(err)));

		// assigning key to the handle

	    err = gcry_cipher_setkey(main_clef, (const void *)hashed_password.c_str(), hashed_password.get_size());
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::init_main_clef",tools_printf(gettext("Error while assigning key to libgcrypt key handle: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	}
	catch(...)
	{
	    detruit();
	    throw;
	};
    }

    void crypto_sym::init_essiv_clef(const secu_string & essiv_password,
				     U_I IV_cipher,
				     U_I main_cipher_algo_block_size)
    {
	gcry_error_t err;

	    // creating a handle for a new handle with algo equal to "IV_cipher"
	err = gcry_cipher_open(&essiv_clef,
			       IV_cipher,
			       GCRY_CIPHER_MODE_ECB,
			       GCRY_CIPHER_SECURE);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::init_essiv_clef",tools_printf(gettext("Error while creating ESSIV handle: %s/%s"), gcry_strsource(err),gcry_strerror(err)));


	    // obtaining key len for IV_cipher

	size_t essiv_key_len;
	err = gcry_cipher_algo_info(IV_cipher, GCRYCTL_GET_KEYLEN, nullptr, &essiv_key_len);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::init_essiv_clef", tools_printf(gettext("Error while setting IV for current block: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	    // failing if the digest size is larger than key size for IV_cipher

	if(essiv_password.get_size() > essiv_key_len
	   && IV_cipher != GCRY_CIPHER_BLOWFISH)
	    throw SRC_BUG;
	    // IV_cipher and IV_hashing must be chosen in coherence!
	    // we do not complain for older format archive... its done now and worked so far.
	    // This bug report is rather to signal possible problem when time will come
	    // to update the cipher and digest algorithms

	    // assiging the essiv_password to the new handle

	err = gcry_cipher_setkey(essiv_clef,
				 essiv_password.c_str(),
				 essiv_password.get_size());
	if(err != GPG_ERR_NO_ERROR)
	{
		// tolerating WEAK key here, we use that key to fill the IV of the real strong key
		// (the encrypted data by this key --- the IV values for each block --- are not stored in the archive)
	    if(gpg_err_code(err) != GPG_ERR_WEAK_KEY)
		throw Erange("crypto_sym::init_essiv_clef",tools_printf(gettext("Error while assigning key to libgcrypt key handle (essiv): %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	}

	    // obtaining the block size of the essiv cipher

	size_t algo_block_size_essiv;

	err = gcry_cipher_algo_info(IV_cipher, GCRYCTL_GET_BLKLEN, nullptr, &algo_block_size_essiv);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::init_essiv_clef",tools_printf(gettext("Failed retrieving from libgcrypt the block size used by the cyphering algorithm (essiv): %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	if(algo_block_size_essiv == 0)
	    throw SRC_BUG;

	if(main_cipher_algo_block_size == 0)
	    throw SRC_BUG;

	    // testing the coherence of block size between main cipher key
	    // and cipher key used to generate IV for the main cipher key

	if(main_cipher_algo_block_size < algo_block_size_essiv)
	    throw SRC_BUG; // cannot cipher less data (IV for main key) than block size of IV key
	else
	    if(main_cipher_algo_block_size % algo_block_size_essiv != 0)
		throw SRC_BUG;
	    // in ECB mode we should encrypt an integer number of blocks according
	    // to IV key block size
    }

    void crypto_sym::init_algo_block_size(crypto_algo algo)
    {
	gcry_error_t err;

	    // obtaining the block length

	err = gcry_cipher_algo_info(get_algo_id(algo), GCRYCTL_GET_BLKLEN, nullptr, &algo_block_size);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::init_algo_block_size",tools_printf(gettext("Failed retrieving from libgcrypt the block size used by the cyphering algorithm: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	if(algo_block_size == 0)
	    throw SRC_BUG;
    }

    void crypto_sym::init_ivec(crypto_algo algo, size_t algo_block_size)
    {

	    // initializing ivec in secure memory

	ivec = (unsigned char *)gcry_malloc_secure(algo_block_size);
	if(ivec == nullptr)
	    throw Esecu_memory("crypto_sym::init_ivec");
    }

    void crypto_sym::detruit()
    {
	if(main_clef != nullptr)
	    gcry_cipher_close(main_clef);
	if(essiv_clef != nullptr)
	    gcry_cipher_close(essiv_clef);
	if(ivec != nullptr)
	{
	    (void)memset(ivec, 0, algo_block_size);
	    gcry_free(ivec);
	}
    }

    void crypto_sym::copy_from(const crypto_sym & ref)
    {
	reading_ver = ref.reading_ver;
	algo = ref.algo;
	hashed_password = ref.hashed_password;
	essiv_password = ref.essiv_password;
	init_main_clef(hashed_password, algo);
	init_algo_block_size(algo);
	init_ivec(algo, algo_block_size);
	U_I IV_cipher;
	U_I IV_hashing;
	get_IV_cipher_and_hashing(reading_ver, get_algo_id(algo), IV_cipher, IV_hashing);
	init_essiv_clef(essiv_password, IV_cipher, algo_block_size);
	sel = ref.sel;
    }

    void crypto_sym::move_from(crypto_sym && ref)
    {
	    // we assume the current object has no field assigned
	    // same as for copy_from():
	reading_ver = std::move(ref.reading_ver);
	algo = std::move(ref.algo);
	hashed_password = std::move(ref.hashed_password);
	essiv_password = std::move(ref.essiv_password);
	main_clef = std::move(ref.main_clef);
	essiv_clef = std::move(ref.essiv_clef);
	algo_block_size = std::move(ref.algo_block_size);
	ivec = std::move(ref.ivec);
	sel = std::move(ref.sel);
	ref.main_clef = nullptr;
	ref.essiv_clef = nullptr;
	ivec = nullptr;
    }

    void crypto_sym::get_IV_cipher_and_hashing(const archive_version & ver, U_I main_cipher, U_I & cipher, U_I & hashing)
    {
	if(ver >= archive_version(8,1)
	   && main_cipher != get_algo_id(crypto_algo::blowfish))
	{
	    cipher = GCRY_CIPHER_AES256; // to have an algorithm available when ligcrypt is used in FIPS mode
	    hashing = GCRY_MD_SHA256; // SHA224 was also ok but as time passes, it would get sooner unsave
	}
	else
	{
	    // due to 8 bytes block_size we keep using
	    // blowfish for the essiv key: other cipher have 16 bytes block size
	    // and generating an IV of eight bytes would require doubling the size
	    // of the data to encrypt and skinking the encrypted data afterward to
	    // the 8 requested bytes of the IV for the main key...
	    //
	    // in the other side, the replacement of blowfish by aes256 starting format 8.1
	    // was requested to have libdar working when libgcrypt is in FIPS mode, which
	    // forbids the use of blowfish... we stay here compatible with FIPS mode.

	    cipher = GCRY_CIPHER_BLOWFISH;
	    hashing = GCRY_MD_SHA1;
	}
    }

    void crypto_sym::make_ivec(const infinint & ref, unsigned char *ivec, U_I size, const gcry_cipher_hd_t & IVkey)
    {

	    // Stronger IV calculation: ESSIV.
	    // ESSIV mode helps to provide (at least) IND-CPA security.

	unsigned char *sect = nullptr;
	infinint ref_cp = ref;
	infinint tmp;
	gcry_error_t err;

	sect = new (nothrow) unsigned char[size];
	if(sect == nullptr)
	    throw Ememory("crypto_sym::make_ivec");

	try
	{
	    U_I i = size;

	    while(i > 0)
	    {
		--i;
		sect[i] = ref_cp[0];
		ref_cp >>= 8;
	    }

		// IV(sector) = E_salt(sector)
	    err = gcry_cipher_encrypt(IVkey, (unsigned char *)ivec, size, (const unsigned char *)sect, size);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::make_ivec",tools_printf(gettext("Error while generating IV: %s/%s"), gcry_strsource(err), gcry_strerror(err)));
	}
	catch(...)
	{
	    delete [] sect;
	    throw;
	}
	delete [] sect;
    }

    secu_string crypto_sym::pkcs5_pass2key(const secu_string & password,
					   const string & salt,
					   U_I iteration_count,
					   U_I hash_gcrypt,
					   U_I output_length)
    {
	    // Password-based key derivation function (PBKDF2) from PKCS#5 v2.0
	    // Using HMAC-SHA1 as the underlying pseudorandom function.

	gcry_error_t err;
	gcry_md_hd_t hmac;
	U_32 l = 0, r = 0;
	secu_string retval;

	if (output_length == 0)
	    return secu_string();

	    // Let l be the number of EVP_MD_size(digest) blocks in the derived key, rounding up.
	    // Let r be the number of octets in the last block.
	l = output_length / gcry_md_get_algo_dlen(hash_gcrypt);
	r = output_length % gcry_md_get_algo_dlen(hash_gcrypt);
	if (r == 0)
	    r = gcry_md_get_algo_dlen(hash_gcrypt);
	else
	    ++l;    // round up

	    // testing SHA1 availability

	err = gcry_md_test_algo(hash_gcrypt);
	if(err != GPG_ERR_NO_ERROR)
	    throw Ecompilation(tools_printf(gettext("Error! SHA1 not available in libgcrypt: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	    // opening a handle for Message Digest

	err = gcry_md_open(&hmac, hash_gcrypt, GCRY_MD_FLAG_SECURE|GCRY_MD_FLAG_HMAC);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::pkcs5_pass2key",tools_printf(gettext("Error while derivating key from password (HMAC open): %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	    // setting the HMAC key

	err = gcry_md_setkey(hmac, password.c_str(), password.get_size());
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::pkcs5_pass2key",tools_printf(gettext("Error while derivating key from password (HMAC set key): %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	    // now ready to compute HMAC-SHA1 message digest using "hmac"

	try
	{
	    U_I UjLen = gcry_md_get_algo_dlen(hash_gcrypt);
	    char *Ti = nullptr, *Uj = nullptr;

	    retval.resize(output_length);
	    Ti = (char *)gcry_malloc_secure(gcry_md_get_algo_dlen(hash_gcrypt));
	    if(Ti == nullptr)
		throw Ememory("crypto_sym::pkcs5_pass2key");
	    try
	    {
		Uj = (char *)gcry_malloc_secure(gcry_md_get_algo_dlen(hash_gcrypt));
		if(Uj == nullptr)
		    throw Ememory("crypto_sym::pkcs5_pass2key");
		try
		{
		    for (U_32 i = 1; i <= l; ++i)
		    {
			    // Ti = U_1 \xor U_2 \xor ... \xor U_c
			    // U_1 = PRF(P, S || INT(i))
			unsigned char ii[4];
			unsigned char *tmp_md = nullptr;

			ii[0] = (i >> 24) & 0xff;
			ii[1] = (i >> 16) & 0xff;
			ii[2] = (i >> 8) & 0xff;
			ii[3] = i & 0xff;

			gcry_md_reset(hmac);
			gcry_md_write(hmac, (const unsigned char *) salt.c_str(), salt.size());
			gcry_md_write(hmac, ii, 4);
			tmp_md = gcry_md_read(hmac, hash_gcrypt);
			(void)memcpy(Uj, tmp_md, gcry_md_get_algo_dlen(hash_gcrypt));
			(void)memcpy(Ti, tmp_md, gcry_md_get_algo_dlen(hash_gcrypt));

			for (U_32 j = 2; j <= iteration_count; ++j)
			{
				// U_j = PRF(P, U_{j-1})

			    gcry_md_reset(hmac);
			    gcry_md_write(hmac, (const unsigned char *) Uj, UjLen);
			    tmp_md = gcry_md_read(hmac, hash_gcrypt);
			    (void)memcpy(Uj, tmp_md, gcry_md_get_algo_dlen(hash_gcrypt));
			    tools_memxor(Ti, tmp_md, gcry_md_get_algo_dlen(hash_gcrypt));
			}

			if (i < l)
			    retval.append(Ti, gcry_md_get_algo_dlen(hash_gcrypt));
			else 	// last block
			    retval.append(Ti, r);

		    } // end of main for() loop
		}
		catch(...)
		{
		    (void)memset(Uj, 0, gcry_md_get_algo_dlen(hash_gcrypt));
		    gcry_free(Uj);
		    throw;
		}
		(void)memset(Uj, 0, gcry_md_get_algo_dlen(hash_gcrypt));
		gcry_free(Uj);
	    }
	    catch(...)
	    {
		(void)memset(Ti, 0, gcry_md_get_algo_dlen(hash_gcrypt));
		gcry_free(Ti);
		throw;
	    }
	    (void)memset(Ti, 0, gcry_md_get_algo_dlen(hash_gcrypt));
	    gcry_free(Ti);
	}
	catch(...)
	{
	    gcry_md_close(hmac);
	    throw;
	}
	gcry_md_close(hmac);

	return retval;
    }

    U_I crypto_sym::get_algo_id(crypto_algo algo)
    {
	U_I algo_id;

	switch(algo)
	{
	case crypto_algo::blowfish:
	    algo_id = GCRY_CIPHER_BLOWFISH;
	    break;
	case crypto_algo::aes256:
	    algo_id = GCRY_CIPHER_AES256;
	    break;
	case crypto_algo::twofish256:
	    algo_id = GCRY_CIPHER_TWOFISH;
	    break;
	case crypto_algo::serpent256:
	    algo_id = GCRY_CIPHER_SERPENT256;
	    break;
	case crypto_algo::camellia256:
	    algo_id = GCRY_CIPHER_CAMELLIA256;
	    break;
	default:
	    throw SRC_BUG;
	}

	return algo_id;
    }

    secu_string crypto_sym::argon2_pass2key(const secu_string & password,
					    const std::string & salt,
					    U_I iteration_count,
					    U_I output_length)
    {
#if LIBARGON2_AVAILABLE
	secu_string ret(output_length);
	S_I err = argon2id_hash_raw(iteration_count,
				    100, // 100 kiB memory used at max
				    1,   // assuming less parallelization leads to longer hash time and thus more difficult dictionnary attack (?)
				    password.c_str(), password.get_size(),
				    salt.c_str(), salt.size(),
				    ret.c_str(), ret.get_allocated_size());

	if(err != ARGON2_OK)
	{
	    throw Erange("crypto_sym::argon2_pas2key",
			 tools_printf(gettext("Error while computing KDF with argon2 algorithm: %d"),
				      err));
	}
	else
	    ret.set_size(output_length);

	return ret;
#else
	throw Efeature("libargon2");
#endif
    }


#ifdef LIBDAR_NO_OPTIMIZATION

    bool crypto_sym::self_tested = false;

    void crypto_sym::self_test(void)
    {
	if(self_tested)
	    return;
	else
	    self_tested = true;

	    //
	    // Test PBKDF2 (test vectors are from RFC 3962.)
	    //
	secu_string result;
	string p1 = "password";
	string p2 = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"; // 64 characters
	string p4 = p2 + "X"; // 65 characters
	secu_string pass = secu_string(100);


	pass.append(p1.c_str(), (U_I)p1.size());
	result = pkcs5_pass2key(pass, "ATHENA.MIT.EDUraeburn", 1, GCRY_MD_SHA1, 16);
	if (result != string("\xcd\xed\xb5\x28\x1b\xb2\xf8\x01\x56\x5a\x11\x22\xb2\x56\x35\x15", 16))
	    throw Erange("crypto_sym::self_test", gettext("Library used for blowfish encryption does not respect RFC 3962"));

	result = pkcs5_pass2key(pass, "ATHENA.MIT.EDUraeburn", 1200, GCRY_MD_SHA1, 32);
	if (result !=  string("\x5c\x08\xeb\x61\xfd\xf7\x1e\x4e\x4e\xc3\xcf\x6b\xa1\xf5\x51\x2b"
			      "\xa7\xe5\x2d\xdb\xc5\xe5\x14\x2f\x70\x8a\x31\xe2\xe6\x2b\x1e\x13", 32)
	    )
	    throw Erange("crypto_sym::self_test", gettext("Library used for blowfish encryption does not respect RFC 3962"));

	pass.clear();
	pass.append(p2.c_str(), (U_I)p2.size());

	result = pkcs5_pass2key(pass,
				"pass phrase equals block size", 1200, GCRY_MD_SHA1, 32);
	if (result != string("\x13\x9c\x30\xc0\x96\x6b\xc3\x2b\xa5\x5f\xdb\xf2\x12\x53\x0a\xc9"
			     "\xc5\xec\x59\xf1\xa4\x52\xf5\xcc\x9a\xd9\x40\xfe\xa0\x59\x8e\xd1", 32))
	    throw Erange("crypto_sym::self_test", gettext("Library used for blowfish encryption does not respect RFC 3962"));

	pass.resize((U_I)p4.size());
	pass.append(p4.c_str(), (U_I)p4.size());
	result = pkcs5_pass2key(pass,
				"pass phrase exceeds block size", 1200, GCRY_MD_SHA1, 32);
	if (result != string("\x9c\xca\xd6\xd4\x68\x77\x0c\xd5\x1b\x10\xe6\xa6\x87\x21\xbe\x61"
			     "\x1a\x8b\x4d\x28\x26\x01\xdb\x3b\x36\xbe\x92\x46\x91\x5e\xc8\x2a", 32))
	    throw Erange("crypto_sym::self_test", gettext("Library used for blowfish encryption does not respect RFC 3962"));

	    //
	    // Test make_ivec
	    //
	struct
	{
	    unsigned int sector;
	    char iv[9];
	} tests[] = { { 0, "\x79\xbf\x81\x22\x26\xe4\x13\x6f" },
		      { 7, "\x61\x03\xd1\x20\x8a\x0d\x22\x2d" },
		      { 8, "\xc9\x61\xce\x29\x2e\x65\x28\xbe" },
		      { 0x10000007, "\x37\xe9\xc0\x92\xc3\x55\xfb\x4b" },
		      { 0xa5a55a5a, "\x08\x7f\x1a\xa9\xec\x4a\xc0\xc5" },
		      { 0xffffffff, "\x7a\x8f\x9c\xd0\xcb\xcc\x56\xec" },
		      { 0xdeadbeef, "" } };
	char ivec[8];
	int i;
	string p3 = string("\0\0\0\0", 4);
	pass.clear();
	pass.append(p3.c_str(), (U_I)p3.size());

	    // temporary object used to build the key to
	    // generate IV
	crypto_sym tester(pass,
			  6,
			  crypto_algo::blowfish,
			  "",
			  0,
			  hash_algo::md5,
			  false);

	    // the following is ugly, we make a reference
	    // in the local block to the field of the tester
	    // object. We can do that because self_test()
	    // is a member (yet a static one) of the crypto_sym
	    // class.
	    // this is done to reduce change in this testing
	    // routine that was getting a gcry_cipher_hd_t key
	    // from another static method of class crpto_sym
	    // in the past. Since then this routine has been
	    // moved and split in several non static methods
	gcry_cipher_hd_t & esivkey = tester.essiv_clef;

	for (i = 0; tests[i].sector != 0xdeadbeef; ++i)
	{
	    make_ivec(tests[i].sector, (unsigned char *) ivec, 8, esivkey);
	    if (memcmp(ivec, tests[i].iv, 8) != 0)
		    //cerr << "sector = " << tests[i].sector << endl;
		    //cerr << "ivec = @@" << string(ivec, 8) << "@@" << endl;
		    //cerr << "should be @@" << string(tests[i].iv, 8) << "@@" << endl;
		throw Erange("crypto_sym::self_test", gettext("Library used for blowfish encryption does not respect RFC 3962"));
	}
    }
#endif

#endif

} // end of namespace
