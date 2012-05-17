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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: crypto.cpp,v 1.12.2.7 2008/05/09 20:58:27 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif

#if CRYPTO_AVAILABLE
#if HAVE_OPENSSL_EVP_H
#include <openssl/evp.h>
#endif
#if HAVE_OPENSSL_HMAC_H
#include <openssl/hmac.h>
#endif
#endif
}

#include "crypto.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "elastic.hpp"

using namespace std;

namespace libdar
{

    void crypto_split_algo_pass(const string & all, crypto_algo & algo, string & pass)
    {
	    // split from "algo:pass" syntax
	string::const_iterator it = all.begin();
	string tmp;

	if(all == "")
	{
	    algo = crypto_none;
	    pass = "";
	}
	else
	{
	    while(it != all.end() && *it != ':')
		++it;

	    if(it != all.end()) // a ':' is present in the given string
	    {
		tmp = string(all.begin(), it);
		++it;
		pass = string(it, all.end());
		if(tmp == "scrambling" || tmp == "scram")
		    algo = crypto_scrambling;
		else
		    if(tmp == "none")
			algo = crypto_none;
		    else
			if(tmp == "blowfish" || tmp == "bf" || tmp == "")
			    algo = crypto_blowfish; // blowfish is the default cypher ("")
			else
			    if(tmp == "blowfish_weak" || tmp == "bfw")
				algo = crypto_blowfish_weak;
			    else
				throw Erange("crypto_split_algo_pass", string(gettext("unknown cryptographic algorithm: ")) + tmp);
	    }
	    else // no ':' using blowfish as default cypher
	    {
		algo = crypto_blowfish;
		pass = all;
	    }
	}
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: crypto.cpp,v 1.12.2.7 2008/05/09 20:58:27 edrusb Rel $";
        dummy_call(id);
    }

///////////////////////////// BLOWFISH IMPLEMENTATION ////////////////////////////////

#define BF_BLOCK_SIZE 8

    blowfish::blowfish(user_interaction & dialog,
		       U_32 block_size,
		       const string & password,
		       generic_file & encrypted_side,
		       const dar_version & reading_ver,
		       bool weak_mode)
	: tronconneuse(dialog, block_size, encrypted_side)
    {
#if CRYPTO_AVAILABLE
	if(!weak_mode)
	    x_weak_mode = !version_greater(reading_ver, "05");
	else
	    x_weak_mode = weak_mode;
	version_copy(reading_version, reading_ver);

	    // self_test();
	if(x_weak_mode)
	    BF_set_key(&clef, password.size(), (unsigned char *)password.c_str());
	else
	    dar_set_key(pkcs5_pass2key(password, "", 2000, 56));
#else
	throw Ecompilation(gettext("blowfish strong encryption support"));
#endif
    }


    U_32 blowfish::encrypted_block_size_for(U_32 clear_block_size)
    {
	return ((clear_block_size / BF_BLOCK_SIZE) + 1) * BF_BLOCK_SIZE;
	    // round to the upper 8 byte block of data.
	    // and add one 8 byte block if no rounding is necessary.
    }

    U_32 blowfish::clear_block_allocated_size_for(U_32 clear_block_size)
    {
	return encrypted_block_size_for(clear_block_size);
    }

    U_32 blowfish::encrypt_data(const infinint & block_num,
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
	    unsigned char ivec[8];
	    elastic stic = elastic(size_to_fill - clear_size);

	    stic.dump((unsigned char *)(const_cast<char *>(clear_buf + clear_size)), (U_32)(clear_allocated - clear_size));
	    make_ivec(block_num, ivec);
	    BF_cbc_encrypt((const unsigned char *)clear_buf, (unsigned char *)crypt_buf, size_to_fill, &clef, ivec, BF_ENCRYPT);
	    return size_to_fill;
	}
	else
	    throw SRC_BUG;
#else
	throw Ecompilation(gettext("blowfish strong encryption support"));
#endif
    }

    U_32 blowfish::decrypt_data(const infinint & block_num, const char *crypt_buf, const U_32 crypt_size, char *clear_buf, U_32 clear_size)
    {
#if CRYPTO_AVAILABLE
	unsigned char ivec[8];

	make_ivec(block_num, ivec);
	BF_cbc_encrypt((const unsigned char *)crypt_buf, (unsigned char *)clear_buf, crypt_size, &clef, ivec, BF_DECRYPT);

	elastic stoc = elastic((unsigned char *)clear_buf, crypt_size, elastic_backward, reading_version);
	return crypt_size - stoc.get_size();
#else
	throw Ecompilation(gettext("blowfish strong encryption support"));
#endif
    }

    void blowfish::make_ivec(const infinint & ref, unsigned char ivec[8])
    {
#if CRYPTO_AVAILABLE
	infinint upper = ref >> 32;
	U_32 high = 0, low = 0;

	high = upper % (U_32)(0xFFFF); // for bytes (high weight)
	low = ref % (U_32)(0xFFFF); // for bytes (lowest weight)

	    // There has been confusion: the modulo is used below to get the last 8 bits of the integer
	    // it has thus to be made on 256 ( % 256 in place of % 8) However the resulting ivec is
	    // not worse or better than the one which would result from the % 256 operation.
	    // We thus keep this algorithm in place for backward compatibility.

	if(x_weak_mode)
	{
		/// old buggy generated IV

	    ivec[0] = low % 8;
	    ivec[1] = (low >> 8) % 8;
	    ivec[2] = (low >> 16) % 8;
	    ivec[3] = (low >> 24) % 8;
	    ivec[4] = high % 8;
	    ivec[5] = (high >> 8) % 8;
	    ivec[6] = (high >> 16) % 8;
	    ivec[7] = (high >> 24) % 8;
	}
	else
	{
		// Stronger IV calculation: ESSIV.
		// ESSIV mode helps to provide (at least) IND-CPA security.

	    unsigned char sect[8];
	    U_32 high = 0, low = 0;
	    infinint tmp;

		// Split 64-bit ref into high and low parts
	    tmp = ref & (U_32) 0xffffffffu;
	    tmp.unstack(low);
	    tmp = (ref >> 32) & (U_32) 0xffffffffu;
	    tmp.unstack(high);

		// 64-bit big-endian representation of the sector number
	    sect[0] = (high >> 24) & 0xff;
	    sect[1] = (high >> 16) & 0xff;
	    sect[2] = (high >> 8) & 0xff;
	    sect[3] = high & 0xff;
	    sect[4] = (low >> 24) & 0xff;
	    sect[5] = (low >> 16) & 0xff;
	    sect[6] = (low >> 8) & 0xff;
	    sect[7] = low & 0xff;

		// IV(sector) = E_salt(sector)
	    BF_ecb_encrypt(sect, ivec, &essiv_clef, BF_ENCRYPT);
	}
#else
	throw Ecompilation(gettext("blowfish strong encryption support"));
#endif
    }


    string blowfish::pkcs5_pass2key(const string & password,
				    const string & salt,
				    U_I iteration_count,
				    U_I output_length)
    {
#if CRYPTO_AVAILABLE
#if CRYPTO_FULL_BF_AVAILABLE
	    // Password-based key derivation function (PBKDF2) from PKCS#5 v2.0
	    // Using HMAC-SHA1 as the underlying pseudorandom function.
	HMAC_CTX hmac;
	const EVP_MD *digest = EVP_sha1();
	U_32 l = 0, r = 0;
	string retval;

	if (output_length == 0)
	    return "";

	    // Let l be the number of EVP_MD_size(digest) blocks in the derived key, rounding up.
	    // Let r be the number of octets in the last block.
	l = output_length / EVP_MD_size(digest);
	r = output_length % EVP_MD_size(digest);
	if (r == 0)
	    r = EVP_MD_size(digest);
	else
	    ++l;    // round up


	HMAC_CTX_init(&hmac);
	try
	{
	    U_I UjLen = 0;
	    char *Ti = NULL, *Uj = NULL;

	    retval.clear();
	    retval.reserve(EVP_MD_size(digest));
	    Ti = new char[EVP_MD_size(digest)];
	    if(Ti == NULL)
		throw Ememory("blowfish::pkcs5_pass2key");
	    try
	    {
		Uj = new char[EVP_MD_size(digest)];
		if(Uj == NULL)
		    throw Ememory("blowfish::pkcs5_pass2key");
		try
		{
		    for (U_32 i = 1; i <= l; ++i)
		    {
			    // Ti = U_1 \xor U_2 \xor ... \xor U_c
			    // U_1 = PRF(P, S || INT(i))
			unsigned char ii[4];
			ii[0] = (i >> 24) & 0xff;
			ii[1] = (i >> 16) & 0xff;
			ii[2] = (i >> 8) & 0xff;
			ii[3] = i & 0xff;
			HMAC_Init_ex(&hmac, password.c_str(), password.size(), digest, NULL);
			HMAC_Update(&hmac, (const unsigned char *) salt.c_str(), salt.size());
			HMAC_Update(&hmac, ii, 4);
			HMAC_Final(&hmac, (unsigned char *) Uj, &UjLen);
			if (UjLen != (U_I) EVP_MD_size(digest))
			    throw Erange("pkcs5_pass2key", gettext("SSL returned Message Authentication Code (MAC) has an incoherent size with provided parameters"));

			memcpy(Ti, Uj, EVP_MD_size(digest));
			for (U_32 j = 2; j <= iteration_count; ++j)
			{
				// U_j = PRF(P, U_{j-1})
			    HMAC_Init_ex(&hmac, password.c_str(), password.size(), digest, NULL);
			    HMAC_Update(&hmac, (const unsigned char *) Uj, UjLen);
			    HMAC_Final(&hmac, (unsigned char *) Uj, &UjLen);
			    if (UjLen != (U_I) EVP_MD_size(digest))
				throw Erange("pkcs5_pass2key", gettext("SSL returned Message Authentication Code (MAC) has an incoherent size with provided parameters"));
			    tools_memxor(Ti, Uj, EVP_MD_size(digest));
			}

			if (i < l)
			    retval.append(Ti, EVP_MD_size(digest));
			else 	// last block
			    retval.append(Ti, r);

		    } // end of main for() loop
		}
		catch(...)
		{
		    memset(Uj, 0, EVP_MD_size(digest));
		    delete [] Uj;
		    throw;
		}
		memset(Uj, 0, EVP_MD_size(digest));
		delete [] Uj;
	    }
	    catch(...)
	    {
		memset(Ti, 0, EVP_MD_size(digest));
		delete [] Ti;
		throw;
	    }
	    memset(Ti, 0, EVP_MD_size(digest));
	    delete [] Ti;
	}
	catch(...)
	{
	    HMAC_CTX_cleanup(&hmac);
	    throw;
	}
	HMAC_CTX_cleanup(&hmac);

	return retval;
#else
  	throw Ecompilation(gettext("New blowfish implementation support"));
#endif
#else
	throw Ecompilation(gettext("blowfish strong encryption support"));
#endif
    }

    void blowfish::dar_set_key(const string & key)
    {
#if CRYPTO_AVAILABLE
#if CRYPTO_FULL_BF_AVAILABLE

	    // Calculate the ESSIV salt.
	    // Recall that ESSIV(sector) = E_salt(sector); salt = H(key).

	const EVP_MD *digest = EVP_sha1();
	unsigned char *salt = NULL;
	unsigned int saltlen = 0;
	EVP_MD_CTX *digest_ctx = EVP_MD_CTX_create();

	try
	{
	    salt = new unsigned char[EVP_MD_size(digest)];
	    if(salt == NULL)
		throw Ememory("blowfish::dar_set_key");
	    try
	    {
		memset(salt, 0, EVP_MD_size(digest)); // a simple deterministic initialization
		    // salt = SHA1(key)
		if(!EVP_DigestInit_ex(digest_ctx, digest, NULL))
		    throw Erange("blowfish::dar_set_key", gettext("libssl call failed: EVP_DigestInit_ex failed"));
		if(!EVP_DigestUpdate(digest_ctx, key.c_str(), key.size()))
		    throw Erange("blowfish::dar_set_key", gettext("libssl call failed: EVP_DigestInit_ex failed"));
		if(!EVP_DigestFinal_ex(digest_ctx, salt, &saltlen))
		    throw Erange("blowfish::dar_set_key", gettext("libssl call failed: EVP_DigestInit_ex failed"));

		    // Use the salt as the blowfish encryption key for essiv
		BF_set_key(&essiv_clef, saltlen, salt);
	    }
	    catch(...)
	    {
		memset(salt, 0, EVP_MD_size(digest)); // attempt to scrub memory
		delete [] salt;
	    }
	    memset(salt, 0, EVP_MD_size(digest)); // attempt to scrub memory
	    delete [] salt;
	}
	catch(...)
	{
	    EVP_MD_CTX_destroy(digest_ctx);
	    throw;
	}
	EVP_MD_CTX_destroy(digest_ctx);

	BF_set_key(&clef, key.size(), (const unsigned char *) key.c_str());
#else
  	throw Ecompilation(gettext("New blowfish implementation support"));
#endif
#else
	throw Ecompilation(gettext("blowfish strong encryption support"));
#endif
    }

    void blowfish::self_test(void)
    {
	    //
	    // Test PBKDF2 (test vectors are from RFC 3962.)
	    //
	string result;

	result = pkcs5_pass2key("password", "ATHENA.MIT.EDUraeburn", 1, 16);
	if (result != string("\xcd\xed\xb5\x28\x1b\xb2\xf8\x01\x56\x5a\x11\x22\xb2\x56\x35\x15", 16))
	    throw Erange("blowfish::self_test", gettext("Library used for blowfish encryption does not respect RFC 3962"));

	result = pkcs5_pass2key("password", "ATHENA.MIT.EDUraeburn", 1200, 32);
	if (result !=  string("\x5c\x08\xeb\x61\xfd\xf7\x1e\x4e\x4e\xc3\xcf\x6b\xa1\xf5\x51\x2b"
			      "\xa7\xe5\x2d\xdb\xc5\xe5\x14\x2f\x70\x8a\x31\xe2\xe6\x2b\x1e\x13", 32)
	    )
	    throw Erange("blowfish::self_test", gettext("Library used for blowfish encryption does not respect RFC 3962"));

	result = pkcs5_pass2key("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
				"pass phrase equals block size", 1200, 32);
	if (result != string("\x13\x9c\x30\xc0\x96\x6b\xc3\x2b\xa5\x5f\xdb\xf2\x12\x53\x0a\xc9"
			     "\xc5\xec\x59\xf1\xa4\x52\xf5\xcc\x9a\xd9\x40\xfe\xa0\x59\x8e\xd1", 32))
	    throw Erange("blowfish::self_test", gettext("Library used for blowfish encryption does not respect RFC 3962"));

	result = pkcs5_pass2key("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
				"pass phrase exceeds block size", 1200, 32);
	if (result != string("\x9c\xca\xd6\xd4\x68\x77\x0c\xd5\x1b\x10\xe6\xa6\x87\x21\xbe\x61"
			     "\x1a\x8b\x4d\x28\x26\x01\xdb\x3b\x36\xbe\x92\x46\x91\x5e\xc8\x2a", 32))
	    throw Erange("blowfish::self_test", gettext("Library used for blowfish encryption does not respect RFC 3962"));

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
	dar_set_key(string("\0\0\0\0", 4));

	for (i = 0; tests[i].sector != 0xdeadbeef; ++i)
	{
	    make_ivec(tests[i].sector, (unsigned char *) ivec);
	    if (memcmp(ivec, tests[i].iv, 8) != 0)
		    //cerr << "sector = " << tests[i].sector << endl;
		    //cerr << "ivec = @@" << string(ivec, 8) << "@@" << endl;
		    //cerr << "should be @@" << string(tests[i].iv, 8) << "@@" << endl;
		throw Erange("blowfish::self_test", gettext("Library used for blowfish encryption does not respect RFC 3962"));
	}
    }

} // end of namespace

