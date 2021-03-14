//*********************************************************************/
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif
}

#include <string>

#include "statistics.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    void statistics::clear()
    {
	if(locking)
	{
	    LOCK_IN;
	    treated = hard_links = skipped = inode_only = ignored = tooold = errored = deleted = ea_treated = byte_amount = fsa_treated = 0;
	    LOCK_OUT;
	}
	else
	    treated = hard_links = skipped = inode_only = ignored = tooold = errored = deleted = ea_treated = byte_amount = fsa_treated = 0;
    }

    infinint statistics::total() const
    {
	infinint ret;

	if(locking)
	{
	    LOCK_IN_CONST;
	    ret = treated+skipped+inode_only+ignored+tooold+errored+deleted;
		// hard_link are also counted in other counters
	    LOCK_OUT_CONST;
	}
	else
	    ret = treated+skipped+inode_only+ignored+tooold+errored+deleted;

	return ret;
    }

    void statistics::init(bool lock)
    {
	locking = lock;

#if MUTEX_WORKS
	if(locking)
	    if(pthread_mutex_init(&lock_mutex, nullptr) < 0)
		throw Erange("statistics::statistics", string(dar_gettext("Error while initializing \"mutex\" for class \"statistics\": ")) + tools_strerror_r(errno));
#else
	if(locking)
	    throw Ecompilation("Thread support not activated, cannot use statistics object with lock activated");
#endif
	if(locking)
	{
	    increment = & statistics::increment_locked;
	    add_to = & statistics::add_to_locked;
	    returned = & statistics::returned_locked;
	    decrement = & statistics::decrement_locked;
	    set_to = & statistics::set_to_locked;
	    sub_from = & statistics::sub_from_locked;
	}
	else
	{
	    increment = & statistics::increment_unlocked;
	    add_to = & statistics::add_to_unlocked;
	    returned = & statistics::returned_unlocked;
	    decrement = & statistics::decrement_unlocked;
	    set_to = & statistics::set_to_unlocked;
	    sub_from = & statistics::sub_from_unlocked;
	}
    }


    void statistics::detruit()
    {
#if MUTEX_WORKS
	if(locking)
	    pthread_mutex_destroy(&lock_mutex);
#endif
    }

    void statistics::copy_from(const statistics & ref)
    {
	init(ref.locking);

	treated = ref.treated;
	hard_links = ref.hard_links;
	skipped = ref.skipped;
	inode_only = ref.inode_only;
	ignored = ref.ignored;
	tooold = ref.tooold;
	errored = ref.errored;
	deleted = ref.deleted;
	ea_treated = ref.ea_treated;
	byte_amount = ref.byte_amount;
	fsa_treated = ref.fsa_treated;
    }

    void statistics::move_from(statistics && ref) noexcept
    {
#if MUTEX_WORKS
	swap(lock_mutex, ref.lock_mutex);
#endif
	swap(locking, ref.locking);
	treated = move(ref.treated);
	hard_links = move(ref.hard_links);
	skipped = move(ref.skipped);
	inode_only = move(ref.inode_only);
	ignored = move(ref.ignored);
	tooold = move(ref.tooold);
	errored = move(ref.errored);
	deleted = move(ref.deleted);
	ea_treated = move(ref.ea_treated);
	byte_amount = move(ref.byte_amount);
	fsa_treated = move(ref.fsa_treated);
    }

    void statistics::dump(user_interaction & dialog) const
    {
	dialog.printf("--------- Statistics DUMP ----------");
	dialog.printf("locking = %c", locking ? 'y' : 'n');
	dialog.printf("treated = %i", &treated);
	dialog.printf("hard_links = %i", &hard_links);
	dialog.printf("skipped = %i", &skipped);
	dialog.printf("inode only = %i", &inode_only);
	dialog.printf("ignored = %i", &ignored);
	dialog.printf("tooold = %i", &tooold);
	dialog.printf("errored = %i", &errored);
	dialog.printf("deleted = %i", &deleted);
	dialog.printf("ea_treated = %i", &ea_treated);
	dialog.printf("byte_amount = %i", &byte_amount);
	dialog.printf("fsa_treated = %i", &fsa_treated);
	dialog.printf("------------------------------------");
    }


} // end of namespace
