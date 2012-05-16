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
// $Id: ea.hpp,v 1.8.4.1 2004/01/28 15:29:46 edrusb Rel $
//
/*********************************************************************/

#ifndef EA_HPP
#define EA_HPP

#pragma interface

#include "../my_config.h"
#include <vector>
#include <string>
#include "generic_file.hpp"
#include "infinint.hpp"
#include "special_alloc.hpp"

namespace libdar
{

    enum ea_mode { ea_insert, ea_del }; // for use later
        // actually the whole EA list is stored/restored, but
        // it could be possible to check which EA have changed to save only thoses
        // and also record EA that have been removed.
        // the problem is the localisation of EA in the archive,
        // comparison implies retrieving each EA list, which could implies
        // reading the whole archive for EA lists.
        // other thing to consider, is the catalogue extraction, it must also contain
        // all EA lists. to be able to compare.
        // solution : store the EA lists after the data and before the catalogue.
        // to be seen.
        //
    enum ea_domain { ea_root, ea_user };

    struct ea_entry
    {
        ea_mode mode;
        enum ea_domain domain;
        std::string key, value;

        ea_entry() { mode = ea_insert; domain = ea_user; key = value = ""; };
        ea_entry(generic_file & f);

        void dump(generic_file & f) const;
    };

    class ea_attributs
    {
    public:
        ea_attributs() { alire = attr.begin(); };
        ea_attributs(generic_file & f);
        ea_attributs(const ea_attributs & ref);

        void dump(generic_file & f) const;
        void add(const ea_entry &x) { attr.push_back(x); };
        void reset_read() const;
        bool read(ea_entry & x) const;
        infinint size() const { return attr.size(); };
        void clear() { attr.clear(); alire = attr.begin(); };
        bool find(ea_domain dom, const std::string &key, ea_mode & found_mode, std::string & found_value) const;
        bool diff(const ea_attributs & other, bool check_ea_root, bool check_ea_user) const;
    
        void check() const {}; // actually empty, but additional checks could be added

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(ea_attributs);
#endif

    private:
        std::vector<ea_entry> attr;
        std::vector<ea_entry>::iterator alire;
    };

} // end of namespace

#endif
