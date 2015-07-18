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

    /// \file cat_nomme.hpp
    /// \brief base class of all objects contained in a catalogue and that can be named
    /// \ingroup Private

#ifndef CAT_NOMME_HPP
#define CAT_NOMME_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include <string>
#include "cat_entree.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// the base class for all entry that have a name
    class cat_nomme : public cat_entree
    {
    public:
        cat_nomme(const std::string & name) { xname = name; };
        cat_nomme(const pile_descriptor & pdesc, bool small);
	bool operator == (const cat_entree & ref) const;
	virtual bool operator < (const cat_nomme & ref) const { return xname < ref.xname; };
	virtual ~cat_nomme() throw(Ebug) {};

        const std::string & get_name() const { return xname; };
        void change_name(const std::string & x) { xname = x; };

	    /// compares two objects
	    ///
            /// \note no need to have a virtual method, as signature will differ in inherited classes (argument type changes)
        bool same_as(const cat_nomme & ref) const { return xname == ref.xname; };

            // signature() is kept as an abstract method
            // clone() is also ketp abstract

    protected:
        void inherited_dump(const pile_descriptor & pdesc, bool small) const;

    private:
        std::string xname;

    };


	/// @}

} // end of namespace

#endif
