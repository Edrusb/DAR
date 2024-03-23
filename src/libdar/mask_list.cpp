/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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

		buffer = new (nothrow) char[buf_size+1]; // one char more to be able to add a '\0' if necessary
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
				else // reached end of buffer
				{
				    if(lu == buf_size && beg == buffer)
				    {
					buffer[buf_size - 1] = '\0';
					throw Erange("mask_list::mask_list",
						     tools_printf(gettext("line exceeding the maximum of %d characters in listing file %S, aborting. Concerned line starts with: %s"),
								  buf_size - 1,
								  &filename_list_st,
								  buffer));
				    }
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
		    delete [] buffer;
		    throw;
		}
		delete [] buffer;
		buffer = nullptr;




		    /////////////
		    // - completing relative paths of the list
		    // - removing the ending part of the possible ending \r when DOS formatting is used
		    //

		if(prefix.is_relative() && !prefix.is_subdir_of(path("<ROOT>"), true))
		    throw Erange("mask_list::mask_list", gettext("Mask_list's prefix must be an absolute path or start with \"<ROOT>\" string for archive merging"));
		else
		{
		    path current("/");
		    list <string>::iterator it = tmp.begin();

		    while(it != tmp.end())
		    {
			try
			{

				// checking for trailing \r

			    if(it->size() < 1)
				throw SRC_BUG; // we should not have empty string in the list
			    if((*it)[it->size() - 1] == '\r') // last char of the string is a '\r'
			    {
				it->erase(it->size() - 1, 1); // removing the last char (thus removing \r here)
				if(it->empty())
				{
				    it = tmp.erase(it);
					// removing the string if "it" got empty
					// and having "it" pointing to the next string in the list
				    continue;  // and looping back
				}
			    }

				// adding prefix path

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

		    // converting the sorted list to deque, to get the indexing feature of this type
		contenu.assign(tmp.begin(), tmp.end());
		taille = contenu.size();
		if(taille < contenu.size())
		    throw Erange("mask_list::mask_list", tools_printf(gettext("Too much line in file %S (integer overflow)"), &filename_list_st));
	    }
	    catch(Egeneric & e)
	    {
		e.prepend_message(tools_printf(gettext("Error met while opening %S: "), &filename_list_st));
		throw;
	    }
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
            if(modified_lexicalorder_a_lessthan_b(contenu[tmp], target))
                min = tmp;
	    else
		max = tmp; // we could set max to tmp-1 but we need to get  min < target < max , if target is absent from the list
        }
	if(min == 0 && modified_lexicalorder_a_lessthan_b(target, contenu[min]))
	    max = min;

        ret = contenu[max] == target || contenu[min] == target;
        if(including && !ret) // if including files, we must also include directories leading to a listed file
	{
	    string c_max = contenu[max];
            ret = path(c_max).is_subdir_of(expression, case_s);
	}

        return ret;
    }

    string mask_list::dump(const string & prefix) const
    {
	deque<string>::const_iterator it = contenu.begin();
	string rec_pref = prefix + "  | ";

	string ret = prefix + "If matches one of the following line(s):\n";
	while(it != contenu.end())
	{
	    ret += rec_pref + *it + "\n";
	    ++it;
	}
	ret += prefix + "  +--";

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
	    return true; // even if bt == b.end(), we have a <= b

	if(bt == b.end())
	    return false; // because at != a.end(), we thus have neither a == b nor a < b
	else
	    throw SRC_BUG; // at != a.end() and bt != b.end() how did we escaped the while loop?
    }

} // end of namespace
