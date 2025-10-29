//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
	if(zero_length)
	    if(ref.zero_length)
		return true;
	    else
		return *(ref.string_size) == 0;
	else
	    if(ref.zero_length)
		return false;
	    else
		return compare_with(ref.mem, *(ref.string_size));
    }


    void secu_string::set(int fd, U_I size)
    {
	U_I offset = 0;
	S_I lu;

	if(size < get_allocated_size())
	{
	    clean_and_destroy();
	    init(size);
	}
	else
	    clear();

	do
	{
	    lu = ::read(fd, mem + offset, *allocated_size - 1 - offset);

	    if(lu < 0)
	    {
		*string_size = offset;
		mem[offset] = '\0';
		throw Erange("secu_string::read", string(gettext("Error while reading data for a secure memory:" )) + tools_strerror_r(errno));
	    }
	    else
		offset += lu;
	}
	while(lu > 0 && offset < size);

	*string_size = offset;
	if(*string_size >= *allocated_size)
	    throw SRC_BUG;
	else
	    mem[*string_size] = '\0';
    }

    void secu_string::append_at(U_I offset, const char *ptr, U_I size)
    {
	if(size == 0)
	    return; // nothing to append

	if(zero_length || offset > *string_size)
	    throw Erange("secu_string::append", gettext("appending data after the end of a secured memory"));

	if(size + offset >= *allocated_size)
	    throw Esecu_memory("secu_string::append");

	(void)memcpy(mem + offset, ptr, size);
	offset += size;
	*string_size = offset;
	mem[*string_size] = '\0';
    }

    void secu_string::append_at(U_I offset, int fd, U_I size)
    {
	if(size == 0)
	    return; // nothing to append

	if(zero_length || offset > *string_size)
	    throw Erange("secu_string::append", gettext("appending data after the end of a secured memory"));

	if(size + offset >= *allocated_size)
	    throw Erange("secu_string::append", gettext("Cannot receive that much data in regard to the allocated memory"));

	S_I lu = ::read(fd, mem + offset, size);
	if(lu < 0)
	{
	    mem[*string_size] = '\0';
	    throw Erange("secu_string::read", string(gettext("Error while reading data for a secure memory:" )) + tools_strerror_r(errno));
	}
	if(lu + offset >= *allocated_size)
	    throw SRC_BUG;
	offset += lu;
	if(*string_size < offset)
	    *string_size = offset;
	mem[*string_size] = '\0';
    }

    void secu_string::reduce_string_size_to(U_I pos)
    {
	if(zero_length || pos > *string_size)
	    throw Erange("secu_string::reduce_string_size_to", gettext("Cannot reduce the string to a size that is larger than its current size"));

	*string_size = pos;
	mem[*string_size] = '\0';
    }

    void secu_string::expand_string_size_to(U_I size)
    {
	if(size == 0)
	    return; // nothing to expand

	if(zero_length || size > get_allocated_size())
	    throw Erange("secu_string::expand_string_size_to", gettext("Cannot expand secu_string size past its allocation size"));
	if(size < *string_size)
	    throw Erange("secu_stering::expand_string_size_to", gettext("Cannot shrink a secu_string"));

	memset(mem + *string_size, '\0', size - *string_size);
	*string_size = size;
    }

    void secu_string::randomize(U_I size)
    {
#if CRYPTO_AVAILABLE
	if(! zero_length)
	{
	    set_size(size);
	    gcry_randomize(mem, *string_size, GCRY_STRONG_RANDOM);
	}
#else
	throw Efeature("string randomization lacks libgcrypt");
#endif
    }

    void secu_string::set_size(U_I size)
    {
	if(zero_length)
	{
	    if(size == 0)
		return; // nothing to do
	}

	if(size > get_allocated_size())
	    throw Erange("secu_string::set_size", gettext("exceeding storage capacity while requesting secu_string::set_size()"));
	*string_size = size;
    }

    char * secu_string::get_array()
    {
	if(zero_length)
	    return nullptr;
	else
	    return mem == nullptr ? throw SRC_BUG : mem;
    }


    char & secu_string::operator[] (U_I index)
    {
	if(! zero_length && index < get_size())
	    return mem[index];
	else
	    throw Erange("secu_string::operator[]", gettext("Out of range index requested for a secu_string"));
    }

    void secu_string::init(U_I size)
    {
	nullifyptr();

	if(size == 0)
	{
	    zero_length = true;
	    return;
	}
	else
	    zero_length = false;

	try
	{
#if CRYPTO_AVAILABLE
	    allocated_size = (U_I *)gcry_malloc_secure(sizeof(U_I));
	    if(allocated_size == nullptr)
		throw Esecu_memory("secu_string::secus_string");
#else
	    allocated_size = new (nothrow) U_I;
	    if(allocated_size == nullptr)
		throw Ememory("secu_string::secus_string");
#endif

	    *allocated_size = size + 1;

#if CRYPTO_AVAILABLE
	    mem = (char *)gcry_malloc_secure(*allocated_size);
	    if(mem == nullptr)
		throw Esecu_memory("secu_string::secus_string");
#else
	    mem = new (nothrow) char[*allocated_size];
	    if(mem == nullptr)
		throw Ememory("secu_string::secus_string");
#endif

#if CRYPTO_AVAILABLE
	    string_size = (U_I *)gcry_malloc_secure(sizeof(U_I));
	    if(string_size == nullptr)
		throw Esecu_memory("secu_string::secus_string");
#else
	    string_size = new (nothrow) U_I;
	    if(string_size == nullptr)
		throw Ememory("secu_string::secus_string");
#endif
	    *string_size = 0;
	    mem[0] = '\0';
	}
	catch(...)
	{
	    clean_and_destroy();
	    throw;
	}
    }

    void secu_string::copy_from(const secu_string & ref)
    {
	if(!ref.zero_length)
	{

		// sanity checks

	    if(ref.allocated_size == nullptr)
		throw SRC_BUG;
	    if(*(ref.allocated_size) == 0)
		throw SRC_BUG;
	    if(ref.mem == nullptr)
		throw SRC_BUG;
	    if(ref.string_size == nullptr)
		throw SRC_BUG;

		// we must not waste secured memory while
		// copying a object, so we allocate it at
		// the minimum sized needed to store the
		// string
	    init(*(ref.string_size) + 1);

	    (void)memcpy(mem, ref.mem, *(ref.string_size) + 1); // +1 to copy the ending '\0'
	    *string_size = *(ref.string_size);
	}
	else
	{
	    zero_length = true;
	    allocated_size = nullptr;
	    mem = nullptr;
	    string_size = nullptr;
	}
    }

    void secu_string::move_from(secu_string && ref) noexcept
    {
	std::swap(zero_length, ref.zero_length);
	std::swap(allocated_size, ref.allocated_size);
	std::swap(mem, ref.mem);
	std::swap(string_size, ref.string_size);
    }

    bool secu_string::compare_with(const char *ptr, U_I size) const
    {
	if(zero_length)
	    return size == 0; // true (same string) if arg is an empty string too, as we are.
	else
	{
	    if(size != *string_size)
		return false;
	    else
	    {
		U_I cur = 0;
		while(cur < size && ptr[cur] == mem[cur])
		    ++cur;
		return cur == size;
	    }
	}
    }

    void secu_string::clean_and_destroy()
    {
	if(string_size != nullptr)
	{
	    (void)memset(string_size, 0, sizeof(U_I));
#if CRYPTO_AVAILABLE
	    gcry_free(string_size);
#else
	    delete string_size;
#endif
	    string_size = nullptr;
	}
	if(mem != nullptr)
	{
	    if(allocated_size != nullptr)
		(void)memset(mem, 0, *allocated_size);
#if CRYPTO_AVAILABLE
	    gcry_free(mem);
#else
	    delete [] mem;
#endif
	    mem = nullptr;
	}
	if(allocated_size != nullptr)
	{
	    (void)memset(allocated_size, 0, sizeof(U_I));
#if CRYPTO_AVAILABLE
	    gcry_free(allocated_size);
#else
	    delete allocated_size;
#endif
	    allocated_size = nullptr;
	}

	zero_length = true;
    }

} // end of namespace
