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
// $Id: mask_list.cpp,v 1.2.2.3 2008/02/09 17:41:29 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

} // end extern "C"

#include "mask_list.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "cygwin_adapt.hpp"

using namespace std;

namespace libdar
{

    mask_list::mask_list(const string & filename_list_st, bool case_sensit, const path & prefix_t, bool include)
    {
        case_s = case_sensit;
        including = include;
        const char *filename_list = filename_list_st.c_str();
        path prefix = prefix_t;

        if(!case_sensit)
        {
            string ptp = prefix_t.display();
            tools_to_upper(ptp);
            prefix = path(ptp);
        }

	char *buffer = NULL;
	const U_I buf_size = 20480; // read at most this number of bytes at a time
	S_I src = ::open(filename_list, O_RDONLY|O_TEXT);
	list <string> tmp;

	if(src < 0)
	    throw Erange("mask_list::mask_list", tools_printf(gettext("Cannot open file %s: %s"), filename_list, strerror(errno)));

	try
	{
	    buffer = new char[buf_size+1]; // one char more to be able to add a '\0' if necessary
	    U_I lu = 0, curs;
	    char *beg = NULL;
	    string current_entry = "";

	    if(buffer == NULL)
		throw Erange("mask_list::mask_list", tools_printf(gettext("Cannot allocate memory for buffer while reading %s"), filename_list));
	    try
	    {
		do
		{
		    lu = ::read(src, buffer, buf_size);

		    if(lu > 0)
		    {
			curs = 0;
			beg = buffer;

			do
			{
			    while(curs < lu && buffer[curs] != '\n' && buffer[curs] != '\0')
				curs++;

			    if(curs < lu)
			    {
				if(buffer[curs] == '\0')
				    throw Erange("mask_list::mask_list", tools_printf(gettext("Found '\0' character in %s, not a plain file, aborting"), filename_list));
				if(buffer[curs] == '\n')
				{
				    buffer[curs] = '\0';
				    if(!case_s)
					tools_to_upper(beg);
				    current_entry += string(beg);
				    if(current_entry != "")
					tmp.push_back(current_entry);
				    current_entry = "";
				    curs++;
				    beg = buffer + curs;
				}
				else
				    throw SRC_BUG;
			    }
			    else // reached end of buffer without having found an end of string
			    {
				buffer[lu] = '\0';
				if(!case_s)
				    tools_to_upper(beg);
				current_entry += string(beg);
			    }
			}
			while(curs < lu);
		    }
		    else
			if(lu < 0)
			    throw Erange("mask_list::mask_list", tools_printf(gettext("Cannot read file %s : %s"), filename_list, strerror(errno)));
		}
		while(lu > 0);

		if(current_entry != "")
		    tmp.push_back(current_entry);
	    }
	    catch(...)
	    {
		delete [] buffer;
		throw;
	    }
	    delete [] buffer;

		// completing relative paths of the list
	    if(prefix.is_relative())
		throw Erange("mask_list::mask_list", gettext("Mask_list's prefix must be an absolute path"));
	    else
	    {
		path current = "/";
		list <string>::iterator it = tmp.begin();

		while(it != tmp.end())
		{
		    try
		    {
			current = *it;
			if(current.is_relative())
			{
			    current = prefix + current;
			    *it = current.display();
			}
		    }
		    catch(Erange & e)
		    {
			e.dump();
			throw SRC_BUG;
		    }
		    it++;
		}
	    }

		// we use the features of lists
	    list<basic_string<my_char> > my_tmp = convert_list_string_char(tmp);
	    my_tmp.sort();   // sort the list ( using the string's < operator )
	    my_tmp.unique(); // remove duplicates

		// but we need the indexing of vectors
	    contenu.assign(my_tmp.begin(), my_tmp.end());
	    taille = contenu.size();
	    if(taille < contenu.size())
		throw Erange("mask_list::mask_list", tools_printf(gettext("Too much line in file %s (integer overflow)"), filename_list));
	}
	catch(...)
	{
	    ::close(src);
	    throw;
	}
	close(src);
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: mask_list.cpp,v 1.2.2.3 2008/02/09 17:41:29 edrusb Rel $";
        dummy_call(id);
    }

    bool mask_list::is_covered(const string & expression) const
    {
        U_I min = 0, max = taille-1, tmp;
        basic_string<my_char> target;
        bool ret;

        if(case_s)
            target = convert_string_char(expression);
        else
        {
            string hidden = expression;
            tools_to_upper(hidden);
            target = convert_string_char(hidden);
        }

            // divide & conquer algorithm on a sorted list
        while(max - min > 1)
        {
            tmp = (min + max)/2;
            if(contenu[tmp] < target)
                min = tmp;
            else
                if(contenu[tmp] == target)
                    max = min = tmp;
                else
                    max = tmp;
        }

        ret = contenu[max] == target || contenu[min] == target;
        if(including && !ret) // if including files, we must also include directories leading to a listed file
	{
	    string c_max = convert_string_my_char(contenu[max]);
            ret = path(c_max).is_subdir_of(expression, case_s);
	}

        return ret;
    }


	//////// private routines implementation


    list<basic_string<mask_list::my_char> > mask_list::convert_list_string_char(const list<string> & src) const
    {
	list<basic_string<my_char> > ret;
	list<string>::const_iterator it = src.begin();

	while(it != src.end())
	{
	    ret.push_back(convert_string_char(*it));
	    ++it;
	}
	return ret;
    }

    basic_string<mask_list::my_char> mask_list::convert_string_char(const string & src) const
    {
	basic_string<my_char> ret;

	string::const_iterator ut = src.begin();
	while(ut != src.end())
	{
	    ret += my_char(*ut);
	    ++ut;
	}

	return ret;
    }

    string mask_list::convert_string_my_char(const basic_string<mask_list::my_char> & src) const
    {
	string ret;

	basic_string<my_char>::const_iterator ut = src.begin();
	while(ut != src.end())
	{
	    ret += char(*ut);
	    ++ut;
	}

	return ret;
    }


} // end of namespace
