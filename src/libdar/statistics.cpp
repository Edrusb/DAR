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
// $Id: statistics.cpp,v 1.5.2.2 2006/02/04 14:47:15 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"

extern "C"
{
#if HAVE_ERRNO_H
#include <errno.h>
#endif
}

#include <string>

#include "statistics.hpp"

using namespace std;

namespace libdar
{

    void statistics::clear()
    {
	if(locking)
	{
	    LOCK_IN;
	    treated = hard_links = skipped = ignored = tooold = errored = deleted = ea_treated = 0;
	    LOCK_OUT;
	}
	else
	    treated = hard_links = skipped = ignored = tooold = errored = deleted = ea_treated = 0;
    }

    infinint statistics::total() const
    {
	infinint ret;

	if(locking)
	{
	    LOCK_IN_CONST;
	    ret = treated+skipped+ignored+tooold+errored+deleted;
		// hard_link are also counted in other counters
	    LOCK_OUT_CONST;
	}
	else
	    ret = treated+skipped+ignored+tooold+errored+deleted;

	return ret;
    }

    void statistics::init(bool lock)
    {
	locking = lock;

#if MUTEX_WORKS
	if(locking)
	    if(pthread_mutex_init(&lock_mutex, NULL) < 0)
		throw Erange("statistics::statistics", string(gettext("Error while initializing mutex for class statistics: ")) + strerror(errno));
#else
	if(locking)
	    throw Ecompilation("Thread support not activated, cannot use statistics object with lock activated");
#endif
	if(locking)
	{
	    increment = & statistics::increment_locked;
	    add_to = & statistics::add_to_locked;
	    returned = & statistics::returned_locked;
	}
	else
	{
	    increment = & statistics::increment_unlocked;
	    add_to = & statistics::add_to_unlocked;
	    returned = & statistics::returned_unlocked;
	}
    }


    void statistics::detruit()
    {
#if MUTEX_WORKS
	if(locking)
	    pthread_mutex_destroy(&lock_mutex);
#endif
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: statistics.cpp,v 1.5.2.2 2006/02/04 14:47:15 edrusb Rel $";
        dummy_call(id);
    }

    void statistics::copy_from(const statistics & ref)
    {
	init(ref.locking);

	treated = ref.treated;
	hard_links = ref.hard_links;
	skipped = ref.skipped;
	ignored = ref.ignored;
	tooold = ref.tooold;
	errored = ref.errored;
	deleted = ref.deleted;
	ea_treated = ref.ea_treated;
    }


} // end of namespace
