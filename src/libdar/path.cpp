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
// $Id: path.cpp,v 1.29 2011/03/01 20:45:36 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif
}

#include <iostream>
#include "path.hpp"
#include "tools.hpp"
#include "nls_swap.hpp"

using namespace std;

namespace libdar
{

	/// extract the first path member of a given path

	/// \param[in,out] p is the given path (in), it receive the new path without the first path member
	/// \param[out] root this argument receive the first path member extracted from argument 'p'
	/// \return false if given path is empty, true else. May throw exception in case of invalid path given
    static bool path_get_root(string & p, string & root);

    path::path(const string & chem, bool x_undisclosed)
    {
	NLS_SWAP_IN;
	try
	{
	    string tmp;
	    string s;

	    undisclosed = x_undisclosed;
	    try
	    {
		dirs.clear();
		if(chem.empty())
		    throw Erange("path::path", gettext("Empty string is not a valid path"));
		if(chem == "/")
		    undisclosed = false;
		relative = (chem[0] != '/');
		if(!relative)
		    s = string(chem.begin()+1, chem.end()); // remove the leading '/'
		else
		    s = chem;
		if(undisclosed) // if last char is '/' need to remove it
		{
		    string::iterator last = tools_find_last_char_of(s, '/');

		    if(last + 1 == s.end()) // this is the last char of s
			s = string(s.begin(), last);
		}

		if(undisclosed)
		    dirs.push_back(s);
		else
		    while(path_get_root(s, tmp))
			dirs.push_back(tmp);
		if(dirs.empty() && relative)
		    throw Erange("path::path", gettext("Empty string is not a valid path"));
		if(!undisclosed)
		    reduce();
		reading = dirs.begin();
	    }
	    catch(Erange & e)
	    {
		string e_tmp = e.get_message();
		throw Erange("path::path", tools_printf(gettext("%S is an not a valid path: %S"), &chem, &e_tmp));
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    path::path(const path & ref)
    {
        dirs = ref.dirs;
        relative = ref.relative;
	undisclosed = ref.undisclosed;
        reading = dirs.begin();
    }

    const path & path::operator = (const path & ref)
    {
        dirs = ref.dirs;
        relative = ref.relative;
	undisclosed = ref.undisclosed;
        reading = dirs.begin();

        return *this;
    }

    bool path::operator == (const path & ref) const
    {
	string me = display();
	string you = ref.display();

	return me == you;
    }

    string path::basename() const
    {
        if(! dirs.empty())
            return dirs.back();
        else
            return "/";
    }

    bool path::read_subdir(string & r)
    {
        if(reading != dirs.end())
        {
            r = *reading;
            ++reading;
            return true;
        }
        else
            return false;
    }

    bool path::pop(string &arg)
    {
        if(relative)
            if(dirs.size() > 1)
            {
                arg = dirs.back();
                dirs.pop_back();
                return true;
            }
            else
                return false;
        else
            if(!dirs.empty())
            {
                arg = dirs.back();
                dirs.pop_back();
                return true;
            }
            else
                return false;
    }

    bool path::pop_front(string & arg)
    {
        if(relative)
            if(dirs.size() > 1)
            {
                arg = dirs.front();
                dirs.pop_front();
                return true;
            }
            else
                return false;
        else
            if(!dirs.empty())
            {
                relative = false;
                arg = "/";
                return true;
            }
            else
                return false;
    }

    path & path::operator += (const path &arg)
    {
        if(!arg.relative)
            throw Erange("path::operator +", dar_gettext("Cannot add an absolute path"));

        list<string>::const_iterator it = arg.dirs.begin();
        list<string>::const_iterator it_fin = arg.dirs.end();
        while(it != it_fin)
        {
            if(*it != string("."))
                dirs.push_back(*it);
            ++it;
        }

	if(arg.undisclosed)
	    undisclosed = true;

	reduce();
        return *this;
    }

    bool path::is_subdir_of(const path & p, bool case_sensit) const
    {
	string me = display();
	string you = p.display();

	if(!case_sensit)
	{
		// converting all string in upper case
	    tools_to_upper(me);
	    tools_to_upper(you);
	}

	if(me.size() >= you.size())
	    if(strncmp(me.c_str(), you.c_str(), you.size()) == 0)
		return true;
	    else // path differs in the common length part, cannot be a subdir of "you"
		return false;
	else // I am shorter in length, cannot be a subdir of "you"
	    return false;
    }

    static void dummy_call(char *x)
    {
	static char id[]="$Id: path.cpp,v 1.29 2011/03/01 20:45:36 edrusb Rel $";
        dummy_call(id);
    }

    string path::display() const
    {
        string ret = relative ? "" : "/";
        list<string>::const_iterator it = dirs.begin();

        if(it != dirs.end())
            ret += *it++;
        while(it != dirs.end())
            ret = ret + "/" + *it++;

        return ret;
    }

    void path::explode_undisclosed() const
    {
	path *me = const_cast<path *>(this);
	if(!undisclosed)
	    return;

	try
	{
	    string res = display();
	    path tmp = path(res, false);
	    *me = tmp;
	}
	catch(...)
	{
	    me->reading = me->dirs.begin();
	}
    }

    void path::reduce()
    {
        dirs.remove(".");
        if(relative && dirs.empty())
	   dirs.push_back(".");
        else
        {
            list<string>::iterator it = dirs.begin();
            list<string>::iterator prev = it;

            while(it != dirs.end())
            {
                if(*it == ".." && *prev != "..")
                {
                    list<string>::iterator tmp = prev;

                    it = dirs.erase(it);
                    if(prev != dirs.begin())
                    {
                        --prev;
                        dirs.erase(tmp);
                    }
                    else
                    {
                        dirs.erase(prev);
                        prev = dirs.begin();
                    }
                }
                else
                {
                    prev = it;
                    ++it;
                }
            }
            if(relative && dirs.empty())
                dirs.push_back(".");
        }
    }

    static bool path_get_root(string & p, string & root)
    {
        string::iterator it = p.begin();

        if(p.empty())
            return false;
        while(it != p.end() && *it != '/' )
            ++it;

        root = string(p.begin(), it);
        if(it != p.end())
            p = string(it+1, p.end());
        else
            p = "";
        if(root.empty())
            throw Erange("path_get_root", dar_gettext("Empty string as subdirectory does not make a valid path"));

        return true;
    }

} // end of namespace
