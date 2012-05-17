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

#include "../my_config.h"
#include <iostream>
#include "path.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    static bool path_get_root(string & p, string & root);

    path::path(const string & chem)
    {
        string tmp;
	string s = chem;

	try
	{
	    dirs.clear();
	    if(s.empty())
		throw Erange("path::path", gettext("Empty string is not a valid path"));
	    relative = s[0] != '/';
	    if(!relative)
		s = string(s.begin()+1, s.end()); // remove the leading '/'
	    while(path_get_root(s, tmp))
		dirs.push_back(tmp);
	    if(dirs.empty() && relative)
		throw Erange("path::path", gettext("Empty string is not a valid path"));
	    reduce();
	    reading = dirs.begin();
	}
	catch(Erange & e)
	{
	    string e_tmp = e.get_message();
	    throw Erange("path::path", tools_printf(gettext("%S is an not a valid path: %S"), &chem, &e_tmp));
	}
    }

    path::path(const path & ref)
    {
        dirs = ref.dirs;
        relative = ref.relative;
        reading = dirs.begin();
    }

    path & path::operator = (const path & ref)
    {
        dirs = ref.dirs;
        relative = ref.relative;
        reading = dirs.begin();

        return *this;
    }

    bool path::operator == (const path & ref) const
    {
        if(ref.dirs.size() != dirs.size() || ref.relative != relative)
            return false;
        else
        {
            list<string>::iterator here = (const_cast<path &>(*this)).dirs.begin();
            list<string>::iterator there = (const_cast<path &>(ref)).dirs.begin();
            list<string>::iterator here_fin = (const_cast<path &>(*this)).dirs.end();
            list<string>::iterator there_fin = (const_cast<path &>(ref)).dirs.end();
            while(here != here_fin && there != there_fin && *here == *there)
            {
                ++here;
                ++there;
            }

            return here == here_fin && there == there_fin;
        }
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
        if(is_relative())
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
        if(!arg.is_relative())
            throw Erange("path::operator +", gettext("Cannot add an absolute path"));
        list<string>::iterator it = (const_cast<path &>(arg)).dirs.begin();
        list<string>::iterator it_fin = (const_cast<path &>(arg)).dirs.end();
        while(it != it_fin)
        {
            if(*it != string("."))
                dirs.push_back(*it);
            ++it;
        }

        return *this;
    }

    bool path::is_subdir_of(const path & p, bool case_sensit) const
    {
        list<string>::iterator it_me = (const_cast<path &>(*this)).dirs.begin();
        list<string>::iterator it_arg = (const_cast<path &>(p)).dirs.begin();
        list<string>::iterator fin_me = (const_cast<path &>(*this)).dirs.end();
        list<string>::iterator fin_arg = (const_cast<path &>(p)).dirs.end();

        while(it_me != fin_me && it_arg != fin_arg
              && ((case_sensit && *it_me == *it_arg)  || (!case_sensit && tools_is_case_insensitive_equal(*it_me, *it_arg))))
        {
            ++it_me;
            ++it_arg;
        }

        return it_arg == fin_arg;
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
            throw Erange("path_get_root", gettext("Empty string as subdirectory does not make a valid path"));

        return true;
    }

} // end of namespace
