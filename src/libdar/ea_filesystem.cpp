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

#ifdef EA_SUPPORT
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_ATTR_XATTR_H && ! HAVE_SYS_XATTR_H
#include <attr/xattr.h>
#endif
#if HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif
#endif
} // end extern "C"

#include <deque>

#include "ea_filesystem.hpp"
#include "ea.hpp"
#include "tools.hpp"

#define MSG_NO_EA_SUPPORT "Extended Attribute support not activated at compilation time"

#if defined(ENOATTR)
#define LIBDAR_NOATTR ENOATTR
#elif defined(ENODATA)
#define LIBDAR_NOATTR ENODATA
#endif

using namespace std;

namespace libdar
{

#ifdef EA_SUPPORT
	// Wrapper functions for l*attr
    inline static ssize_t my_lgetxattr(const char *path, const char *name, void *value, size_t size);
    inline static int     my_lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags);
    inline static ssize_t my_llistxattr(const char *path, char *list, size_t size);
    inline static int     my_lremovexattr(const char *path, const char *name);

    static bool remove_ea(const string & name, const ea_attributs & val, const mask & filter);
    static ea_attributs * read_ea(const string & name, const mask & filter);
    static deque<string> ea_filesystem_get_ea_list_for(const char *filename);
#endif
    static bool write_ea(const string & chemin, const ea_attributs & val, const mask & filter);

    bool ea_filesystem_write_ea(const string & chemin, const ea_attributs & val, const mask & filter)
    {
        return write_ea(chemin, val, filter);
    }

    ea_attributs * ea_filesystem_read_ea(const string & name, const mask & filter)
    {
#ifdef EA_SUPPORT
        return read_ea(name, filter);
#else
	return nullptr;
#endif
    }

    void ea_filesystem_clear_ea(const string &name, const mask & filter)
    {
#ifdef EA_SUPPORT
        ea_attributs * eat =  ea_filesystem_read_ea(name, filter);
	try
	{
	    if(eat != nullptr)
		remove_ea(name, *eat, bool_mask(true));
	}
	catch(...)
	{
	    if(eat != nullptr)
		delete eat;
	    throw;
	}
	if(eat != nullptr)
	    delete eat;
#else
        throw Efeature(gettext(MSG_NO_EA_SUPPORT));
#endif
    }

    bool ea_filesystem_has_ea(const string & name)
    {
#ifdef EA_SUPPORT
	deque<string> val = ea_filesystem_get_ea_list_for(name.c_str());
	return ! val.empty();
#else
	return false;
#endif
    }

    bool ea_filesystem_has_ea(const string & name, const ea_attributs & list, const mask & filter)
    {
#ifdef EA_SUPPORT
	const char *p_name = name.c_str();
	bool ret = false;

	deque<string> val = ea_filesystem_get_ea_list_for(p_name);
	deque<string>::iterator it = val.begin();
	string tmp;

	while(it != val.end() && ! ret)
	{
	    if(filter.is_covered(*it))
		ret = list.find(*it, tmp);
	    it++;
	}

	return ret;
#else
	return false;
#endif
    }

    static bool write_ea(const string & chemin, const ea_attributs & val, const mask & filter)
    {
        U_I num = 0;
#ifdef EA_SUPPORT
	    // avoid unused variable compilation warning when EA_SUPPORT is not set
        const char *p_chemin = chemin.c_str();
#endif
	string key, value;

	val.reset_read();
	while(val.read(key, value))
	{
		// doing this for each attribute

	    if(!filter.is_covered(key))
		continue; // silently skipping this EA

		// now, action !

#ifdef EA_SUPPORT

	    if(my_lsetxattr(p_chemin, key.c_str(), value.c_str(), value.size(), 0) < 0)
	    {
		string tmp = tools_strerror_r(errno);
		throw Erange("ea_filesystem write_ea", tools_printf(gettext("Error while adding EA %s : %s"),
								    key.c_str(), tmp.c_str()));
	    }
	    else
		num++;
#else
	    throw Efeature(gettext(MSG_NO_EA_SUPPORT));
#endif
	}

	return num > 0;
    }

#ifdef EA_SUPPORT
    static bool remove_ea(const string & chemin, const ea_attributs & val, const mask & filter)
    {
        U_I num = 0;
        const char *p_chemin = chemin.c_str();
	string key, value;

	val.reset_read();
	while(val.read(key, value))
	{
		// doing this for each attribute

	    if(!filter.is_covered(key))
		continue; // silently skipping this EA

		// now, action !

	    const char *k = key.c_str();
	    if(my_lremovexattr(p_chemin, k) < 0)
	    {
		string tmp = tools_strerror_r(errno);
#ifdef LIBDAR_NOATTR
		if(errno != LIBDAR_NOATTR)
#endif
		    throw Erange("ea_filesystem write_ea", tools_printf(gettext("Error while removing %s : %s"),
									k, tmp.c_str()));
	    }
	    else
		num++;
	}

        return num > 0;
    }

    static ea_attributs * read_ea(const string & name, const mask & filter)
    {
        const char *n_ptr = name.c_str();
	ea_attributs *ret = nullptr;

	deque<string> ea_liste = ea_filesystem_get_ea_list_for(n_ptr);
	deque<string>::iterator it = ea_liste.begin();

	try
	{
	    while(it != ea_liste.end())
	    {
		if(filter.is_covered(*it))
		{
		    const char *a_name = it->c_str();
		    const U_I MARGIN = 2;
		    string ea_ent_key, ea_ent_value;
		    S_64 taille = my_lgetxattr(n_ptr, a_name, nullptr, 0);
		    char *value = nullptr;
		    if(taille < 0)
		    {
			string tmp = tools_strerror_r(errno);
			throw Erange("ea_filesystem read_ea", tools_printf(gettext("Error reading attribute %s of file %s : %s"),
									   a_name, n_ptr, tmp.c_str()));
		    }

		    if(ret == nullptr)
		    {
			ret = new (nothrow) ea_attributs();
			if(ret == nullptr)
			    throw Ememory("read_ea");
			ret->clear();
		    }

		    if(taille > 0)
		    {
			value = new (nothrow) char[taille+MARGIN];
			if(value == nullptr)
			    throw Ememory("filesystem : read_ea_from");
			try
			{
			    taille = my_lgetxattr(n_ptr, a_name, value, taille+MARGIN);
				// if the previous call overflows the buffer this may need to SEGFAULT and so on.
			    if(taille < 0)
			    {
				string tmp = tools_strerror_r(errno);
				throw Erange("ea_filesystem read_ea", tools_printf(gettext("Error reading attribute %s of file %s : %s"),
										   a_name, n_ptr, tmp.c_str()));
			    }
			    ea_ent_key = *it;
			    ea_ent_value = string((char *)value, (char *)value+taille);
			    ret->add(ea_ent_key, ea_ent_value);
			}
			catch(...)
			{
			    delete [] value;
			    throw;
			}
			delete [] value;
		    }
		    else // trivial case where the value has a length of zero
		    {
			ea_ent_key = *it;
			ea_ent_value = string("");
			ret->add(ea_ent_key, ea_ent_value);
		    }
		}
		it++;
	    }
	}
	catch(...)
	{
	    if(ret != nullptr)
	    {
		delete ret;
		ret = nullptr;
	    }
	    throw;
	}

	return ret;
    }

    static deque<string> ea_filesystem_get_ea_list_for(const char *filename)
    {
        deque<string> ret;
        const U_I MARGIN = 2;
        ssize_t taille = my_llistxattr(filename, nullptr, 0);
        char *liste = nullptr;

        if(taille < 0)
        {
            if(errno == ENOSYS || errno == ENOTSUP)
                return ret;
	    string tmp = tools_strerror_r(errno);
            throw Erange("ea_filesystem_get_ea_list_for",
			 tools_printf(gettext("Error retrieving EA list for %s : %s"),
				      filename, tmp.c_str()));
        }

	liste = new (nothrow) char[taille+MARGIN];
        if(liste == nullptr)
            throw Ememory("filesystem : get_ea_list_for");
        try
        {
            S_64 cursor = 0;
            taille = my_llistxattr(filename, liste, taille+MARGIN);
            if(taille < 0)
	    {
		string tmp = tools_strerror_r(errno);
                throw Erange("ea_filesystem_get_ea_list_for",
			     tools_printf(gettext("Error retrieving EA list for %s : %s"),
					  filename, tmp.c_str()));
	    }
            while(cursor < taille)
            {
                ret.push_back(string(liste+cursor));
                cursor += strlen(liste+cursor)+1;
            }
        }
        catch(...)
        {
	    delete [] liste;
            throw;
        }
	delete [] liste;

        return ret;
    }

#ifdef OSX_EA_SUPPORT

    inline static ssize_t my_lgetxattr(const char *path, const char *name, void *value, size_t size)
    {
        return getxattr(path, name, value, size, 0, XATTR_NOFOLLOW);
    }

    inline static int     my_lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags)
    {
        return setxattr(path, name, value, size, 0, XATTR_NOFOLLOW | flags);
    }

    inline static ssize_t my_llistxattr(const char *path, char *list, size_t size)
    {
        return listxattr(path, list, size, XATTR_NOFOLLOW);
    }

    inline static int     my_lremovexattr(const char *path, const char *name)
    {
        return removexattr(path, name, XATTR_NOFOLLOW);
    }

#else

    inline static ssize_t my_lgetxattr(const char *path, const char *name, void *value, size_t size)
    {
        return lgetxattr(path, name, value, size);
    }

    inline static int     my_lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags)
    {
        return lsetxattr(path, name, value, size, flags);
    }

    inline static ssize_t my_llistxattr(const char *path, char *list, size_t size)
    {
        return llistxattr(path, list, size);
    }

    inline static int     my_lremovexattr(const char *path, const char *name)
    {
        return lremovexattr(path, name);
    }

#endif

#endif

} // end of namespace
