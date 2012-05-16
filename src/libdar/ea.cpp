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
// $Id: ea.cpp,v 1.7.4.1 2004/07/13 22:37:33 edrusb Rel $
//
/*********************************************************************/
//

#pragma implementation

#include "../my_config.h"
#include "ea.hpp"
#include "tools.hpp"
#include "integers.hpp"

// theses MACRO are used only when dumping in file
#define EA_ROOT 0x80
#define EA_DEL 0x40
#define EA_DEFAULT_USER_INSERT 0x00

using namespace std;

namespace libdar
{

    ea_attributs::ea_attributs(const ea_attributs & ref)
    {
        attr = ref.attr;
        alire = attr.begin();
    }

    ea_entry::ea_entry(generic_file & f)
    {
        unsigned char fl;

        f.read((char *)(&fl), 1);
        domain = (fl & EA_ROOT) != 0 ? ea_domain_root : ea_domain_user;
        mode = (fl &
                EA_DEL) != 0 ? ea_del : ea_insert;
        tools_read_string(f, key);
        infinint tmp = infinint(NULL, &f);
        tools_read_string_size(f, value, tmp);
    }

    void ea_entry::dump(generic_file & f) const
    {
        unsigned char fl = 0;
        infinint tmp = value.size();
        if(domain == ea_domain_root)
            fl |= EA_ROOT;
        if(mode == ea_del)
            fl |= EA_DEL;
        f.write((char *)(&fl), 1);
        tools_write_string(f, key);
        tmp.dump(f);
        tools_write_string_all(f, value);
    }

///////////// EA_ATTRIBUTS IMPLEMENTATION //////////

    ea_attributs::ea_attributs(generic_file & f)
    {
        U_32 tmp2 = 0;
        infinint tmp = infinint(NULL, &f);

        tmp.unstack(tmp2);

        do
        {
            while(tmp2 > 0)
            {
                attr.push_back(ea_entry(f));
                tmp2--;
            }
            tmp.unstack(tmp2);
        }
        while(tmp2 > 0);

        alire = attr.begin();
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: ea.cpp,v 1.7.4.1 2004/07/13 22:37:33 edrusb Rel $";
        dummy_call(id);
    }

    void ea_attributs::dump(generic_file & f) const
    {
        vector<ea_entry>::iterator it = const_cast<ea_attributs &>(*this).attr.begin();
        vector<ea_entry>::iterator fin = const_cast<ea_attributs &>(*this).attr.end();


        size().dump(f);
        while(it != fin)
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

    bool ea_attributs::diff(const ea_attributs & other, bool check_ea_root, bool check_ea_user) const
    {
        ea_entry ea;
        string value;
        ea_mode mode;
        bool diff = false;

        reset_read();
        while(!diff && read(ea))
            if(ea.mode == ea_insert)
                if((ea.domain == ea_domain_user && check_ea_user) || (ea.domain == ea_domain_root && check_ea_root))
                    if(other.find(ea.domain, ea.key, mode, value))
                    {
                        if(value != ea.value) // found but different
                            diff = true;
                    }
                    else // not found
                        diff = true;
        return diff;
    }

    bool ea_attributs::find(ea_domain dom, const string &key, ea_mode & found_mode, string & found_value) const
    {
        vector<ea_entry>::iterator it = const_cast<vector<ea_entry> &>(attr).begin();
        vector<ea_entry>::iterator fin = const_cast<vector<ea_entry> &>(attr).end();

        while(it != fin && (it->domain != dom || it->key != key))
            it++;
        if(it != fin)
        {
            found_mode = it->mode;
            found_value = it->value;
            return true;
        }
        else
            return false;
    }

} // end of namespace
