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

extern "C"
{
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif
}

#include "erreurs.hpp"
#include "thread_cancellation.hpp"
#include "tools.hpp"

#define CRITICAL_START						\
    sigset_t Critical_section_mask_memory;			\
    tools_block_all_signals(Critical_section_mask_memory);	\
    pthread_mutex_lock(&access)

#define CRITICAL_END pthread_mutex_unlock(&access);			\
    tools_set_back_blocked_signals(Critical_section_mask_memory)

using namespace std;

namespace libdar
{

	// class static variables
#if MUTEX_WORKS
    pthread_mutex_t thread_cancellation::access = PTHREAD_MUTEX_INITIALIZER;
    list<thread_cancellation *> thread_cancellation::info;
    list<thread_cancellation::fields> thread_cancellation::preborn;
    multimap<pthread_t, pthread_t> thread_cancellation::thread_asso;
#endif

    thread_cancellation::thread_cancellation()
    {
#if MUTEX_WORKS
	bool bug = false;

	status.tid = pthread_self();
	list<thread_cancellation *>::iterator ptr;

	CRITICAL_START;
	ptr = info.begin();
	while(ptr != info.end() && *ptr != nullptr && (*ptr)->status.tid != status.tid)
	    ptr++;
	if(ptr == info.end()) // first object in that thread
	{
	    list<fields>::iterator it = preborn.begin();
	    while(it != preborn.end() && it->tid != status.tid)
		it++;
	    if(it == preborn.end()) // no pending cancellation for that thread
	    {
		status.block_delayed = false;
		status.immediate = true;
		status.cancellation = false;
		status.flag = 0;
	    }
	    else // pending cancellation for that thread
	    {
		status = *it;
		preborn.erase(it);
	    }
	}
	else  // an object already exist for that thread
	    if(*ptr == nullptr) // bug
		bug = true;
	    else  // an object already exists for that thread
		status = (*ptr)->status;

	if(!bug)
	    info.push_back(this);
	CRITICAL_END;

	if(bug)
	    throw SRC_BUG;
#endif
    }

    thread_cancellation::~thread_cancellation() noexcept(false)
    {
#if MUTEX_WORKS
	list<thread_cancellation *>::iterator ptr;
	bool bug = false;

	CRITICAL_START;
	ptr = info.begin();
	while(ptr != info.end() && *ptr != this)
	    ptr++;
	if(ptr == info.end())
	    bug = true;
	else
	    if(*ptr == nullptr)
		bug = true;
	    else
	    {
		if((*ptr)->status.cancellation) // cancellation for that thread
		    preborn.push_back((*ptr)->status);
		info.erase(ptr);
	    }
	CRITICAL_END;

	if(bug)
	    throw SRC_BUG;
#endif
    }

    void thread_cancellation::check_self_cancellation() const
    {
#if MUTEX_WORKS
	if(status.cancellation && (status.immediate || !status.block_delayed))
	{
	    (void)clear_pending_request(status.tid); // avoid other object of that thread to throw exception
	    throw Ethread_cancel(status.immediate, status.flag); // we can throw the exception now
	}
#endif
    }

    void thread_cancellation::block_delayed_cancellation(bool mode)
    {
#if MUTEX_WORKS
	list<thread_cancellation *>::iterator ptr;

	    // we update all object of the current thread
	CRITICAL_START;
	ptr = info.begin();
	while(ptr != info.end())
	{
	    if(*ptr == nullptr)
		throw SRC_BUG;
	    if((*ptr)->status.tid == status.tid)
		(*ptr)->status.block_delayed = mode;
	    ptr++;
	}
	CRITICAL_END;

	if(status.block_delayed != mode)
	    throw SRC_BUG;
	if(!mode)
	    check_self_cancellation();
#endif
    }


#if MUTEX_WORKS
    void thread_cancellation::cancel(pthread_t tid, bool x_immediate, U_64 x_flag)
    {
	bool found = false, bug = false, notused = false;
	multimap<pthread_t, pthread_t>::iterator debut;
	multimap<pthread_t, pthread_t>::iterator fin;

	CRITICAL_START;
	set_cancellation_in_info_for(tid, true, x_immediate, x_flag, found, notused, bug);

	if(!found && !bug)  // no thread_cancellation object exist for that thread
	    add_to_preborn(tid, x_immediate, x_flag);

	find_asso_tid_with(tid,
			   debut,
			   fin);
	while(debut != fin && !bug)
	{
	    set_cancellation_in_info_for(debut->second, true, x_immediate, x_flag, found, notused, bug);
	    if(!found && !bug)
		add_to_preborn(debut->second, x_immediate, x_flag);
	    ++debut;
	}
	CRITICAL_END;

	if(bug)
	    throw SRC_BUG;
    }
#endif

#if MUTEX_WORKS
    bool thread_cancellation::cancel_status(pthread_t tid)
    {
	bool ret, bug = false;
	list<thread_cancellation *>::iterator ptr;

	CRITICAL_START;
	ptr = info.begin();
	while(ptr != info.end() && (*ptr) != nullptr && (*ptr)->status.tid != tid)
	    ptr++;
	if(ptr == info.end())
	{
	    list<fields>::iterator it = preborn.begin();
	    while(it != preborn.end() && it->tid != tid)
		it++;

	    if(it == preborn.end())
		ret = false;
	    else
		ret = it->cancellation;
	}
	else
	    if(*ptr == nullptr)
		bug = true;
	    else
		ret = (*ptr)->status.cancellation;
	CRITICAL_END;

	if(bug)
	    throw SRC_BUG;

	return ret;
    }

    bool thread_cancellation::clear_pending_request(pthread_t tid)
    {
	bool ret = false, bug = false, found = false;
	multimap<pthread_t, pthread_t>::iterator debut;
	multimap<pthread_t, pthread_t>::iterator fin;

	CRITICAL_START;
	set_cancellation_in_info_for(tid, false, false, 0, found, ret, bug);
	if(!found && !bug)
	    remove_from_preborn(tid, found, ret);

	find_asso_tid_with(tid,
			   debut,
			   fin);

	while(debut != fin && !bug)
	{
	    set_cancellation_in_info_for(debut->second, false, false, 0, found, ret, bug);
	    if(!found && !bug)
		remove_from_preborn(debut->second, found, ret);
	    ++debut;
	}
	CRITICAL_END;

	if(bug)
	    throw SRC_BUG;

	return ret;
    }

    void thread_cancellation::associate_tid_to_tid(pthread_t src, pthread_t dst)
    {
	CRITICAL_START;
	thread_asso.insert(pair<pthread_t, pthread_t>(src,dst));
	CRITICAL_END;
    }

    void thread_cancellation::remove_association_for_tid(pthread_t src)
    {
	CRITICAL_START;
	thread_asso.erase(src);
	CRITICAL_END;
    }

    void thread_cancellation::remove_association_targeted_at(pthread_t dst)
    {
	CRITICAL_START;
	multimap<pthread_t, pthread_t>::iterator it = thread_asso.begin();
	multimap<pthread_t, pthread_t>::iterator next = it;

	while(it != thread_asso.end())
	{
	    if(it->second == dst)
	    {
		next = it;
		++next;
		thread_asso.erase(it);
		it = next;
	    }
	    else
		++it;
	}

	CRITICAL_END;
    }

    void thread_cancellation::dead_thread(pthread_t tid)
    {
	bool found, prev;
	remove_association_for_tid(tid);
	remove_association_targeted_at(tid);
	remove_from_preborn(tid, found, prev);
    }

    void thread_cancellation::set_cancellation_in_info_for(pthread_t tid,
							   bool cancel_status,
							   bool x_immediate,
							   U_64 x_flag,
							   bool & found,
							   bool & previous_val,
							   bool & bug)
    {
	list<thread_cancellation *>::iterator ptr = info.begin();

	found = false;
	bug = false;
	while(ptr != info.end() && !bug)
	{
	    if(*ptr == nullptr)
		bug = true;
	    else
		if((*ptr)->status.tid == tid)
		{
		    found = true;
		    (*ptr)->status.immediate = x_immediate;
		    previous_val = (*ptr)->status.cancellation;
		    (*ptr)->status.cancellation = cancel_status;
		    (*ptr)->status.flag = x_flag;
		}
	    ptr++;
	}
    }

    void thread_cancellation::add_to_preborn(pthread_t tid, bool x_immediate, U_64 x_flag)
    {
	list<fields>::iterator it = preborn.begin();
	fields tmp;

	tmp.tid = tid;
	tmp.block_delayed = false;
	tmp.immediate = x_immediate;
	tmp.cancellation = true;
	tmp.flag = x_flag;

	while(it != preborn.end() && it->tid != tid)
	    it++;

	if(it != preborn.end())
	    *it = tmp;
	else
	    preborn.push_back(tmp);
    }

    void thread_cancellation::remove_from_preborn(pthread_t tid, bool & found, bool & prev)
    {
	list<fields>::iterator it = preborn.begin();
	found = false;

	while(it != preborn.end())
	{
	    if(it->tid == tid)
	    {
		found = true;
		prev = it->cancellation;
		preborn.erase(it);
		it = preborn.begin();
	    }
	    else
		it++;
	}
    }

    void thread_cancellation::find_asso_tid_with(pthread_t tid,
						 multimap<pthread_t, pthread_t>::iterator & debut,
						 multimap<pthread_t, pthread_t>::iterator & fin)
    {
	pair< multimap<pthread_t, pthread_t>::iterator, multimap<pthread_t, pthread_t>::iterator > tmp = thread_asso.equal_range(tid);
	debut = tmp.first;
	fin = tmp.second;
    }

#endif

} // end of namespace
