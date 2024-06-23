/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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

    /// \file cat_ignored.hpp
    /// \brief class used to remember that an entry has been ignored and shall not be recorded as deleted using a detruit object in a catalogue
    /// \ingroup Private

#ifndef CAT_IGNORED_HPP
#define CAT_IGNORED_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "cat_nomme.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the present file to ignore (not to be recorded as deleted later)

    class cat_ignored : public cat_nomme
    {
    public :
        cat_ignored(const std::string & name): cat_nomme(name, saved_status::saved) {};
        cat_ignored(const smart_pointer<pile_descriptor> & pdesc, bool small) : cat_nomme(pdesc, small, saved_status::saved) { throw SRC_BUG; };
	cat_ignored(const cat_ignored & ref) = default;
	cat_ignored(cat_ignored && ref) noexcept = default;
	cat_ignored & operator = (const cat_ignored & ref) = default;
	cat_ignored & operator = (cat_ignored && ref) = default;
	~cat_ignored() = default;

	virtual bool operator == (const cat_entree & ref) const override;

        virtual unsigned char signature() const override { return 'i'; };
	virtual std::string get_description() const override { return "ignored entry"; };
        virtual cat_entree *clone() const override { return new (std::nothrow) cat_ignored(*this); };

    protected:
        virtual void inherited_dump(const pile_descriptor & pdesc, bool small) const override { throw SRC_BUG; };

    };

	/// @}

} // end of namespace

#endif
