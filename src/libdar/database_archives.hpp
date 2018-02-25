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

    /// \file database_archives.hpp
    /// \brief this file stores the list of archive a database has been built from
    /// \ingroup API


#ifndef DATABASE_ARCHIVE_HPP
#define DATABASE_ARCHIVE_HPP

#include "../my_config.h"

#include <deque>
#include <string>

namespace libdar
{
    class database_archives
    {
    public:
	database_archives() = default;
	database_archives(const database_archives & ref) = default;
	database_archives(database_archives && ref) noexcept = default;
	database_archives & operator = (const database_archives & ref) = default;
	database_archives & operator = (database_archives && ref) noexcept = default;
	~database_archives() = default;

	void set_path(const std::string & val) { chemin = val; };
	void set_basename(const std::string & val) { base = val; };

	const std::string & get_path() const { return chemin; };
	const std::string & get_basename() const { return base; };

    private:
	std::string chemin;
	std::string base;
    };

	// index 0 is not used, list starts at index 1
    using database_archives_list = std::deque<database_archives>;

} // end of namespace

#endif
