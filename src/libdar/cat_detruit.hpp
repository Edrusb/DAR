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

    /// \file cat_detruit.hpp
    /// \brief used to record information in a catalogue about a deleted file (differential backup context)
    /// \ingroup Private

#ifndef CAT_DETRUIT_HPP
#define CAT_DETRUIT_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "cat_nomme.hpp"
#include "datetime.hpp"
#include "archive_version.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


	/// the deleted file entry
    class cat_detruit : public cat_nomme
    {
    public :
        cat_detruit(const std::string & name, unsigned char firm, const datetime & date) : cat_nomme(name) , del_date(date) { signe = firm; };
        cat_detruit(const pile_descriptor & pdesc, const archive_version & reading_ver, bool small);
	cat_detruit(const cat_nomme & ref) : cat_nomme(ref.get_name()), del_date(0) { signe = ref.signature(); };

	bool operator == (const cat_entree & ref) const;

        unsigned char get_signature() const { return signe; };
        void set_signature(unsigned char x) { signe = x; };

	const datetime & get_date() const { return del_date; };
	void set_date(const datetime & ref) { del_date = ref; };

	    /// inherited from cat_entree
        unsigned char signature() const { return 'x'; };
	    /// inherited from cat_entree
        cat_entree *clone() const { return new (get_pool()) cat_detruit(*this); };

    protected:
        void inherited_dump(const pile_descriptor & pdesc, bool small) const;

    private :
        unsigned char signe;
	datetime del_date;
    };

	/// @}

} // end of namespace

#endif
