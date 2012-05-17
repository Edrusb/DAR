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

#if HAVE_STRING_H
#include <string.h>
#endif

} // end extern "C"

#include "infinint.hpp"
#include "generic_file.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "cygwin_adapt.hpp"
#include "int_tools.hpp"
#include "fichier.hpp"
#include "tools.hpp"

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

    fichier::fichier(user_interaction & dialog, S_I fd) : generic_file(generic_file_get_mode(fd))
    {
	init_dialog(dialog);
        filedesc = fd;
    }

    fichier::fichier(user_interaction & dialog, const char *name,
		     gf_mode m,
		     U_I perm,
		     bool furtive_mode) : generic_file(m)
    {
	init_dialog(dialog);
        fichier::open(name, m, perm, furtive_mode);
    }

    fichier::fichier(user_interaction & dialog,
		     const string &chemin,
		     gf_mode m,
		     U_I perm,
		     bool furtive_mode) : generic_file(m)
    {
	init_dialog(dialog);
        fichier::open(chemin.c_str(), m, perm, furtive_mode);
    }

    fichier::fichier(const string & chemin, bool furtive_mode) : generic_file(gf_read_only)
    {
	x_dialog = NULL; // we should never need it in read_only mode
	fichier::open(chemin.c_str(), gf_read_only, tools_octal2int("0777"), furtive_mode);
    }

    void fichier::change_ownership(const std::string & user, const std::string & group)
    {
	if(is_terminated())
	    throw SRC_BUG;

	    // this method cannot be inlined to avoid cyclic dependency in headers files
	    // fichier.hpp would then needs tools.hpp, which need limitint.hpp which relies
	    // back on fichier.hpp
	tools_set_ownership(filedesc, user, group);
    }

    void fichier::change_permission(U_I perm)
    {
	if(is_terminated())
	    throw SRC_BUG;

	    // this method cannot be inlined to avoid cyclic dependency in headers files
	    // fichier.hpp would then needs tools.hpp, which need limitint.hpp which relies
	    // back on fichier.hpp
	tools_set_permission(filedesc, perm);
    }

    infinint fichier::get_size() const
    {
        struct stat dat;
        infinint filesize;

	if(is_terminated())
	    throw SRC_BUG;

        if(filedesc < 0)
            throw SRC_BUG;

        if(fstat(filedesc, &dat) < 0)
            throw Erange("fichier::get_size()", string(gettext("Error getting size of file: ")) + strerror(errno));
        else
            filesize = dat.st_size;

        return filesize;
    }

    bool fichier::skip(const infinint &q)
    {
        off_t delta;
        infinint pos = q;

	if(is_terminated())
	    throw SRC_BUG;

        if(lseek(filedesc, 0, SEEK_SET) < 0)
            return false;

        do
	{
            delta = 0;
            pos.unstack(delta);
            if(delta > 0)
                if(lseek(filedesc, delta, SEEK_CUR) < 0)
                    return false;
        }
	while(delta > 0);

        return true;
    }

    bool fichier::skip_to_eof()
    {
	if(is_terminated())
	    throw SRC_BUG;

        return lseek(filedesc, 0, SEEK_END) >= 0;
    }

    bool fichier::skip_relative(S_I x)
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(x > 0)
	{
            if(lseek(filedesc, x, SEEK_CUR) < 0)
                return false;
            else
                return true;
	}

        if(x < 0)
        {
            bool ret = true;
            off_t actu = lseek(filedesc, 0, SEEK_CUR);

            if(actu < -x)
            {
                actu = 0;
                ret = false;
            }
            else
                actu += x; // x is negative
            if(lseek(filedesc, actu, SEEK_SET) < 0)
                ret = false;

            return ret;
        }

        return true;
    }

    infinint fichier::get_position()
    {
	if(is_terminated())
	    throw SRC_BUG;

        off_t ret = lseek(filedesc, 0, SEEK_CUR);

        if(ret == -1)
            throw Erange("fichier::get_position", string(gettext("Error getting file reading position: ")) + strerror(errno));

        return ret;
    }

    U_I fichier::inherited_read(char *a, U_I size)
    {
        ssize_t ret;
        U_I lu = 0;

#ifdef MUTEX_WORKS
	check_self_cancellation();
#endif
        do
        {
#ifdef SSIZE_MAX
	    U_I to_read = size - lu > SSIZE_MAX ? SSIZE_MAX : size - lu;
#else
	    U_I to_read = size - lu;
#endif

            ret = ::read(filedesc, a+lu, to_read);
            if(ret < 0)
            {
                switch(errno)
                {
                case EINTR:
                    break;
                case EAGAIN:
                    throw SRC_BUG;
			// non blocking read not compatible with
                        // generic_file
                case EIO:
                    throw Ehardware("fichier::inherited_read", string(gettext("Error while reading from file: ")) + strerror(errno));
                default :
                    throw Erange("fichier::inherited_read", string(gettext("Error while reading from file: ")) + strerror(errno));
                }
            }
            else
                lu += ret;
        }
        while(lu < size && ret != 0);

        return lu;
    }

    void fichier::inherited_write(const char *a, U_I size)
    {
        ssize_t ret;
        U_I total = 0;

	    // due to posix inconsistence in write(2)
	    // we cannot write more than SSIZE_MAX/2 byte
	    // to be able to have a positive returned value
	    // in case of success, a negative else. As the
	    // returned type of write() is signed size_t,
	    // which max positive value is SSIZE_MAX/2 (rounded
	    // to the lower interger)
#ifdef SSIZE_MAX
	static const U_I step = SSIZE_MAX/2;
#else
	const U_I step = size; // which is no limit...
#endif


#ifdef MUTEX_WORKS
	check_self_cancellation();
#endif
        while(total < size)
        {
	    if(size - total > step)
		ret = ::write(filedesc, a+total, step);
	    else
		ret = ::write(filedesc, a+total, size - total);
            if(ret < 0)
            {
                switch(errno)
                {
                case EINTR:
                    break;
                case EIO:
                    throw Ehardware("fichier::inherited_write", string(gettext("Error while writing to file: ")) + strerror(errno));
                case ENOSPC:
		    if(get_mode() == gf_read_only)
			throw SRC_BUG; // read-only mode should not lead to asking more space !
		    if(x_dialog == NULL)
			throw SRC_BUG;
                    x_dialog->pause(gettext("No space left on device, you have the opportunity to make room now. When ready : can we continue ?"));
                    break;
                default :
                    throw Erange("fichier::inherited_write", string(gettext("Error while writing to file: ")) + strerror(errno));
                }
            }
            else
                total += ret;
        }
    }

    void fichier::open(const char *name, gf_mode m, U_I perm, bool furtive_mode)
    {
        S_I mode;

        switch(m)
        {
        case gf_read_only :
            mode = O_RDONLY;
            break;
        case gf_write_only :
            mode = O_WRONLY|O_CREAT;
            break;
        case gf_read_write :
            mode = O_RDWR|O_CREAT;
            break;
        default:
            throw SRC_BUG;
        }

#if FURTIVE_READ_MODE_AVAILABLE
	if(furtive_mode) // only used for read-only, but available for write-only and read-write modes
	    mode |= O_NOATIME;
#else
	if(furtive_mode)
	    throw Ecompilation(gettext("Furtive read mode"));
#endif

        do
        {
            filedesc = ::open(name, mode|O_BINARY, perm);
            if(filedesc < 0)
	    {
                if(errno == ENOSPC)
		{
		    if(get_mode() == gf_read_only)
			throw SRC_BUG; // in read_only mode we do not need to create a new inode !!!
		    if(x_dialog == NULL)
			throw SRC_BUG;
                    x_dialog->pause(gettext("No space left for inode, you have the opportunity to make some room now. When done : can we continue ?"));
		}
                else
                    throw Erange("fichier::open", string(gettext("Cannot open file : ")) + strerror(errno));
	    }
        }
        while(filedesc < 0 && errno == ENOSPC);
    }

    void fichier::copy_from(const fichier & ref)
    {
	filedesc = dup(ref.filedesc);
	if(filedesc <0)
	    throw Erange("fichier::copy_from", tools_printf(gettext("Cannot dup() filedescriptor while copying \"fichier\" object: %s"), strerror(errno)));
	if(ref.x_dialog != NULL)
	    init_dialog(*ref.x_dialog);
	else
	    x_dialog = NULL;
    }

    void fichier::init_dialog(user_interaction & dialog)
    {
	x_dialog = dialog.clone();
	if(x_dialog == NULL)
	    throw SRC_BUG;
    }

} // end of namespace
