//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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
#if HAVE_GCRYPT_H
#ifndef GCRYPT_NO_DEPRECATED
#define GCRYPT_NO_DEPRECATED
#endif
#include <gcrypt.h>
#endif
}

#include "archive_aux.hpp"
#include "erreurs.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    string hash_algo_to_string(hash_algo algo)
    {
	switch(algo)
	{
	case hash_algo::none:
	    throw SRC_BUG;
	case hash_algo::md5:
	    return "md5";
	case hash_algo::sha1:
	    return "sha1";
	case hash_algo::sha512:
	    return "sha512";
	case hash_algo::argon2:
	    return "argon2";
	case hash_algo::whirlpool:
	    return "whirlpool";
	default:
	    throw SRC_BUG;
	}
    }

    bool string_to_hash_algo(const string & arg, hash_algo & val)
    {
	if(strcasecmp(arg.c_str(), "md5") == 0)
	    val = hash_algo::md5;
	else if(strcasecmp(arg.c_str(), "sha1") == 0)
	    val = hash_algo::sha1;
	else if(strcasecmp(arg.c_str(), "sha512") == 0)
	    val = hash_algo::sha512;
	else if(strcasecmp(arg.c_str(), "none") == 0)
	    val = hash_algo::none;
	else if(strcasecmp(arg.c_str(), "argon2") == 0)
	    val = hash_algo::argon2;
	else if(strcasecmp(arg.c_str(), "whirlpool") == 0)
	    val = hash_algo::whirlpool;
	else
	    return false;
	return true;
    }

    U_I hash_algo_to_gcrypt_hash(hash_algo algo)
    {
#if CRYPTO_AVAILABLE
	U_I hash_gcrypt;

	switch(algo)
	{
	case hash_algo::none:
	    throw SRC_BUG;
	case hash_algo::md5:
	    hash_gcrypt = GCRY_MD_MD5;
	    break;
	case hash_algo::sha1:
	    hash_gcrypt = GCRY_MD_SHA1;
	    break;
	case hash_algo::sha512:
	    hash_gcrypt = GCRY_MD_SHA512;
	    break;
	case hash_algo::argon2:
	    throw SRC_BUG; // not a gcrypt_hash
	default:
	    throw SRC_BUG;
	}

	return hash_gcrypt;
#else
	throw Ecompilation("linking with libgcrypt");
#endif
    }


    unsigned char hash_algo_to_char(hash_algo algo)
    {
	switch(algo)
	{
	case hash_algo::none:
	    return 'n';
	case hash_algo::md5:
	    return 'm';
	case hash_algo::sha1:
	    return '1';
	case hash_algo::sha512:
	    return '5';
	case hash_algo::argon2:
	    return 'a';
	case hash_algo::whirlpool:
	    return 'w';
	default:
	    throw SRC_BUG;
	}
    }

    hash_algo char_to_hash_algo(unsigned char arg)
    {
	switch(arg)
	{
	case '1':
	    return hash_algo::sha1;
	case '5':
	    return hash_algo::sha512;
	case 'm':
	    return hash_algo::md5;
	case 'n':
	    return hash_algo::none;
	case 'a':
	    return hash_algo::argon2;
	case 'w':
	    return hash_algo::whirlpool;
	default:
	    throw Erange("char_to_hash_algo", tools_printf(gettext("unknown hash algorithm corresponding to char `%c'"), arg));
	}
    }


} // end of namespace
