/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
#include "tools.hpp"
#include "fichier_local.hpp"
#include "user_interaction_blind.hpp"

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


    fichier_local::fichier_local(const shared_ptr<user_interaction> & dialog,
				 const string & chemin,
				 gf_mode m,
				 U_I permission,
				 bool fail_if_exists,
				 bool erase,
				 bool furtive_mode) : fichier_global(dialog, m)
    {
	fichier_local::open(chemin, m, permission, fail_if_exists, erase, furtive_mode);
    }

    fichier_local::fichier_local(const string & chemin, bool furtive_mode) : fichier_global(shared_ptr<user_interaction>(new (nothrow) user_interaction_blind()), gf_read_only)
    {
	    // in read-only mode the user_interaction is not expected to be used
	fichier_local::open(chemin, gf_read_only, 0, false, false, furtive_mode);
    }

    void fichier_local::change_ownership(const std::string & user, const std::string & group)
    {
	if(is_terminated())
	    throw SRC_BUG;

	    // this method cannot be inlined to avoid cyclic dependency in headers files
	    // fichier_global.hpp would then needs tools.hpp, which need limitint.hpp which relies
	    // back on fichier.hpp
	tools_set_ownership(filedesc, user, group);
    }

    void fichier_local::change_permission(U_I perm)
    {
	if(is_terminated())
	    throw SRC_BUG;

	    // this method cannot be inlined to avoid cyclic dependency in headers files
	    // fichier_global.hpp would then needs tools.hpp, which need limitint.hpp which relies
	    // back on fichier.hpp
	tools_set_permission(filedesc, perm);
    }

    infinint fichier_local::get_size() const
    {
        struct stat dat;
        infinint filesize;

	if(is_terminated())
	    throw SRC_BUG;

        if(filedesc < 0)
            throw SRC_BUG;

        if(fstat(filedesc, &dat) < 0)
            throw Erange("fichier_local::get_size()", string(gettext("Error getting size of file: ")) + tools_strerror_r(errno));
        else
            filesize = dat.st_size;

        return filesize;
    }


    void fichier_local::fadvise(advise adv) const
    {
	if(is_terminated())
	    throw SRC_BUG;

#if HAVE_POSIX_FADVISE
	int ret = posix_fadvise(filedesc, 0, 0, advise_to_int(adv));

	if(ret == EBADF)
	    throw SRC_BUG; // filedesc not a valid file descriptor !?!
	if(ret != 0)
	    throw Erange("fichier_local::fadvise", string("Set posix advise failed: ") + tools_strerror_r(errno));
#endif
    }

    void fichier_local::fsync() const
    {
	if(is_terminated())
	    throw SRC_BUG;
#if HAVE_FDATASYNC
	S_I st = ::fdatasync(filedesc);
#else
	S_I st = ::fsync(filedesc);
#endif
	if(st < 0)
	    throw Erange("fichier_local::fsync", string("Failed sync the slice (fdatasync): ") + tools_strerror_r(errno));
    }

    bool fichier_local::skip(const infinint &q)
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

    bool fichier_local::skip_to_eof()
    {
	if(is_terminated())
	    throw SRC_BUG;

        return lseek(filedesc, 0, SEEK_END) >= 0;
    }

    bool fichier_local::skip_relative(S_I x)
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

    infinint fichier_local::get_position() const
    {
	if(is_terminated())
	    throw SRC_BUG;

        off_t ret = lseek(filedesc, 0, SEEK_CUR);

        if(ret == -1)
            throw Erange("fichier_local::get_position", string(gettext("Error getting file reading position: ")) + tools_strerror_r(errno));

        return ret;
    }

    void fichier_local::inherited_truncate(const infinint & pos)
    {
	off_t offset = 0;
	int ret;
	infinint tmp_pos = pos;

	if(is_terminated())
	    throw SRC_BUG;

	tmp_pos.unstack(offset);
	if(!tmp_pos.is_zero())
	    throw Erange("fichier_local::inherited_truncate", gettext("File too large for the operating system to be truncate at the requested position"));

	if(offset >= get_eof_offset())
	    return; // will not expand the file size

	ret = ftruncate(filedesc, offset);
	if(ret != 0)
	    throw Erange("fichier_local::inherited_truncate", string(dar_gettext("Error while calling system call truncate(): ")) + tools_strerror_r(errno));

	if(get_position() > pos)
	    skip_to_eof();
    }


    bool fichier_local::fichier_global_inherited_read(char *a, U_I size, U_I & read, string & message)
    {
        ssize_t ret = -1;
        read = 0;

#ifdef MUTEX_WORKS
	check_self_cancellation();
#endif
        do
        {
#ifdef SSIZE_MAX
	    U_I to_read = size - read > SSIZE_MAX ? SSIZE_MAX : size - read;
#else
	    U_I to_read = size - read;
#endif

            ret = ::read(filedesc, a+read, to_read);
            if(ret < 0)
            {
                switch(errno)
                {
                case EINTR:
                    break;
                case EAGAIN:
                    throw SRC_BUG;
			// "non blocking" read is not expected in this implementation
                case EIO:
                    throw Ehardware("fichier_local::inherited_read", string(gettext("Error while reading from file: ")) + tools_strerror_r(errno));
                default :
                    throw Erange("fichier_local::inherited_read", string(gettext("Error while reading from file: ")) + tools_strerror_r(errno));
                }
            }
            else
                read += ret;
        }
        while(read < size && ret != 0);

	if(adv == advise_dontneed)
	    fadvise(adv);

        return true; // we never make partial reading, here
    }

    U_I fichier_local::fichier_global_inherited_write(const char *a, U_I size)
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
                    throw Ehardware("fichier_local::inherited_write", string(gettext("Error while writing to file: ")) + tools_strerror_r(errno));
                case ENOSPC:
		    return total; // partial writing, we stop here returning the amount of data wrote so far
			// because there is no space left on device. The parent class manages the user interaction
			// to allow abortion or action that frees up some storage space.
                default :
                    throw Erange("fichier_local::inherited_write", string(gettext("Error while writing to file: ")) + tools_strerror_r(errno));
                }
            }
            else
                total += ret;
        }

	if(adv == advise_dontneed)
		// we should call fsync() here but we do not to to avoid blocking the process:
		// we assume the system has flushed some blocks of possibly previous writes
		// so we just inform the system to remove from cache blocks associated to this
		// file that have already been flushed.
	    fadvise(adv);

	return total;
    }

    void fichier_local::open(const string & chemin,
			     gf_mode m,
			     U_I permission,
			     bool fail_if_exists,
			     bool erase,
			     bool furtive_mode)
    {
	U_I o_mode = O_BINARY;
	const char *name = chemin.c_str();
	adv = advise_normal;

        switch(m)
        {
        case gf_read_only :
            o_mode |= O_RDONLY;
            break;
        case gf_write_only :
	    o_mode |= O_WRONLY;
            break;
        case gf_read_write :
            o_mode |= O_RDWR;
            break;
        default:
            throw SRC_BUG;
        }

	if(m != gf_read_only)
	{
	    o_mode |= O_CREAT;

	    if(fail_if_exists)
		o_mode |= O_EXCL;

	    if(erase)
		o_mode |= O_TRUNC;
	}


#if FURTIVE_READ_MODE_AVAILABLE
	if(furtive_mode) // only used for read-only, but available for write-only and read-write modes
	    o_mode |= O_NOATIME;
#else
	if(furtive_mode)
	    throw Ecompilation(gettext("Furtive read mode"));
#endif
	try
	{
	    do
	    {
		if(m != gf_read_only)
		    filedesc = ::open(name, o_mode, permission);
		else
		    filedesc = ::open(name, o_mode);

		if(filedesc < 0)
		{
		    switch(errno)
		    {
		    case ENOSPC:
			if(get_mode() == gf_read_only)
			    throw SRC_BUG; // in read_only mode we do not need to create a new inode !!!
			get_ui().pause(gettext("No space left for inode, you have the opportunity to make some room now. When done : can we continue ?"));
			break;
		    case EEXIST:
			throw Esystem("fichier_local::open", tools_strerror_r(errno), Esystem::io_exist);
		    case ENOENT:
			throw Esystem("fichier_local::open", tools_strerror_r(errno), Esystem::io_absent);
		    case EACCES:
			throw Esystem("fichier_local::open", tools_strerror_r(errno), Esystem::io_access);
		    case EROFS:
			throw Esystem("fichier_local::open", tools_strerror_r(errno), Esystem::io_ro_fs);
		    default:
			throw Erange("fichier_local::open", string(gettext("Cannot open file : ")) + tools_strerror_r(errno));
		    }
		}
	    }
	    while(filedesc < 0 && errno == ENOSPC);
	}
	catch(...)
	{
	    if(filedesc >= 0)
	    {
		::close(filedesc);
		filedesc = -1;
	    }
	    throw;
	}
    }

    void fichier_local::copy_from(const fichier_local & ref)
    {
	filedesc = dup(ref.filedesc);
	if(filedesc < 0)
	{
	    string tmp = tools_strerror_r(errno);
	    throw Erange("fichier_local::copy_from", tools_printf(gettext("Cannot dup() filedescriptor while copying \"fichier_local\" object: %s"), tmp.c_str()));
	}
	adv = ref.adv;
    }

    void fichier_local::move_from(fichier_local && ref) noexcept
    {
	swap(filedesc, ref.filedesc);
	swap(adv, ref.adv);
    }

    int fichier_local::advise_to_int(advise arg) const
    {
#if HAVE_POSIX_FADVISE
	switch(arg)
	{
	case advise_normal:
	    return POSIX_FADV_NORMAL;
	case advise_sequential:
	    return POSIX_FADV_SEQUENTIAL;
	case advise_random:
	    return POSIX_FADV_RANDOM;
	case advise_noreuse:
	    return POSIX_FADV_NOREUSE;
	case advise_willneed:
	    return POSIX_FADV_WILLNEED;
	case advise_dontneed:
	    return POSIX_FADV_DONTNEED;
	default:
	    throw SRC_BUG;
	}
#else
	return 0;
#endif
    }

    off_t fichier_local::get_eof_offset() const
    {
	off_t ret;
	off_t tmp;
	off_t cur = lseek(filedesc, 0, SEEK_CUR);

	if(cur < 0)
	    throw Erange("fichier_local::get_eof_offset()", string("Error while reading current file offset: ") + tools_strerror_r(errno));

	ret = lseek(filedesc, 0, SEEK_END);
	if(ret < 0)
	    throw Erange("fichier_local::get_eof_offset()", string("Error while reading current file offset: ") + tools_strerror_r(errno));

	tmp = lseek(filedesc, cur, SEEK_SET);
	if(tmp < 0)
	    throw Erange("fichier_local::get_eof_offset()", string("Error while seeking back to previous offset: ") + tools_strerror_r(errno));
	if(tmp != cur)
	    throw SRC_BUG;

	return ret;
    }

} // end of namespace
