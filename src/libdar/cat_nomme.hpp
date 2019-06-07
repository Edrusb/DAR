/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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
        cat_nomme(const std::string & name, saved_status arg): cat_entree(arg) { xname = name; };
        cat_nomme(const smart_pointer<pile_descriptor> & pdesc, bool small, saved_status val);
	cat_nomme(const cat_nomme & ref) = default;
	cat_nomme(cat_nomme && ref) noexcept = default;
	cat_nomme & operator = (const cat_nomme & ref) = default;
	cat_nomme & operator = (cat_nomme && ref) noexcept = default;
	virtual ~cat_nomme() = default;

	virtual bool operator == (const cat_entree & ref) const override;
	virtual bool operator < (const cat_nomme & ref) const { return xname < ref.xname; };
        const std::string & get_name() const { return xname; };
        void change_name(const std::string & x) { xname = x; };

	    /// compares two objects
	    ///
            /// \note no need to have a virtual method, as signature will differ in inherited classes (argument type changes)
        bool same_as(const cat_nomme & ref) const { return cat_entree::same_as(ref) && xname == ref.xname; };

            // signature() is kept as an abstract method
            // clone() is also ketp abstract

    protected:
        virtual void inherited_dump(const pile_descriptor & pdesc, bool small) const override;

    private:
        std::string xname;

    };

	/// @}

} // end of namespace

#endif
