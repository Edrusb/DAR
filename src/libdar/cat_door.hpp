/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

    /// \file cat_door.hpp
    /// \brief class used in a catalogue to store solaris door filesystem entries
    /// \ingroup Private

#ifndef CAT_DOOR_HPP
#define CAT_DOOR_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "cat_file.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


	/// the class for Door IPC (mainly for Solaris)
    class cat_door : public cat_file
    {
    public:
        cat_door(const infinint & xuid,
		 const infinint & xgid,
		 U_16 xperm,
		 const datetime & last_access,
		 const datetime & last_modif,
		 const datetime & last_change,
		 const std::string & src,
		 const path & che,
		 const infinint & fs_device) : cat_file(xuid, xgid, xperm, last_access, last_modif,
							last_change, src, che, 0, fs_device, false) {};
        cat_door(const std::shared_ptr<user_interaction> & dialog,
		 const smart_pointer<pile_descriptor> & pdesc,
		 const archive_version & reading_ver,
		 saved_status saved,
		 compression default_algo,
		 bool small) : cat_file(dialog, pdesc, reading_ver, saved, default_algo, small) {};

	cat_door(const cat_door & ref) = default;
	cat_door(cat_door && ref) = delete;
	cat_door & operator = (const cat_door & ref) = delete;
	cat_door & operator = (cat_door && ref) = delete;
	~cat_door() = default;

	virtual bool operator == (const cat_entree & ref) const override;

        virtual unsigned char signature() const override { return 'o'; };
	virtual std::string get_description() const override { return "door"; };


	    // inherited from class cat_file
        virtual generic_file *get_data(get_data_mode mode,
				       std::shared_ptr<memory_file> delta_sig,
				       U_I signature_block_size,
				       std::shared_ptr<memory_file> delta_ref,
				       const crc**checksum) const override;

    };

	/// @}

} // end of namespace

#endif

