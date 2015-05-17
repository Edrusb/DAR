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

#include "sar.hpp"
#include "deci.hpp"
#include "user_interaction.hpp"
#include "tools.hpp"
#include "erreurs.hpp"
#include "cygwin_adapt.hpp"
#include "deci.hpp"
#include "entrepot.hpp"

using namespace std;

namespace libdar
{

    static bool sar_extract_num(const string & filename, const string & base_name, const infinint & min_digits, const string & ext, infinint & ret);
    static bool sar_get_higher_number_in_dir(entrepot & entr, const string & base_name, const infinint & min_digits, const string & ext, infinint & ret);
    static string sar_make_padded_number(const string & num, const infinint & min_digits);

    sar::sar(user_interaction & dialog,
	     const string & base_name,
	     const string & extension,
	     const entrepot & where,
	     bool by_the_end,
	     const infinint & x_min_digits,
	     bool x_lax,
	     const string & execute) : generic_file(gf_read_only), mem_ui(dialog)
    {
	opt_warn_overwrite = true;
	opt_allow_overwrite = false;
        natural_destruction = true;
        base = base_name;
        ext = extension;
        initial = true;
        hook = execute;
        set_info_status(CONTEXT_INIT);
	slicing.older_sar_than_v8 = false; // will be set to true at header read time a bit further if necessary
	hash = hash_none;
	lax = x_lax;
	min_digits = x_min_digits;
	entr = NULL;
	force_perm = false;
	to_read_ahead = 0;

        open_file_init();
	try
	{
	    entr = where.clone();
	    if(entr == NULL)
		throw Ememory("sar::sar");

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
	    if(entr != NULL)
		delete entr;
	    throw;
	}
    }

    sar::sar(user_interaction & dialog,
	     gf_mode open_mode,
	     const string & base_name,
	     const string & extension,
	     const infinint & file_size,
	     const infinint & first_file_size,
	     bool x_warn_overwrite,
	     bool x_allow_overwrite,
	     const infinint & x_pause,
	     const entrepot & where,
	     const label & internal_name,
	     const label & data_name,
	     bool force_permission,
	     U_I permission,
	     hash_algo x_hash,
	     const infinint & x_min_digits,
	     bool format_07_compatible,
	     const string & execute) : generic_file(open_mode), mem_ui(dialog)
    {
	if(open_mode == gf_read_only)
	    throw SRC_BUG;

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
        slicing.other_size = file_size;
        slicing.first_size = first_file_size;
        hook = execute;
	pause = x_pause;
	hash = x_hash;
	min_digits = x_min_digits;
        set_info_status(CONTEXT_OP);
	of_internal_name = internal_name;
	of_data_name = data_name;
	force_perm = force_permission;
	perm = permission;
	of_fd = NULL;
	of_flag = '\0';
	slicing.older_sar_than_v8 = format_07_compatible;
	entr = NULL;
	to_read_ahead = 0;

	try
	{
	    entr = where.clone();
	    if(entr == NULL)
		throw Ememory("sar::sar");

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
	    if(entr != NULL)
		delete entr;
	    throw;
	}
    }

    void sar::inherited_terminate()
    {
        close_file(true);
        if(get_mode() == gf_read_write && natural_destruction)
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
	if(entr != NULL)
	    delete entr;
    }

    bool sar::skippable(skippability direction, const infinint & amount)
    {
	if(hash != hash_none)
	    return false;

	switch(direction)
	{
	case generic_file::skip_backward:
	    if(of_current == 1)
		return file_offset - slicing.first_slice_header >= amount;
	    else
		return file_offset - slicing.other_slice_header >= amount;
	case generic_file::skip_forward:
	    if(of_current == 1)
		return (file_offset
			+ amount
			+ (slicing.older_sar_than_v8 ? 0 : 1))
		    < slicing.first_size;
	    else
		return (file_offset
			+ amount
			+ (slicing.older_sar_than_v8 ? 0 : 1))
		    < slicing.other_size;
	default:
	    throw SRC_BUG;
	}
    }

    bool sar::skip(const infinint & pos)
    {
	infinint dest_file, offset;

	if(is_terminated())
	    throw SRC_BUG;

        if(get_position() == pos)
            return true; // no need to skip

	to_read_ahead = 0;

            ///////////////////////////
            // determination of the file to go and its offset to seek in
            //
	slicing.which_slice(pos,
			    dest_file, offset);

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
	to_read_ahead = 0;
        ret = of_fd->skip_to_eof();
	switch(get_mode())
	{
	case gf_read_only:
	    if(!slicing.older_sar_than_v8)
		of_fd->skip_relative(-1);
	    file_offset = of_fd->get_position();
	    set_offset(file_offset);
	    break;
	case gf_read_write:
	case gf_write_only:
	    file_offset = of_fd->get_position();
	    if(of_current == 1)
	    {
		if(file_offset == slicing.first_size)
		{
			// we point to the slice trailer, which is not sar data
			// so we skip back one byte
		    --file_offset;
		    of_fd->skip(file_offset);
		}
		else
		    if(file_offset > slicing.first_size)
			throw SRC_BUG; // should not be possible to have an initial slice larger than first_size
	    }
	    else
	    {
		if(file_offset == slicing.other_size)
		{
			// we point to the slice trailer, which is not sar data
			// so we skip back one byte
		    --file_offset;
		    of_fd->skip(file_offset);
		}
		else
		    if(file_offset > slicing.other_size)
			throw SRC_BUG; // should not be possible to have a slice larger than size
	    }
	    break;
	default:
	    throw SRC_BUG;
	}
        return ret;
    }

    bool sar::skip_forward(U_I x)
    {
        infinint number = of_current;
        infinint offset = file_offset + x;
	infinint delta = slicing.older_sar_than_v8 ? 0 : 1; // one byte less per slice with archive format >= 8

	if(is_terminated())
	    throw SRC_BUG;

	to_read_ahead = 0;

        while((number == 1 ? offset+delta >= slicing.first_size : offset+delta >= slicing.other_size)
              && (!of_last_file_known || number <= of_last_file_num))
        {
            offset -= number == 1 ? slicing.first_size - delta : slicing.other_size - delta;
            offset += slicing.other_slice_header;
            number++;
        }

        if(number == 1 ? offset+delta < slicing.first_size : offset+delta < slicing.other_size)
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
	infinint delta = slicing.older_sar_than_v8 ? 0 : 1; // one byte less per slice with archive format >= 8

	if(is_terminated())
	    throw SRC_BUG;

        while(number > 1 && offset_neg + slicing.other_slice_header > offset)
        {
            offset_neg -= offset - slicing.other_slice_header + 1;
            number--;
            if(number > 1)
                offset = slicing.other_size - 1 - delta;
            else
                offset = slicing.first_size - 1 - delta;
        }

        if((number > 1 ? offset_neg + slicing.other_slice_header : offset_neg + slicing.first_slice_header) <= offset)
        {
            open_file(number);
            file_offset = offset - offset_neg;
            set_offset(file_offset);
            return true;
        }
        else
        {   // seek to beginning of file
            open_file(1);
            set_offset(slicing.first_slice_header);
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
	infinint delta = slicing.older_sar_than_v8 ? 0 : 1; // one byte less per slice with archive format >= 8

	if(is_terminated())
	    throw SRC_BUG;

        if(of_current > 1)
            return slicing.first_size - slicing.first_slice_header - delta + (of_current-2)*(slicing.other_size - slicing.other_slice_header - delta) + file_offset - slicing.other_slice_header;
        else
            return file_offset - slicing.first_slice_header;
    }

    void sar::inherited_read_ahead(const infinint & amount)
    {
	infinint avail_in_slice;

	if(of_current == 1)
	    avail_in_slice = slicing.first_size;
	else
	    avail_in_slice = slicing.other_size;
	avail_in_slice -= file_offset;
	avail_in_slice -= (slicing.older_sar_than_v8 ? 0 : 1);

	if(avail_in_slice > amount)
	{
	    if(of_fd != NULL)
		of_fd->read_ahead(amount);
	    to_read_ahead = 0;
	}
	else
	{
	    if(of_fd != NULL)
		of_fd->read_ahead(avail_in_slice + (slicing.older_sar_than_v8 ? 0 : 1));
	    to_read_ahead = amount - avail_in_slice;
	}
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
		    if(!slicing.older_sar_than_v8 && of_fd->get_position() == size_of_current)
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
	U_I trailer_size = slicing.older_sar_than_v8 ? 0 : 1;

	to_read_ahead = 0;

        while(to_write > 0)
        {
	    max_at_once = of_current == 1 ? (slicing.first_size - file_offset) - trailer_size : (slicing.other_size - file_offset) - trailer_size;
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
	    if(get_mode() == gf_read_write || get_mode() == gf_write_only)
	    {
		if(slicing.older_sar_than_v8)
		{
		    header h = make_write_header(of_current, terminal ? flag_type_terminal : flag_type_non_terminal);
		    of_fd->skip(0);
		    h.write(get_ui(), *of_fd);
		}
		else
		{
		    if(of_fd->get_position() != of_fd->get_size())
			bug = true; // we should be at the end of the file
		    else
			of_fd->write(&flag, 1);
		}
	    }

	    of_fd->terminate();

            delete of_fd;
            of_fd = NULL;
        }

	if(bug)
	    throw SRC_BUG;
    }

    void sar::open_readonly(const string & fic, const infinint &num)
    {
        header h;

        while(of_fd == NULL)
        {
                // launching user command if any
            hook_execute(num);

                // trying to open the file
                //
	    try
	    {
		of_fd = entr->open(get_ui(),
				   fic,
				   gf_read_only,
				   false, //< force permission
				   0,     //< permission to enforce (not used here)
				   false, //<  fail if exists
				   false, //<  erase
				   hash_none);
		if(of_fd == NULL)
		    throw SRC_BUG;
		of_fd->fadvise(fichier_global::advise_normal);
		    // we have no advise to give to the system when reading a slice
		size_of_current = of_fd->get_size();
	    }
	    catch(Euser_abort & e)
	    {
		if(lax)
		{
		    get_ui().warning(string(gettext("LAX MODE: Caught exception: "))+ e.get_message());
		    get_ui().pause(tools_printf(gettext("LAX MODE: %S is missing, You have the possibility to create a zero byte length file under the name of this slice, to replace this missing file. This will of course generate error messages about the information that is missing in this slice, but at least libdar will be able to continue. Can we continue now?"), &fic));
		    continue; // we restart the while loop
		}
		else
		    throw;
	    }
	    catch(Esystem & e)
	    {
		switch(e.get_code())
		{
		case Esystem::io_absent:
		    if(!lax)
			get_ui().pause(tools_printf(gettext("%S is required for further operation, please provide the file."), &fic));
		    else
			get_ui().pause(tools_printf(gettext("%S is required for further operation, please provide the file if you have it."), &fic));
		    break;
		case Esystem::io_exist:
		    throw SRC_BUG;
		default:
		    throw SRC_BUG;
		}
		continue; // we restart the while loop
	    }



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
		    get_ui().pause(tools_printf(gettext("%S has a bad or corrupted header, please provide the correct file."), &fic));
		    continue;
		}
		else
		    get_ui().warning(tools_printf(gettext("LAX MODE: %S has a bad or corrupted header, trying to guess original values and continuing if possible"), &fic));
            }

                // checking against the magic number
                //
            if(h.get_set_magic() != SAUV_MAGIC_NUMBER)
            {
		if(!lax)
		{
		    close_file(false);
		    get_ui().pause(tools_printf(gettext("%S is not a valid file (wrong magic number), please provide the good file."), &fic));
		    continue;
		}
		else
		    get_ui().warning(tools_printf(gettext("LAX MODE: In spite of its name, %S does not appear to be a dar slice, assuming a data corruption took place and continuing"), &fic));
            }

	    if(h.is_old_header() && slicing.first_slice_header == 0 && num != 1)
		throw Erange("sar::open_readonly", gettext("This is an old archive, it can only be opened starting by the first slice"));

                // checking the ownership of the set of file (= slice of the same archive or not)
                //
            if(slicing.first_slice_header == 0) // this is the first time we open a slice for this archive, we don't even know the slices size
            {
                of_internal_name = h.get_set_internal_name();
		of_data_name = h.get_set_data_name();
                try
                {
		    if(!h.get_slice_size(slicing.other_size))
		    {
			if(!lax)
			    throw SRC_BUG;  // slice size should be known or determined by header class
			else
			    slicing.other_size = 0;
		    }
		    if(!h.get_first_slice_size(slicing.first_size))
			slicing.first_size = slicing.other_size;

		    if(slicing.first_size == 0 || slicing.other_size == 0) // only possible to reach this statment in lax mode
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

                    slicing.first_slice_header = of_fd->get_position();
		    slicing.other_slice_header = h.is_old_header() ? header::min_size() : slicing.first_slice_header;
		    if(slicing.first_slice_header >= slicing.first_size && !lax)
			throw Erange("sar::sar", gettext("Incoherent slice header: First slice size too small"));
		    if(slicing.other_slice_header >= slicing.other_size && !lax)
			throw Erange("sar::sar", gettext("incoherent slice header: Slice size too small"));
		    slicing.older_sar_than_v8 = h.is_old_header();
                }
                catch(Erange & e)
                {
                    close_file(false);
                    get_ui().pause(tools_printf(gettext("Error opening %S : "), &fic) + e.get_message() + gettext(" . Retry ?"));
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
			get_ui().pause(fic + gettext(" is a slice from another backup, please provide the correct slice."));
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
		    get_ui().pause(tools_printf(gettext("Slice %S has an unknown flag (neither terminal nor non_terminal file)."), &fic));
		    continue;
		}
		else
		    if(of_max_seen <= num)
		    {
			string answ;

			do
			{
			    answ = get_ui().get_string(tools_printf(gettext("Due to data corruption, it is not possible to know if slice %S is the last slice of the archive or not. I need your help to figure out this. At the following prompt please answer either one of the following words: \"last\" or \"notlast\" according to the nature of this slice (you can also answer with \"abort\" to abort the program immediately): "), &fic), true);
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
			of_fd->skip(slicing.first_slice_header);
		    else
			of_fd->skip(slicing.other_slice_header);
		}
	    }
        }
    }

    void sar::open_writeonly(const string & fic, const infinint &num)
    {
	bool unlink_on_error = false;
	bool do_erase = false;

	    // open for writing but succeeds only if this file does NOT already exist
	try
	{
	    try
	    {
		of_fd = entr->open(get_ui(),
				   fic,
				   hash == hash_none ? gf_read_write : gf_write_only, // yes, no more writeonly as stated in the name of this method
				   force_perm,
				   perm,
				   true,   //< fail_if_exists
				   false,  //< erase
				   hash);
	    }
	    catch(Erange & e)
	    {
		string tmp = e.get_message();
		get_ui().warning(tools_printf(gettext("failed openning slice %S: %S. Will try to erase it first, if allowed"), &fic, &tmp));
		throw Esystem("sar::open_writeonly", "failed openning, will try erasing first", Esystem::io_exist);
	    }
	}
	catch(Esystem & e)
	{
	    switch(e.get_code())
	    {
	    case Esystem::io_exist:
		try
		{
		    try
		    {
			    // the file does not exist, re-trying opening it without fail_if_exists

			of_fd = entr->open(get_ui(),
					   fic,
					   gf_read_only,
					   false,  //< force permission
					   0,      //< permission to enforce (not used here)
					   false,  //< fail if exists
					   false,  //< erase
					   hash_none);

			if(of_fd == NULL)
			    throw SRC_BUG;

			try
			{
			    header h;

			    try
			    {
				h.read(get_ui(), *of_fd);
			    }
			    catch(Erange & e)
			    {
				h.get_set_internal_name() = of_internal_name;
				h.get_set_internal_name().invert_first_byte();
				    // this way we are sure that the file is not considered as part of the current SAR
			    }
			    if(h.get_set_internal_name() != of_internal_name)
				do_erase = true; // this is not a slice of the current archive
			    delete of_fd;
			    of_fd = NULL;
			}
			catch(...)
			{
			    if(of_fd != NULL)
			    {
				delete of_fd;
				of_fd = NULL;
			    }
			    throw;
			}
		    }
		    catch(Esystem & f)
		    {
			switch(f.get_code())
			{
			case Esystem::io_exist:
			    throw SRC_BUG;
			case Esystem::io_absent:
			    throw SRC_BUG;
			default:
			    throw SRC_BUG;
			}
		    }
		}
		catch(Ebug & e)
		{
		    throw;
		}
		catch(...)
		{
		    do_erase = true;     // reading failed, trying overwriting (if allowed)
		}

		try
		{
		    if(do_erase)
		    {
			if(!opt_allow_overwrite)
			    throw Erange("sar::open_writeonly", gettext("file exists, and DONT_ERASE option is set."));
			if(opt_warn_overwrite)
			{
			    try
			    {
				get_ui().pause(fic + gettext(" is about to be overwritten."));
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
			of_fd = entr->open(get_ui(),
					   fic,
					   hash == hash_none ? gf_read_write : gf_write_only, // yes, no more write only as stated in the name of this method
					   force_perm,
					   perm,
					   false,    //< fail if exists
					   true,     //< erase
					   hash);
		    }
		    else // open without overwriting
			if(hash == hash_none)
			    of_fd = entr->open(get_ui(),
					       fic,
					       hash == hash_none ? gf_read_write : gf_write_only, // yes, no more write only as stated in the name of this method
					       force_perm,
					       perm,
					       false, //< fail if exists
					       false, //< erase
					       hash);
			else
			    throw SRC_BUG; // cannot calculate a hash on a just openned file that is not empty
		}
		catch(Esystem & e)
		{
		    switch(e.get_code())
		    {
		    case Esystem::io_exist:
			throw SRC_BUG; // not called with fail_if_exists set
		    case Esystem::io_absent:
			throw SRC_BUG; // not called in read mode
		    default:
			throw SRC_BUG;
		    }
		}
		break;
	    case Esystem::io_absent:
		throw SRC_BUG;
	    default:
		throw SRC_BUG;
	    }
	}

	if(of_fd == NULL)
	    throw SRC_BUG;

	try
	{
	    header h;

		// telling the system to write data directly to disk not going through the cache
	    of_fd->fadvise(fichier_global::advise_dontneed);
		// useless under Linux, because the corresponding implementation does not avoid the
		// generated data to pass out of the cache. Maybe some other system can prevent the data
		// from filling the cache ... so we keep it here in respect to the posix semantic

	    of_flag = flag_type_located_at_end_of_slice;
	    h = make_write_header(num, of_flag);
	    h.write(get_ui(), *of_fd);
	    if(num == 1)
	    {
		slicing.first_slice_header = of_fd->get_position();
		if(slicing.first_slice_header == 0)
		    throw SRC_BUG;
		slicing.other_slice_header = slicing.first_slice_header; // same header in all slice since release 2.4.0
		if(slicing.first_slice_header >= slicing.first_size)
		    throw Erange("sar::sar", gettext("First slice size is too small to even just be able to drop the slice header"));
		if(slicing.other_slice_header >= slicing.other_size)
		    throw Erange("sar::sar", gettext("Slice size is too small to even just be able to drop the slice header"));
	    }
	}
	catch(...)
	{
	    if(unlink_on_error)
		entr->unlink(fic);
	    if(of_fd != NULL)
	    {
		delete of_fd;
		of_fd = NULL;
	    }
	    throw;
	}
    }

    void sar::open_file_init()
    {
        of_max_seen = 0;
        of_last_file_known = false;
        of_fd = NULL;
	of_flag = '\0';
        slicing.first_slice_header = 0; // means that the sizes have to be determined from file or wrote to file
	slicing.other_slice_header = 0;
	size_of_current = 0; // not used in write mode
    }

    void sar::open_file(infinint num)
    {
        if(of_fd == NULL || of_current != num)
        {
	    const string display = sar_make_filename(base, num, min_digits, ext);

	    switch(get_mode())
	    {
	    case gf_read_only:
		close_file(false);
		    // launch the shell command before reading a slice
		open_readonly(display, num);
		break;
	    case gf_write_only:
	    case gf_read_write:
		if(num < of_current)
		    throw Erange("sar::open_file", "Skipping backward would imply accessing/modifying previous slice");

		    // adding the trailing flag
		if(of_fd != NULL)
		    close_file(false);

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

		open_writeonly(display, num);
		break;
	    default :
		close_file(false);
		throw SRC_BUG;
	    }

	    if(of_max_seen < num)
		of_max_seen = num;
	    file_offset = num == 1 ? slicing.first_slice_header : slicing.other_slice_header;
	    if(num == of_current + 1 && to_read_ahead > 0)
	    {
		of_current = num;
		inherited_read_ahead(to_read_ahead);
	    }
	    else
		of_current = num;
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

	switch(get_mode())
	{
	case gf_read_only:
	    if(of_last_file_known)
		open_file(of_last_file_num);
	    else // last slice number is not known
	    {
		bool ask_user = false;

		while(of_fd == NULL || of_flag != flag_type_terminal)
		{
		    if(sar_get_higher_number_in_dir(*entr, base, min_digits, ext, num))
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
				get_ui().pause(string(gettext("The last file of the set is not present in ")) + entr->get_url() + gettext(" , please provide it."));
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
			    string chem = entr->get_url();
			    close_file(false);
			    get_ui().pause(tools_printf(gettext("No backup file is present in %S for archive %S, please provide the last file of the set."), &chem, &base));
			}
		}
	    }
	case gf_read_write:
	case gf_write_only:
	    open_file(of_max_seen);
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    header sar::make_write_header(const infinint & num, char flag)
    {
        header hh;

        hh.get_set_magic() = SAUV_MAGIC_NUMBER;
        hh.get_set_internal_name() = of_internal_name;
	hh.get_set_data_name() = of_data_name;
        hh.get_set_flag() = flag;
	if(slicing.older_sar_than_v8)
	{
	    if(num == 1)
	    {
		hh.set_slice_size(slicing.other_size);
		if(slicing.other_size != slicing.first_size)
		    hh.set_first_slice_size(slicing.first_size);
	    }
	    hh.set_format_07_compatibility();
	}
	else
	{
	    hh.set_slice_size(slicing.other_size);
	    if(slicing.other_size != slicing.first_size)
		hh.set_first_slice_size(slicing.first_size);
	}

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
						  entr->get_full_path().display(),
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
	infinint delta = slicing.older_sar_than_v8 ? 0 : 1; // one byte less per slice with archive format >= 8

	if(of_last_file_known && of_last_file_num == of_current) // we are in the last slice, thus eof may occur at any place
	    return true;

	    // we are not in the last slice, thus we can determine at which offset the eof must be met for this slice

	if(of_current == 1)
	    return file_offset >= slicing.first_size - delta;
	else
	    return file_offset >= slicing.other_size - delta;
    }

    infinint sar::bytes_still_to_read_in_slice() const
    {
	infinint delta = slicing.older_sar_than_v8 ? 0 : 1; // one byte less per slice with archive format >= 8

	if(of_last_file_known && of_last_file_num == of_current)
	    throw SRC_BUG; // cannot figure out the expected slice size of the last slice of the archive

	if(of_current == 1)
	    if(file_offset > slicing.first_size - delta)
		return 0;
	    else
		return slicing.first_size - file_offset - delta;
	else
	    if(file_offset > slicing.other_size - delta)
		return 0;
	    else
		return slicing.other_size - file_offset - delta;
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

    static bool sar_get_higher_number_in_dir(entrepot & entr, const string & base_name, const infinint & min_digits, const string & ext, infinint & ret)
    {
        infinint cur;
        bool somme = false;
	string entry;

	entr.read_dir_reset();

	ret = 0;
	somme = false;
	while(entr.read_dir_next(entry))
	    if(sar_extract_num(entry, base_name, min_digits, ext, cur))
	    {
		if(cur > ret)
		    ret = cur;
		somme = true;
	    }

        return somme;
    }

/*****************************************************/


    trivial_sar::trivial_sar(user_interaction & dialog,
			     gf_mode open_mode,
			     const std::string & base_name,
			     const std::string & extension,
			     const entrepot & where,
			     const label & internal_name,
			     const label & data_name,
			     const std::string & execute,
			     bool allow_over,
			     bool warn_over,
			     bool force_permission,
			     U_I permission,
			     hash_algo x_hash,
			     const infinint & x_min_digits,
			     bool format_07_compatible) : generic_file(open_mode), mem_ui(dialog)
    {

	    // some local variables to be used

	fichier_global *tmp = NULL;
	const string filename = sar_make_filename(base_name, 1, x_min_digits, extension);

	    // sanity checks
	if(open_mode == gf_read_only)
	    throw SRC_BUG;

	    // initializing object fields from constructor arguments

	reference = NULL;
	offset = 0;
	cur_pos = 0;
	end_of_slice = 0;
	hook = execute;
	base = base_name;
	ext = extension;
	of_data_name = data_name;
	old_sar = false;
	min_digits = x_min_digits;
	hook_where = where.get_full_path().display();
	old_sar = format_07_compatible;

	    // creating the slice if it does not exist else failing
	try
	{
	    try
	    {
		tmp = where.open(dialog,
				 filename,
				 open_mode,
				 force_permission,
				 permission,
				 true,    //< fail if exists
				 false,   //< erase
				 x_hash);
	    }
	    catch(Esystem & e)
	    {
		switch(e.get_code())
		{
		case Esystem::io_exist:
		    if(tmp != NULL)
			throw SRC_BUG;

		    if(!allow_over)
			throw Erange("trivial_sar::trivial_sar", tools_printf(gettext("%S already exists, and overwritten is forbidden, aborting"), &filename));
		    if(warn_over)
			dialog.pause(tools_printf(gettext("%S is about to be overwritten, continue ?"), &filename));

		    try
		    {
			tmp = where.open(dialog,
					 filename,
					 open_mode,
					 force_permission,
					 permission,
					 false,  //< fail if exists
					 true,   //< erase
					 x_hash);

		    }
		    catch(Esystem & e)
		    {
			switch(e.get_code())
			{
			case Esystem::io_exist:
			    throw SRC_BUG;
			case Esystem::io_absent:
			    throw SRC_BUG;
			default:
			    throw SRC_BUG;
			}
		    }
		    break;
		case Esystem::io_absent:
		    if(tmp != NULL)
			throw SRC_BUG;
		    else
			throw SRC_BUG; // not for the same reason, must know that reporting the same error but on a different line
		default:
		    if(tmp != NULL)
			throw SRC_BUG;
		    else
			throw SRC_BUG; // not for the same reason, must know that reporting the same error but on a different line
		}
	    }

	    if(tmp == NULL)
		throw SRC_BUG;

		// telling the system to write data directly to disk not going through the cache
	    tmp->fadvise(fichier_global::advise_dontneed);
		// useless under Linux, because the corresponding implementation does not avoid the
		// generated data to pass out of the cache. Maybe some other system can prevent the data
		// from filling the cache ... so we keep it here in respect to the posix semantic

	    set_info_status(CONTEXT_LAST_SLICE);
	    reference = tmp;
	    init(internal_name);
	    tmp = NULL; // setting it to null only now was necesary to be able to release the object in case of exception
	}
	catch(...)
	{
	    if(tmp != NULL)
		delete tmp;
	    throw;
	}

	if(tmp != NULL)
	    throw SRC_BUG;
    }


    trivial_sar::trivial_sar(user_interaction & dialog,
			     const std::string & pipename,
			     bool lax) : generic_file(gf_read_only) , mem_ui(dialog)
    {
	label for_init;
	reference = NULL;
	offset = 0;
	cur_pos = 0;
	end_of_slice = 0;
	hook = "";
	base = "";
	ext = "";
	old_sar = false;
	min_digits = 0;
	hook_where = "";

	set_info_status(CONTEXT_INIT);
	try
	{
	    if(pipename == "-")
		reference = new (get_pool()) tuyau(dialog, 0, gf_read_only);
	    else
		reference = new (get_pool()) tuyau(dialog, pipename, gf_read_only);

	    if(reference == NULL)
		throw Ememory("trivial_sar::trivial_sar");

	    for_init.clear();
	    init(for_init);
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
			     const label & internal_name,
			     const label & data_name,
			     bool format_07_compatible,
			     const std::string & execute) : generic_file(gf_write_only), mem_ui(dialog)
    {
	if(f == NULL)
	    throw SRC_BUG;

	reference = f;
	offset = 0;
	cur_pos = 0;
	end_of_slice = 0;
	hook = execute;
	base = "";
	ext = "";
	of_data_name = data_name;
	old_sar = format_07_compatible;
	min_digits = 0;
	hook_where = "";

	set_info_status(CONTEXT_LAST_SLICE);
	init(internal_name);
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

    bool trivial_sar::skip(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(pos == cur_pos)
	    return true;

	return reference->skip(pos + offset);
    }

    void trivial_sar::inherited_terminate()
    {
	if(reference != NULL)
	{
	    char last = flag_type_terminal;

	    switch(get_mode())
	    {
	    case gf_read_only:
		break; // explicitely accepting other value
	    case gf_write_only:
	    case gf_read_write:
		if(!old_sar)
		    reference->write(&last, 1); // adding the trailing flag
		break;
	    default:
		throw SRC_BUG;
	    }

	    delete reference; // this closes the slice so we can now eventually play with it:
	    reference = NULL;
	}
	if(hook != "")
	{
	    switch(get_mode())
	    {
	    case gf_read_only:
		break;
	    case gf_write_only:
	    case gf_read_write:
		tools_hook_substitute_and_execute(get_ui(),
						  hook,
						  hook_where,
						  base,
						  "1",
						  sar_make_padded_number("1", min_digits),
						  ext,
						  get_info_status());
		break;
	    default:
		throw SRC_BUG;
	    }
	}
    }

    bool trivial_sar::skip_relative(S_I x)
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(x > 0)
	{
	    bool ret = reference->skip_relative(x);
            if(ret) // skip succeeded
		cur_pos += x;
	    else   // skip failed
		where_am_i();
	    return ret;
	}
	else
	{
	    U_I x_opposit = -x;
	    if(reference->get_position() > offset + x_opposit)
		return reference->skip_relative(x);
	    else
		return reference->skip(offset); // start of file

	    if(cur_pos > x_opposit)
	    {
		bool ret = reference->skip_relative(x);
		if(ret)
		    cur_pos -= x_opposit;
		else
		    where_am_i();
		return ret;
	    }
	    else
	    {
		bool ret = reference->skip(offset);
		cur_pos = 0;
		return ret;
	    }
	}
    }

    void trivial_sar::init(const label & internal_name)
    {
        header tete;

	switch(reference->get_mode())
	{
	case gf_read_only:
	    tete.read(get_ui(), *reference);
	    if(tete.get_set_flag() == flag_type_non_terminal)
		throw Erange("trivial_sar::trivial_sar", gettext("This archive has slices and is not possible to read from a pipe"));
		// if flag is flag_type_located_at_end_of_slice, we will warn at end of slice
	    offset = reference->get_position();
	    of_data_name = tete.get_set_data_name();
	    old_sar = tete.is_old_header();
	    cur_pos = 0;
	    break;
	case gf_write_only:
	case gf_read_write:
	    tete.get_set_magic() = SAUV_MAGIC_NUMBER;
	    tete.get_set_internal_name() = internal_name;
	    tete.get_set_flag() = flag_type_terminal;
	    tete.get_set_data_name() = of_data_name;
	    if(old_sar)
		tete.set_format_07_compatibility();
	    tete.write(get_ui(), *reference);
	    offset = reference->get_position();
	    cur_pos = 0;
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    U_I trivial_sar::inherited_read(char *a, U_I size)
    {
	U_I ret = reference->read(a, size);
	tuyau *tmp = dynamic_cast<tuyau *>(reference);

	if(tmp != NULL && !tmp->has_next_to_read())
	{
	    if(ret > 0)
	    {
		if(!old_sar)
		{
		    --ret;
		    if(a[ret] != flag_type_terminal)
			throw Erange("trivial_sar::inherited_read", gettext("This archive is not single sliced, more data exists in the next slices but cannot be read from the current pipe, aborting"));
		    else
			end_of_slice = 1;
		}
		else
		    end_of_slice = 1;
	    }
		// else assuming EOF has already been reached
	}

	cur_pos += ret;

	return ret;
    }

    void trivial_sar::inherited_write(const char *a, U_I size)
    {
	cur_pos += size;

	try
	{
	    reference->write(a, size);
	}
	catch(...)
	{
	    where_am_i();
	    throw;
	}
    }

    void trivial_sar::where_am_i()
    {
	cur_pos = reference->get_position();
	if(cur_pos >= offset)
	    cur_pos -= offset;
	else // we are at an invalid offset (in the slice header)
	{
	    if(!reference->skip(offset))
		throw Edata(string("trivial_sar: ")+ gettext("Cannot skip to a valid position in file"));
	    cur_pos = 0;
	}
    }

} // end of namespace
