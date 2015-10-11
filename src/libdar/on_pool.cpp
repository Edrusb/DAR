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

#include "../my_config.h"

#include "erreurs.hpp"
#include "on_pool.hpp"

using namespace std;

namespace libdar
{
#ifdef LIBDAR_SPECIAL_ALLOC
#ifdef CYGWIN_BUILD
    on_pool *on_pool::alloc_not_constructed = nullptr;
#else
    thread_local on_pool *on_pool::alloc_not_constructed = nullptr;
#endif
#endif

    void *on_pool::alloc(size_t size, memory_pool *p)
    {
#ifdef LIBDAR_SPECIAL_ALLOC

	pool_ptr *tmp = nullptr;

	size += sizeof(pool_ptr);
	if(p != nullptr)
	    tmp = (pool_ptr *)p->alloc(size);
	else
	    tmp = (pool_ptr *)(::new (nothrow) char[size]);
	if(tmp != nullptr)
	{
	    tmp->reserve = p;
	    ++tmp;
	}

	alloc_not_constructed = (on_pool *)tmp;
	return tmp;
#else
	return ::new (nothrow) char[size];
#endif
    }

    void on_pool::dealloc(void *ptr)
    {
#ifdef LIBDAR_SPECIAL_ALLOC
	if(ptr == nullptr)
	    throw SRC_BUG;
	else
	{
	    pool_ptr *tmp = ((pool_ptr *)ptr) - 1;

	    if(tmp->reserve == nullptr)
		::delete[] tmp;
	    else
		tmp->reserve->release((void *)tmp);
	}
#else
	::delete[] (char *)(ptr);
#endif
    }

}  // end of namespace
