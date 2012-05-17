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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: user_group_bases.hpp,v 1.1.2.1 2007/07/24 16:29:32 edrusb Rel $
//
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

    class user_group_bases
    {
    public:
	user_group_bases() : filled(false) {};

	    /// return the user name corresponding to the given uid

	    /// \note if the entry is not known, returns an empty string
	const std::string & get_username(const infinint & uid) const;


	    /// return the group name corresponding to the given gid

	    /// \note if the entry is not known, returns an empty string
	const std::string & get_groupname(const infinint & gid) const;

	    /// initialize mutex for the class, to be called only once for all and before any object instantiation

	static void class_init();

    private:
	bool filled;
	std::map<infinint, std::string> user_database;
	std::map<infinint, std::string> group_database;

	void fill() const;

	static const std::string empty_string;
#if MUTEX_WORKS
	    // the system call used to read the password and group database are not re-entrant, the mutex
	    // must block all other thread while a given thread is reading the databases
	    // the best solution would have to let the user provide such a database object already filled
	    // but that would rely on it not to destroy this object while threads are using it
	    // for this reason, here the mutex is 'static'
	static pthread_mutex_t lock_fill;
#endif
	static bool class_initialized;

    };

}  // end of namespace

#endif

#endif
