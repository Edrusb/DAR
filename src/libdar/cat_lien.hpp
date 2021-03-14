/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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

    class cat_lien : public cat_inode
    {
    public :
        cat_lien(const infinint & uid, const infinint & gid, U_16 perm,
		 const datetime & last_access,
		 const datetime & last_modif,
		 const datetime & last_change,
		 const std::string & name,
		 const std::string & target,
		 const infinint & fs_device);
        cat_lien(const std::shared_ptr<user_interaction> & dialog,
		 const smart_pointer<pile_descriptor> & pdesc,
		 const archive_version & reading_ver,
		 saved_status saved,
		 bool small);
       	cat_lien(const cat_lien & ref) = default;
	cat_lien(cat_lien && ref) noexcept = default;
	cat_lien & operator = (const cat_lien & ref) = default;
	cat_lien & operator = (cat_lien && ref) noexcept = default;
	~cat_lien() = default;


	virtual bool operator == (const cat_entree & ref) const override;

        const std::string & get_target() const;
        void set_target(std::string x);

            // using the method is_more_recent_than() from cat_inode
            // using method has_changed_since() from cat_inode class

	    /// inherited from cat_entree
        virtual unsigned char signature() const override { return 'l'; };

	    /// inherited from cat_entree
	virtual std::string get_description() const override { return "symlink"; };

	    /// inherited from cat_entree
        virtual cat_entree *clone() const override { return new (std::nothrow) cat_lien(*this); };

    protected :
        virtual void sub_compare(const cat_inode & other, bool isolated_mode) const override;
        virtual void inherited_dump(const pile_descriptor & pdesc, bool small) const override;

    private :
        std::string points_to;
    };

	/// @}

} // end of namespace

#endif
