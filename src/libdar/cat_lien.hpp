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

    /// \file cat_lien.hpp
    /// \brief class used to store symbolic links in a catalogue
    /// \ingroup Private

#ifndef CAT_LIEN_HPP
#define CAT_LIEN_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "cat_inode.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the symbolic link inode class
    class lien : public inode
    {
    public :
        lien(const infinint & uid, const infinint & gid, U_16 perm,
             const datetime & last_access,
             const datetime & last_modif,
	     const datetime & last_change,
             const std::string & name,
	     const std::string & target,
	     const infinint & fs_device);
        lien(user_interaction & dialog,
	     generic_file & f,
	     const archive_version & reading_ver,
	     saved_status saved,
	     compressor *efsa_loc,
	     escape *ptr);

        const std::string & get_target() const;
        void set_target(std::string x);

            // using the method is_more_recent_than() from inode
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('l', get_saved_status()); };
        entree *clone() const { return new (get_pool()) lien(*this); };

    protected :
        void sub_compare(const inode & other, bool isolated_mode) const;
        void inherited_dump(generic_file & f, bool small) const;

    private :
        std::string points_to;
    };

	/// @}

} // end of namespace

#endif
