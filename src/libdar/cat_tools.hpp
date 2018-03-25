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

    /// \file cat_tools.hpp
    /// \brief set of routines used by  catalogue related classes
    /// \ingroup Private

#ifndef CAT_TOOLS_HPP
#define CAT_TOOLS_HPP

#include "../my_config.h"

#include <string>

#include "cat_inode.hpp"


extern "C"
{
} // end extern "C"


#define REMOVE_TAG gettext("[--- REMOVED ENTRY ----]")

namespace libdar
{

	/// \addtogroup Private
	/// @{


    extern std::string local_perm(const cat_inode & ref, bool hard); ///< voir list_entry::get_perm()
    extern std::string local_uid(const cat_inode & ref);             ///< voir list_entry::get_uid(true);
    extern std::string local_gid(const cat_inode & ref);             ///< voir list_entry::get_gid(true);
    extern std::string local_size(const cat_inode & ref, bool sizes_in_bytes); ///< voir list_entry::get_file_size(size_in_bytes)
    extern std::string local_storage_size(const cat_inode & ref);    ///< voir list_entry::get_storage_size_for_data()
    extern std::string local_date(const cat_inode & ref);            ///< voir list_entry::get_last_modif()
    extern std::string local_flag(const cat_inode & ref, bool isolated, bool dirty_seq);  ///< list_entry::get_data_flag()+get_delta_flag()+get_ea_flag()+get_fsa_flag()+get_compression_ratio_flag()+get_sparse_flag()
    extern void xml_listing_attributes(user_interaction & dialog, //< for user interaction
				       const std::string & beginning,  //< character std::string to use as margin
				       const std::string & data,       //< ("saved" | "referenced" | "deleted")
				       const std::string & metadata,   //< ("saved" | "referenced" | "absent")
				       const cat_entree * obj = nullptr, //< the object to display cat_inode information about
				       bool list_ea = false);     //< whether to list Extended Attributes
    extern void local_display_ea(user_interaction & dialog,
				 const cat_inode * ino,
				 const std::string &prefix,
				 const std::string &suffix,
				 bool xml_output = false);

	/// returns a string describing the object type
	///
	/// \note argument must point to a valid object
    extern std::string entree_to_string(const cat_entree *obj);   ///< voir list_entry::get_type_description

	/// @}

} // end of namespace

#endif
