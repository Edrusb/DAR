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
// $Id: ea.hpp,v 1.23 2011/01/09 17:25:58 edrusb Rel $
//
/*********************************************************************/

    /// \file ea.hpp
    /// \brief contains a set of routines to manage EA values associated to a file
    /// \ingroup Private


#ifndef EA_HPP
#define EA_HPP

#include "../my_config.h"
#include <map>
#include <string>
#include "infinint.hpp"
#include "generic_file.hpp"
#include "special_alloc.hpp"
#include "mask.hpp"
#include "header_version.hpp"

namespace libdar
{

	/// \ingroup Private
	/// @}

	/// the class ea_attributs manages the set of EA that can be associated to an inode

    class ea_attributs
    {
    public:
        ea_attributs() { alire = attr.begin(); };
        ea_attributs(generic_file & f, const archive_version & edit);
        ea_attributs(const ea_attributs & ref);

        void dump(generic_file & f) const;
        void add(const std::string & key, const std::string & value) { attr[key] = value; };
        void reset_read() const;
        bool read(std::string & key, std::string & value) const;
        infinint size() const { return attr.size(); };
        void clear() { attr.clear(); alire = attr.begin(); };
        bool find(const std::string &key, std::string & found_value) const;
        bool diff(const ea_attributs & other, const mask & filter) const;
	infinint space_used() const;

	    /// addition operator.

	    /// \param[in] arg ea_attributs to add to self
	    /// \return a ea_attributs object containing all EA of the current object enriched and possibly overwritten
	    /// by those of "arg".
	    /// \note this operator is not reflexive (or symetrical if you prefer) unlike it is in arithmetic. Here instead
	    /// "a + b" is possibly not equal to "b + a"
	ea_attributs operator + (const ea_attributs & arg) const;

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(ea_attributs);
#endif

    private:
	std::map<std::string, std::string> attr;
	std::map<std::string, std::string>::iterator alire;
    };

	/// @}

} // end of namespace

#endif
