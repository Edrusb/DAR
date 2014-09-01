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
#define SAVED_FAKE_BIT 0x80

namespace libdar
{

	/// \addtogroup Private
	/// @{


    extern std::string local_perm(const inode & ref, bool hard);
    extern std::string local_uid(const inode & ref);
    extern std::string local_gid(const inode & ref);
    extern std::string local_size(const inode & ref);
    extern std::string local_storage_size(const inode & ref);
    extern std::string local_date(const inode & ref);
    extern std::string local_flag(const inode & ref, bool isolated, bool dirty_seq);
    extern void xml_listing_attributes(user_interaction & dialog, //< for user interaction
				       const std::string & beginning,  //< character std::string to use as margin
				       const std::string & data,       //< ("saved" | "referenced" | "deleted")
				       const std::string & metadata,   //< ("saved" | "referenced" | "absent")
				       const entree * obj = NULL, //< the object to display inode information about
				       bool list_ea = false);     //< whether to list Extended Attributes
    extern bool extract_base_and_status(unsigned char signature, unsigned char & base, saved_status & saved);
    extern void local_display_ea(user_interaction & dialog, const inode * ino, const std::string &prefix, const std::string &suffix, bool xml_output = false);


	/// @}

} // end of namespace

#endif
