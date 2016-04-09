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

    /// \file cat_prise.hpp
    /// \brief class to record filesystem (UNIX) sockets in a catalogue
    /// \ingroup Private

#ifndef CAT_PRISE_HPP
#define CAT_PRISE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "cat_inode.hpp"
#include "cat_tools.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


	/// the Unix socket inode class
    class cat_prise : public cat_inode
    {
    public :
        cat_prise(const infinint & xuid, const infinint & xgid, U_16 xperm,
		  const datetime & last_access,
		  const datetime & last_modif,
		  const datetime & last_change,
		  const std::string & xname,
		  const infinint & fs_device):
	    cat_inode(xuid, xgid, xperm, last_access, last_modif, last_change, xname, fs_device)
	{ set_saved_status(s_saved); };

	cat_prise(user_interaction & dialog,
		  const smart_pointer<pile_descriptor> & pdesc,
		  const archive_version & reading_ver,
		  saved_status saved,
		  bool small): cat_inode(dialog, pdesc, reading_ver, saved, small) {};

	bool operator == (const cat_entree & ref) const;

            // using dump from cat_inode class
            // using method is_more_recent_than() from cat_inode class
            // using method has_changed_since() from cat_inode class

	    /// inherited from cat_entree
        unsigned char signature() const { return mk_signature('s', get_saved_status()); };

	    /// inherited from cat_entree
        cat_entree *clone() const { return new (get_pool()) cat_prise(*this); };
    };

	/// @}

} // end of namespace

#endif
