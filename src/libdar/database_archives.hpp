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

    /// \file database_archives.hpp
    /// \brief this file stores the list of archives a database has been built from.
    /// it is part of the database header
    /// \ingroup API


#ifndef DATABASE_ARCHIVE_HPP
#define DATABASE_ARCHIVE_HPP

#include "../my_config.h"

#include <deque>
#include <string>

#include "crypto.hpp"

namespace libdar
{

	/// \addtogroup API
        /// @{

	/// datastructure managing a member of the list of archives used by a database

	/// only two methods are useful from API point of view
	/// - get_path()
        /// - get_basename()
	/// - get_crypto_algo()

    class database_archives
    {
    public:
	database_archives(): algo(crypto_algo::none) {}; // fields "chemin" and "base" are objects and get initialized by the std::string default constructor
	database_archives(const database_archives & ref) = default;
	database_archives(database_archives && ref) noexcept = default;
	database_archives & operator = (const database_archives & ref) = default;
	database_archives & operator = (database_archives && ref) noexcept = default;
	~database_archives() = default;

	void set_path(const std::string & val) { chemin = val; };
	void set_basename(const std::string & val) { base = val; };
	void set_crypto_algo(crypto_algo val) { algo = val; };

	    /// this provides the path where is located this archive
	const std::string & get_path() const { return chemin; };

	    /// this provides the basename of the archive
	const std::string & get_basename() const { return base; };

	    /// this provides the crypto algo of the archive
	crypto_algo get_crypto_algo() const { return algo; };

    private:
	std::string chemin;
	std::string base;
	crypto_algo algo;
    };

	/// list of archives found in a database

	/// \note index 0 is not used, list starts at index 1
    using database_archives_list = std::deque<database_archives>;


	/// @}

} // end of namespace

#endif
