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
// $Id: thread_cancellation.cpp,v 1.1 2004/12/07 18:04:52 edrusb Rel $
//
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

#define CRITICAL_START if(!initialized)  \
             throw Elibcall("thread_cancellation", gettext("Thread-safe not initialized for libdar, read manual or contact maintainer of the application that uses libdar")); \
             pthread_mutex_lock(&access)
#define CRITICAL_END pthread_mutex_unlock(&access)

using namespace std;

namespace libdar
{
#if MUTEX_WORKS
    bool thread_cancellation::cancellation = false;
    pthread_t thread_cancellation::to_cancel;
    pthread_mutex_t thread_cancellation::access;
    bool thread_cancellation::initialized = false;
#endif

    thread_cancellation::thread_cancellation()
    {
#if MUTEX_WORKS
	tid = pthread_self();
#endif
    }

    void thread_cancellation::check_self_cancellation() const
    {
#if MUTEX_WORKS
	bool abort = false;

	CRITICAL_START;
	if(cancellation && tid == to_cancel)
	{
	    abort = true;
	    cancellation = false;
	}
	CRITICAL_END;

	if(abort)
	    throw Euser_abort(string(gettext("Thread canceled by application")));
#endif
    }

    void thread_cancellation::init()
    {
#if MUTEX_WORKS
	if(!initialized)
	{
	    initialized = true;
	    if(pthread_mutex_init(&access, NULL) < 0)
	    {
		initialized = false;
		throw Erange("thread_cancellation::init", string(gettext("Cannot initialize mutex: ")) + strerror(errno));
	    }
	}
#endif
    }

    bool thread_cancellation::cancel(pthread_t tid)
    {
#if MUTEX_WORKS
	bool ret;

	CRITICAL_START;
	if(!cancellation)
	{
	    to_cancel  = tid;
	    cancellation = true;
	    ret = true;
	}
	else
	    ret = false;
	CRITICAL_END;

	return ret;
#else
	throw Ecompilation(gettext("multi-thread support"));
#endif
    }

    static void dummy_call(char *x)
    {
	static char id[]="$Id: thread_cancellation.cpp,v 1.1 2004/12/07 18:04:52 edrusb Rel $";
	dummy_call(id);
    }

    bool thread_cancellation::current_cancel(pthread_t & tid)
    {
#if MUTEX_WORKS
	bool ret;

	CRITICAL_START;
	ret = cancellation;
	if(ret)
	    tid = to_cancel;
	CRITICAL_END;

	return ret;
#else
	throw Ecompilation(gettext("multi-thread support"));
#endif
    }

    bool thread_cancellation::clear_pending_request()
    {
	bool ret = false;
#if MUTEX_WORKS
	CRITICAL_START;
	ret = cancellation;
	cancellation = false;
	CRITICAL_END;
#else
	throw Ecompilation(gettext("multi-thread support"));
#endif
	return ret;
    }

} // end of namespace


