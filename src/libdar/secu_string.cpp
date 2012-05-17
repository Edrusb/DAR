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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/
// $Id: secu_string.cpp,v 1.4 2011/06/02 13:17:37 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_GCRYPT_H
#include <gcrypt.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
}

#include "erreurs.hpp"
#include "secu_string.hpp"

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

    void secu_string::read(int fd, U_I size)
    {
	clean_and_destroy();
	init(size);
	S_I lu = ::read(fd, mem, *allocated_size - 1);
	if(lu < 0)
	{
	    *string_size = 0;
	    mem[0] = '\0';
	    throw Erange("secu_string::read", string(gettext("Error while reading data for a secure memory:" )) + strerror(errno));
	}
	else
	    *string_size = lu;

	if(*string_size > *allocated_size - 1)
	    throw SRC_BUG;
	else
	    mem[*string_size] = '\0';
    }

    void secu_string::append(const char *ptr, U_I size)
    {
	if(size + *string_size >= *allocated_size)
	    throw Esecu_memory("secu_string::append");

	memcpy(mem + *string_size, ptr, size);
	*string_size += size;
	mem[*string_size] = '\0';
    }

    void secu_string::append(int fd, U_I size)
    {
	if(*string_size + size > *allocated_size)
	    throw Erange("secu_string::append", gettext("Cannot receive that much data in regard to the allocated memory"));
	S_I lu = ::read(fd, mem + *string_size, size);
	if(lu < 0)
	{
	    mem[*string_size] = '\0';
	    throw Erange("secu_string::read", string(gettext("Error while reading data for a secure memory:" )) + strerror(errno));
	}
	if(*string_size + lu > *allocated_size)
	    throw SRC_BUG;

	*string_size += lu;
	mem[*string_size] = '\0';
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: secu_string.cpp,v 1.4 2011/06/02 13:17:37 edrusb Rel $";
        dummy_call(id);
    }

    void secu_string::reduce_string_size_to(U_I pos)
    {
	if(pos > *string_size)
	    throw Erange("secu_string::reduce_string_size_to", gettext("Cannot reduce the string to a size that is larger than its current size"));

	*string_size = pos;
	mem[*string_size] = '\0';
    }

    void secu_string::init(U_I size)
    {
	allocated_size = NULL;
	mem = NULL;
	string_size = NULL;

	try
	{
#if CRYPTO_AVAILABLE
	    allocated_size = (U_I *)gcry_malloc_secure(sizeof(U_I));
	    if(allocated_size == NULL)
		throw Esecu_memory("secu_string::secus_string");
#else
	    allocated_size = new U_I;
	    if(allocated_size == NULL)
		throw Ememory("secu_string::secus_string");
#endif

	    *allocated_size = size + 1;

#if CRYPTO_AVAILABLE
	    mem = (char *)gcry_malloc_secure(*allocated_size);
	    if(mem == NULL)
		throw Esecu_memory("secu_string::secus_string");
#else
	    mem = new char[*allocated_size];
	    if(mem == NULL)
		throw Ememory("secu_string::secus_string");
#endif

#if CRYPTO_AVAILABLE
	    string_size = (U_I *)gcry_malloc_secure(sizeof(U_I));
	    if(string_size == NULL)
		throw Esecu_memory("secu_string::secus_string");
#else
	    string_size = new U_I;
	    if(string_size == NULL)
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
	if(*ref.allocated_size > 0)
	{
	    if(ref.mem == NULL)
		throw SRC_BUG;
	    init(*ref.allocated_size - 1);
	    (void)memcpy(mem, ref.mem, *ref.string_size + 1); // +1 to copy the ending '\0'
	}
	*string_size = *ref.string_size;
    }

    bool secu_string::compare_with(const char *ptr, U_I size) const
    {
	if(size != *string_size)
	    return false;
	else
	    return strncmp(mem, ptr, size) == 0;
    }

    void secu_string::clean_and_destroy()
    {
#if CRYPTO_AVAILABLE
	    if(string_size != NULL)
	    {
		bzero(string_size, sizeof(U_I));
		gcry_free(string_size);
		string_size = NULL;
	    }
	    if(mem != NULL)
	    {
		bzero(mem, *allocated_size);
		gcry_free(mem);
		mem = NULL;
	    }
	    if(allocated_size != NULL)
	    {
		bzero(allocated_size, sizeof(U_I));
		gcry_free(allocated_size);
		allocated_size = NULL;
	    }
#else
	    if(string_size != NULL)
	    {
		bzero(string_size, sizeof(U_I));
		delete string_size;
		string_size = NULL;
	    }
	    if(mem != NULL)
	    {
		bzero(mem, *allocated_size);
		delete [] mem;
		mem = NULL;
	    }
	    if(allocated_size != NULL)
	    {
		bzero(allocated_size, sizeof(U_I));
		delete allocated_size;
		allocated_size = NULL;
	    }
#endif
    }

} // end of namespace

