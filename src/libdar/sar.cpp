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

#include "../my_config.h"

extern "C"
{
// this was necessary to compile under Mac OS-X (boggus dirent.h)
#if HAVE_STDINT_H
#include <stdint.h>
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

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

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if STDC_HEADERS
#include <stdlib.h>
#endif


} // end extern "C"

#include <new>

#include "sar.hpp"
#include "deci.hpp"
#include "user_interaction.hpp"
#include "tools.hpp"
#include "erreurs.hpp"
#include "cygwin_adapt.hpp"
#include "deci.hpp"
#include "fichier.hpp"

using namespace std;

namespace libdar
{

    static bool sar_extract_num(const string & filename, const string & base_name, const infinint & min_digits, const string & ext, infinint & ret);
    static bool sar_get_higher_number_in_dir(const path & dir, const string & base_name, const infinint & min_digits, const string & ext, infinint & ret);
    static string sar_make_padded_number(const string & num, const infinint & min_digits);

    sar::sar(user_interaction & dialog,
	     const string & base_name,
	     const string & extension,
	     const path & dir,
	     bool by_the_end,
	     const infinint & x_min_digits,
	     bool x_lax,
	     const string & execute) : generic_file(gf_read_only), mem_ui(dialog), archive_dir(dir)
    {
	opt_warn_overwrite = true;
	opt_allow_overwrite = false;
        natural_destruction = true;
        base = base_name;
        ext = extension;
        initial = true;
        hook = execute;
        set_info_status(CONTEXT_INIT);
	old_sar = false; // will be set to true at header read time a bit further if necessary
	perm = 0;
	slice_user = "";
	slice_group = "";
	hash = hash_none;
	lax = x_lax;
	min_digits = x_min_digits;

        open_file_init();
	try
	{
	    if(by_the_end)
	    {
		try
		{
		    skip_to_eof();
		}
		catch(Erange & e)
		{
		    string tmp = e.get_message();
 		    dialog.printf(gettext("Error met while opening the last slice: %S. Trying to open the archive using the first slice..."), &tmp);
		    open_file(1);
		}
	    }
	    else
		open_file(1);
	}
	catch(...)
	{
	    try
	    {
		close_file(true);
	    }
	    catch(...)
	    {
		if(of_fd != NULL)
		{
		    delete of_fd;
		    of_fd = NULL;
		}
	    }
	    throw;
	}
    }

    sar::sar(user_interaction & dialog,
	     const string & base_name,
	     const string & extension,
	     const infinint & file_size,
	     const infinint & first_file_size,
	     bool x_warn_overwrite,
	     bool x_allow_overwrite,
	     const infinint & x_pause,
	     const path & dir,
	     const label & data_name,
	     const string & slice_permission,
	     const string & slice_user_ownership,
	     const string & slice_group_ownership,
	     hash_algo x_hash,
	     const infinint & x_min_digits,
	     const string & execute) : generic_file(gf_write_only), mem_ui(dialog), archive_dir(dir)
    {
        if(file_size < header::min_size() + 1)  //< one more byte to store at least one byte of data
            throw Erange("sar::sar", gettext("File size too small"));
	    // note that this test does not warranty that the file is large enough to hold a header structure

	if(first_file_size < header::min_size() + 1)
	    throw Erange("sar::sar", gettext("First file size too small"));
	    // note that this test does not warranty that the file is large enough to hold a header structure

        initial = true;
	lax = false;
	opt_warn_overwrite = x_warn_overwrite;
	opt_allow_overwrite = x_allow_overwrite;
        natural_destruction = true;
        base = base_name;
        ext = extension;
        size = file_size;
        first_size = first_file_size;
        hook = execute;
	pause = x_pause;
	perm = tools_octal2int(slice_permission);
	slice_user = slice_user_ownership;
	slice_group = slice_group_ownership;
	hash = x_hash;
	min_digits = x_min_digits;
        set_info_status(CONTEXT_OP);
	of_internal_name.generate_internal_filename();
	if(data_name.is_cleared())
	    of_data_name = of_internal_name;
	else
	    of_data_name = data_name;
	of_fd = NULL;
	of_flag = '\0';
	old_sar = false; // sar creation does not makes a sar using an old header, this is always false within this constructor

	try
	{
	    open_file_init();
	    open_file(1);
	}
	catch(...)
	{
	    try
	    {
		close_file(true);
	    }
	    catch(...)
	    {
		if(of_fd != NULL)
		{
		    delete of_fd;
		    of_fd = NULL;
		}
	    }
	    throw;
	}
    }

    void sar::inherited_terminate()
    {
        close_file(true);
        if(get_mode() == gf_write_only && natural_destruction)
	{
	    set_info_status(CONTEXT_LAST_SLICE);
            hook_execute(of_current);
	}
    }

    sar::~sar()
    {
	try
	{
	    terminate();
	}
	catch(...)
	{
		// ignore all exception
	}
    }

    bool sar::skip(const infinint &pos)
    {
        infinint byte_in_first_file = first_size - first_file_offset;
        infinint byte_per_file = size - other_file_offset;
        infinint dest_file, offset;

	if(is_terminated())
	    throw SRC_BUG;

	if(!old_sar)
	{
	    --byte_in_first_file;
	    --byte_per_file;
		// this is due to the trailing flag (one byte length)
	}

        if(get_position() == pos)
            return true; // no need to skip

            ///////////////////////////
            // determination of the file to go and its offset to seek in
            //
        if(pos < byte_in_first_file)
        {
            dest_file = 1;
            offset = pos + first_file_offset;
        }
        else
        {
	    euclide(pos - byte_in_first_file, byte_per_file, dest_file, offset);
            dest_file += 2;
                // "+2" because file number starts to 1 and first file is already counted
            offset += other_file_offset;
        }

            ///////////////////////////
            // checking whether the required position is acceptable
            //
        if(of_last_file_known && dest_file > of_last_file_num)
        {
                // going to EOF
            open_file(of_last_file_num);
            of_fd->skip_to_eof();
            file_offset = of_fd->get_position();
            return false;
        }
        else
        {
            try
            {
                open_file(dest_file);
                set_offset(offset);
                file_offset = offset;
                return true;
            }
            catch(Erange & e)
            {
                return false;
            }
        }
    }

    bool sar::skip_to_eof()
    {
        bool ret;

	if(is_terminated())
	    throw SRC_BUG;

        open_last_file();
	if(of_fd == NULL)
	    throw SRC_BUG;
        ret = of_fd->skip_to_eof();
	if(!old_sar)
	    of_fd->skip_relative(-1);
        file_offset = of_fd->get_position();
        set_offset(file_offset);

        return ret;
    }

    bool sar::skip_forward(U_I x)
    {
        infinint number = of_current;
        infinint offset = file_offset + x;
	infinint delta = old_sar ? 0 : 1; // one byte less per slice with archive format >= 8

	if(is_terminated())
	    throw SRC_BUG;

        while((number == 1 ? offset+delta >= first_size : offset+delta >= size)
              && (!of_last_file_known || number <= of_last_file_num))
        {
            offset -= number == 1 ? first_size-delta : size-delta;
            offset += other_file_offset;
            number++;
        }

        if(number == 1 ? offset+delta < first_size : offset+delta < size)
        {
            open_file(number);
            file_offset = offset;
            set_offset(file_offset);
            return true;
        }
        else
            return false;
    }

    bool sar::skip_backward(U_I x)
    {
        infinint number = of_current;
        infinint offset = file_offset;
        infinint offset_neg = x;
	infinint delta = old_sar ? 0 : 1; // one byte less per slice with archive format >= 8

	if(is_terminated())
	    throw SRC_BUG;

        while(number > 1 && offset_neg + other_file_offset > offset)
        {
            offset_neg -= offset - other_file_offset + 1;
            number--;
            if(number > 1)
                offset = size - 1 - delta;
            else
                offset = first_size - 1 - delta;
        }

        if((number > 1 ? offset_neg + other_file_offset : offset_neg + first_file_offset) <= offset)
        {
            open_file(number);
            file_offset = offset - offset_neg;
            set_offset(file_offset);
            return true;
        }
        else
        {   // seek to beginning of file
            open_file(1);
            set_offset(first_file_offset);
            return false;
        }
    }

    bool sar::skip_relative(S_I x)
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(x > 0)
            return skip_forward(x);

        if(x < 0)
            return skip_backward(-x);

        return true; // when x == 0
    }

    infinint sar::get_position()
    {
	infinint delta = old_sar ? 0 : 1; // one byte less per slice with archive format >= 8

	if(is_terminated())
	    throw SRC_BUG;

        if(of_current > 1)
            return first_size - first_file_offset - delta + (of_current-2)*(size - other_file_offset - delta) + file_offset - other_file_offset;
        else
            return file_offset - first_file_offset;
    }

    U_I sar::inherited_read(char *a, U_I sz)
    {
        U_I lu = 0;
        bool loop = true;

        while(lu < sz && loop)
        {
	    U_I tmp;
	    if(of_fd != NULL)
	    {
		try
		{
		    tmp = of_fd->read(a+lu, sz-lu);
		    if(!old_sar && of_fd->get_position() == size_of_current)
			if(tmp > 0)
			    --tmp; // we do not "read" the terminal flag
		}
		catch(Euser_abort & e)
		{
		    natural_destruction = false;
			// avoid the execution of "between slice" user commands
		    throw;
		}
	    }
	    else
		tmp = 0; // simulating an end of slice

	    if(tmp == 0)
		if(of_flag == flag_type_terminal)
		    loop = false;
		else
		    if(is_current_eof_a_normal_end_of_slice())
			open_file(of_current + 1);
		    else // filling zeroed bytes in place of the missing part of the slice
		    {
			infinint avail = bytes_still_to_read_in_slice();
			U_I place = sz-lu;

			if(avail < place)
			{
			    place = 0;
			    avail.unstack(place);
			}
			(void)memset(a+lu, 0, place);
			lu += place;
			file_offset += place;
		    }
	    else
	    {
		lu += tmp;
		file_offset += tmp;
	    }
        }

        return lu;
    }

    void sar::inherited_write(const char *a, U_I sz)
    {
        infinint to_write = sz;
        infinint max_at_once;
        infinint tmp_wrote;
        U_I micro_wrote;

        while(to_write > 0)
        {
            max_at_once = of_current == 1 ? (first_size - file_offset) - 1 : (size - file_offset) - 1;
            tmp_wrote = max_at_once > to_write ? to_write : max_at_once;
            if(tmp_wrote > 0)
            {
                micro_wrote = 0;
                tmp_wrote.unstack(micro_wrote);
		try
		{
		    of_fd->write(a, micro_wrote);
		    to_write -= micro_wrote;
		    file_offset += micro_wrote;
		    a += micro_wrote;
		}
		catch(Euser_abort & e)
		{
		    natural_destruction = false;
			// avoid the execution of "between slice" user commands
		    throw;
		}
            }
            else
            {
                open_file(of_current + 1);
                continue;
            }
        }
    }

    void sar::close_file(bool terminal)
    {
	bool bug = false;

        if(of_fd != NULL)
        {
	    char flag = terminal ? flag_type_terminal : flag_type_non_terminal;
	    if(get_mode() == gf_write_only)
	    {
		if(of_fd->get_position() != of_fd->get_size())
		    bug = true; // we should be at the end of the file
		else
		    of_fd->write(&flag, 1);
	    }
	    of_fd->terminate();

            delete of_fd;
            of_fd = NULL;
        }

	if(bug)
	    throw SRC_BUG;
    }

    void sar::open_readonly(const char *fic, const infinint &num)
    {
        header h;

        while(of_fd == NULL)
        {
                // launching user command if any
            hook_execute(num);

                // trying to open the file
                //
            S_I fd = ::open(fic, O_RDONLY|O_BINARY);
            if(fd < 0)
	    {
		try
		{
		    if(errno == ENOENT)
		    {
			if(!lax)
			    get_ui().pause(tools_printf(gettext("%s is required for further operation, please provide the file."), fic));
			else
			    get_ui().pause(tools_printf(gettext("%s is required for further operation, please provide the file if you have it."), fic));
			continue; // we restart the while loop
		    }
		    else
			throw Erange("sar::open_readonly", tools_printf(gettext("Error opening %s : "), fic) +  strerror(errno));
		}
		catch(Euser_abort & e)
		{
		    if(lax)
		    {
			get_ui().warning(string(gettext("LAX MODE: Caught exception: "))+ e.get_message());
			get_ui().pause(tools_printf(gettext("LAX MODE: %s is missing, You have the possibility to create a zero byte length file under the name of this slice, to replace this missing file. This will of course generate error messages about the information that is missing in this slice, but at least libdar will be able to continue. Can we continue now?"), fic));
			continue; // we restart the while loop
		    }
		    else
			throw;
		}
	    }
            else
	    {
                of_fd = new (nothrow) fichier(get_ui(), fd);
		if(of_fd == NULL)
		    throw Ememory("sar::open_readonly");
		size_of_current = of_fd->get_size();
	    }


	    if(of_fd == NULL)
		throw SRC_BUG;

                // trying to read the header
                //
            try
            {
                h.read(get_ui(), *of_fd, lax);
            }
	    catch(Ethread_cancel & e)
	    {
		throw;
	    }
	    catch(Euser_abort & e)
	    {
		throw;
	    }
	    catch(Efeature & e)
	    {
		throw;
	    }
	    catch(Elimitint & e)
	    {
		throw;
	    }
            catch(Egeneric & e)
            {
		if(!lax)
		{
		    close_file(false);
		    get_ui().pause(tools_printf(gettext("%s has a bad or corrupted header, please provide the correct file."), fic));
		    continue;
		}
		else
		    get_ui().warning(tools_printf(gettext("LAX MODE: %s has a bad or corrupted header, trying to guess original values and continuing if possible"), fic));
            }

                // checking against the magic number
                //
            if(h.get_set_magic() != SAUV_MAGIC_NUMBER)
            {
		if(!lax)
		{
		    close_file(false);
		    get_ui().pause(tools_printf(gettext("%s is not a valid file (wrong magic number), please provide the good file."), fic));
		    continue;
		}
		else
		    get_ui().warning(tools_printf(gettext("LAX MODE: In spite of its name, %s does not appear to be a dar slice, assuming a data corruption took place and continuing"), fic));
            }

	    if(h.is_old_header() && first_file_offset == 0 && num != 1)
		throw Erange("sar::open_readonly", gettext("This is an old archive, it can only be opened starting by the first slice"));

                // checking the ownership of the set of file (= slice of the same archive or not)
                //
            if(first_file_offset == 0) // this is the first time we open a slice for this archive, we don't even know the slices size
            {
                of_internal_name = h.get_set_internal_name();
		of_data_name = h.get_set_data_name();
                try
                {
		    if(!h.get_slice_size(size))
		    {
			if(!lax)
			    throw SRC_BUG;  // slice size should be known or determined by header class
			else
			    size = 0;
		    }
		    if(!h.get_first_slice_size(first_size))
			first_size = size;

		    if(first_size == 0 || size == 0) // only possible to reach this statment in lax mode
		    {
			try
			{
			    infinint tmp_num = 0;
			    string answ;

			    get_ui().pause(gettext("LAX MODE: Due to probable data corruption, dar could not determine the correct size of slices in this archive. For recent archive, this information is duplicated in each slice, do you want to try opening another slice to get this value if present?"));

			    do
			    {
				answ = get_ui().get_string(gettext("LAX MODE: Please provide the slice number to read: "), true);
				try
				{
				    deci tmp = answ;
				    tmp_num = tmp.computer();
				}
				catch(Edeci &e)
				{
				    get_ui().warning(gettext("LAX MODE: Please provide an strictly positive integer number"));
				    tmp_num = 0;
				}
			    }
			    while(tmp_num == 0);

			    get_ui().printf(gettext("LAX MODE: opening slice %i to read its slice header"), &tmp_num);
			    open_file(tmp_num);
			    get_ui().printf(gettext("LAX MODE: closing slice %i, header properly fetched"), &tmp_num);
			    close_file(false);
			    continue;
			}
			catch(Euser_abort & e)
			{
			    get_ui().warning(gettext("LAX MODE: In spite of a the absence of a known slice size, continuing anyway"));
			}
		    }

                    first_file_offset = of_fd->get_position();
		    other_file_offset = h.is_old_header() ? header::min_size() : first_file_offset;
		    if(first_file_offset >= first_size && !lax)
			throw Erange("sar::sar", gettext("Incoherent slice header: First slice size too small"));
		    if(other_file_offset >= size && !lax)
			throw Erange("sar::sar", gettext("incoherent slice header: Slice size too small"));
		    old_sar = h.is_old_header();
                }
                catch(Erange & e)
                {
                    close_file(false);
                    get_ui().pause(tools_printf(gettext("Error opening %s : "), fic) + e.get_message() + gettext(" . Retry ?"));
                    continue;
                }
            }
            else
	    {
                if(of_internal_name != h.get_set_internal_name())
                {
		    if(!lax)
		    {
			close_file(false);
			get_ui().pause(string(fic) + gettext(" is a slice from another backup, please provide the correct slice."));
			continue;
		    }
		    else
		    {
			get_ui().warning(gettext("LAX MODE: internal name of the slice leads dar to consider it is not member of the same archive. Assuming data corruption occurred and relying on the filename of this slice as proof of its membership to the archive"));
		    }
                }
	    }

                // checking the flag
                //
	    if(h.get_set_flag() == flag_type_located_at_end_of_slice)
	    {
		infinint current_pos = of_fd->get_position();
		char end_flag;

		of_fd->skip_to_eof();
		of_fd->skip_relative(-1);
		of_fd->read(&end_flag, 1); // reading the last char of the slice
		of_fd->skip(current_pos);

		switch(end_flag)
		{
		case flag_type_terminal:
		case flag_type_non_terminal:
		    h.get_set_flag() = end_flag;
		    break;
		case flag_type_located_at_end_of_slice:
		    if(!lax)
			throw Erange("sar::open_readonly", gettext("Data corruption met at end of slice, forbidden flag found at this position"));
		    else
			h.get_set_flag() = end_flag;
		    break;
		default:
		    if(!lax)
			throw Erange("sar::open_readonly", gettext("Data corruption met at end of slice, unknown flag found"));
		    else
			h.get_set_flag() = end_flag;
		    break;
		}
	    }

            switch(h.get_set_flag())
	    {
            case flag_type_terminal:
		if(of_last_file_known)
		{
		    if(of_last_file_num != num)
		    {
			if(!lax)
			    throw Erange("sar::open_readonly", tools_printf(gettext("Two different slices (%i and %i) are marked as the last slice of the backup!"), &of_last_file_num, &num));
			else
			{
			    get_ui().warning(tools_printf(gettext("LAX MODE: slices %i and %i are both recorded as last slice of the archive, keeping the higher number as the real last slice"), &of_last_file_num, &num));
			    if(num > of_last_file_num)
			    {
				of_last_file_num = num;
				of_last_file_size = of_fd->get_size();
			    }
			}
		    }
			// else nothing to do.
		}
		else
		{
		    of_last_file_known = true;
		    of_last_file_num = num;
		    of_last_file_size = of_fd->get_size();
		}
                break;
            case flag_type_non_terminal:
                break;
            default :
		if(!lax)
		{
		    close_file(false);
		    get_ui().pause(tools_printf(gettext("Slice %s has an unknown flag (neither terminal nor non_terminal file)."), fic));
		    continue;
		}
		else
		    if(of_max_seen <= num)
		    {
			string answ;

			do
			{
			    answ = get_ui().get_string(tools_printf(gettext("Due to data corruption, it is not possible to know if slice %s is the last slice of the archive or not. I need your help to figure out this. At the following prompt please answer either one of the following words: \"last\" or \"notlast\" according to the nature of this slice (you can also answer with \"abort\" to abort the program immediately): "), fic), true);
			}
			while(answ != gettext("last") && answ != gettext("notlast") && answ != gettext("abort"));

			if(answ == gettext("abort"))
			    throw Euser_abort("LAX MODE: Help the compression used...");
			if(answ == gettext("last"))
			{
			    of_last_file_known = true;
			    of_last_file_num = num;
			    of_last_file_size = of_fd->get_size();
			    h.get_set_flag() = flag_type_terminal;
			}
			else
			    h.get_set_flag() = flag_type_non_terminal;
		    }
		    else
		    {
			get_ui().warning(gettext("LAX MODE: Slice flag corrupted, but a slice of higher number has been seen, thus the header flag was surely not indicating this slice as the last of the archive. Continuing"));
			h.get_set_flag() = flag_type_non_terminal;
		    }
            }
            of_flag = h.get_set_flag();
	    if(lax)
	    {
		infinint tmp;
		if(!h.get_slice_size(tmp) || tmp == 0)
		{
			// a problem occured while reading slice header, however we know what is its expected size
			// so we seek the next read to the end of the slice header
		    if(num == 1)
			of_fd->skip(first_file_offset);
		    else
			of_fd->skip(other_file_offset);
		}
	    }
        }
    }

    void sar::open_writeonly(const char *fic, const infinint &num)
    {
	header h;
        S_I open_mode = perm;
        S_I fd = -1;
	bool unlink_on_error = false;

	try
	{
		// open succeeds only if this file does NOT already exist
	     fd = ::open(fic, O_WRONLY|O_BINARY|O_CREAT|O_EXCL, open_mode);

	    if(fd < 0) // open failed
	    {
		bool do_overwrite = false;

		if(errno == EEXIST) // file already exists
		{
		    S_I fd_tmp = ::open(fic, O_RDONLY|O_BINARY);

		    try
		    {
			if(fd_tmp >= 0)
			{
			    try
			    {
				h.read(get_ui(), fd_tmp);
			    }
			    catch(Erange & e)
			    {
				h.get_set_internal_name() = of_internal_name;
				h.get_set_internal_name().invert_first_byte();
				    // this way we are sure that the file is not considered as part of the current SAR
			    }
			    if(h.get_set_internal_name() != of_internal_name)
				do_overwrite = true; // this is not a slice of the current archive
			    close(fd_tmp);
			    fd_tmp = -1;
			}
			else // open read_only failed
			    do_overwrite = true;     // reading failed, trying overwriting (if allowed)
		    }
		    catch(...)
		    {
			if(fd_tmp >= 0)
			    close(fd_tmp);
			throw;
		    }
		}
		else // other error
		    throw Erange("sar::open_writeonly open()", string(gettext("Error opening file ")) + fic + " : " + strerror(errno));

		if(do_overwrite)
		{
		    if(!opt_allow_overwrite)
			throw Erange("sar::open_writeonly", gettext("file exists, and DONT_ERASE option is set."));
		    if(opt_warn_overwrite)
		    {
			try
			{
			    get_ui().pause(string(fic) + gettext(" is about to be overwritten."));
			    unlink_on_error = true;
			}
			catch(...)
			{
			    natural_destruction = false;
			    throw;
			}
		    }
		    else
			unlink_on_error = true;

			// open with overwriting
		    fd = ::open(fic, O_WRONLY|O_BINARY|O_TRUNC, open_mode);
		}
		else // open without overwriting
		    fd = ::open(fic, O_WRONLY|O_BINARY, open_mode);

		if(fd < 0)
		    throw Erange("sar::open_writeonly open()", string(gettext("Error opening file ")) + fic + " : " + strerror(errno));
		else
		    tools_set_ownership(fd, slice_user, slice_group);
	    }
	    else
		tools_set_ownership(fd, slice_user, slice_group);


	    if(fd < 0)
		throw SRC_BUG;

		// OK, now that the testing of the existence of the slice that we want to open is
		// done we can start the main stuf (writing data to the slice)

	    try
	    {
		of_flag = flag_type_located_at_end_of_slice;

		if(hash == hash_none)
		{
		    of_fd = new (nothrow) fichier(get_ui(), fd);
		    fd = -1;
		}
		else
		{
		    hash_fichier *tmp;
		    of_fd = tmp = new (nothrow) hash_fichier(get_ui(), fd);
		    fd = -1;
		    if(tmp != NULL)
		    {
			tmp->set_hash_file_name(fic, hash, hash_algo_to_string(hash));
			tmp->change_permission(perm);
			tmp->change_ownership(slice_user, slice_group);
		    }
		}
		if(of_fd == NULL)
		    throw Ememory("sar::open_writeonly");

		h = make_write_header(num, of_flag);
		h.write(get_ui(), *of_fd);
		if(num == 1)
		{
		    first_file_offset = of_fd->get_position();
		    if(first_file_offset == 0)
			throw SRC_BUG;
		    other_file_offset = first_file_offset; // same header in all slice since release 2.4.0
		    if(first_file_offset >= first_size)
			throw Erange("sar::sar", gettext("First slice size is too small to even just be able to drop the slice header"));
		    if(other_file_offset >= size)
			throw Erange("sar::sar", gettext("Slice size is too small to even just be able to drop the slice header"));
		}
	    }
	    catch(...)
	    {
		if(unlink_on_error)
		    unlink(fic);
		throw;
	    }
	}
	catch(...)
	{
	    if(fd >= 0)
		close(fd);
	    throw;
	}

    }

    void sar::open_file_init()
    {
        of_max_seen = 0;
        of_last_file_known = false;
        of_fd = NULL;
	of_flag = '\0';
        first_file_offset = 0; // means that the sizes have to be determined from file or wrote to file
	other_file_offset = 0;
	size_of_current = 0; // not used in write mode
    }

    void sar::open_file(infinint num)
    {
        if(of_fd == NULL || of_current != num)
        {
	    const string display = (archive_dir + path(sar_make_filename(base, num, min_digits, ext))).display();
	    const char *fic = display.c_str();

	    switch(get_mode())
	    {
	    case gf_read_only :
		close_file(false);
		    // launch the shell command before reading a slice
		open_readonly(fic, num);
		break;
	    case gf_write_only :

		    // adding the trailing flag

		if(of_fd != NULL)
		{
		    if(num > of_current || of_max_seen > of_current)
			close_file(false);
		    else // last slice of the set (at least it seems so)
			throw SRC_BUG; // should not skip backward in write_only mode
		}

		if(!initial)
		{

			// launch the shell command after the slice has been written
		    hook_execute(of_current);
		    if(pause != 0 && ((num-1) % pause == 0))
		    {
			deci conv = of_current;
			bool ready = false;

			while(!ready)
			{
			    try
			    {
				get_ui().pause(string(gettext("Finished writing to file ")) + conv.human() + gettext(", ready to continue ? "));
				ready = true;
			    }
			    catch(Euser_abort & e)
			    {
				get_ui().warning(string(gettext("If you really want to abort the archive creation hit CTRL-C, then press enter.")));
				ready = false;
			    }
			}
		    }
		}
		else
		    initial = false;

		open_writeonly(fic, num);
		break;
	    default :
		close_file(false);
		throw SRC_BUG;
	    }
	    of_current = num;
	    if(of_max_seen < of_current)
		of_max_seen = of_current;
	    file_offset = of_current == 1 ? first_file_offset : other_file_offset;
        }
    }

    void sar::set_offset(infinint offset)
    {
        if(of_fd == NULL)
            throw Erange("sar::set_offset", gettext("file not open"));
        else
            of_fd->skip(offset);
    }

    void sar::open_last_file()
    {
        infinint num;

        if(of_last_file_known)
            open_file(of_last_file_num);
        else // last slice number is not known
        {
            bool ask_user = false;

            while(of_fd == NULL || of_flag != flag_type_terminal)
            {
                if(sar_get_higher_number_in_dir(archive_dir, base, min_digits, ext, num))
                {
                    open_file(num);
                    if(of_flag != flag_type_terminal)
		    {
                        if(!ask_user)
                        {
                            close_file(false);
                            hook_execute(0); // %n replaced by 0 means last file is about to be requested
                            ask_user = true;
                        }
                        else
                        {
                            close_file(false);
			    get_ui().pause(string(gettext("The last file of the set is not present in ")) + archive_dir.display() + gettext(" , please provide it."));
                        }
		    }
                }
                else // not slice available in the directory
                    if(!ask_user)
                    {
                        hook_execute(0); // %n replaced by 0 means last file is about to be requested
                        ask_user = true;
                    }
                    else
                    {
			string chem = archive_dir.display();
                        close_file(false);
                        get_ui().pause(tools_printf(gettext("No backup file is present in %S for archive %S, please provide the last file of the set."), &chem, &base));
                    }
            }
        }
    }

    header sar::make_write_header(const infinint & num, char flag)
    {
        header hh;

        hh.get_set_magic() = SAUV_MAGIC_NUMBER;
        hh.get_set_internal_name() = of_internal_name;
	hh.get_set_data_name() = of_data_name;
        hh.get_set_flag() = flag;
	hh.set_slice_size(size);
	if(size != first_size)
	    hh.set_first_slice_size(first_size);

        return hh;
    }

    void sar::hook_execute(const infinint &num)
    {
        if(hook != "")
        {
	    try
	    {
		deci conv = num;
		string num_str = conv.human();

		tools_hook_substitute_and_execute(get_ui(),
						  hook,
						  archive_dir.display(),
						  base,
						  num_str,
						  sar_make_padded_number(num_str, min_digits),
						  ext,
						  get_info_status());
	    }
	    catch(Escript & g)
	    {
		natural_destruction = false;
		throw;
	    }
	}
    }

    bool sar::is_current_eof_a_normal_end_of_slice() const
    {
	infinint delta = old_sar ? 0 : 1; // one byte less per slice with archive format >= 8

	if(of_last_file_known && of_last_file_num == of_current) // we are in the last slice, thus eof may occur at any place
	    return true;

	    // we are not in the last slice, thus we can determine at which offset the eof must be met for this slice

	if(of_current == 1)
	    return file_offset >= first_size - delta;
	else
	    return file_offset >= size - delta;
    }

    infinint sar::bytes_still_to_read_in_slice() const
    {
	infinint delta = old_sar ? 0 : 1; // one byte less per slice with archive format >= 8

	if(of_last_file_known && of_last_file_num == of_current)
	    throw SRC_BUG; // cannot figure out the expected slice size of the last slice of the archive

	if(of_current == 1)
	    if(file_offset > first_size - delta)
		return 0;
	    else
		return first_size - file_offset - delta;
	else
	    if(file_offset > size - delta)
		return 0;
	    else
		return size - file_offset - delta;
    }

    static string sar_make_padded_number(const string & num, const infinint & min_digits)
    {
	string ret = num;

	while(infinint(ret.size()) < min_digits)
	    ret = string("0") + ret;

	return ret;
    }

    string sar_make_filename(const string & base_name, const infinint & num, const infinint & min_digits, const string & ext)
    {
        deci conv = num;
	string digits = conv.human();

        return base_name + '.' + sar_make_padded_number(digits, min_digits) + '.' + ext;
    }

    static bool sar_extract_num(const string & filename, const string & base_name, const infinint & min_digits, const string & ext, infinint & ret)
    {
        try
        {
	    U_I min_size = base_name.size() + ext.size() + 2; // 2 for two dot characters
	    if(filename.size() <= min_size)
		return false; // filename is too short

            if(infinint(filename.size() - min_size) < min_digits && min_digits != 0)
                return false; // not enough room for all digits

            if(filename.find(base_name) != 0) // checking that base_name is present at the beginning
                return false;

            if(filename.rfind(ext) != filename.size() - ext.size()) // checking that extension is at the end
                return false;

            deci conv = string(filename.begin()+base_name.size()+1, filename.begin() + (filename.size() - ext.size()-1));
            ret = conv.computer();
            return true;
        }
	catch(Ethread_cancel & e)
	{
	    throw;
	}
        catch(Egeneric &e)
        {
            return false;
        }
    }

    static bool sar_get_higher_number_in_dir(const path & dir, const string & base_name, const infinint & min_digits, const string & ext, infinint & ret)
    {
        infinint cur;
        bool somme = false;
        struct dirent *entry;
	const string display = dir.display();
        const char *folder = display.c_str();
        DIR *ptr = opendir(folder);

        try
        {
            if(ptr == NULL)
                throw Erange("sar_get_higher_number_in_dir", string(gettext("Error opening directory ")) + folder + " : " + strerror(errno));

            ret = 0;
            somme = false;
            while((entry = readdir(ptr)) != NULL)
                if(sar_extract_num(entry->d_name, base_name, min_digits, ext, cur))
                {
                    if(cur > ret)
                        ret = cur;
                    somme = true;
                }
        }
        catch(Egeneric & e)
        {
            if(ptr != NULL)
                closedir(ptr);
            throw;
        }

        if(ptr != NULL)
            closedir(ptr);
        return somme;
    }

/*****************************************************/


    trivial_sar::trivial_sar(user_interaction & dialog,
			     const std::string & base_name,
			     const std::string & extension,
			     const path & dir,
			     const label & data_name,
			     const std::string & execute,
			     bool allow_over,
			     bool warn_over,
			     const string & slice_permission,
			     const string & slice_user_ownership,
			     const string & slice_group_ownership,
			     hash_algo x_hash,
			     const infinint & min_digits) : generic_file(gf_write_only), mem_ui(dialog), archive_dir(dir)
    {
	S_I fd;
	generic_file *tmp = NULL;

	    // some local variables to be used

	const string filename = sar_make_filename((dir+base_name).display(), 1, min_digits, extension);
	const char *name = filename.c_str();

	    // initializing object fields from constructor arguments

	end_of_slice = 0;
	base = base_name;
	ext = extension;
	x_min_digits = min_digits;

	    // creating the slice if it does not exist else failing

	fd = ::open(name, O_WRONLY|O_BINARY|O_CREAT|O_EXCL, tools_octal2int(slice_permission));

	if(fd < 0) // open failed
	{
	    if(errno == EEXIST)
	    {
		if(!allow_over)
		    throw Erange("trivial_sar::trivial_sar", tools_printf(gettext("%S already exists, and overwritten is forbidden, aborting"), &filename));
		if(warn_over)
		    dialog.pause(tools_printf(gettext("%S is about to be overwritten, continue ?"), &filename));

		fd = ::open(name, O_WRONLY|O_BINARY|O_TRUNC, tools_octal2int(slice_permission));
		if(fd < 0)
		    throw Erange("trivial_sar::trivial_sar", tools_printf(gettext("Error opening file %s : %s"), name, strerror(errno)));
	    }
	    else
		throw Erange("trivial_sar::trivial_sar", tools_printf(gettext("Error opening file %s : %s"), name, strerror(errno)));
	}

	try
	{
	    tools_set_ownership(fd, slice_user_ownership, slice_group_ownership);
	    if(x_hash == hash_none)
		tmp = new (nothrow) fichier(dialog, fd);
	    else
	    {
		hash_fichier *tmp_hash;
		tmp = tmp_hash = new (nothrow) hash_fichier(dialog, fd);
		if(tmp_hash != NULL)
		{
		    tmp_hash->set_hash_file_name(name, x_hash, hash_algo_to_string(x_hash));
		    tmp_hash->change_permission(tools_octal2int(slice_permission));
		    tmp_hash->change_ownership(slice_user_ownership, slice_group_ownership);
		}
	    }
	    if(tmp == NULL)
		throw Ememory("trivial_sar::trivial_sar");
	}
	catch(...)
	{
	    if(tmp != NULL)
		delete tmp;
	    else
		::close(fd);
	    throw;
	}

	try
	{
	    build(dialog, tmp, data_name, execute);
	}
	catch(...)
	{
	    delete tmp;
	    throw;
	}
    }


    trivial_sar::trivial_sar(user_interaction & dialog,
			     const std::string & pipename,
			     bool lax) : generic_file(gf_read_only) , mem_ui(dialog), archive_dir("/")
    {
	reference = NULL;
	offset = 0;
	end_of_slice = 0;
	hook = "";
	base = "";
	ext = "";
	old_sar = false;
	x_min_digits = 0;
	set_info_status(CONTEXT_INIT);

	try
	{
	    if(pipename == "-")
		reference = new (nothrow) tuyau(dialog, 0);
	    else
		reference = new (nothrow) tuyau(dialog, pipename, gf_read_only);

	    if(reference == NULL)
		throw Ememory("trivial_sar::trivial_sar");

	    init();
	}
	catch(...)
	{
	    if(reference != NULL)
	    {
		delete reference;
		reference = NULL;
	    }
	    throw;
	}
    }

    trivial_sar::trivial_sar(user_interaction & dialog,
			     generic_file *f,
			     const label & data_name,
			     const std::string & execute) : generic_file(gf_write_only), mem_ui(dialog), archive_dir("/")
    {
	reference = NULL;
	offset = 0;
	end_of_slice = 0;
	hook = "";
	base = "";
	ext = "";
	old_sar = false;
	x_min_digits = 0;
	build(dialog, f, data_name, execute);
    }

    void trivial_sar::build(user_interaction & dialog,
			    generic_file *f,
			    const label & data_name,
			    const std::string & execute)
    {

	    // sanity check

	if(f == NULL)
	    throw SRC_BUG;

	    // contextual object part

	set_info_status(CONTEXT_LAST_SLICE);


	    // initializing object fields from constructor arguments

	hook = execute;
	reference = f;
	old_sar = false;
	of_data_name = data_name;

	init();

	    // in case of exception the generic_file "f" is not released, this is the duty of the caller to do so.
    }

    trivial_sar::~trivial_sar()
    {
	try
	{
	    terminate();
	}
	catch(...)
	{
		/// ignore all exceptions
	}
	if(reference != NULL)
	    delete reference;
    }

    void trivial_sar::inherited_terminate()
    {
	if(reference != NULL)
	{
	    if(get_mode() == gf_write_only)
	    {
		char last = flag_type_terminal;
		reference->write(&last, 1); // adding the trailing flag
	    }

	    delete reference; // this closes the slice so we can now eventually play with it:
	    reference = NULL;
	}
	if(hook != "")
	{
	    if(get_mode() == gf_write_only)
		tools_hook_substitute_and_execute(get_ui(),
						  hook,
						  archive_dir.display(),
						  base,
						  "1",
						  sar_make_padded_number("1", x_min_digits),
						  ext,
						  get_info_status());
	}
    }

    bool trivial_sar::skip_relative(S_I x)
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(x > 0)
            return reference->skip_relative(x);
	else
	{
	    U_I x_opposit = -x;
	    if(reference->get_position() > offset + x_opposit)
		return reference->skip_relative(x);
	    else
		return reference->skip(offset); // start of file
	}
    }

    infinint trivial_sar::get_position()
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(reference->get_position() >= offset + end_of_slice)
            return reference->get_position() - offset - end_of_slice;
        else
            throw Erange("trivial_sar::get_position", gettext("Position out of range"));
    }

    void trivial_sar::init()
    {
        header tete;

	if(reference->get_mode() == gf_write_only)
	{
	    tete.get_set_magic() = SAUV_MAGIC_NUMBER;
	    tete.get_set_internal_name().generate_internal_filename();
	    tete.get_set_flag() = flag_type_terminal;
	    if(of_data_name.is_cleared())
	    {
		tete.get_set_data_name() = tete.get_set_internal_name();
		of_data_name = tete.get_set_data_name();
	    }
	    else
		tete.get_set_data_name() = of_data_name;
	    tete.write(get_ui(), *reference);
	    offset = reference->get_position();
	}
	else
	    if(reference->get_mode() == gf_read_only)
	    {
		tete.read(get_ui(), *reference);
		if(tete.get_set_flag() == flag_type_non_terminal)
                    throw Erange("trivial_sar::trivial_sar", gettext("This archive has slices and is not possible to read from a pipe"));
		    // if flag is flag_type_located_at_end_of_slice, we will warn at end of slice
                offset = reference->get_position();
		of_data_name = tete.get_set_data_name();
		old_sar = tete.is_old_header();
	    }
	    else
		throw Efeature(gettext("Read-write mode not supported for \"trivial_sar\""));
    }

    U_I trivial_sar::inherited_read(char *a, U_I size)
    {
	U_I ret = reference->read(a, size);
	tuyau *tmp = dynamic_cast<tuyau *>(reference);

	if(tmp == NULL)
	    throw SRC_BUG; // should always read over a tuyau object
	else
	    if(!tmp->has_next_to_read())
	    {
		if(ret > 0)
		{
		    --ret;
		    if(a[ret] != flag_type_terminal)
			throw Erange("trivial_sar::inherited_read", gettext("This archive is not single sliced, more data exists in the next slices but cannot be read from the current pipe, aborting"));
		    else
			end_of_slice = 1;
		}
		    // else assuming EOF has already been reached
	    }

	return ret;
    }

} // end of namespace
