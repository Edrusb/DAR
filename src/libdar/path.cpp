/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
}

#include <iostream>
#include "path.hpp"
#include "tools.hpp"
#include "nls_swap.hpp"

using namespace std;

namespace libdar
{

    const std::string PSEUDO_ROOT = "<ROOT>";
    const path FAKE_ROOT(PSEUDO_ROOT, true);


	/// splits a string in its path components

	/// \param[in] aggregate the path to split
	/// \param[out] splitted the resulted list of components
    static void path_split(const string & aggregate, list<string> & splitted);

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
		    path_split(s, dirs);
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

    path & path::operator = (const path & ref)
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

    bool path::read_subdir(string & r) const
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
                relative = true;
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

    path & path::operator += (const std::string & sub)
    {
	dirs.push_back(sub);
	reduce();
	return *this;
    }

    bool path::is_subdir_of(const path & p, bool case_sensit) const
    {
	string me;
	string you;

	if(!case_sensit)
	{
		// converting all string in upper case

	    tools_to_upper(display(), me);
	    tools_to_upper(p.display(), you);
	}
	else
	{
	    me = display();
	    you = p.display();
	}

	if(me.size() >= you.size())
	    if(strncmp(me.c_str(), you.c_str(), you.size()) == 0)
		if(me.size() > you.size())
		    return (you.size() > 1 && me[you.size()] == '/')
			|| (you.size() == 1 && you[0] == '/');
		else  // thus, me.size() == you.size(), thus I'm a subdir of myself
		    return true;
	    else // path differs in the common length part, cannot be a subdir of "you"
		return false;
	else // I am shorter in length, cannot be a subdir of "you"
	    return false;
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

    string path::display_without_root() const
    {
        string ret = "";
        list<string>::const_iterator it = dirs.begin();

	if(relative)
	    ++it;
	    // else we as we ignore the leading / we are all set

        if(it != dirs.end())
            ret += *it++;
        while(it != dirs.end())
            ret += string("/") + *it++;

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
	    reading = dirs.begin();
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

    static void path_split(const string & aggregate, list<string> & splitted)
    {
	splitted.clear();

	string::const_iterator it = aggregate.begin();
	string::const_iterator bk = it;

	while(it != aggregate.end())
	{
	    if(*it == '/')
	    {
		string part(bk, it);

		if(! part.empty())
		{
		    if(part[part.size() - 1] == ':' // word ending with : thus :/ was read
		       && aggregate[0] != '/'       // not an absolute path
		       && splitted.empty())         // this is the first word we add so we have
			                            // something in the form of "<protocol>:/"
			                            // at the beginning of "aggregate"
		    {
			if(aggregate.size() > part.size() + 1)
			    if(aggregate[part.size() + 1] == '/')
				    // "<protocol>://" is found (with a second slash)
				    // at the beginning of aggregate, we must not
				    // drop this URL-type of information:
				part += '/';
			    // we store '<protocol>:/' as first word, which will
			    // be reconstructed as <protocol>://something
			    // if the "something" part is present in "aggregate".
			    // But if there is no "something" after, the reconstruction will provide "<protocol>:/"
			    // instead of "<protocol>://" (but this is not a valid URL) it match
			    // is a subdirectory get added later: "<protocol>://subdir" so we
			    // dont add a second / at the end of "part" in that case
		    }

		    splitted.push_back(part);
		}
		    // if path contains one or more following slashes
		    // it will generate here empty strings as part,
		    // we just ignore them, which will result in
		    // subtituing more than one sticked slashs
		    // by a single one when comparing it to strings.
		    // Path normalization performed:
		    // - replace several following / by a single one
		    // - ignore trailing slash
		    // - ignore initial slash (relative/absolute path)
		    // - no / is accept as part of a directory name or
		    //   of a filename.
		++it;
		bk = it;
	    }
	    else
		++it;
	}

	if(bk != aggregate.end())
	{
	    string part(bk, it);
	    if(! part.empty())
		splitted.push_back(part);
	}
    }


} // end of namespace
