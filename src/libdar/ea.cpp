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

#include "ea.hpp"
#include "tools.hpp"
#include "integers.hpp"
#include "infinint.hpp"

// these MACRO are used only when dumping in file (obsolete since archive format "05")
#define EA_ROOT 0x80
#define EA_DEL 0x40
#define EA_DEFAULT_USER_INSERT 0x00

using namespace std;

namespace libdar
{

///////////// STATIC FUNCTION DECLARATION ///////////

    static void read_pair_string(generic_file & f, const archive_version & edit, string & key, string & val);
    static void write_pair_key(generic_file & f, const string & key, const string & val);

///////////// EA_ATTRIBUTS IMPLEMENTATION //////////

    ea_attributs::ea_attributs(generic_file & f, const archive_version & edit)
    {
        infinint tmp = infinint(f); // number of EA
	string key, val;
	U_32 tmp2 = 0;


	tmp.unstack(tmp2);

	do
	{
	    while(tmp2 > 0)
	    {
		read_pair_string(f, edit, key, val);
		attr[key] = val;
		tmp2--;
	    }
	    tmp.unstack(tmp2);
	}
	while(tmp2 > 0);

        alire = attr.begin();
    }

    ea_attributs::ea_attributs(const ea_attributs & ref)
    {
        attr = ref.attr;
        alire = attr.begin();
    }

    ea_attributs::ea_attributs(ea_attributs && ref) noexcept
    {
        attr = move(ref.attr);
        alire = attr.begin();
    }

    ea_attributs & ea_attributs::operator = (const ea_attributs & ref)
    {
        attr = ref.attr;
        alire = attr.begin();

	return *this;
    }

    ea_attributs & ea_attributs::operator = (ea_attributs && ref) noexcept
    {
        attr = move(ref.attr);
        alire = attr.begin();

	return *this;
    }

    void ea_attributs::dump(generic_file & f) const
    {
        map<string, string>::const_iterator it = attr.begin();

        size().dump(f);
        while(it != attr.end())
        {
	    write_pair_key(f, it->first, it->second);
            it++;
        }
    }

    void ea_attributs::reset_read() const
    {
	    // "moi" is necessary to avoid assigning a const_iterator to an iterator
        ea_attributs *moi = const_cast<ea_attributs *>(this);
        moi->alire = moi->attr.begin();
    }

    bool ea_attributs::read(string & key, string & value) const
    {
        if(alire != attr.end())
        {
            key = alire->first;
	    value = alire->second;
	    ++alire;
            return true;
        }
        else
            return false;
    }

    bool ea_attributs::diff(const ea_attributs & other, const mask & filter) const
    {
	string key;
	string val;
        string value;
        bool diff = false;

        reset_read();
        while(!diff && read(key, val))
	    if(filter.is_covered(key))
	    {
		if(!other.find(key, value) || value != val) // not found or different
		    diff = true;
	    }

        return diff;
    }

    bool ea_attributs::find(const string & key, string & found_value) const
    {
        map<string, string>::const_iterator it = attr.find(key);

        if(it != attr.end())
        {
	    found_value = it->second;
	    if(it->first != key)
		throw SRC_BUG;
            return true;
        }
        else
            return false;
    }

    infinint ea_attributs::space_used() const
    {
	map<string, string>::const_iterator it = attr.begin();
	infinint ret = 0;

	while(it != attr.end())
	{
	    ret += it->first.size() + it->second.size();
	    ++it;
	}

	return ret;
    }

    ea_attributs ea_attributs::operator + (const ea_attributs & arg) const
    {
	ea_attributs ret = *this; // copy constructor
	string key, value;

	arg.reset_read();
	while(arg.read(key, value))
	    ret.add(key, value);

	return ret;
    }

///////////// STATIC FUNCTION IMPLEMENTATION ///////

    static void read_pair_string(generic_file & f, const archive_version & edit, string & key, string & val)
    {
	infinint tmp;
	unsigned char fl;
	string pre_key = "";

	if(edit < 5) // old format
	{
	    f.read((char *)(&fl), 1);
	    if((fl & EA_ROOT) != 0)
		pre_key = "system.";
	    else
		pre_key = "user.";
	}
        tools_read_string(f, key);
	key = pre_key + key;
        tmp = infinint(f);
        tools_read_string_size(f, val, tmp);
    }

    static void write_pair_key(generic_file & f, const string & key, const string & val)
    {
        infinint tmp = val.size();

        tools_write_string(f, key);
        tmp.dump(f);
        tools_write_string_all(f, val);
    }

} // end of namespace
