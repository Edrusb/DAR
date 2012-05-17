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

#include "../my_config.h"
#include "special_alloc.hpp"
#include "erreurs.hpp"
#include "user_interaction.hpp"
#include "tools.hpp"

#include <list>
#include <iostream>
#include <errno.h>

#ifdef LIBDAR_SPECIAL_ALLOC

#ifdef MUTEX_WORKS
extern "C"
{
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif
}
static bool alloc_mutex_initialized = false;
static pthread_mutex_t alloc_mutex;

#define CRITICAL_START if(!alloc_mutex_initialized)                         \
             throw Elibcall("alloc_mutex_initialized", gettext("Thread-safe not initialized for libdar, read manual or contact maintainer of the application that uses libdar"));\
             sigset_t Critical_section_mem_mask_memory;                     \
             tools_block_all_signals(Critical_section_mem_mask_memory);     \
             pthread_mutex_lock(&alloc_mutex)

#define CRITICAL_END pthread_mutex_unlock(&alloc_mutex);                    \
             tools_set_back_blocked_signals(Critical_section_mem_mask_memory)


#else // MUTEX does not work or is not available
#define CRITICAL_START //
#define CRITICAL_END   //
#endif // MUTEX_WORKS

using namespace std;

namespace libdar
{

    const unsigned long CHUNK_SIZE = 1048576; // must stay greater than storage.cpp:storage::alloc_size


    struct segment
    {
        char *alloc;
        char *ptr;
        unsigned long reste;
        unsigned long ref;
    };

    static list<struct segment> alloc;

    void *special_alloc_new(size_t taille)
    {
        void *ret = NULL;

	CRITICAL_START;

	try
	{

	    if(alloc.empty() || alloc.back().reste < taille)
	    {
		struct segment seg;

		seg.alloc = ::new char[CHUNK_SIZE];
		if(seg.alloc == NULL)
		    throw Ememory("special_alloc_new");
		seg.ptr = seg.alloc;
		seg.reste = CHUNK_SIZE;
		seg.ref = 0;

		alloc.push_back(seg);

		if(alloc.empty())
		    throw SRC_BUG;

		if(alloc.back().reste < taille)
		{
		    cerr << "Requested chunk = "<< taille << endl;
		    throw SRC_BUG;
		}
	    }

	    ret = alloc.back().ptr;
	    alloc.back().ptr += taille;
	    alloc.back().reste -= taille;
	    alloc.back().ref++;
	}
	catch(...)
	{
	    CRITICAL_END;
	    throw;
	}
	CRITICAL_END;

        return ret;
    }

    void special_alloc_delete(void *ptr)
    {
        list<struct segment>::iterator rit;

	CRITICAL_START;

	try
	{
	    rit = alloc.begin();
	    while(rit != alloc.end() && (ptr < rit->alloc || ptr >= rit->alloc+CHUNK_SIZE))
		++rit;

	    if(rit != alloc.end())
	    {
		(rit->ref)--;
		if(rit->ref == 0)
		{
		    ::delete [] rit->alloc;
		    alloc.erase(rit);
		}
	    }
	    else
		throw SRC_BUG; // unknown memory, not allocated here !?
	}
	catch(...)
	{
	    CRITICAL_END;
	    throw;
	}

	CRITICAL_END;
    }

    void special_alloc_init_for_thread_safe()
    {
#ifdef MUTEX_WORKS
	if(alloc_mutex_initialized)
	    throw SRC_BUG; // should not be initialized twice
	alloc_mutex_initialized = true; // we must not be in multi-thread
	    // environment at that time.

	if(pthread_mutex_init(&alloc_mutex, NULL) < 0)
	{
	    alloc_mutex_initialized = false;
	    throw Erange("special_alloca_init_for_thread_safe", string(gettext("Cannot initialize mutex: ")) + strerror(errno));
	}
#endif
    }
} // end of namespace

#endif

