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

#include "../my_config.h"

#include "hash_fichier.hpp"
#include "erreurs.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    string hash_algo_to_string(hash_algo algo)
    {
	switch(algo)
	{
	case hash_none:
	    throw SRC_BUG;
	case hash_md5:
	    return "md5";
	case hash_sha1:
	    return "sha1";
	default:
	    throw SRC_BUG;
	}
    }

    hash_fichier::hash_fichier(user_interaction & dialog, S_I fd)
	: fichier(dialog, fd)
    {
	if(get_mode() != gf_write_only) // at that time the fichier and generic_file part of the object is already built,
	    throw SRC_BUG; // so we can call get_mode() to retrieve the openning mode of the file
	only_hash = false;
	hash_ready = false;
	force_perm = false;
	try
	{
	    x_perm = tools_get_permission(fd);
	}
	catch(Erange & e)
	{
	    throw Erange("hash_fichier::hash_fichier", string(gettext("Cannot determine the permission to use for hash files: "))+ e.get_message());
	}
	user_ownership = "";
	group_ownership = "";
    }

    hash_fichier::hash_fichier(user_interaction & dialog, const char *name, gf_mode m, U_I perm, bool furtive_mode)
	: fichier(dialog, name, m, perm, furtive_mode)
    {
	if(m != gf_write_only)
	    throw SRC_BUG;
	only_hash = false;
	hash_ready = false;
	force_perm = false;
	x_perm = perm;
	user_ownership = "";
	group_ownership = "";
    }

    hash_fichier::hash_fichier(user_interaction & dialog, const std::string & chemin, gf_mode m, U_I perm, bool furtive_mode)
	: fichier(dialog, chemin, m, perm, furtive_mode)
    {
	if(m != gf_write_only)
	    throw SRC_BUG;
	only_hash = false;
	hash_ready = false;
	force_perm = false;
	x_perm = perm;
	user_ownership = "";
	group_ownership = "";
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
    }

    void hash_fichier::inherited_terminate()
    {
	if(hash_ready)
	{
	    try
	    {
		dump_hash();
	    }
	    catch(...)
	    {
#if CRYPTO_AVAILABLE
		gcry_md_close(hash_handle);
#endif
		hash_ready = false;
		throw;
	    }
#if CRYPTO_AVAILABLE
	    gcry_md_close(hash_handle);
#endif
	    hash_ready = false;
	}

	fichier::inherited_terminate();
    }


    void hash_fichier::set_hash_file_name(const std::string & filename, hash_algo algo, const string & extension)
    {
#if CRYPTO_AVAILABLE
	gcry_error_t err;

	hash_filename = filename;
	hash_extension = extension;
	eof = false;
	switch(algo)
	{
	case hash_none:
	    throw SRC_BUG;
	case hash_md5:
	    hash_gcrypt = GCRY_MD_MD5;
	    break;
	case hash_sha1:
	    hash_gcrypt = GCRY_MD_SHA1;
	    break;
	default:
	    throw SRC_BUG;
	}

	err = gcry_md_test_algo(hash_gcrypt);
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("hash_fichier::set_hash_file_name",tools_printf(gettext("Error while initializing hash: Hash algorithm not available in libgcrypt: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	err = gcry_md_open(&hash_handle, hash_gcrypt, 0); // no need of secure memory here
	if(err != GPG_ERR_NO_ERROR)
	    throw Erange("hash_fichier::set_hash_file_name",tools_printf(gettext("Error while creating hash handle: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

	hash_ready = true;
#else
	throw Ecompilation(gettext("Missing hashing algorithms support (which is part of strong encryption support, using libgcrypt)"));
#endif
    }

    void hash_fichier::inherited_write(const char *a, U_I size)
    {
#if CRYPTO_AVAILABLE
	if(!hash_ready)
	    throw SRC_BUG;
	if(eof)
	    throw SRC_BUG;
	gcry_md_write(hash_handle, (const void *)a, size);
	if(!only_hash)
	    fichier::inherited_write(a, size);
#else
	throw Ecompilation(gettext("Missing hashing algorithms support (which is part of strong encryption support, using libgcrypt)"));
#endif
    }

    void hash_fichier::dump_hash()
    {
#if CRYPTO_AVAILABLE
	    // first we obtain the hash result;
	const unsigned char *digest = gcry_md_read(hash_handle, hash_gcrypt);
	const U_I digest_size = gcry_md_get_algo_dlen(hash_gcrypt);
	user_interaction_blind aveugle;

	    // avoid subsequent writings (yeld a bug report if that occurs)
	eof = true;

	try
	{
	    fichier where = fichier(aveugle, hash_filename +"."+hash_extension, gf_write_only, x_perm, false);
	    string hexa = tools_string_to_hexa(string((char *)digest, digest_size));
	    path no_path = hash_filename;
	    string slice_name = no_path.basename();

	    where.change_ownership(user_ownership, group_ownership);
	    if(force_perm)
		where.change_permission(x_perm); // needed to set it explicitely to avoid umask reducing permission
	    where.write((const char *)hexa.c_str(), hexa.size());
	    where.write("  ", 2); // two spaces sperator used by md5sum and sha1sum
	    where.write(slice_name.c_str(), slice_name.size());
	    where.write("\n", 1); // we finish by a new-line character
	}
	catch(Egeneric & e)
	{
	    throw Erange("hash_fichier::dump_hash", gettext("Failed writing down the hash: ") + e.get_message());
	}
#endif
	    // no #else clause (routing used from constructor, if binary lack support
	    // for strong encryption this has already been returned to the user
	    // and we must not trouble the destructor for that
    }


} // end of namespace
