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

    /// \file filesystem_tools.hpp
    /// \brief a set of tools used by filesystem_* classes
    /// \ingroup Private

#ifndef FILESYSTEM_TOOLS_HPP
#define FILESYSTEM_TOOLS_HPP

#include "../my_config.h"

extern "C"
{

} // end extern "C"

#include "infinint.hpp"
#include "generic_file.hpp"
#include "fsa_family.hpp"
#include "cat_all_entrees.hpp"
#include "crc.hpp"
#include "user_interaction.hpp"
#include "path.hpp"
#include "fichier_local.hpp"

#include <string>

namespace libdar
{
	/// \addtogroup Private
	/// @{

    bool filesystem_tools_has_immutable(const cat_inode & arg);
    void filesystem_tools_set_immutable(const std::string & target, bool val, user_interaction &ui);
    void filesystem_tools_supprime(user_interaction & ui, const std::string & ref);
    void filesystem_tools_make_owner_perm(const std::shared_ptr<user_interaction> & dialog,
					  const cat_inode & ref,
					  const std::string & chem,
					  bool dir_perm,
					  comparison_fields what_to_check,
					  const fsa_scope & scope);
    void filesystem_tools_make_date(const cat_inode & ref,
				    const std::string & chem,
				    comparison_fields what_to_check,
				    const fsa_scope & scope);

    void filesystem_tools_attach_ea(const std::string &chemin,
				    cat_inode *ino,
				    const mask & ea_mask);
    bool filesystem_tools_is_nodump_flag_set(const std::shared_ptr<user_interaction> & dialog,
					     const path & chem, const std::string & filename,
					     bool info);
    path *filesystem_tools_get_root_with_symlink(const std::shared_ptr<user_interaction> & dialog,
						 const path & root,
						 bool info_details);
    mode_t filesystem_tools_get_file_permission(const std::string & path);

    void filesystem_tools_make_delta_patch(const std::shared_ptr<user_interaction> & dialog,
					   const cat_file & existing,
					   const std::string & existing_pathname,
					   const cat_file & patcher,
					   const path & directory);

	/// create in dirname a brand-new filename which name derives from filename
	///
	/// \return a read-write object the caller has the duty to destroy, exception thrown
	/// if no filename could be created
    fichier_local *filesystem_tools_create_non_existing_file_based_on(const std::shared_ptr<user_interaction> & dialog,
								      std::string filename,
								      path where,
								      std::string & new_filename);

    void filesystem_tools_copy_content_from_to(const std::shared_ptr<user_interaction> & dialog,
					       const std::string & source_path,
					       const std::string & destination_path,
					       const crc *expected_crc);
	/// @}

} // end of namespace

#endif
