/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
	{ set_saved_status(saved_status::saved); };

	cat_prise(const std::shared_ptr<user_interaction> & dialog,
		  const smart_pointer<pile_descriptor> & pdesc,
		  const archive_version & reading_ver,
		  saved_status saved,
		  bool small): cat_inode(dialog, pdesc, reading_ver, saved, small) {};

	cat_prise(const cat_prise & ref) = default;
	cat_prise(cat_prise && ref) noexcept = default;
	cat_prise & operator = (const cat_prise & ref) = default;
	cat_prise & operator = (cat_prise && ref) = default;
	~cat_prise() = default;


	virtual bool operator == (const cat_entree & ref) const override;

            // using dump from cat_inode class
            // using method is_more_recent_than() from cat_inode class
            // using method has_changed_since() from cat_inode class

	    /// inherited from cat_entree
        virtual unsigned char signature() const override { return 's'; };
	virtual std::string get_description() const override { return "socket"; };


	    /// inherited from cat_entree
        virtual cat_entree *clone() const override { return new (std::nothrow) cat_prise(*this); };
    };

	/// @}

} // end of namespace

#endif
