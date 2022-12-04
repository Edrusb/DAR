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

#if CRYPTO_AVAILABLE
	gcry_error_t err;

	hash_gcrypt = hash_algo_to_gcrypt_hash(algo);

	err = gcry_md_test_algo(hash_gcrypt);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("hash_fichier::hash_fichier",tools_printf(gettext("Error while initializing hash: Hash algorithm not available in libgcrypt: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	err = gcry_md_open(&hash_handle, hash_gcrypt, 0); // no need of secure memory here
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("hash_fichier::hash_fichier",tools_printf(gettext("Error while creating hash handle: %s/%s"), gcry_strsource(err),gcry_strerror(err)));
#else
	throw Ecompilation(gettext("Missing hashing algorithms support (which is part of strong encryption support, using libgcrypt)"));
#endif
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
#if CRYPTO_AVAILABLE
	if(eof)
	    throw SRC_BUG;
	gcry_md_write(hash_handle, (const void *)a, size);
	if(!only_hash)
	    ref->write(a, size);
	return size;
#else
	throw Ecompilation(gettext("Missing hashing algorithms support (which is part of strong encryption support, using libgcrypt)"));
#endif
    }

    bool hash_fichier::fichier_global_inherited_read(char *a, U_I size, U_I & read, std::string & message)
    {
#if CRYPTO_AVAILABLE
	if(eof)
	    throw SRC_BUG;
	read = ref->read(a, size);
	message = "BUG! This should never show!";
	if(read > 0)
	    gcry_md_write(hash_handle, (const void *)a, read);
	return true;
#else
	throw Ecompilation(gettext("Missing hashing algorithms support (which is part of strong encryption support, using libgcrypt)"));
#endif
    }

    void hash_fichier::inherited_terminate()
    {
	ref->terminate();
	if(!hash_dumped)
	{
		// avoids subsequent writings (yeld a bug report if that occurs)
	    eof = true;
		// avoid a second run of dump_hash()
	    hash_dumped = true;
	    try
	    {
#if CRYPTO_AVAILABLE
		    // first we obtain the hash result;
		const unsigned char *digest = gcry_md_read(hash_handle, hash_gcrypt);
		const U_I digest_size = gcry_md_get_algo_dlen(hash_gcrypt);

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
		    throw Erange("hash_fichier::dump_hash", gettext("Failed writing down the hash: ") + e.get_message());
		}
#endif
		    // no #else clause (routine used from constructor, if binary lack support
		    // for strong encryption this has already been returned to the user
		    // and we must not trouble the destructor for that

	    }
	    catch(...)
	    {
#if CRYPTO_AVAILABLE
		gcry_md_close(hash_handle);
#endif
		throw;
	    }
#if CRYPTO_AVAILABLE
	    gcry_md_close(hash_handle);
#endif
	}
    }


} // end of namespace
