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

    class database_header
    {
    public:
	database_header() { version = database_version; algo = compression::gzip; compression_level = 9; };
	database_header(const database_header & ref) = default;
	database_header(database_header && ref) noexcept = default;
	database_header & operator = (const database_header & ref) = default;
	database_header & operator = (database_header && ref) noexcept = default;
	~database_header() = default;

	void read(generic_file & f);
	void write(generic_file & f);

	void set_compression(compression algozip, U_I level);

	void set_crypto(crypto_algo val) { crypto = val; };
	void set_kdf_hash(hash_algo val) { kdf_hash = val; };
	void set_kdf_interation(const infinint & val) { kdf_count = val; };
	void set_crypto_block_size(U_32 val) { crypto_bs = val; };
	void set_kdf_salt(const std::string & val) { salt = val; };

	U_I get_version() const { return version; };
	compression get_compression() const { return algo; };
	U_I get_compression_level() const { return compression_level; };
	crypto_algo get_crypto_algo() const { return crypto; };
	hash_algo get_kdf_hash() const { return kdf_hash; };
	const infinint & get_kdf_interation() const { return kdf_count; };
	U_32 get_crypto_block_size() const { return crypto_bs; };
	const std::string & get_salt() const { return salt; };

    private:
	unsigned char version;
	compression algo;
	U_I compression_level;
	crypto_algo crypto;
	hash_algo kdf_hash;
	infinint kdf_count;
	U_32 crypto_bs;
	std::string salt;
    };

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

    void database_header::write(generic_file & f)
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
	    infinint(compression_level).dump(f);
	}
	if(crypto != crypto_algo::none)
	{
	    char tmp;
	    infinint itmp;

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

    void database_header::set_compression(compression algozip, U_I level)
    {
	algo = algozip;
	compression_level = level;
    }

    generic_file *database_header_create(const shared_ptr<user_interaction> & dialog,
					 const string & filename,
					 bool overwrite,
					 compression algozip,
					 U_I compr_level)
    {
	pile* stack = new (nothrow) pile();
	generic_file *tmp = nullptr;

	struct stat buf;
	database_header h;
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

	    h.set_compression(algozip, compr_level);
	    h.write(*stack);

	    comp = macro_tools_build_streaming_compressor(algozip,
							  *(stack->top()),
							  compr_level,
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
				       unsigned char & db_version,
				       compression & algozip,
				       U_I & compr_level)
    {
	pile *stack = new (nothrow) pile();
	generic_file *tmp = nullptr;

	if(stack == nullptr)
	    throw Ememory("database_header_open");

	try
	{
	    database_header h;

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

	    h.read(*stack);
	    db_version = h.get_version();
	    algozip = h.get_compression();
	    compr_level = h.get_compression_level();

	    tmp = macro_tools_build_streaming_compressor(algozip,
							 *(stack->top()),
							 compr_level, // not used for decompression (here)
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

    extern const unsigned char database_header_get_supported_version()
    {
	return database_version;
    }

} // end of namespace
