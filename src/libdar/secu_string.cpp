//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2026 Denis Corbin
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

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif


#if HAVE_GCRYPT_H
#ifndef GCRYPT_NO_DEPRECATED
#define GCRYPT_NO_DEPRECATED
#endif
#include <gcrypt.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
}

#include "erreurs.hpp"
#include "secu_string.hpp"
#include "tools.hpp"


using namespace std;

namespace libdar
{

    bool secu_string::is_string_secured()
    {
#if CRYPTO_AVAILABLE
	return true;
#else
	return false;
#endif
    }

    bool secu_string::operator == (const string &ref) const
    {
	return compare_with(ref.c_str(),(U_I)(ref.size()));
    }

    bool secu_string::operator == (const secu_string &ref) const
    {
	if(ptr == nullptr)
	    if(ref.ptr == nullptr)
		return true;
	    else
		return ref.ptr->string_size == 0;
	else
	    if(ref.ptr == nullptr)
		return ptr->string_size == 0;
	    else
		return compare_with(&(ref.ptr->mem), ref.ptr->string_size);
    }


    void secu_string::set(int fd, U_I size)
    {
	if(size > get_allocated_size())
	{
	    clean_and_destroy();
	    init(size);
	}
	else
	    clear();

	append_at(0, fd, size);
    }

    void secu_string::append_at(U_I offset, const char *a, U_I size)
    {
	if(size == 0)
	    return; // nothing to append

	if(ptr == nullptr || offset > ptr->string_size)
	    throw Erange("secu_string::append", gettext("appending data after the end of a secured memory"));

	    // now we know that ptr != nullptr, else exception would have been thrown above

	if(size + offset > get_allocated_size())
	    throw Erange("secu_string::append", gettext("Cannot receive that much data in regard to the allocated memory"));

	(void)memcpy(&(ptr->mem) + offset, a, size);
	offset += size;
	if(offset > ptr->string_size)
	    ptr->string_size = offset;
	get_array()[ptr->string_size] = '\0';
    }

    void secu_string::append_at(U_I offset, int fd, U_I size)
    {
	U_I lu = 0;
	S_I tmp = 0;

	if(size == 0)
	    return; // nothing to append

	if(ptr == nullptr || offset > ptr->string_size)
	    throw Erange("secu_string::append", gettext("appending data after the end of a secured memory"));

	    // now we know that ptr != nullptr, else exception would have been thrown above

	if(size + offset > get_allocated_size())
	    throw Erange("secu_string::append", gettext("Cannot receive that much data in regard to the allocated memory"));

	do
	{
	    tmp = ::read(fd, &(ptr->mem) + offset + lu, size - lu);
	    if(tmp > 0)
		lu += tmp;
	}
	while(tmp > 0 && lu < size);

	offset += lu;
	if(offset > ptr->string_size)
	    ptr->string_size = offset;
	get_array()[ptr->string_size] = '\0';
	if(tmp < 0) // if tmp == 0 we reached eof, no issue thus.
	    throw Erange("secu_string::read", string(gettext("Error while reading data for a secure memory:" )) + tools_strerror_r(errno));
    }

    void secu_string::reduce_string_size_to(U_I pos)
    {
	if(ptr == nullptr || pos > ptr->string_size)
	    throw Erange("secu_string::reduce_string_size_to", gettext("Cannot reduce the string to a size that is larger than its current size"));

	ptr->string_size = pos;
	get_array()[ptr->string_size] = '\0';
    }

    void secu_string::expand_string_size_to(U_I size)
    {
	if(size == 0)
	    return; // nothing to expand

	if(ptr == nullptr || size > get_allocated_size())
	    throw Erange("secu_string::expand_string_size_to", gettext("Cannot expand secu_string size past its allocation size"));
	if(size < ptr->string_size)
	    throw Erange("secu_stering::expand_string_size_to", gettext("asking to expand no to shrink a secu_string"));

	memset(get_array() + ptr->string_size, '\0', size - ptr->string_size);
	ptr->string_size = size;
    }

    void secu_string::randomize(U_I size)
    {
#if CRYPTO_AVAILABLE
	if(ptr != nullptr)
	{
	    set_size(size);
	    gcry_randomize(&(ptr->mem), ptr->string_size, GCRY_STRONG_RANDOM);
	}
#else
	throw Efeature("string randomization lacks libgcrypt");
#endif
    }

    void secu_string::set_size(U_I size)
    {
	if(ptr == nullptr)
	{
	    if(size == 0)
		return; // nothing to do
	}

	if(size > get_allocated_size())
	    throw Erange("secu_string::set_size", gettext("exceeding storage capacity while requesting secu_string::set_size()"));
	if(ptr == nullptr)
	    throw SRC_BUG;
	    // if ptr is nullptr, get_allocated_size() returns zero
	    // thus size > get_allocated_size() is true and we should
	    // not reach this statement

	ptr->string_size = size;
    }

    char* secu_string::get_array()
    {
	if(ptr == nullptr)
	    throw SRC_BUG;
	else
	    return &(ptr->mem);
    }

    char & secu_string::operator[] (U_I index)
    {
	if(ptr != nullptr && index < get_size())
	    return get_array()[index];
	else
	    throw Erange("secu_string::operator[]", gettext("Out of range index requested for a secu_string"));
    }

    void secu_string::init(U_I size)
    {
	    // objective of this call is
	    // to set ptr, thus the current
	    // value is undefined (random value)
	    // and no allocated memory has been done

	U_I alloc_size = sizeof(allocated) + size; // +1 char (allocated.mem) for the terminal '\0' of ptr->mem

	if(size == 0)
	{
	    clean_and_destroy();
	    return;
	}

	try
	{
	    if(get_allocated_size() != size)
	    {
#if CRYPTO_AVAILABLE
		ptr = (allocated*)gcry_malloc_secure(alloc_size);
		if(ptr == nullptr)
		    throw Esecu_memory("secu_string::secus_string");
#else
		ptr = (allocated*)(new (nothrow) char[alloc_size]);
		if(ptr == nullptr)
		    throw Ememory("secu_string::secus_string");
#endif
		ptr->allocated_size = size + 1;
	    }
	    ptr->string_size = 0;
	    ptr->mem = '\0';
	}
	catch(...)
	{
	    clean_and_destroy();
	    throw;
	}
    }

    void secu_string::copy_from(const secu_string & ref)
    {
	if(ref.ptr != nullptr)
	{

		// sanity checks

	    if(ref.ptr->allocated_size == 0)
		throw SRC_BUG;

		// we must not waste secured memory while
		// copying a object, so we allocate it at
		// the minimum size needed to store the
		// string (not the allocated size of ref)
	    init(ref.ptr->string_size);

	    if(ref.ptr->string_size > 0)
	    {
		if(ptr == nullptr)
		    throw SRC_BUG; // init() call above should have set it

		if(ptr->allocated_size < ref.ptr->string_size + 1)
		    throw SRC_BUG;

		(void)memcpy(&(ptr->mem), &(ref.ptr->mem), ref.ptr->string_size + 1); // +1 to copy the ending '\0'
		ptr->string_size = ref.ptr->string_size;
	    }
		// else this->ptr is not allocated at all, meaning an empty string
	}
	// else nothing to do, "this" object should be unallocated
	// and ptr set to null
    }

    void secu_string::move_from(secu_string && ref) noexcept
    {
	std::swap(ptr, ref.ptr);
    }

    bool secu_string::compare_with(const char *a, U_I size) const
    {
	if(ptr == nullptr)
	    return size == 0; // true (same string) if arg is an empty string too, as we are.
	else
	{
	    if(size != ptr->string_size)
		return false;
	    else
	    {
		U_I cur = 0;
		while(cur < size && a[cur] == c_str()[cur])
		    ++cur;
		return cur == size;
	    }
	}
    }

    void secu_string::clean_and_destroy()
    {
	if(ptr != nullptr)
	{
	    (void)memset(ptr, 0, ptr->allocated_size + 2*sizeof(U_I)); // not add 1 here as allocated_size counts for it
#if CRYPTO_AVAILABLE
	    gcry_free(ptr);
#else
	    delete [] ptr;
#endif
	    ptr = nullptr;
	}
    }

} // end of namespace
