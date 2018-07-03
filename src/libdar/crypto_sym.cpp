/*********************************************************************/
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

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
    char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif
}

#include "crypto_sym.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "elastic.hpp"

using namespace std;

namespace libdar
{

    crypto_sym::crypto_sym(U_32 block_size,
			   const secu_string & password,
			   generic_file & encrypted_side,
			   bool no_initial_shift,
			   const archive_version & reading_ver,
			   crypto_algo algo,
			   bool use_pkcs5)
	: tronconneuse(block_size, encrypted_side, no_initial_shift, reading_ver)
    {
#if CRYPTO_AVAILABLE
	ivec = nullptr;
	clef = nullptr;
	essiv_clef = nullptr;

	if(reading_ver <= 5)
	    throw Erange("crypto_sym::blowfish", gettext("Current implementation of blowfish encryption is not compatible with old (weak) implementation, use dar-2.3.x software or later (or other software based on libdar-4.4.x or greater) to read this archive"));
	else
	{
	    secu_string hashed_password;
	    gcry_error_t err;

	    algo_id = get_algo_id(algo);

		// checking for algorithm availability
	    err = gcry_cipher_algo_info(algo_id, GCRYCTL_TEST_ALGO, nullptr, nullptr);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::crypto_sym",tools_printf(gettext("Cyphering algorithm not available in libgcrypt: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

		// obtaining the block length

	    err = gcry_cipher_algo_info(algo_id, GCRYCTL_GET_BLKLEN, nullptr, &algo_block_size);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::crypto_sym",tools_printf(gettext("Failed retrieving from libgcrypt the block size used by the cyphering algorithm: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	    if(algo_block_size == 0)
		throw SRC_BUG;

		// initializing ivec in secure memory
	    ivec = (unsigned char *)gcry_malloc_secure(algo_block_size);
	    if(ivec == nullptr)
		throw Esecu_memory("crypto_sym::crypto_sym");

	    try
	    {
		hashed_password = use_pkcs5 ? pkcs5_pass2key(password, "", 2000, max_key_len_libdar(algo)) : password;

		    // key handle initialization

		err = gcry_cipher_open(&clef, algo_id, GCRY_CIPHER_MODE_CBC, GCRY_CIPHER_SECURE);
		if(err != GPG_ERR_NO_ERROR)
		    throw Erange("crypto_sym::crypto_sym",tools_printf(gettext("Error while opening libgcrypt key handle: %s/%s"),
								       gcry_strsource(err),
								       gcry_strerror(err)));

		    // assigning key to the handle

		err = gcry_cipher_setkey(clef, (const void *)hashed_password.c_str(), hashed_password.get_size());
		if(err != GPG_ERR_NO_ERROR)
		    throw Erange("crypto_sym::crypto_sym",tools_printf(gettext("Error while assigning key to libgcrypt key handle: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

		    // essiv initialization

		dar_set_essiv(hashed_password, essiv_clef, get_reading_version(), algo);
	    }
	    catch(...)
	    {
		detruit();
		throw;
	    };

#ifdef LIBDAR_NO_OPTIMIZATION
	    self_test();
#endif
	}
#else
	throw Ecompilation(gettext("Missing strong encryption support (libgcrypt)"));
#endif
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
	    throw Erange("crypto_sym::crypto_sym",tools_printf(gettext("Cyphering algorithm not available in libgcrypt: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	    // obtaining the maximum key length
	key_len = gcry_cipher_get_algo_keylen(algo_id);
	if(key_len == 0)
	    throw Erange("crypto_sym::crypto_sym",gettext("Failed retrieving from libgcrypt the maximum key length"));

	return key_len;
#else
	throw Ecompilation("Strong encryption");
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
	throw Ecompilation("Strong encryption");
#endif
    }

    bool crypto_sym::is_a_strong_password(crypto_algo algo, const secu_string & password)
    {
#if CRYPTO_AVAILABLE
	bool ret = true;
	gcry_error_t err;
	gcry_cipher_hd_t clef;
	U_I algo_id = get_algo_id(algo);

	err = gcry_cipher_open(&clef, algo_id, GCRY_CIPHER_MODE_CBC, GCRY_CIPHER_SECURE);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::crypto_sym",tools_printf(gettext("Error while opening libgcrypt key handle to check password strength: %s/%s"),
							       gcry_strsource(err),
							       gcry_strerror(err)));

	try
	{
	    err = gcry_cipher_setkey(clef, (const void *)password.c_str(), password.get_size());
	    if(err != GPG_ERR_NO_ERROR)
	    {
		if(gcry_err_code(err) == GPG_ERR_WEAK_KEY)
		    ret = false;
		else
		    throw Erange("crypto_sym::crypto_sym",tools_printf(gettext("Error while assigning key to libgcrypt key handle to check password strength: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	    }
	}
	catch(...)
	{
	    gcry_cipher_close(clef);
	    throw;
	}
	gcry_cipher_close(clef);

	return ret;
#else
	throw Ecompilation("Strong encryption");
#endif
    }


    void crypto_sym::detruit()
    {
#if CRYPTO_AVAILABLE
	if(clef != nullptr)
	    gcry_cipher_close(clef);
	if(essiv_clef != nullptr)
	    gcry_cipher_close(essiv_clef);
	if(ivec != nullptr)
	{
	    (void)memset(ivec, 0, algo_block_size);
	    gcry_free(ivec);
	}
#endif
    }


    U_32 crypto_sym::encrypted_block_size_for(U_32 clear_block_size)
    {
	return ((clear_block_size / algo_block_size) + 1) * algo_block_size;
	    // round to the upper "algo_block_size" byte block of data.
	    // and add an additional "algo_block_size" block if no rounding is necessary.
	    // (we need some place to add the elastic buffer at the end of the block)
    }

    U_32 crypto_sym::clear_block_allocated_size_for(U_32 clear_block_size)
    {
	return encrypted_block_size_for(clear_block_size);
    }

    U_32 crypto_sym::encrypt_data(const infinint & block_num,
				  const char *clear_buf, const U_32 clear_size, const U_32 clear_allocated,
				  char *crypt_buf, U_32 crypt_size)
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
	    err = gcry_cipher_reset(clef);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::crypto_encrypt_data",tools_printf(gettext("Error while resetting encryption key for a new block: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	    make_ivec(block_num, ivec, algo_block_size, essiv_clef);
	    err = gcry_cipher_setiv(clef, (const void *)ivec, algo_block_size);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::crypto_encrypt_data",tools_printf(gettext("Error while setting IV for current block: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	    err = gcry_cipher_encrypt(clef, (unsigned char *)crypt_buf, size_to_fill, (const unsigned char *)clear_buf, size_to_fill);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::crypto_encrypt_data",tools_printf(gettext("Error while cyphering data: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	    return size_to_fill;
	}
	else
	    throw SRC_BUG;
#else
	throw Ecompilation(gettext("blowfish strong encryption support"));
#endif
    }

    U_32 crypto_sym::decrypt_data(const infinint & block_num, const char *crypt_buf, const U_32 crypt_size, char *clear_buf, U_32 clear_size)
    {
#if CRYPTO_AVAILABLE
	gcry_error_t err;

	if(crypt_size == 0)
	    return 0; // nothing to decipher

	make_ivec(block_num, ivec, algo_block_size, essiv_clef);
	err = gcry_cipher_setiv(clef, (const void *)ivec, algo_block_size);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::crypto_encrypt_data",tools_printf(gettext("Error while setting IV for current block: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	err = gcry_cipher_decrypt(clef, (unsigned char *)clear_buf, crypt_size, (const unsigned char *)crypt_buf, crypt_size);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::crypto_encrypt_data",tools_printf(gettext("Error while decyphering data: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	elastic stoc = elastic((unsigned char *)clear_buf, crypt_size, elastic_backward, get_reading_version());
	if(stoc.get_size() > crypt_size)
	    throw Erange("crypto_sym::crypto_encrypt_data",gettext("Data corruption may have occurred, cannot decrypt data"));
	return crypt_size - stoc.get_size();
#else
	throw Ecompilation(gettext("blowfish strong encryption support"));
#endif
    }

#if CRYPTO_AVAILABLE
    void crypto_sym::make_ivec(const infinint & ref, unsigned char *ivec, U_I size, const gcry_cipher_hd_t & IVkey)
    {

	    // Stronger IV calculation: ESSIV.
	    // ESSIV mode helps to provide (at least) IND-CPA security.

	unsigned char *sect = nullptr;
	infinint ref_cp = ref;
	infinint mask = 0xFF;
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
		throw Erange("crypto_sym::crypto_encrypt_data",tools_printf(gettext("Error while generating IV: %s/%s"), gcry_strsource(err), gcry_strerror(err)));
	}
	catch(...)
	{
	    delete [] sect;
	    throw;
	}
	delete [] sect;
    }
#endif

#if CRYPTO_AVAILABLE
    secu_string crypto_sym::pkcs5_pass2key(const secu_string & password,
					   const string & salt,
					   U_I iteration_count,
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
	l = output_length / gcry_md_get_algo_dlen(GCRY_MD_SHA1);
	r = output_length % gcry_md_get_algo_dlen(GCRY_MD_SHA1);
	if (r == 0)
	    r = gcry_md_get_algo_dlen(GCRY_MD_SHA1);
	else
	    ++l;    // round up

	    // testing SHA1 availability

	err = gcry_md_test_algo(GCRY_MD_SHA1);
	if(err != GPG_ERR_NO_ERROR)
	    throw Ecompilation(tools_printf(gettext("Error! SHA1 not available in libgcrypt: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	    // opening a handle for Message Digest

	err = gcry_md_open(&hmac, GCRY_MD_SHA1, GCRY_MD_FLAG_SECURE|GCRY_MD_FLAG_HMAC);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::pkcs5_pass2key",tools_printf(gettext("Error while derivating key from password (HMAC open): %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	    // setting the HMAC key

	err = gcry_md_setkey(hmac, password.c_str(), password.get_size());
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_sym::pkcs5_pass2key",tools_printf(gettext("Error while derivating key from password (HMAC set key): %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	    // now ready to compute HMAC-SHA1 message digest using "hmac"

	try
	{
	    U_I UjLen = gcry_md_get_algo_dlen(GCRY_MD_SHA1);
	    char *Ti = nullptr, *Uj = nullptr;

	    retval.resize(output_length);
	    Ti = (char *)gcry_malloc_secure(gcry_md_get_algo_dlen(GCRY_MD_SHA1));
	    if(Ti == nullptr)
		throw Ememory("crypto_sym::pkcs5_pass2key");
	    try
	    {
		Uj = (char *)gcry_malloc_secure(gcry_md_get_algo_dlen(GCRY_MD_SHA1));
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
			tmp_md = gcry_md_read(hmac, GCRY_MD_SHA1);
			(void)memcpy(Uj, tmp_md, gcry_md_get_algo_dlen(GCRY_MD_SHA1));
			(void)memcpy(Ti, tmp_md, gcry_md_get_algo_dlen(GCRY_MD_SHA1));

			for (U_32 j = 2; j <= iteration_count; ++j)
			{
				// U_j = PRF(P, U_{j-1})

			    gcry_md_reset(hmac);
			    gcry_md_write(hmac, (const unsigned char *) Uj, UjLen);
			    tmp_md = gcry_md_read(hmac, GCRY_MD_SHA1);
			    (void)memcpy(Uj, tmp_md, gcry_md_get_algo_dlen(GCRY_MD_SHA1));
			    tools_memxor(Ti, tmp_md, gcry_md_get_algo_dlen(GCRY_MD_SHA1));
			}

			if (i < l)
			    retval.append(Ti, gcry_md_get_algo_dlen(GCRY_MD_SHA1));
			else 	// last block
			    retval.append(Ti, r);

		    } // end of main for() loop
		}
		catch(...)
		{
		    (void)memset(Uj, 0, gcry_md_get_algo_dlen(GCRY_MD_SHA1));
		    gcry_free(Uj);
		    throw;
		}
		(void)memset(Uj, 0, gcry_md_get_algo_dlen(GCRY_MD_SHA1));
		gcry_free(Uj);
	    }
	    catch(...)
	    {
		(void)memset(Ti, 0, gcry_md_get_algo_dlen(GCRY_MD_SHA1));
		gcry_free(Ti);
		throw;
	    }
	    (void)memset(Ti, 0, gcry_md_get_algo_dlen(GCRY_MD_SHA1));
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
#endif

#if CRYPTO_AVAILABLE
    void crypto_sym::dar_set_essiv(const secu_string & key,
				   gcry_cipher_hd_t & IVkey,
				   const archive_version & ver,
				   crypto_algo main_cipher)
    {
	    // Calculate the ESSIV salt.
	    // Recall that ESSIV(sector) = E_salt(sector); salt = H(key).

	unsigned int IV_cipher;
	unsigned int IV_hashing;

	if(ver >= archive_version(8,1)
	    && main_cipher != crypto_algo::blowfish)
	{
	    IV_cipher = GCRY_CIPHER_AES256; // to have an algorithm available when ligcrypt is used in FIPS mode
	    IV_hashing = GCRY_MD_SHA256; // SHA224 was also ok but as time passes, it would get sooner unsave
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

	    IV_cipher = GCRY_CIPHER_BLOWFISH;
	    IV_hashing = GCRY_MD_SHA1;
	}

	void *digest = nullptr;
	U_I digest_len = gcry_md_get_algo_dlen(IV_hashing);
	gcry_error_t err;

	if(digest_len == 0)
	    throw SRC_BUG;
	digest = gcry_malloc_secure(digest_len);
	if(digest == nullptr)
	    throw Ememory("crypto_sym::dar_set_essiv");

	try
	{
		// making a hash of the provided password into the digest variable

	    gcry_md_hash_buffer(IV_hashing, digest, (const void *)key.c_str(), key.get_size());

		// creating a handle for a new handle with algo equal to "IV_cipher"
	    err = gcry_cipher_open(&IVkey,
				   IV_cipher,
				   GCRY_CIPHER_MODE_ECB,
				   GCRY_CIPHER_SECURE);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::dar_set_essiv",tools_printf(gettext("Error while creating ESSIV handle: %s/%s"), gcry_strsource(err),gcry_strerror(err)));


		// obtaining key len for IV_cipher

	    size_t essiv_key_len;
	    err = gcry_cipher_algo_info(IV_cipher, GCRYCTL_GET_KEYLEN, nullptr, &essiv_key_len);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::crypto_sym",tools_printf(gettext("Failed retrieving from libgcrypt the key length to use (essiv key): %s/%s"), gcry_strsource(err),gcry_strerror(err)));

		// failing if the digest size is larger than key size for IV_cipher

	    if(digest_len > essiv_key_len
	       && IV_cipher != GCRY_CIPHER_BLOWFISH)
		throw SRC_BUG;
		// IV_cipher and IV_hashing must be chosen in coherence!
		// we do not complain for older format archive... its done now and worked so far.
		// This bug report is rather to signal possible problem when time will come
		// to update the cipher and digest algorithms

		// assiging the digest as key for the new handle

	    err = gcry_cipher_setkey(IVkey,
				     digest,
				     digest_len);
	    if(err != GPG_ERR_NO_ERROR)
	    {
		    // tolerating WEAK key here, we use that key to fill the IV of the real strong key
		if(gpg_err_code(err) != GPG_ERR_WEAK_KEY)
		    throw Erange("crypto_sym::dar_set_essiv",tools_printf(gettext("Error while assigning key to libgcrypt key handle (essiv): %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	    }

		// obtaining the block size of the essiv cipher

	    size_t algo_block_size_essiv;

	    err = gcry_cipher_algo_info(IV_cipher, GCRYCTL_GET_BLKLEN, nullptr, &algo_block_size_essiv);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::crypto_sym",tools_printf(gettext("Failed retrieving from libgcrypt the block size used by the cyphering algorithm (essiv): %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	    if(algo_block_size_essiv == 0)
		throw SRC_BUG;

		// obtaining the block size of the main cipher

	    size_t local_algo_block_size;

	    err = gcry_cipher_algo_info(get_algo_id(main_cipher), GCRYCTL_GET_BLKLEN, nullptr, &local_algo_block_size);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("crypto_sym::crypto_sym",tools_printf(gettext("Failed retrieving from libgcrypt the block size used by the cyphering algorithm: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
	    if(local_algo_block_size == 0)
		throw SRC_BUG;

		// testing the coherence of block size between main cipher key
		// and cipher key used to generate IV for the main cipher key

	    if(local_algo_block_size < algo_block_size_essiv)
		throw SRC_BUG; // cannot cipher less data (IV for main key) than block size of IV key
	    else
		if(local_algo_block_size % algo_block_size_essiv != 0)
		    throw SRC_BUG;
		// in ECB mode we should encrypt an integer number of blocks according
		// to IV key block size

	}
	catch(...)
	{
	    (void)memset(digest, 0, digest_len); // attempt to scrub memory
	    gcry_free(digest);
	    throw;
	}
	(void)memset(digest, 0, digest_len); // attempt to scrub memory
	gcry_free(digest);
    }
#endif


#ifdef LIBDAR_NO_OPTIMIZATION
    void crypto_sym::self_test(void)
    {
#if CRYPTO_AVAILABLE
	    //
	    // Test PBKDF2 (test vectors are from RFC 3962.)
	    //
	secu_string result;
	string p1 = "password";
	string p2 = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"; // 64 characters
	string p4 = p2 + "X"; // 65 characters
	secu_string pass = secu_string(100);
	gcry_cipher_hd_t esivkey;

	pass.append(p1.c_str(), (U_I)p1.size());
	result = pkcs5_pass2key(pass, "ATHENA.MIT.EDUraeburn", 1, 16);
	if (result != string("\xcd\xed\xb5\x28\x1b\xb2\xf8\x01\x56\x5a\x11\x22\xb2\x56\x35\x15", 16))
	    throw Erange("crypto_sym::self_test", gettext("Library used for blowfish encryption does not respect RFC 3962"));

	result = pkcs5_pass2key(pass, "ATHENA.MIT.EDUraeburn", 1200, 32);
	if (result !=  string("\x5c\x08\xeb\x61\xfd\xf7\x1e\x4e\x4e\xc3\xcf\x6b\xa1\xf5\x51\x2b"
			      "\xa7\xe5\x2d\xdb\xc5\xe5\x14\x2f\x70\x8a\x31\xe2\xe6\x2b\x1e\x13", 32)
	    )
	    throw Erange("crypto_sym::self_test", gettext("Library used for blowfish encryption does not respect RFC 3962"));

	pass.clear();
	pass.append(p2.c_str(), (U_I)p2.size());

	result = pkcs5_pass2key(pass,
				"pass phrase equals block size", 1200, 32);
	if (result != string("\x13\x9c\x30\xc0\x96\x6b\xc3\x2b\xa5\x5f\xdb\xf2\x12\x53\x0a\xc9"
			     "\xc5\xec\x59\xf1\xa4\x52\xf5\xcc\x9a\xd9\x40\xfe\xa0\x59\x8e\xd1", 32))
	    throw Erange("crypto_sym::self_test", gettext("Library used for blowfish encryption does not respect RFC 3962"));

	pass.resize((U_I)p4.size());
	pass.append(p4.c_str(), (U_I)p4.size());
	result = pkcs5_pass2key(pass,
				"pass phrase exceeds block size", 1200, 32);
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
	dar_set_essiv(pass, esivkey, 1, crypto_algo::blowfish);

	try
	{

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
	catch(...)
	{
	    gcry_cipher_close(esivkey);
	    throw;
	}
	gcry_cipher_close(esivkey);
#endif
    }
#endif

#if CRYPTO_AVAILABLE
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
#endif

} // end of namespace
