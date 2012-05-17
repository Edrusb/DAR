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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: user_group_bases.cpp,v 1.1.2.1 2007/07/24 16:29:32 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

#ifdef __DYNAMIC__

extern "C"
{
#if HAVE_SYS_TYPE_H
#include <sys/types.h>
#endif

#if HAVE_PWD_H
#include <pwd.h>
#endif

#if HAVE_GRP_H
#include <grp.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif
}

#include "erreurs.hpp"
#include "user_group_bases.hpp"
#include "tools.hpp"
#include "erreurs.hpp"

using namespace std;

#if MUTEX_WORKS
#define CRITICAL_START if(!class_initialized)                                     \
             throw Elibcall("user_group_bases", gettext("Thread-safe not initialized for libdar, read manual or contact maintainer of the application that uses libdar")); \
             sigset_t Critical_section_mask_memory;                         \
             tools_block_all_signals(Critical_section_mask_memory);         \
             pthread_mutex_lock(&lock_fill)

#define CRITICAL_END pthread_mutex_unlock(&lock_fill);                         \
             tools_set_back_blocked_signals(Critical_section_mask_memory)
#else
#define CRITICAL_START // not a thread-safe implementation
#define CRITICAL_END   // not a thread-safe implementation
#endif

namespace libdar
{

    const std::string user_group_bases::empty_string = "";
    bool user_group_bases::class_initialized = false;
    pthread_mutex_t user_group_bases::lock_fill;

    void user_group_bases::fill() const
    {
	if(!filled)
	{
	    user_group_bases *me = const_cast<user_group_bases *>(this);
	    struct passwd *pwd;
	    struct group *grp;

	    CRITICAL_START;

		// filling the user name base

	    if(me == NULL)
		throw SRC_BUG;

	    setpwent(); // reset password reading

	    while((pwd = getpwent()) != NULL)
		me->user_database[pwd->pw_uid] = pwd->pw_name;

	    endpwent();

		// filling the group name base

	    setgrent();
	    while((grp = getgrent()) != NULL)
		me->group_database[grp->gr_gid] = grp->gr_name;

	    endgrent();

	    CRITICAL_END;

	    me->filled = true;
	}
    }


    const string & user_group_bases::get_username(const infinint & uid) const
    {
	map<infinint, string>::const_iterator it = user_database.find(uid);

	fill();
	if(it != user_database.end())
	    return it->second;
	else
	    return empty_string;
    }


    const string & user_group_bases::get_groupname(const infinint & gid) const
    {
	map<infinint, string>::const_iterator it = group_database.find(gid);

	fill();
	if(it != group_database.end())
	    return it->second;
	else
	    return empty_string;
    }

    void user_group_bases::class_init()
    {
#if MUTEX_WORKS
	if(!class_initialized)
	    if(pthread_mutex_init(&lock_fill, NULL) < 0)
		throw Erange("user_group_bases::class_init", string(gettext("Cannot initialize mutex: ")) + strerror(errno));
#endif
	class_initialized = true;
    }

} // end of namespace

#endif

