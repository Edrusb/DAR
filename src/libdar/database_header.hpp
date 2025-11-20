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

    /// \file database_header.hpp
    /// \brief defines the database structure in file
    /// \ingroup Private

#ifndef DATABASE_HEADER_HPP
#define DATABASE_HEADER_HPP

#include "../my_config.h"
#include "generic_file.hpp"
#include "user_interaction.hpp"
#include "compression.hpp"
#include "crypto.hpp"
#include "archive_aux.hpp"
#include "secu_string.hpp"

#include <memory>

namespace libdar
{

    class database_header
    {
    public:
	database_header() { clear(); };
	database_header(const database_header & ref) = default;
	database_header(database_header && ref) noexcept = default;
	database_header & operator = (const database_header & ref) = default;
	database_header & operator = (database_header && ref) noexcept = default;
	~database_header() = default;

	void clear();

	void read(generic_file & f);
	void write(generic_file & f) const;

	void set_compression(compression algozip) { algo = algozip; };
	void set_compression_level(U_I level) { compression_level = level; };
	void set_crypto(crypto_algo val);
	void set_pass(const secu_string & val);
	void set_kdf_hash(hash_algo val);
	void set_kdf_iteration(const infinint & val);
	void set_crypto_block_size(U_32 val);
	void set_kdf_salt(const std::string & val) const { salt = val; };

	U_I get_version() const { return version; };
	compression get_compression() const { return algo; };
	U_I get_compression_level() const { return compression_level; };
	crypto_algo get_crypto() const { return crypto; };
	const secu_string & get_pass() const { return pass; };
	hash_algo get_kdf_hash() const { return kdf_hash; };
	const infinint & get_kdf_iteration() const { return kdf_count; };
	U_32 get_crypto_block_size() const { return crypto_bs; };
	const std::string & get_salt() const { return salt; };
	archive_version get_archive_format() const { return format; };

    private:
	unsigned char version;
	compression algo;
	secu_string pass;
	U_I compression_level;
	crypto_algo crypto;
	hash_algo kdf_hash;
	infinint kdf_count;
	U_32 crypto_bs;
	mutable std::string salt;
	mutable archive_version format; ///< as we share the encryption layer with archive objects we need to store the archive format used

    };

	/// \addtogroup Private
	/// @{

	/// create the header for a dar_manager database

	/// \param[in] dialog is used for user interaction
	/// \param[in] filename is the file's name to create/overwrite
	/// \param[in] overwrite set to true to allow file overwriting (else generates an error if file exists)
	/// \param[in,out] params parameter to use to create the database (compression, encryption and so forth)
	/// \param[in] info_details set to true to get informational messages
	/// \return the database header has been read and checked the database can now be read from the returned generic_file pointed by the returned value
	/// then it must be destroyed with the delete operator.
    extern generic_file *database_header_create(const std::shared_ptr<user_interaction> & dialog,
						const std::string & filename,
						bool overwrite,
						const database_header & params,
						bool info_details);

	/// read the header of a dar_manager database

	/// \param[in] dialog for user interaction
	/// \param[in] filename is the filename to read from
	/// \param[in] pass ignored if empty string, else used as password if the db is encrypted
	/// \param[in,out] params parameters of the openned database (password[in], version[out], compression[out], encryption[out]...)
	/// \param[in] info_details set to true to get informational messages
	/// \return the generic_file where the database header has been put
    extern generic_file *database_header_open(const std::shared_ptr<user_interaction> & dialog,
					      const std::string & filename,
					      const secu_string & pass,
					      database_header & params,
					      bool info_details);

    extern const unsigned char database_header_get_supported_version();

	///@}

} // end of namespace

#endif
