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

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif
} // end extern "C"

#include "tuyau.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "integers.hpp"
#include "cygwin_adapt.hpp"

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

    static gf_mode generic_file_get_mode(S_I fd);

    tuyau::tuyau(const shared_ptr<user_interaction> & dialog, S_I fd) : generic_file(generic_file_get_mode(fd)), mem_ui(dialog)
    {
	gf_mode tmp;

        if(fd < 0)
            throw Erange("tuyau::tuyau", "Bad file descriptor given");
	tmp = generic_file_get_mode(fd);
	if(tmp == gf_read_write)
	    throw Erange("tuyau::tuyau", tools_printf("A pipe cannot be in read and write mode at the same time, I need precision on the mode to use for the given filedscriptor"));
	pipe_mode = pipe_fd;
        filedesc = fd;
        position = 0;
	other_end_fd = -1;
	has_one_to_read = false;
    }

    tuyau::tuyau(const shared_ptr<user_interaction> & dialog, S_I fd, gf_mode mode) : generic_file(mode), mem_ui(dialog)
    {
        gf_mode tmp;

        if(fd < 0)
            throw Erange("tuyau::tuyau", "Bad file descriptor given");
	if(mode == gf_read_write)
	    throw Erange("tuyau::tuyau", tools_printf("A pipe cannot be in read and write mode at the same time"));
        tmp = generic_file_get_mode(fd);
        if(tmp != gf_read_write && tmp != mode)
            throw Erange("tuyau::tuyau", tools_printf("%s cannot be restricted to %s", generic_file_get_name(tmp), generic_file_get_name(mode)));
	pipe_mode = pipe_fd;
        filedesc = fd;
        position = 0;
	other_end_fd = -1;
	has_one_to_read = false;
    }

    tuyau::tuyau(const shared_ptr<user_interaction> & dialog, const string & filename, gf_mode mode) : generic_file(mode), mem_ui(dialog)
    {
	pipe_mode = pipe_path;
        chemin = filename;
        position = 0;
	other_end_fd = -1;
	has_one_to_read = false;
    }

    tuyau::tuyau(const shared_ptr<user_interaction> & dialog) : generic_file(gf_write_only), mem_ui(dialog)
    {
	int tube[2];

	if(pipe(tube) < 0)
	    throw Erange("tuyau::tuyau", string(gettext("Error while creating anonymous pipe: ")) + tools_strerror_r(errno));
	pipe_mode = pipe_both;
	position = 0;
	filedesc = tube[1];
	other_end_fd = tube[0];
	has_one_to_read = false;
    }

    tuyau::~tuyau()
    {
	try
	{
	    terminate();
	}
	catch(...)
	{
		// ignore all exceptions
	}
    }

    int tuyau::get_read_fd() const
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(pipe_mode == pipe_both)
	    return other_end_fd;
	else
	    throw Erange("tuyau::get_read_fd", gettext("Pipe's other end is not known, cannot provide a filedescriptor on it"));
    }

    void tuyau::close_read_fd()
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(pipe_mode == pipe_both)
	{
	    close(other_end_fd);
	    pipe_mode = pipe_fd;
	}
	else
	    throw Erange("tuyau::get_read_fd", gettext("Pipe's other end is not known, cannot close any filedescriptor pointing on it"));
    }

    void tuyau::do_not_close_read_fd()
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(pipe_mode == pipe_both)
	    pipe_mode = pipe_fd;
	else
	    throw Erange("tuyau::get_read_fd", "Pipe's other end is not known, there is no reason to ask not to close a filedescriptor on it");
    }

    bool tuyau::skippable(skippability direction, const infinint & amount)
    {
	if(get_mode() == gf_read_only)
	    return direction == skip_forward;
	else
	    return false;
    }

    bool tuyau::skip(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(pos < position)
            throw Erange("tuyau::skip", "Skipping backward is not possible on a pipe");
	else
	    if(pos == position)
		return true;
	    else
		return read_and_drop(pos - position);
    }

    bool tuyau::skip_to_eof()
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(get_mode() != gf_write_only)
	    return read_to_eof();
	else
	    return true;
    }

    bool tuyau::skip_relative(S_I x)
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(x < 0)
            throw Erange("tuyau::skip", "Skipping backward is not possible on a pipe");
	else
	    return read_and_drop(x);
    }

    bool tuyau::has_next_to_read()
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(has_one_to_read)
	    return true;
	else
	{
	    S_I ret = ::read(filedesc, &next_to_read, 1);
	    if(ret <= 0)
		return false;
	    else
	    {
		has_one_to_read = true;
		return true;
	    }
	}
    }

    U_I tuyau::inherited_read(char *a, U_I size)
    {
        S_I ret;
        U_I lu = 0;

#ifdef MUTEX_WORKS
	check_self_cancellation();
#endif
	ouverture();

	switch(pipe_mode)
	{
	case pipe_fd:
	case pipe_both:
	    break;
	case pipe_path:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}

	if(size == 0)
	    return 0;

	if(has_one_to_read)
	{
	    a[lu] = next_to_read;
	    ++lu;
	    has_one_to_read = false;
	}

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
                case EIO:
                    throw Ehardware("tuyau::inherited_read", "");
                default:
                    throw Erange("tuyau::inherited_read", string(gettext("Error while reading from pipe: "))+tools_strerror_r(errno));
                }
            }
            else
                if(ret > 0)
                    lu += ret;
        }
        while(lu < size && ret > 0);
        position += lu;

        return lu;
    }

    void tuyau::inherited_write(const char *a, U_I size)
    {
        U_I total = 0;
	ssize_t ret;
#ifdef SSIZE_MAX
	static const U_I step = SSIZE_MAX/2;
#else
	const U_I step = size; // which is no limit...
#endif


#ifdef MUTEX_WORKS
	check_self_cancellation();
#endif

	ouverture();

	switch(pipe_mode)
	{
	case pipe_fd:
	case pipe_both:
	    break;
	case pipe_path:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}

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
                    throw Ehardware("tuyau::inherited_write", string(gettext("Error while writing data to pipe: ")) + tools_strerror_r(errno));
                case ENOSPC:
                    get_ui().pause(gettext("No space left on device, you have the opportunity to make room now. When ready : can we continue ?"));
                    break;
                default :
                    throw Erange("tuyau::inherited_write", string(gettext("Error while writing data to pipe: ")) + tools_strerror_r(errno));
                }
            }
            else
                total += (U_I)ret;
        }

        position += total;
    }

    void tuyau::inherited_terminate()
    {
	switch(pipe_mode)
	{
	case pipe_both:
	    close(other_end_fd);
		// no break here !
	case pipe_fd:
	    other_end_fd = -1;
	    close(filedesc);
	    filedesc = -1;
	    break;
	case pipe_path:
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    void tuyau::ouverture()
    {
        if(pipe_mode == pipe_path)
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
	    filedesc = ::open(chemin.c_str(), flag|O_BINARY);
	    if(filedesc < 0)
		throw Erange("tuyau::ouverture", string(gettext("Error opening pipe: "))+tools_strerror_r(errno));
	    pipe_mode = pipe_fd;
        }
    }

    bool tuyau::read_and_drop(infinint byte)
    {
	char buffer[BUFFER_SIZE];
	U_I u_step;
	U_I step, max_i_step = 0;
	S_I lu;
	bool eof = false;


	max_i_step = int_tools_maxof_agregate(max_i_step);
	if(max_i_step <= 0)
	    throw SRC_BUG; // error in max positive value calculation, just above

	if(max_i_step > BUFFER_SIZE)
	    max_i_step = BUFFER_SIZE; // max read a time

	if(get_mode() != gf_read_only)
	    throw Erange("tuyau::read_and_drop", "Cannot skip in pipe in writing mode");

	u_step = 0;
	byte.unstack(u_step);

	do
	{
	    while(u_step > 0 && !eof)
	    {
		if(u_step > max_i_step)
		    step = max_i_step;
		else
		    step = u_step;

		lu = read(buffer, step);
		if(lu < 0)
		    throw SRC_BUG;
		if((U_I)lu < step)
		    eof = true;
		u_step -= lu;
		position += lu;
	    }
	    if(!eof)
	    {
		u_step = 0;
		byte.unstack(u_step);
	    }
	}
	while(u_step > 0 && !eof);

	if(!byte.is_zero())
	    throw SRC_BUG;

	return !eof;
    }

    bool tuyau::read_to_eof()
    {
	char buffer[BUFFER_SIZE];
	S_I lu = 0;

	if(get_mode() != gf_read_only)
	    throw Erange("tuyau::read_and_drop", "Cannot skip in pipe in writing mode");

	while((lu = read(buffer, BUFFER_SIZE)) > 0)
	    position += lu;

	return true;
    }

    static gf_mode generic_file_get_mode(S_I fd)
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


} // end of namespace
