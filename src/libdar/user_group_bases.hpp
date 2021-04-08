/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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

    /// \file user_group_bases.hpp
    /// \brief defines class that speed up the uid to username and gid to group name lookup
    /// \ingroup Private

#ifndef USER_GROUP_BASES_HPP
#define USER_GROUP_BASES_HPP

#include "../my_config.h"

#ifdef __DYNAMIC__

extern "C"
{
#if MUTEX_WORKS
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#endif
}

#include <map>
#include <string>
#include "infinint.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class user_group_bases
    {
    public:
	user_group_bases() : filled(false) {};
	user_group_bases(const user_group_bases & ref) = default;
	user_group_bases(user_group_bases && ref) noexcept = default;
	user_group_bases & operator = (const user_group_bases & ref) = default;
	user_group_bases & operator = (user_group_bases && ref) noexcept = default;
	~user_group_bases() = default;

	    /// return the user name corresponding to the given uid

	    /// \note if the entry is not known, returns an empty string
	const std::string & get_username(const infinint & uid) const;


	    /// return the group name corresponding to the given gid

	    /// \note if the entry is not known, returns an empty string
	const std::string & get_groupname(const infinint & gid) const;


    private:
	bool filled;
	std::map<infinint, std::string> user_database;
	std::map<infinint, std::string> group_database;

	void fill() const;

	static const std::string empty_string;
#if MUTEX_WORKS
	    /// mutex to access the operating system information

	    /// the system call used to read the password and group database are not re-entrant, the mutex
	    /// must block all other thread while a given thread is reading the databases
	    /// the best solution would have to let the user provide such a database object already filled
	    /// but that would rely on it not to destroy this object while threads are using it
	    /// for this reason, here the mutex is 'static'
	static pthread_mutex_t lock_fill;
#endif
    };


	/// @}

}  // end of namespace

#endif

#endif
