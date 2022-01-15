//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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

#include "hash_fichier.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "path.hpp"

using namespace std;

namespace libdar
{

    hash_fichier::hash_fichier(const shared_ptr<user_interaction> & dialog,
			       fichier_global *under,
			       const std::string & under_filename,
			       fichier_global *hash_file,
			       hash_algo algo) : fichier_global(dialog, under->get_mode())
    {
	if(under == nullptr)
	    throw SRC_BUG;
	if(hash_file == nullptr)
	    throw SRC_BUG;
	if(under->get_mode() == gf_read_write)
	    throw SRC_BUG;
	if(hash_file->get_mode() != gf_write_only)
	    throw SRC_BUG;
	only_hash = false;
	ref = under;
	hash_ref = hash_file;
	path tmp = under_filename;
	ref_filename = tmp.basename();
	eof = false;
	hash_dumped = false;
	algor = algo;

	switch(algo)
	{
	case hash_algo::none:
	    throw SRC_BUG;

	case hash_algo::md5:
	case hash_algo::sha1:
	case hash_algo::sha512:
#if CRYPTO_AVAILABLE
	    gcry_error_t err;

	    hash_gcrypt = hash_algo_to_gcrypt_hash(algo);

	    err = gcry_md_test_algo(hash_gcrypt);
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("hash_fichier::hash_fichier",
			     tools_printf(gettext("Error while initializing hash: Hash algorithm not available in libgcrypt: %s/%s"),
					  gcry_strsource(err),
					  gcry_strerror(err)));

	    err = gcry_md_open(&hash_handle, hash_gcrypt, 0); // no need of secure memory here
	    if(err != GPG_ERR_NO_ERROR)
		throw Erange("hash_fichier::hash_fichier",
			     tools_printf(gettext("Error while creating hash handle: %s/%s"),
					  gcry_strsource(err),
					  gcry_strerror(err)));
#else
	    throw Ecompilation(gettext("Missing %s hash algorithm support (which is provided with strong encryption support, using libgcrypt)"),
			       hash_algo_to_string(algo).c_str());
#endif
	    break;

	case hash_algo::argon2:
	    throw SRC_BUG;

	case hash_algo::whirlpool:
#if RHASH_AVAILABLE
	    rhash_ctxt = rhash_init(RHASH_WHIRLPOOL);
	    if(rhash_ctxt == nullptr)
		throw Erange("hash_fichier::hash_fichier",
			     tools_printf(gettext("Error while initializing hash using librhash: %s"),
					  tools_strerror_r(errno).c_str()));
#else
	    throw Ecompilation(tools_printf(gettext("Missing %s hash algorithm support (which is provided using librhash)"),
					    hash_algo_to_string(algo).c_str()));
#endif
	    break;

	default:
	    throw SRC_BUG;
	}
    }

    hash_fichier::~hash_fichier()
    {
	try
	{
	    terminate();
	}
	catch(...)
	{
		// ignore all errors
	}

	if(ref != nullptr)
	{
	    delete ref;
	    ref = nullptr;
	}

	if(hash_ref != nullptr)
	{
	    delete hash_ref;
	    hash_ref = nullptr;
	}
    }

    U_I hash_fichier::fichier_global_inherited_write(const char *a, U_I size)
    {
	if(eof)
	    throw SRC_BUG;
	switch(algor)
	{
	case hash_algo::none:
	    throw SRC_BUG;
	case hash_algo::md5:
	case hash_algo::sha1:
	case hash_algo::sha512:
#if CRYPTO_AVAILABLE
	    gcry_md_write(hash_handle, (const void *)a, size);
#else
	    throw SRC_BUG;
#endif
	    break;
	case hash_algo::argon2:
	    throw SRC_BUG;
	case hash_algo::whirlpool:
#if RHASH_AVAILABLE
	    if(rhash_update(rhash_ctxt, (const void *)a, size) != 0)
		throw Erange("hash_fichier::fichier_global_inherited_write",
			     tools_printf(gettext("Error adding updating %s hash with new data: %s"),
					  hash_algo_to_string(algor).c_str(),
					  tools_strerror_r(errno).c_str()));
#else
	    throw SRC_BUG;
#endif
	    break;
	default:
	    throw SRC_BUG;
	}

	if(!only_hash)
	    ref->write(a, size);
	return size;
    }

    bool hash_fichier::fichier_global_inherited_read(char *a, U_I size, U_I & read, std::string & message)
    {
	if(eof)
	    throw SRC_BUG;
	read = ref->read(a, size);
	message = "BUG! This should never show!";
	if(read > 0)
	{
	    switch(algor)
	    {
	    case hash_algo::none:
		throw SRC_BUG;
	    case hash_algo::md5:
	    case hash_algo::sha1:
	    case hash_algo::sha512:
#if CRYPTO_AVAILABLE
		gcry_md_write(hash_handle, (const void *)a, read);

#else
		throw SRC_BUG;
#endif
		break;
	    case hash_algo::argon2:
		throw SRC_BUG;
	    case hash_algo::whirlpool:
#if RHASH_AVAILABLE
		if(rhash_update(rhash_ctxt, (const void *)a, size) != 0)
		    throw Erange("hash_fichier::fichier_global_inherited_write",
				 tools_printf(gettext("Error adding updating %s hash with new data: %s"),
					      hash_algo_to_string(algor).c_str(),
					      tools_strerror_r(errno).c_str()));
#else
		throw SRC_BUG;
#endif
		break;
	    default:
		throw SRC_BUG;
	    }
	}
	return true;
    }

    void hash_fichier::inherited_terminate()
    {
	if(!hash_dumped)
	{
	    unsigned char *digest = nullptr;
	    U_I digest_size = 0;

		// avoids subsequent writings (yeld a bug report if that occurs)
	    eof = true;
		// avoid a second run of dump_hash()
	    hash_dumped = true;
	    try
	    {
		switch(algor)
		{
		case hash_algo::none:
		    throw SRC_BUG;

		case hash_algo::md5:
		case hash_algo::sha1:
		case hash_algo::sha512:
#if CRYPTO_AVAILABLE
			// first we obtain the hash result;
		    digest = gcry_md_read(hash_handle, hash_gcrypt);
		    digest_size = gcry_md_get_algo_dlen(hash_gcrypt);
#else
		    throw SRC_BUG;
#endif
		    break;
		case hash_algo::argon2:
		    throw SRC_BUG;

		case hash_algo::whirlpool:
#if RHASH_AVAILABLE
		    digest_size = rhash_get_digest_size(RHASH_WHIRLPOOL);
		    digest = new unsigned char[digest_size];
		    if(digest == nullptr)
			throw Ememory("hash_fichier::inherited_terminate");
		    if(rhash_print((char *)digest, rhash_ctxt, RHASH_WHIRLPOOL, RHPR_RAW) != digest_size)
			throw SRC_BUG;
#else
		    throw SRC_BUG;
#endif
		    break;
		default:
		    throw SRC_BUG;
		}

		write_hash_in_hexa(digest, digest_size);

		    // no #else clause (routine used from constructor, if binary lack support
		    // for strong encryption this has already been returned to the user
		    // and we must not trouble the destructor for that
	    }
	    catch(...)
	    {
		switch(algor)
		{
		case hash_algo::none:
		    break;
		case hash_algo::md5:
		case hash_algo::sha1:
		case hash_algo::sha512:
#if CRYPTO_AVAILABLE
		    gcry_md_close(hash_handle);
#endif
		    break;
		case hash_algo::argon2:
		    break;
		case hash_algo::whirlpool:
		    if(digest != nullptr)
			delete [] digest;
		    break;
		default:
		    break;
		}

		throw;
	    }
	    switch(algor)
	    {
	    case hash_algo::none:
		break;
	    case hash_algo::md5:
	    case hash_algo::sha1:
	    case hash_algo::sha512:
#if CRYPTO_AVAILABLE
		gcry_md_close(hash_handle);
#endif
		break;
	    case hash_algo::argon2:
		break;
	    case hash_algo::whirlpool:
		if(digest != nullptr)
		    delete [] digest;
		break;
	    default:
		break;
	    }
	}
    }

    void hash_fichier::write_hash_in_hexa(void const* digest, U_I digest_size)
    {
	try
	{
	    string hexa = tools_string_to_hexa(string((char *)digest, digest_size));

	    if(hash_ref == nullptr)
		throw SRC_BUG;
	    hash_ref->write((const char *)hexa.c_str(), hexa.size());
	    hash_ref->write("  ", 2); // two spaces sperator used by md5sum and sha1sum
	    hash_ref->write(ref_filename.c_str(), ref_filename.size());
	    hash_ref->write("\n", 1); // we finish by a new-line character
	    hash_ref->terminate();
	}
	catch(Egeneric & e)
	{
	    throw Erange("hash_fichier::write_hash_in_hexa", gettext("Failed writing down the hash: ") + e.get_message());
	}
    }


} // end of namespace
