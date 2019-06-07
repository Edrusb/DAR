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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
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

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
    char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
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
	if(offset > *string_size)
	    throw Erange("secu_string::append", gettext("appending data over secure_memory its end"));

	if(size + offset >= *allocated_size)
	    throw Esecu_memory("secu_string::append");

	(void)memcpy(mem + offset, ptr, size);
	offset += size;
	*string_size = offset;
	mem[*string_size] = '\0';
    }

    void secu_string::append_at(U_I offset, int fd, U_I size)
    {
	if(offset > *string_size)
	    throw Erange("secu_string::append", gettext("appending data after the end of a secure_memory"));

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
	if(pos > *string_size)
	    throw Erange("secu_string::reduce_string_size_to", gettext("Cannot reduce the string to a size that is larger than its current size"));

	*string_size = pos;
	mem[*string_size] = '\0';
    }

    void secu_string::randomize(U_I size)
    {
#if CRYPTO_AVAILABLE
	if(size > get_allocated_size())
	    throw Erange("secu_string::randomize", gettext("secu_string randomization requested exceeds storage capacity"));
	*string_size = size;
	gcry_randomize(mem, *string_size, GCRY_STRONG_RANDOM);
#else
	throw Efeature("string randomization lacks libgcrypt");
#endif
    }

    char & secu_string::operator[] (U_I index)
    {
	if(index < get_size())
	    return mem[index];
	else
	    throw Erange("secu_string::operator[]", gettext("Out of range index requested for a secu_string"));
    }

    void secu_string::init(U_I size)
    {
	allocated_size = nullptr;
	mem = nullptr;
	string_size = nullptr;

	try
	{
#if CRYPTO_AVAILABLE
	    allocated_size = (U_I *)gcry_malloc_secure(sizeof(U_I));
	    if(allocated_size == nullptr)
		throw Esecu_memory("secu_string::secus_string");
#else
	    meta_new(allocated_size, 1);
	    if(allocated_size == nullptr)
		throw Ememory("secu_string::secus_string");
#endif

	    *allocated_size = size + 1;

#if CRYPTO_AVAILABLE
	    mem = (char *)gcry_malloc_secure(*allocated_size);
	    if(mem == nullptr)
		throw Esecu_memory("secu_string::secus_string");
#else
	    meta_new(mem, *allocated_size);
	    if(mem == nullptr)
		throw Ememory("secu_string::secus_string");
#endif

#if CRYPTO_AVAILABLE
	    string_size = (U_I *)gcry_malloc_secure(sizeof(U_I));
	    if(string_size == nullptr)
		throw Esecu_memory("secu_string::secus_string");
#else
	    meta_new(string_size, 1);
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
	if(ref.allocated_size == nullptr)
	    throw SRC_BUG;
	if(*(ref.allocated_size) == 0)
	    throw SRC_BUG;
	if(ref.mem == nullptr)
	    throw SRC_BUG;
	if(ref.string_size == nullptr)
	    throw SRC_BUG;

	init(*(ref.allocated_size) - 1);
	(void)memcpy(mem, ref.mem, *(ref.string_size) + 1); // +1 to copy the ending '\0'
	*string_size = *(ref.string_size);
    }

    bool secu_string::compare_with(const char *ptr, U_I size) const
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

    void secu_string::clean_and_destroy()
    {
	if(string_size != nullptr)
	{
	    (void)memset(string_size, 0, sizeof(U_I));
#if CRYPTO_AVAILABLE
	    gcry_free(string_size);
#else
	    meta_delete(string_size);
#endif
	    string_size = nullptr;
	}
	if(mem != nullptr)
	{
	    if(allocated_size == nullptr)
		throw SRC_BUG;
	    (void)memset(mem, 0, *allocated_size);
#if CRYPTO_AVAILABLE
	    gcry_free(mem);
#else
	    meta_delete(mem);
#endif
	    mem = nullptr;
	}
	if(allocated_size != nullptr)
	{
	    (void)memset(allocated_size, 0, sizeof(U_I));
#if CRYPTO_AVAILABLE
	    gcry_free(allocated_size);
#else
	    meta_delete(allocated_size);
#endif
	    allocated_size = nullptr;

	}
    }

} // end of namespace
