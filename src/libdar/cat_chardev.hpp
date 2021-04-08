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
// to contact the author, see the AUTHOR file
/*********************************************************************/

    /// \file cat_chardev.hpp
    /// \brief class used to record character special devices in a catalogue
    /// \ingroup Private

#ifndef CAT_CHARDEV_HPP
#define CAT_CHARDEV_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "cat_device.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the char device class
    class cat_chardev : public cat_device
    {
    public:
        cat_chardev(const infinint & uid, const infinint & gid, U_16 perm,
		    const datetime & last_access,
		    const datetime & last_modif,
		    const datetime & last_change,
		    const std::string & name,
		    U_16 major,
		    U_16 minor,
		    const infinint & fs_device) : cat_device(uid, gid, perm,
							     last_access,
							     last_modif,
							     last_change,
							     name,
							     major, minor, fs_device) {};
        cat_chardev(const std::shared_ptr<user_interaction> & dialog,
		    const smart_pointer<pile_descriptor> & pdesc,
		    const archive_version & reading_ver,
		    saved_status saved,
		    bool small) : cat_device(dialog, pdesc, reading_ver, saved, small) {};

	cat_chardev(const cat_chardev & ref) = default;
	cat_chardev(cat_chardev && ref) noexcept = default;
	cat_chardev & operator = (const cat_chardev & ref) = default;
	cat_chardev & operator = (cat_chardev && ref) noexcept = default;
	~cat_chardev() = default;


	virtual bool operator == (const cat_entree & ref) const override;

            // using dump from cat_device class
            // using method is_more_recent_than() from cat_device class
            // using method has_changed_since() from cat_device class
        virtual unsigned char signature() const override { return 'c'; };
	virtual std::string get_description() const override { return "char device"; };
        virtual cat_entree *clone() const override { return new (std::nothrow) cat_chardev(*this); };
    };

	/// @}

} // end of namespace

#endif
