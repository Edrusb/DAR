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
// $Id: generic_file.cpp,v 1.23.2.1 2005/03/13 20:07:51 edrusb Rel $
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
} // end extern "C"

#include "infinint.hpp"
#include "generic_file.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "user_interaction.hpp"
#include "cygwin_adapt.hpp"
#include "int_tools.hpp"

using namespace std;

namespace libdar
{

    void clear(crc & value)
    {
        for(S_I i = 0; i < CRC_SIZE; i++)
            value[i] = '\0';
    }

    bool same_crc(const crc &a, const crc &b)
    {
        S_I i = 0;
        while(i < CRC_SIZE && a[i] == b[i])
            i++;
        return i == CRC_SIZE;
    }

    void copy_crc(crc & dst, const crc & src)
    {
        for(S_I i = 0; i < CRC_SIZE; i++)
            dst[i] = src[i];
    }

#define BUFFER_SIZE 102400

    S_I generic_file::read(char *a, size_t size)
    {
        if(rw == gf_write_only)
            throw Erange("generic_file::read", gettext("Reading a write only generic_file"));
        else
            return (this->*active_read)(a, size);
    }

    S_I generic_file::write(char *a, size_t size)
    {
        if(rw == gf_read_only)
            throw Erange("generic_file::write", gettext("Writing to a read only generic_file"));
        else
            return (this->*active_write)(a, size);
    }

    S_I generic_file::write(const string & arg)
    {
        S_I ret = 0;
        if(arg.size() > int_tools_maxof_agregate(size_t(0)))
            throw SRC_BUG;
        size_t size = arg.size();

        char *ptr = tools_str2charptr(arg);
        try
        {
            ret = write(ptr, size);
        }
        catch(...)
        {
            delete [] ptr;
            throw;
        }
        delete [] ptr;

        return ret;
    }

    S_I generic_file::read_back(char &a)
    {
        if(skip_relative(-1))
        {
            S_I ret = read(&a,1);
            skip_relative(-1);
            return ret;
        }
        else
            return 0;
    }

    void generic_file::copy_to(generic_file & ref)
    {
        char buffer[BUFFER_SIZE];
        S_I lu, ret = 0;

        do
        {
            lu = this->read(buffer, BUFFER_SIZE);
            if(lu > 0)
                ret = ref.write(buffer, lu);
        }
        while(lu > 0 && ret > 0);
    }

    void generic_file::copy_to(generic_file & ref, crc & value)
    {
        reset_crc();
        copy_to(ref);
        get_crc(value);
    }

    U_32 generic_file::copy_to(generic_file & ref, U_32 size)
    {
        char buffer[BUFFER_SIZE];
        S_I lu = 1, ret = 1, pas;
        U_32 wrote = 0;

        while(wrote < size && ret > 0 && lu > 0)
        {
            pas = size > BUFFER_SIZE ? BUFFER_SIZE : size;
            lu = read(buffer, pas);
            if(lu > 0)
            {
                ret = ref.write(buffer, lu);
                wrote += lu;
            }
        }

        return wrote;
    }

    infinint generic_file::copy_to(generic_file & ref, infinint size)
    {
        U_32 tmp = 0, delta;
        infinint wrote = 0;

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

    bool generic_file::diff(generic_file & f)
    {
        char buffer1[BUFFER_SIZE];
        char buffer2[BUFFER_SIZE];
        S_I lu1 = 0, lu2 = 0;
        bool diff = false;

        if(get_mode() == gf_write_only || f.get_mode() == gf_write_only)
            throw Erange("generic_file::diff", gettext("Cannot compare files in write only mode"));
        skip(0);
        f.skip(0);
        do
        {
            lu1 = read(buffer1, BUFFER_SIZE);
            lu2 = f.read(buffer2, BUFFER_SIZE);
            if(lu1 == lu2)
            {
                register S_I i = 0;
                while(i < lu1 && buffer1[i] == buffer2[i])
                    i++;
                if(i < lu1)
                    diff = true;
            }
            else
                diff = true;
        }
        while(!diff && lu1 > 0);

        return diff;
    }

    void generic_file::reset_crc()
    {
        if(active_read == &generic_file::read_crc)
            throw SRC_BUG; // crc still active, previous CRC value never read
        clear(value);
        enable_crc(true);
        crc_offset = 0;
    }

    void generic_file::enable_crc(bool mode)
    {
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

    void generic_file::compute_crc(char *a, S_I size)
    {
	S_I initial = crc_offset == 0 ? 0 : CRC_SIZE - crc_offset;
	S_I final = size < initial ? 0 : ((size - initial) / CRC_SIZE)*CRC_SIZE + initial;
	S_I i;
	S_I index = crc_offset;

	    // initial bytes
	if(initial > size)
	    initial = size;

	for(i = 0; i < initial; i++, index++)
	    value[index % CRC_SIZE] ^= a[i];


	    // block bytes

	register U_16 *bcrc = (U_16 *)(&value);
	register U_16 *blck = (U_16 *)(a + initial);
	register U_16 *stop = (U_16 *)(a + final);

	if(sizeof(*blck) != CRC_SIZE)
	    throw SRC_BUG;

	while(blck < stop)
	    *bcrc ^= *(blck++);


	    // final bytes

	index = 0;
	for(i = final; i < size; i++, index++)
	    value[index % CRC_SIZE] ^= a[i];

	crc_offset = (crc_offset + size) % CRC_SIZE;
    }

    S_I generic_file::read_crc(char *a, size_t size)
    {
        S_I ret = inherited_read(a, size);
        compute_crc(a, ret);
        return ret;
    }

    S_I generic_file::write_crc(char *a, size_t size)
    {
        S_I ret = inherited_write(a, size);
        compute_crc(a, ret);
        return ret;
    }

    void generic_file::copy_from(const generic_file & ref)
    {
	rw = ref.rw;
	copy_crc(value, ref.value);
	crc_offset = ref.crc_offset;
	gf_ui = ref.gf_ui->clone();
    }

    fichier::fichier(user_interaction & dialog, S_I fd) : generic_file(dialog, generic_file_get_mode(fd))
    {
        filedesc = fd;
    }

    fichier::fichier(user_interaction & dialog, char *name, gf_mode m) : generic_file(dialog, m)
    {
        fichier::open(name, m);
    }

    fichier::fichier(user_interaction & dialog, const path &chemin, gf_mode m) : generic_file(dialog, m)
    {
        char *name = tools_str2charptr(chemin.display());

        try
        {
            open(name, m);
        }
        catch(Egeneric & e)
        {
            delete [] name;
            throw;
        }
        delete [] name;
    }

    infinint fichier::get_size() const
    {
        struct stat dat;
        infinint filesize;

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
        if(lseek(filedesc, 0, SEEK_SET) < 0)
            return false;

        do
	{
            delta = 0;
            pos.unstack(delta);
            if(delta > 0)
                if(lseek(filedesc, delta, SEEK_CUR) < 0)
                    return false;
        } while(delta > 0);

        return true;
    }

    bool fichier::skip_to_eof()
    {
        return lseek(filedesc, 0, SEEK_END) >= 0;
    }

    bool fichier::skip_relative(S_I x)
    {
        if(x > 0)
            if(lseek(filedesc, x, SEEK_CUR) < 0)
                return false;
            else
                return true;

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

    static void dummy_call(char *x)
    {
        static char id[]="$Id: generic_file.cpp,v 1.23.2.1 2005/03/13 20:07:51 edrusb Rel $";
        dummy_call(id);
    }

    infinint fichier::get_position()
    {
        off_t ret = lseek(filedesc, 0, SEEK_CUR);

        if(ret == -1)
            throw Erange("fichier::get_position", string(gettext("Error getting file reading position: ")) + strerror(errno));

        return ret;
    }

    S_I fichier::inherited_read(char *a, size_t size)
    {
        S_I ret;
        U_I lu = 0;

#ifdef MUTEX_WORKS
	check_self_cancellation();
#endif
        do
        {
            ret = ::read(filedesc, a+lu, size-lu);
            if(ret < 0)
            {
                switch(errno)
                {
                case EINTR:
                    break;
                case EAGAIN:
                    throw SRC_BUG; // non blocking read not compatible with
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

    S_I fichier::inherited_write(char *a, size_t size)
    {
        S_I ret;
        size_t total = 0;

#ifdef MUTEX_WORKS
	check_self_cancellation();
#endif
        while(total < size)
        {
            ret = ::write(filedesc, a+total, size-total);
            if(ret < 0)
            {
                switch(errno)
                {
                case EINTR:
                    break;
                case EIO:
                    throw Ehardware("fichier::inherited_write", string(gettext("Error while writing to file: ")) + strerror(errno));
                case ENOSPC:
                    get_gf_ui().pause(gettext("No space left on device, you have the opportunity to make room now. When ready : can we continue ?"));
                    break;
                default :
                    throw Erange("fichier::inherited_write", string(gettext("Error while writing to file: ")) + strerror(errno));
                }
            }
            else
                total += ret;
        }

        return total;
    }

    void fichier::open(const char *name, gf_mode m)
    {
        S_I mode;
        S_I perm = 0777;

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

        do
        {
            filedesc = ::open(name, mode|O_BINARY, perm);
            if(filedesc < 0)
                if(filedesc == ENOSPC)
                    get_gf_ui().pause(gettext("No space left for inode, you have the opportunity to make some room now. When done : can we continue ?"));
                else
                    throw Erange("fichier::open", string(gettext("Cannot open file : ")) + strerror(errno));
        }
        while(filedesc == ENOSPC);
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
