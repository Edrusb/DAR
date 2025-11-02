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
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

} // end extern "C"

#include "database_header.hpp"
#include "tools.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"
#include "cygwin_adapt.hpp"
#include "fichier_local.hpp"
#include "compressor.hpp"
#include "pile.hpp"
#include "macro_tools.hpp"
#include "tronco_with_elastic.hpp"

using namespace std;

namespace libdar
{

    static const unsigned char database_version = 7;

#define HEADER_OPTION_NONE 0x00
#define HEADER_OPTION_COMPRESSOR 0x01
#define HEADER_OPTION_CRYPTO 0x02
#define HEADER_OPTION_EXTENSION 0x80
	// if EXTENSION bit is set, the option field is two bytes wide
	// this mechanism can be extended in the future by a second extension bit 0x8080
	// and so on forth

    static infinint database_clear_data_callback(generic_file &below, const archive_version & reading_ver);


    void database_header::clear()
    {
	version = database_version;
	algo = compression::gzip;
	pass.clear();
	compression_level = 9;
	crypto = crypto_algo::none;
	kdf_hash = hash_algo::argon2;
	kdf_count = default_iteration_count_argon2;
	crypto_bs = default_crypto_size;
	salt.clear();
	format = archive_format_supported_version;
    }

    void database_header::read(generic_file & f)
    {
	unsigned char options;

	f.read((char *)&version, 1);
	if(version > database_version)
	    throw Erange("database_header::read", gettext("The format version of this database is too high for that software version, use a more recent software to read or modify this database"));
	f.read((char *)&options, 1);

	if((options & HEADER_OPTION_EXTENSION) != 0)
	    throw Erange("database_header::read",  gettext("Unknown header option in database, aborting"));

	if((options & HEADER_OPTION_COMPRESSOR) != 0)
	{
	    char tmp;
	    f.read(&tmp, 1);
	    algo = char2compression(tmp);
	    if(version > 5) // must read the compression level too
	    {
		infinint tmp(f); // reading an infinint from f

		compression_level = 0;
		tmp.unstack(compression_level);
	    }
	}
	else
	{
	    algo = compression::gzip; // this was the default setting in earlier format
	    compression_level = 9;
	}

	if((options & HEADER_OPTION_CRYPTO) != 0)
	{
	    char tmp;
	    infinint itmp;

	    format.read(f);

	    f.read(&tmp, 1);
	    crypto = char_2_crypto_algo(tmp);

	    f.read(&tmp, 1);
	    kdf_hash = char_to_hash_algo(tmp);

	    kdf_count.read(f);

	    itmp.read(f);
	    crypto_bs = 0;
	    itmp.unstack(crypto_bs);
	    if(!itmp.is_zero()) // valid infinint but too large value for crypto_bs
		throw Erange("database_header::read", gettext("Format error in database crypto header, aborting"));

	    itmp.read(f); // salt size
	    tools_read_string_size(f, salt, itmp);
	}
	else
	{
	    crypto = crypto_algo::none;
	    kdf_hash = hash_algo::none;
	    kdf_count = 0;
	    crypto_bs = 0;
	    salt.clear();
	}
    }

    void database_header::write(generic_file & f) const
    {
	unsigned char options = HEADER_OPTION_NONE;

	if(algo != compression::gzip || compression_level != 9)
	    options |= HEADER_OPTION_COMPRESSOR;
	else
	    options &= ~HEADER_OPTION_COMPRESSOR;

	if(crypto != crypto_algo::none)
	    options |= HEADER_OPTION_CRYPTO;
	else
	    options &= ~HEADER_OPTION_CRYPTO;

	f.write((char *)&version, 1);
	f.write((char *)&options, 1);
	if((options & HEADER_OPTION_COMPRESSOR) != 0)
	{
	    char tmp = compression2char(algo);
	    f.write(&tmp, 1);
	    infinint(compression_level).dump(f); // not needed at openning time, just stored for information
	}
	if(crypto != crypto_algo::none)
	{
	    char tmp;
	    infinint itmp;

		// we always write header using the latest known format
		// as it is the one that will be used to structure the
		// encryption layer
	    archive_format_supported_version.dump(f);
	    format = archive_format_supported_version;

	    tmp = crypto_algo_2_char(crypto);
	    f.write(&tmp, 1);

	    tmp = hash_algo_to_char(kdf_hash);
	    f.write(&tmp, 1);

	    kdf_count.dump(f);

	    itmp = crypto_bs;
	    itmp.dump(f);

	    itmp = salt.size();
	    itmp.dump(f);
	    tools_write_string_all(f, salt);
	}
    }

    void database_header::set_crypto(crypto_algo val)
    {
	switch(val)
	{
	case crypto_algo::none:
	    break;
	case crypto_algo::scrambling:
	    throw Erange("database_header::set_crypto",
			 gettext("weak crypto algorithm requested, aborting"));
	case crypto_algo::blowfish: // databases should barely exceed 4GB of size... even uncompressed.
	case crypto_algo::aes256:
	case crypto_algo::twofish256:
	case crypto_algo::serpent256:
	case crypto_algo::camellia256:
	    break;
	default:
	    throw SRC_BUG;
	}

	crypto = val;
    }

    void database_header::set_pass(const secu_string & val)
    {
	if(val.empty())
	    throw SRC_BUG; // should be caught before reaching here to interactively ask for a real password
	pass = val;
    }

    void database_header::set_kdf_hash(hash_algo val)
    {
	switch(val)
	{
	case hash_algo::none:
	case hash_algo::md5:
	case hash_algo::sha1:
	    throw Erange("database_header::set_kdf_hash",
			 gettext("invalid hash algorithm set for the key derivation function"));
	case hash_algo::sha512:
	case hash_algo::argon2:
	case hash_algo::whirlpool:
	    break;
	default:
	    throw SRC_BUG;
	}

	kdf_hash = val;
    }

    void database_header::set_kdf_iteration(const infinint & val)
    {
	if(val < default_iteration_count_argon2) // even if hash algo is not argon2
	    throw Erange("database_header::set_kdf_iteration",
			 gettext("Setting KDF iteration count too low would deserve the benefit of KDF cost, facilitating dictionnary attack type"));

	kdf_count = val;

    }


    void database_header::set_crypto_block_size(U_32 val)
    {
	if(val < default_crypto_size)
	    throw Erange("database_header::set_crypto_block_size",
		gettext("Setting crypto block size below the default value, would lead to lower performance and weaken security"));

	crypto_bs = val;
    }


	//////////

    generic_file *database_header_create(const shared_ptr<user_interaction> & dialog,
					 const string & filename,
					 bool overwrite,
					 const database_header & params,
					 bool info_details)
    {
	pile* stack = new (nothrow) pile();
	generic_file *tmp = nullptr;

	struct stat buf;
	proto_compressor *comp;

	if(stack == nullptr)
	    throw Ememory("database_header_create");

	try
	{
	    if(stat(filename.c_str(), &buf) >= 0 && !overwrite)
		throw Erange("database_header_create", gettext("Cannot create database, file exists"));
	    tmp = new (nothrow) fichier_local(dialog, filename, gf_write_only, 0666, !overwrite, overwrite, false);
	    if(tmp == nullptr)
		throw Ememory("database_header_create");

	    stack->push(tmp); // now the fichier_local is managed by stack

	    if(params.get_crypto() != crypto_algo::none)
	    {
		tronco_with_elastic* cipher = new (nothrow) tronco_with_elastic(dialog,
										2, // 2 threads at most
										params.get_crypto_block_size(),
										*(stack->top()),
										params.get_pass(),
										params.get_archive_format(),
										params.get_crypto(),
										"", // asking to use pkcs5 to set the salt
										params.get_kdf_iteration(),
										params.get_kdf_hash(),
										true, // asking to use pkcs5
										info_details);
		if(cipher == nullptr)
		    throw Ememory("database_header_create");

		try
		{

			// adding the newly calculated salt to the database header to be exposed there in clear text
		    params.set_kdf_salt(cipher->get_salt());

			// now dropping the header in clear text
		    params.write(*stack);


			// adding the initial elastic buffer
		    cipher->get_ready_for_writing(stack->get_position());

		    stack->push(cipher); // cipher pointed to object now managed by the stack
		}
		catch(...)
		{
		    if(cipher != nullptr)
			delete cipher;

		    throw;
		}
	    }
	    else
	    {
		params.set_kdf_salt("");
		params.write(*stack);
	    }

	    comp = macro_tools_build_streaming_compressor(params.get_compression(),
							  *(stack->top()),
							  params.get_compression_level(),
							  2); // using 2 workers at most
	    if(comp == nullptr)
		throw Ememory("database_header_create");

	    stack->push(comp);
	}
	catch(...)
	{
	    delete stack;
	    throw;
	}

	return stack;
    }

    generic_file *database_header_open(const shared_ptr<user_interaction> & dialog,
				       const string & filename,
				       database_header & params,
				       bool info_details)
    {
	pile *stack = new (nothrow) pile();
	generic_file *tmp = nullptr;

	if(stack == nullptr)
	    throw Ememory("database_header_open");

	try
	{
	    try
	    {
		tmp = new (nothrow) fichier_local(filename, false);
	    }
	    catch(Erange & e)
	    {
		throw Erange("database_header_open", tools_printf(gettext("Error reading database %S : "), &filename) + e.get_message());
	    }
	    if(tmp == nullptr)
		throw Ememory("database_header_open");

	    stack->push(tmp);

	    params.read(*stack);

	    if(params.get_crypto() != crypto_algo::none)
	    {
		tronco_with_elastic* cipher = new (nothrow) tronco_with_elastic(dialog,
										2,
										params.get_crypto_block_size(),
										*(stack->top()),
										params.get_pass(),
										params.get_archive_format(),
										params.get_crypto(),
										params.get_salt(),
										params.get_kdf_iteration(),
										params.get_kdf_hash(),
										true, // activating KDF
										info_details);

		if(cipher == nullptr)
		    throw Ememory("database_header_open");

		try
		{
		    cipher->get_ready_for_reading(database_clear_data_callback,
						  true);

		    stack->push(cipher);
		}
		catch(...)
		{
		    if(cipher != nullptr)
			delete cipher;

		    throw;
		}
	    }

	    tmp = macro_tools_build_streaming_compressor(params.get_compression(),
							 *(stack->top()),
							 params.get_compression_level(), // not used for decompression (here)
							 2); // using 2 workers at most
	    if(tmp == nullptr)
		throw Ememory("database_header_open");

	    stack->push(tmp);
	}
	catch(...)
	{
	    delete stack;
	    throw;
	}

	return stack;
    }

    const unsigned char database_header_get_supported_version()
    {
	return database_version;
    }

    static infinint database_clear_data_callback(generic_file &below, const archive_version & reading_ver)
    {
	if(!below.skip_to_eof())
	    throw Erange("database_clear_data_callback",
			 "Probable data corruption in encrypted database file, aborting");

	    // we return the offset of end of file as we don't
	    // expect clear data after encrypted data when treating
	    // as here a dar_manager database. If we got called back
	    // here, this is very probable due to data corruption in
	    // the database (encrypted) file, as all data should be
	    // possible to decipher at or near end of file.
	return below.get_position();
    }

} // end of namespace
