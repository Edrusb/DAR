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

    /// \file cat_device.hpp
    /// \brief parent class for all special devices inodes
    /// \ingroup Private

#ifndef CAT_DEVICE_HPP
#define CAT_DEVICE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "cat_inode.hpp"
#include "integers.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the special cat_device root class
    class cat_device : public cat_inode
    {
    public :
        cat_device(const infinint & uid, const infinint & gid, U_16 perm,
		   const datetime & last_access,
		   const datetime & last_modif,
		   const datetime &last_change,
		   const std::string & name,
		   U_16 major,
		   U_16 minor,
		   const infinint & fs_device);
        cat_device(user_interaction & dialog,
		   const pile_descriptor & pdesc,
		   const archive_version & reading_ver,
		   saved_status saved,
		   bool small);

	bool operator == (const cat_entree & ref) const;

        int get_major() const { if(get_saved_status() != s_saved) throw SRC_BUG; else return xmajor; };
        int get_minor() const { if(get_saved_status() != s_saved) throw SRC_BUG; else return xminor; };
        void set_major(int x) { xmajor = x; };
        void set_minor(int x) { xminor = x; };

            // using method is_more_recent_than() from cat_inode class
            // using method has_changed_since() from cat_inode class
            // signature is left pure abstract

    protected :
        void sub_compare(const cat_inode & other, bool isolated_mode) const;
        void inherited_dump(const pile_descriptor & pdesc, bool small) const;

    private :
        U_16 xmajor, xminor;
    };

	/// @}

} // end of namespace

#endif
