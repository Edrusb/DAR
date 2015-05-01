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

    /// \file cat_eod.hpp
    /// \brief object exchanged with a catalogue (never stored in it) to signal the end of a directory
    /// \ingroup Private

#ifndef CAT_EOD_HPP
#define CAT_EOD_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "cat_entree.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


	/// the End of Directory entry class
    class cat_eod : public cat_entree
    {
    public :
        cat_eod() {};
        cat_eod(const pile_descriptor & pdesc, bool small): cat_entree(pdesc, small) {};
            // dump defined by cat_entree
	virtual bool operator == (const cat_entree & ref) const { return true; };
        unsigned char signature() const { return 'z'; };
        cat_entree *clone() const { return new (get_pool()) cat_eod(); };

    };

	/// @}

} // end of namespace

#endif
