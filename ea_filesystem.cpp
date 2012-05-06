/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: ea_filesystem.cpp,v 1.7 2002/06/27 17:42:35 denis Rel $
//
/*********************************************************************/

#ifdef EA_SUPPORT
#include <sys/types.h>
#include <attr/xattr.h>
#endif
#include "ea.hpp"
#include "tools.hpp"
#include "ea_filesystem.hpp"
#include "user_interaction.hpp"

#ifdef EA_SUPPORT
static string ea_convert[] = { "system", "user" };

static bool write_ea(const string & chemin, const ea_attributs & val, bool ea_root, bool ea_user);
static void read_ea(const string & name, ea_attributs & val, bool read_ea_root, bool read_ea_user);
static vector<string> ea_filesystem_get_ea_list_for(const char *filename);
static string ea_domain2string(ea_domain d);
static ea_domain string2ea_domain(const string & x);
static void split_ea_name(const string & src, ea_domain & d, string & key);
static string glue_ea_name(const ea_domain & d, const string & key);
#endif

bool ea_filesystem_write_ea(const string & chemin, const ea_attributs & val, bool root, bool user)
{
#ifdef EA_SUPPORT
    return write_ea(chemin, val, root, user);
#else
    return false;
#endif
}

void ea_filesystem_read_ea(const string & name, ea_attributs & val, bool root, bool user)
{
#ifdef EA_SUPPORT
    read_ea(name, val, root, user);
#else
    val.clear();
#endif
}

void ea_filesystem_clear_ea(const string &name, ea_domain dom)
{
#ifdef EA_SUPPORT
    ea_attributs eat;
    ea_attributs res;
    ea_entry one;
	
    ea_filesystem_read_ea(name, eat, true, true);
    eat.reset_read();    
    while(eat.read(one))
    {
	if(one.domain == dom)
	{
	    one.mode = ea_del;
	    res.add(one);
	}
    }
    ea_filesystem_write_ea(name, res, true, true);
#endif
}

bool ea_filesystem_is_present(const string & name, ea_domain dom)
{
    ea_attributs tmp;
    ea_entry ea_ent;
    bool found = false;

    ea_filesystem_read_ea(name, tmp, true, true);
    tmp.reset_read();
    while(!found && tmp.read(ea_ent))
    {
	if(ea_ent.domain == dom)
	    found = true;
    }

    return found;
}	

#ifdef EA_SUPPORT

static bool write_ea(const string & chemin, const ea_attributs & val, bool rest_ea_root, bool rest_ea_user)
{
    char *p_chemin = NULL;
    unsigned int num = 0;

    if(!ea_root && !ea_user)
	return false; // no EA can be restored

    p_chemin = tools_str2charptr(chemin);
    try
    {
	ea_entry ea_ent;
	
	val.reset_read();
	while(val.read(ea_ent))
	{
		// doing this for each attribute
	    
	    if(ea_ent.domain == ea_root && !rest_ea_root)
		continue; // silently skipping this EA 
	    if(ea_ent.domain == ea_user && !rest_ea_user)
		continue; // silently skipping this EA

	    char *k = tools_str2charptr(glue_ea_name(ea_ent.domain, ea_ent.key));
	    try
	    {
		char *v = tools_str2charptr(ea_ent.value);
		unsigned long v_size = ea_ent.value.size();
		try
		{
				// now, action !
		    switch(ea_ent.mode)
		    {
		    case ea_insert:
			if(lsetxattr(p_chemin, k, v, v_size, 0) < 0)
			    throw Erange("ea_filesystem write_ea", string("aborting operations for the EA of ")+chemin+ " : error while adding EA "+ k + " : " + strerror(errno));
			else
			    num++;
			break;
		    case ea_del:
			if(lremovexattr(p_chemin, k) < 0)
			{
			    if(errno != ENOATTR)
				throw Erange("ea_filesystem write_ea", string("aborting operations for the EAs of ")+chemin+ " : error while removing " + k + " : " + strerror(errno));
			}
			else
			    num++;
			break;
		    default:
			throw SRC_BUG;
		    }
		}
		catch(...)
		{
		    delete v;
		    throw;
		}
		delete v;
	    }
	    catch(...)
	    {
		delete k;
		throw;
	    }
	    delete k;
	}
    }
    catch(Egeneric & e)
    {
	delete p_chemin;
	throw;
    }
    delete p_chemin;

    return num > 0;
}

static void read_ea(const string & name, ea_attributs & val, bool read_ea_root, bool read_ea_user)
{
    val.clear();
    if(! read_ea_root && ! read_ea_user)
	return; // nothing to do

    char *n_ptr = tools_str2charptr(name);
    if(n_ptr == NULL)
	throw Ememory("read_ea_from");
    try
    {
	vector<string> ea_liste = ea_filesystem_get_ea_list_for(n_ptr);
	vector<string>::iterator it = ea_liste.begin();
	
	while(it != ea_liste.end())
	{
	    char *a_name = tools_str2charptr(*it);
	    if(a_name == NULL)
		throw Ememory("filesystem : read_ea_from");
	    try
	    {
		const unsigned int MARGIN = 10;
		ea_entry ea_ent;
		long taille = lgetxattr(n_ptr, a_name, NULL, 0);
		char *value = NULL;
		if(taille < 0)
		    throw Erange("ea_filesystem read_ea", string("error reading attribut ") + a_name + " of file " + n_ptr + "  : " + strerror(errno));
		value = new char[taille+MARGIN];
		if(value == NULL)
		    throw Ememory("filesystem : read_ea_from");
		try
		{
		    taille = lgetxattr(n_ptr, a_name, value, taille+MARGIN);
		    if(taille < 0)
			throw Erange("ea_filesystem read_ea", string("error reading attribut ") + a_name + " of file " + n_ptr + "  : " + strerror(errno));
		    split_ea_name(*it, ea_ent.domain, ea_ent.key);
		    ea_ent.mode = ea_insert;
		    ea_ent.value = string(&(value[0]), &(value[taille]));
		    if((ea_ent.domain == ea_root && read_ea_root) ||
		       (ea_ent.domain == ea_user && read_ea_user))
			val.add(ea_ent);
		}
		catch(...)
		{
		    delete value;
		    throw;
		}
		delete value;
	    }
	    catch(...)
	    {
		delete a_name;
		throw;
	    }
	    delete a_name;
	    it++;
	}
    }
    catch(...)
    {
	delete n_ptr;
	throw;
    }
    delete n_ptr;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: ea_filesystem.cpp,v 1.7 2002/06/27 17:42:35 denis Rel $";

    dummy_call(id);
}

static vector<string> ea_filesystem_get_ea_list_for(const char *filename)
{
    vector<string> ret;
    const unsigned int MARGIN = 20;
    ssize_t taille = llistxattr(filename, NULL, 0);
    char *liste = NULL;

    if(taille < 0)
	throw Erange("ea_filesystem_get_ea_list_for", string("error retreiving EA list for ")+filename+ " : " + strerror(errno));
    
    liste = new char[taille+MARGIN];
    if(liste == NULL)
	throw Ememory("filesystem : get_ea_list_for");
    try
    {
	long cursor = 0;
	taille = llistxattr(filename, liste, taille+MARGIN);
	if(taille < 0)
	    throw Erange("ea_filesystem_get_ea_list_for", string("error retreiving EA list for ")+filename+ " : " + strerror(errno));
	while(cursor < taille)
	{
	    ret.push_back(string(liste+cursor));
	    cursor += strlen(liste+cursor)+1;
	}
    }
    catch(...)
    {
	delete liste;
	throw;
    }
    delete liste;
    return ret;
}

static string ea_domain2string(ea_domain d)
{
    return ea_convert[d];
}

static ea_domain string2ea_domain(const string & x)
{
    if(x == ea_convert[ea_root])
	return ea_root;
    else if(x == ea_convert[ea_user])
	return ea_user;
    else
	throw Erange("ea_filesystem : string2ea_domain", string("unknow EA namespace : ") + x);
}

static void split_ea_name(const string & src, ea_domain & d, string & key)
{
    unsigned int cesure = src.find_first_of(".");

    if(cesure >= src.size() || cesure < 0)
	throw Erange("ea_filesystem split_ea_name", string("unknown EA attribute name format : ") + src);
    d = string2ea_domain(string(src.begin(), src.begin()+cesure));
    key = string(src.begin()+cesure+1, src.end());
}

static string glue_ea_name(const ea_domain & d, const string & key)
{
    return ea_domain2string(d) + "." + key;
}

#endif
