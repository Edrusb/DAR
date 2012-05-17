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

    /// \file ea.hpp
    /// \brief contains a set of routines to manage EA values associated to a file
    /// \ingroup Private


#ifndef EA_HPP
#define EA_HPP

#include "../my_config.h"
#include <vector>
#include <string>
#include "infinint.hpp"
#include "generic_file.hpp"
#include "special_alloc.hpp"
#include "mask.hpp"
#include "header_version.hpp"

namespace libdar
{

    struct ea_entry
    {
        std::string key, value;

        ea_entry() { key = value = ""; };
        ea_entry(user_interaction & dialog, generic_file & f, const dar_version & edit);

        void dump(generic_file & f) const;
    };

    class ea_attributs
    {
    public:
        ea_attributs() { alire = attr.begin(); };
        ea_attributs(user_interaction & dialog, generic_file & f, const dar_version & edit);
        ea_attributs(const ea_attributs & ref);

        void dump(generic_file & f) const;
        void add(const ea_entry &x) { attr.push_back(x); };
        void reset_read() const;
        bool read(ea_entry & x) const;
        infinint size() const { return attr.size(); };
        void clear() { attr.clear(); alire = attr.begin(); };
        bool find(const std::string &key, std::string & found_value) const;
        bool diff(const ea_attributs & other, const mask & filter) const;

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(ea_attributs);
#endif

    private:
        std::vector<ea_entry> attr;
        std::vector<ea_entry>::iterator alire;
    };

} // end of namespace

#endif
