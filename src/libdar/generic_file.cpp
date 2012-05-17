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
// $Id: generic_file.cpp,v 1.46 2011/04/17 13:12:29 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
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

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

} // end extern "C"

#include "infinint.hpp"
#include "generic_file.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "cygwin_adapt.hpp"
#include "int_tools.hpp"

#include <iostream>
#include <sstream>

#define BUFFER_SIZE 102400
#ifdef SSIZE_MAX
#if SSIZE_MAX < BUFFER_SIZE
#undef BUFFER_SIZE
#define BUFFER_SIZE SSIZE_MAX
#endif
#endif

using namespace std;

namespace libdar
{

    void generic_file::terminate() const
    {
	generic_file *ceci = const_cast<generic_file *>(this);
	try
	{
	    if(!terminated)
		ceci->inherited_terminate();
	}
	catch(...)
	{
	    ceci->terminated = true;
	    throw;
	}
	ceci->terminated = true;
    }

    U_I generic_file::read(char *a, U_I size)
    {
	if(terminated)
	    throw SRC_BUG;

        if(rw == gf_write_only)
            throw Erange("generic_file::read", gettext("Reading a write only generic_file"));
        else
            return (this->*active_read)(a, size);
    }

    void generic_file::write(const char *a, U_I size)
    {
	if(terminated)
	    throw SRC_BUG;
        if(rw == gf_read_only)
            throw Erange("generic_file::write", gettext("Writing to a read only generic_file"));
        else
            (this->*active_write)(a, size);
    }

    void generic_file::write(const string & arg)
    {
	if(terminated)
	    throw SRC_BUG;

        if(arg.size() > int_tools_maxof_agregate(U_I(0)))
            throw SRC_BUG;

        write(arg.c_str(), arg.size());
    }

    S_I generic_file::read_back(char &a)
    {
	if(terminated)
	    throw SRC_BUG;

        if(skip_relative(-1))
        {
            U_I ret = read(&a,1);
            skip_relative(-1);
            return ret;
        }
        else
            return 0;
    }

    void generic_file::copy_to(generic_file & ref)
    {
        char buffer[BUFFER_SIZE];
        U_I lu;

	if(terminated)
	    throw SRC_BUG;

        do
        {
	    try
	    {
		lu = this->read(buffer, BUFFER_SIZE);
	    }
	    catch(Egeneric & e)
	    {
		e.stack("generic_file::copy_to", "read");
		throw;
	    }
            if(lu > 0)
	    {
		try
		{
		    ref.write(buffer, lu);
		}
		catch(Egeneric & e)
		{
		    e.stack("generic_file::copy_to", "write");
		    throw;
		}
	    }
        }
        while(lu > 0);
    }

    void generic_file::copy_to(generic_file & ref, crc & value)
    {
	if(terminated)
	    throw SRC_BUG;

        reset_crc(value.get_size());
        copy_to(ref);
        get_crc(value);
    }

    U_32 generic_file::copy_to(generic_file & ref, U_32 size)
    {
        char buffer[BUFFER_SIZE];
        S_I lu = 1, pas;
        U_32 wrote = 0;

	if(terminated)
	    throw SRC_BUG;

        while(wrote < size && lu > 0)
        {
            pas = size > BUFFER_SIZE ? BUFFER_SIZE : size;
	    try
	    {
		lu = read(buffer, pas);
	    }
	    catch(Egeneric & e)
	    {
		e.stack("generic_file::copy_to", "read");
		throw;
	    }
            if(lu > 0)
            {
		try
		{
		    ref.write(buffer, lu);
		}
		catch(Egeneric & e)
		{
		    e.stack("generic_file::copy_to", "write");
		    throw;
		}
                wrote += lu;
            }
        }

        return wrote;
    }

    infinint generic_file::copy_to(generic_file & ref, infinint size)
    {
        U_32 tmp = 0, delta;
        infinint wrote = 0;

	if(terminated)
	    throw SRC_BUG;

        size.unstack(tmp);

        do
        {
            delta = copy_to(ref, tmp);
            wrote += delta;
            tmp -= delta;
            if(tmp == 0)
                size.unstack(tmp);
        }
        while(tmp > 0);

        return wrote;
    }

    bool generic_file::diff(generic_file & f, crc & value)
    {
        char buffer1[BUFFER_SIZE];
        char buffer2[BUFFER_SIZE];
        U_I lu1 = 0, lu2 = 0;
        bool diff = false;

	if(terminated)
	    throw SRC_BUG;

        if(get_mode() == gf_write_only || f.get_mode() == gf_write_only)
            throw Erange("generic_file::diff", gettext("Cannot compare files in write only mode"));
        skip(0);
        f.skip(0);
	value.clear();
        do
        {
            lu1 = read(buffer1, BUFFER_SIZE);
            lu2 = f.read(buffer2, BUFFER_SIZE);
            if(lu1 == lu2)
            {
                register U_I i = 0;
                while(i < lu1 && buffer1[i] == buffer2[i])
                    ++i;
                if(i < lu1)
                    diff = true;
		else
		    value.compute(buffer1, lu1);
            }
            else
                diff = true;
        }
        while(!diff && lu1 > 0);

        return diff;
    }

    void generic_file::sync_write()
    {
	if(terminated)
	    throw SRC_BUG;

	if(rw == gf_write_only || rw == gf_read_write)
	    inherited_sync_write();
	else
	    throw Erange("generic_file::read", gettext("Cannot sync write on a read-only generic_file"));
    }

    void generic_file::reset_crc(const infinint & width)
    {
	if(terminated)
	    throw SRC_BUG;

        if(active_read == &generic_file::read_crc)
            throw SRC_BUG; // crc still active, previous CRC value never read
        enable_crc(true);
	checksum.resize(width);
    }

    void generic_file::enable_crc(bool mode)
    {
	if(terminated)
	    throw SRC_BUG;

        if(mode) // routines with crc features
        {
            active_read = &generic_file::read_crc;
            active_write = &generic_file::write_crc;
        }
        else
        {
            active_read = &generic_file::inherited_read;
            active_write = &generic_file::inherited_write;
        }
    }

    U_I generic_file::read_crc(char *a, U_I size)
    {
	if(terminated)
	    throw SRC_BUG;
	else
	{
	    S_I ret = inherited_read(a, size);
	    checksum.compute(a, ret);
	    return ret;
	}
    }

    void generic_file::write_crc(const char *a, U_I size)
    {
	if(terminated)
	    throw SRC_BUG;

        inherited_write(a, size);
	checksum.compute(a, size);
    }

    void generic_file::copy_from(const generic_file & ref)
    {
	if(terminated)
	    throw SRC_BUG;

	rw = ref.rw;
	checksum = ref.checksum;
	terminated = ref.terminated;
	active_read = ref.active_read;
	active_write = ref.active_write;
    }


    static void dummy_call(char *x)
    {
        static char id[]="$Id: generic_file.cpp,v 1.46 2011/04/17 13:12:29 edrusb Rel $";
        dummy_call(id);
    }

    gf_mode generic_file_get_mode(S_I fd)
    {
	S_I flags = fcntl(fd, F_GETFL) & O_ACCMODE;
        gf_mode ret;

        switch(flags)
        {
        case O_RDONLY:
            ret = gf_read_only;
            break;
        case O_WRONLY:
            ret = gf_write_only;
            break;
        case O_RDWR:
            ret = gf_read_write;
            break;
        default:
            throw Erange("generic_file_get_mode", gettext("File mode is neither read nor write"));
        }

        return ret;
    }

    const char * generic_file_get_name(gf_mode mode)
    {
        char *ret = NULL;

        switch(mode)
        {
        case gf_read_only:
            ret = gettext("read only");
            break;
        case gf_write_only:
            ret = gettext("write only");
            break;
        case gf_read_write:
            ret = gettext("read and write");
            break;
        default:
            throw SRC_BUG;
        }

        return ret;
    }

} // end of namespace
