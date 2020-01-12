/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif
} // end extern "C"

#include "sar_tools.hpp"
#include "erreurs.hpp"
#include "user_interaction.hpp"
#include "sar.hpp"
#include "tools.hpp"
#include "tuyau.hpp"
#include "cygwin_adapt.hpp"
#include "deci.hpp"

using namespace std;

namespace libdar
{

    string sar_tools_make_filename(const string & base_name,
				   const infinint & num,
				   const infinint & min_digits,
				   const string & ext)
    {
        deci conv = num;
	string digits = conv.human();

        return base_name + '.' + sar_tools_make_padded_number(digits, min_digits) + '.' + ext;
    }

    bool sar_tools_extract_num(const string & filename,
			       const string & base_name,
			       const infinint & min_digits,
			       const string & ext,
			       infinint & ret)
    {
        try
        {
	    U_I min_size = base_name.size() + ext.size() + 2; // 2 for two dot characters
	    if(filename.size() <= min_size)
		return false; // filename is too short

            if(infinint(filename.size() - min_size) < min_digits && !min_digits.is_zero())
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

    bool sar_tools_get_higher_number_in_dir(entrepot & entr,
					    const string & base_name,
					    const infinint & min_digits,
					    const string & ext,
					    infinint & ret)
    {
        infinint cur;
        bool somme = false;
	string entry;

	try
	{
	    entr.read_dir_reset();
	}
	catch(Erange & e)
	{
	    return false;
	}

	ret = 0;
	somme = false;
	while(entr.read_dir_next(entry))
	    if(sar_tools_extract_num(entry, base_name, min_digits, ext, cur))
	    {
		if(cur > ret)
		    ret = cur;
		somme = true;
	    }

        return somme;
    }

    void sar_tools_remove_higher_slices_than(entrepot & entr,
					     const string & base_name,
					     const infinint & min_digits,
					     const string & ext,
					     const infinint & higher_slice_num_to_keep,
					     user_interaction & ui)
    {
        infinint cur;
	string entry;

	try
	{
	    entr.read_dir_reset();
	}
	catch(Erange & e)
	{
	    return;
	}

	while(entr.read_dir_next(entry))
	    if(sar_tools_extract_num(entry, base_name, min_digits, ext, cur))
	    {
		if(cur > higher_slice_num_to_keep)
		    entr.unlink(entry);
	    }
    }

    string sar_tools_make_padded_number(const string & num,
					const infinint & min_digits)
    {
	string ret = num;

	while(infinint(ret.size()) < min_digits)
	    ret = string("0") + ret;

	return ret;
    }

} // end of namespace
