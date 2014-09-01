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
    class ignored : public nomme
    {
    public :
        ignored(const std::string & name) : nomme(name) {};
        ignored(generic_file & f) : nomme(f) { throw SRC_BUG; };

        unsigned char signature() const { return 'i'; };
        cat_entree *clone() const { return new (get_pool()) ignored(*this); };

    protected:
        void inherited_dump(generic_file & f, bool small) const { throw SRC_BUG; };

    };

	/// @}

} // end of namespace

#endif
