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

#include "ea.hpp"
#include "tools.hpp"
#include "integers.hpp"

// these MACRO are used only when dumping in file (obsolete since archive format "05")
#define EA_ROOT 0x80
#define EA_DEL 0x40
#define EA_DEFAULT_USER_INSERT 0x00

using namespace std;

namespace libdar
{

    ea_entry::ea_entry(user_interaction & dialog, generic_file & f, const dar_version & edit)
    {
	infinint tmp;
	unsigned char fl;
	string pre_key = "";

	if(version_greater("05", edit)) // "05" > edit => old format
	{
	    f.read((char *)(&fl), 1);
	    if((fl & EA_ROOT) != 0)
		pre_key = "system.";
	    else
		pre_key = "user.";
	}
        tools_read_string(f, key);
	key = pre_key + key;
        tmp = infinint(dialog, NULL, &f);
        tools_read_string_size(f, value, tmp);
    }

    void ea_entry::dump(generic_file & f) const
    {
        infinint tmp = value.size();

        tools_write_string(f, key);
        tmp.dump(f);
        tools_write_string_all(f, value);
    }

///////////// EA_ATTRIBUTS IMPLEMENTATION //////////

    ea_attributs::ea_attributs(user_interaction & dialog, generic_file & f, const dar_version & edit)
    {
        U_32 tmp2 = 0;
        infinint tmp = infinint(dialog, NULL, &f); // number of EA

        tmp.unstack(tmp2);

        do
        {
            while(tmp2 > 0)
            {
                attr.push_back(ea_entry(dialog, f, edit));
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

    void ea_attributs::dump(generic_file & f) const
    {
        vector<ea_entry>::const_iterator it = attr.begin();

        size().dump(f);
        while(it != attr.end())
        {
            it->dump(f);
            it++;
        }
    }

    void ea_attributs::reset_read() const
    {
        ea_attributs *moi = const_cast<ea_attributs *>(this);
        moi->alire = moi->attr.begin();
    }

    bool ea_attributs::read(ea_entry & x) const
    {
        ea_attributs *moi = const_cast<ea_attributs *>(this);
        if(alire != attr.end())
        {
            x = *(moi->alire)++;
            return true;
        }
        else
            return false;
    }

    bool ea_attributs::diff(const ea_attributs & other, const mask & filter) const
    {
        ea_entry ea;
        string value;
        bool diff = false;

        reset_read();
        while(!diff && read(ea))
	    if(filter.is_covered(ea.key))
	    {
		if(!other.find(ea.key, value) || value != ea.value) // not found or different
		    diff = true;
	    }

        return diff;
    }

    bool ea_attributs::find(const string & key, string & found_value) const
    {
        vector<ea_entry>::const_iterator it = attr.begin();

        while(it != attr.end() && it->key != key)
            it++;
        if(it != attr.end())
        {
	    found_value = it->value;
            return true;
        }
        else
            return false;
    }

} // end of namespace
