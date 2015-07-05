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
#include "fichier_local.hpp"
#include "nls_swap.hpp"

using namespace std;

namespace libdar
{

    static bool modified_lexicalorder_a_lessthan_b(const std::string & a, const std::string & b);

    mask_list::mask_list(const string & filename_list_st, bool case_sensit, const path & prefix_t, bool include)
    {
	NLS_SWAP_IN;
	try
	{
	    case_s = case_sensit;              //< object's field
	    including = include;               //< object's field
	    fichier_local source = filename_list_st; //< where we read data from
	    char *buffer = nullptr;               //< hold the just read data
	    static const U_I buf_size = 20480; //< size of buffer: we read at most this number of bytes at a time
	    list <string> tmp;                 //< list of all raw lines read, without any prefix
	    U_I lu = 0, curs;                  //< cursor used as cisors to split data in line
	    char *beg = nullptr;                  //< points to the beginning of the next line inside buffer, when more than one line can be found in buffer
	    string str_beg;                    //< holds the std::string copy of beg, eventually uppercased
	    string current_entry = "";         //< holds the current line converted to string between each read()
	    path prefix = prefix_t;            //< the prefix to add to relative paths



		/////////////
		// changing the prefix to uppercase if case sensitivity is disabled

	    if(!case_sensit)
	    {
		string ptp = prefix_t.display();
		string upper;
		tools_to_upper(ptp, upper);
		prefix = path(upper);
	    }



		/////////////
		// building buffer that will be used to split read data line by line

	    meta_new(buffer, buf_size+1); // one char more to be able to add a '\0' if necessary
	    if(buffer == nullptr)
		throw Erange("mask_list::mask_list", tools_printf(gettext("Cannot allocate memory for buffer while reading %S"), &filename_list_st));



		/////////////
		// filling 'tmp' with with each line read

	    try
	    {
		do
		{
		    lu = source.read(buffer, buf_size);

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
				    throw Erange("mask_list::mask_list", tools_printf(gettext("Found '\0' character in %S, not a plain file, aborting"), &filename_list_st));
				if(buffer[curs] == '\n')
				{
				    buffer[curs] = '\0';
				    if(!case_s)
					tools_to_upper(beg, str_beg);
				    else
					str_beg = string(beg);
				    current_entry += str_beg;
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
				    tools_to_upper(beg, str_beg);
				else
				    str_beg = string(beg);
				current_entry += str_beg;
			    }
			}
			while(curs < lu);
		    }
		}
		while(lu > 0);

		if(current_entry != "")
		    tmp.push_back(current_entry);
	    }
	    catch(...)
	    {
		meta_delete(buffer);
		throw;
	    }
	    meta_delete(buffer);
	    buffer = nullptr;


		/////////////
		// completing relative paths of the list

	    if(prefix.is_relative() && !prefix.is_subdir_of("<ROOT>", true))
		throw Erange("mask_list::mask_list", gettext("Mask_list's prefix must be an absolute path or start with \"<ROOT>\" string for archive merging"));
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
		    catch(Egeneric & e)
		    {
			string err = e.get_message();
			string line = *it;

			throw Erange("mask_list::mask_list", tools_printf(gettext("Error met while reading line\n\t%S\n from file %S: %S"), &line, &filename_list_st, &err));
		    }
		    it++;
		}
	    }

		/////////////
		// sorting the list of entry

		// sorting the list with a modified lexicographical order where the / as is lowest character, other letter order unchanged
	    tmp.sort(&modified_lexicalorder_a_lessthan_b);
	    tmp.unique(); // remove duplicates

		// converting the sorted list to vector, to get the indexing feature of this type
	    contenu.assign(tmp.begin(), tmp.end());
	    taille = contenu.size();
	    if(taille < contenu.size())
		throw Erange("mask_list::mask_list", tools_printf(gettext("Too much line in file %S (integer overflow)"), &filename_list_st));
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    bool mask_list::is_covered(const string & expression) const
    {
	if(taille == 0)
	    return false;

	U_I min = 0, max = taille-1, tmp;
        string target;
        bool ret;

        if(case_s)
            target = expression;
        else
            tools_to_upper(expression, target);

            // divide & conquer algorithm on a sorted list (aka binary search)
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
	    string c_max = contenu[max];
            ret = path(c_max).is_subdir_of(expression, case_s);
	}

        return ret;
    }

    static bool modified_lexicalorder_a_lessthan_b(const string & a, const string & b)
    {
	string::const_iterator at = a.begin();
	string::const_iterator bt = b.begin();

	while(at != a.end() && bt != b.end())
	{
	    if(*at == '/')
	    {
		if(*bt != '/')
		    return true;

		    // else both a and b current letter are equal to '/'
		    // reading further
	    }
	    else
	    {
		if(*bt == '/')
		    return false;
		else
		{
		    if(*at != *bt)
			return *at < *bt;

			// else a and b letter are equal
			// reading further to find a difference
		}
	    }

	    ++at;
	    ++bt;
	}

	if(at == a.end())
	    return true; // even if bt == b.end() too, we assume a < b

	if(bt == b.end())
	    return false;
	else
	    throw SRC_BUG; // at != a.end() and bt != b.end() how did we escaped the while loop?
    }

} // end of namespace
