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
// $Id: tuyau.cpp,v 1.19.2.1 2005/03/13 20:07:54 edrusb Rel $
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

#if HAVE_ERRNO_H
#include <errno.h>
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
} // end extern "C"

#include "tuyau.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"
#include "cygwin_adapt.hpp"

using namespace std;

namespace libdar
{

    tuyau::tuyau(user_interaction & dialog, S_I fd) : generic_file(dialog, generic_file_get_mode(fd))
    {
        if(filedesc < 0)
            throw Erange("tuyau::tuyau", gettext("Bad file descriptor given"));
        filedesc = fd;
        position = 0;
        chemin = "";
    }

    tuyau::tuyau(user_interaction & dialog, S_I fd, gf_mode mode) : generic_file(dialog, mode)
    {
        gf_mode tmp;

        if(filedesc < 0)
            throw Erange("tuyau::tuyau", gettext("Bad file descriptor given"));
        tmp = generic_file_get_mode(fd);
        if(tmp != gf_read_write && tmp != mode)
            throw Erange("tuyau::tuyau", tools_printf(gettext("%s cannot be restricted to %s"), generic_file_get_name(tmp), generic_file_get_name(mode)));
        filedesc = fd;
        position = 0;
        chemin = "";
    }

    tuyau::tuyau(user_interaction & dialog, const string & filename, gf_mode mode) : generic_file(dialog, mode)
    {
        chemin = filename;
        position = 0;
        filedesc = -1; // not yet openned
    }

    bool tuyau::skip(const infinint & pos)
    {
        if(pos != position)
            throw Erange("tuyau::skip", gettext("Skipping is not possible on a pipe"));
        return true;
    }

    bool tuyau::skip_to_eof()
    {
        throw Erange("tuyau::skip", gettext("Skipping is not possible on a pipe"));
    }

    bool tuyau::skip_relative(S_I x)
    {
        if(x != 0)
            throw Erange("tuyau::skip", gettext("Skipping is not possible on a pipe"));
        return true;
    }

    S_I tuyau::inherited_read(char *a, size_t size)
    {
        S_I ret;
        U_I lu = 0;

#ifdef MUTEX_WORKS
	check_self_cancellation();
#endif
        if(filedesc < 0)
            ouverture();

        do
        {
            ret = ::read(filedesc, a+lu, size-lu);
            if(ret < 0)
            {
                switch(errno)
                {
                case EINTR:
                    break;
                case EIO:
                    throw Ehardware("tuyau::inherited_read", "");
                default:
                    throw Erange("tuyau::inherited_read", string(gettext("Error while reading from pipe: "))+strerror(errno));
                }
            }
            else
                if(ret > 0)
                    lu += ret;
        }
        while(lu < size && ret != 0);
        position += lu;

        return lu;
    }

    S_I tuyau::inherited_write(char *a, size_t size)
    {
        size_t total = 0;

#ifdef MUTEX_WORKS
	check_self_cancellation();
#endif
        if(filedesc < 0)
            ouverture();

        while(total < size)
        {
            S_I ret = ::write(filedesc, a+total, size-total);
            if(ret < 0)
            {
                switch(errno)
                {
                case EINTR:
                    break;
                case EIO:
                    throw Ehardware("tuyau::inherited_write", string(gettext("Error while writing data to pipe: ")) + strerror(errno));
                case ENOSPC:
                    get_gf_ui().pause(gettext("No space left on device, you have the opportunity to make room now. When ready : can we continue ?"));
                    break;
                default :
                    throw Erange("tuyau::inherited_write", string(gettext("Error while writing data to pipe: ")) + strerror(errno));
                }
            }
            else
                total += (U_I)ret;
        }

        position += total;
        return total;
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: tuyau.cpp,v 1.19.2.1 2005/03/13 20:07:54 edrusb Rel $";
        dummy_call(id);
    }

    void tuyau::ouverture()
    {
        if(chemin != "")
        {
            char *ch = tools_str2charptr(chemin);
            try
            {
                S_I flag;

                switch(get_mode())
                {
                case gf_read_only:
                    flag = O_RDONLY;
                    break;
                case gf_write_only:
                    flag = O_WRONLY;
                    break;
                case gf_read_write:
                    flag = O_RDWR;
                    break;
                default:
                    throw SRC_BUG;
                }
                filedesc = ::open(ch, flag|O_BINARY);
                if(filedesc < 0)
                    throw Erange("tuyau::ouverture", string(gettext("Error openning pipe: "))+strerror(errno));
            }
            catch(...)
            {
                delete [] ch;
                throw;
            }
            delete [] ch;
        }
        else
            throw SRC_BUG; // no path nor file descriptor
    }

} // end of namespace
