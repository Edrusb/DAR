/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

#if HAVE_STRING_H
#include <string.h>
#endif
}

#include "erreurs.hpp"
#include "user_group_bases.hpp"
#include "tools.hpp"
#include "erreurs.hpp"

using namespace std;

#if MUTEX_WORKS
#define CRITICAL_START						\
    sigset_t Critical_section_mask_memory;			\
    tools_block_all_signals(Critical_section_mask_memory);	\
    pthread_mutex_lock(&lock_fill)

#define CRITICAL_END pthread_mutex_unlock(&lock_fill);			\
    tools_set_back_blocked_signals(Critical_section_mask_memory)
#else
#define CRITICAL_START // not a thread-safe implementation
#define CRITICAL_END   // not a thread-safe implementation
#endif

namespace libdar
{

    const std::string user_group_bases::empty_string = "";
#if MUTEX_WORKS
    pthread_mutex_t user_group_bases::lock_fill = PTHREAD_MUTEX_INITIALIZER;
#endif

    void user_group_bases::fill() const
    {
	if(!filled)
	{
	    user_group_bases *me = const_cast<user_group_bases *>(this);
	    struct passwd *pwd;
	    struct group *grp;

	    CRITICAL_START;

		// filling the user name base

	    if(me == nullptr)
		throw SRC_BUG;

	    setpwent(); // reset password reading

	    while((pwd = getpwent()) != nullptr)
		me->user_database[pwd->pw_uid] = pwd->pw_name;

	    endpwent();

		// filling the group name base

	    setgrent();
	    while((grp = getgrent()) != nullptr)
		me->group_database[grp->gr_gid] = grp->gr_name;

	    endgrent();

	    CRITICAL_END;

	    me->filled = true;
	}
    }


    const string & user_group_bases::get_username(const infinint & uid) const
    {
	map<infinint, string>::const_iterator it;

	fill();
	it = user_database.find(uid);
	if(it != user_database.end())
	    return it->second;
	else
	    return empty_string;
    }

    const string & user_group_bases::get_groupname(const infinint & gid) const
    {
	map<infinint, string>::const_iterator it;

	fill();
	it = group_database.find(gid);
	if(it != group_database.end())
	    return it->second;
	else
	    return empty_string;
    }

} // end of namespace

#endif
