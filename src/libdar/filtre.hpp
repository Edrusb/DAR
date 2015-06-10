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

    /// \file filtre.hpp
    /// \brief here is all the core routines for the operations
    /// \ingroup Private

#ifndef FILTRE_HPP
#define FILTRE_HPP

#include "../my_config.h"
#include <vector>
#include "mask.hpp"
#include "pile.hpp"
#include "catalogue.hpp"
#include "path.hpp"
#include "statistics.hpp"
#include "criterium.hpp"
#include "archive_options.hpp"

namespace libdar
{

	/// \ingroup Private
	/// @}

    extern void filtre_restore(user_interaction & dialog, //< for user interaction
			       const mask &filtre,        //< which filename to restore
                               const mask & subtree,      //< which directory and paths to restore
                               catalogue & cat,           //< table of content to extract information from
                               const path & fs_racine,    //< root path under which to restore directiry tree and files
                               bool fs_warn_overwrite,    //< whether to warn before overwriting (to be replaced by overwriting policy)
                               bool info_details,         //< whether to be verbose
                               statistics & st,           //< statistics result about the operation
                               const mask & ea_mask,      //< defines EA to restore/not restore
                               bool flat,                 //< if true, directories are not restores, all files are placed directly at in fs_racine directory
			       inode::comparison_fields what_to_check, //< which file properties to restore
			       bool warn_remove_no_match, //< wether to warn for file to remove not matching the expected type
			       bool empty,                //< dry-run execution
			       bool display_skipped,      //< whether to display skipped files
			       bool empty_dir,            //< whether to restore directories that do contain any file to restore
			       const crit_action & x_overwrite, //< how and whether to overwrite files
 			       archive_options_extract::t_dirty dirty, //< whether to restore dirty files
			       bool only_deleted,         //< whether to only consider deleted files
			       bool not_deleted);         //< <wether to consider deleted files

    extern void filtre_sauvegarde(user_interaction & dialog,
				  const mask &filtre,
                                  const mask &subtree,
                                  pile & stack,
                                  catalogue & cat,
                                  catalogue &ref,
                                  const path & fs_racine,
                                  bool info_details,
                                  statistics & st,
                                  bool make_empty_dir,
				  const mask & ea_mask,
                                  const mask &compr_mask,
                                  const infinint & min_compr_size,
                                  bool nodump,
                                  const infinint & hourshift,
				  bool alter_time,
				  bool furtive_read_mode,
				  bool same_fs,
				  inode::comparison_fields what_to_check,
				  bool snapshot,
				  bool cache_directory_tagging,
				  bool display_skipped,
				  bool security_check,
				  const infinint & repeat_count,
				  const infinint & repeat_byte,
				  const infinint & fixed_date,
				  const infinint & sparse_file_min_size,
				  const std::string & backup_hook_file_execute,
				  const mask & backup_hook_file_mask,
				  bool ignore_unknown);

    extern void filtre_difference(user_interaction & dialog,
				  const mask &filtre,
                                  const mask &subtree,
                                  catalogue & cat,
                                  const path & fs_racine,
                                  bool info_details,
				  statistics & st,
				  const mask & ea_mask,
				  bool alter_time,
				  bool furtive_read_mode,
				  inode::comparison_fields what_to_check,
				  bool display_skipped,
				  const infinint & hourshift,
				  bool compare_symlink_date);

    extern void filtre_test(user_interaction & dialog,
			    const mask &filtre,
                            const mask &subtree,
                            catalogue & cat,
                            bool info_details,
			    bool empty,
                            statistics & st,
			    bool display_skipped);

    extern void filtre_isolate(user_interaction & dialog,
			       catalogue & cat,
                               catalogue & ref,
                               bool info_details);

    extern void filtre_merge(user_interaction & dialog,
			     const mask & filtre,
			     const mask & subtree,
			     pile & stack,
			     catalogue & cat,
			     catalogue * ref1,
			     catalogue * ref2,
			     bool info_details,
			     statistics & st,
			     bool make_empty_dir,
			     const mask & ea_mask,
			     const mask & compr_mask,
			     const infinint & min_compr_size,
			     bool display_skipped,
			     bool keep_compressed,
			     const crit_action & overwrite,
			     bool warn_overwrite,
			     bool decremental_mode,
			     const infinint & sparse_file_min_size);

    void filtre_sequentially_read_all_catalogue(catalogue & cat,
						user_interaction & dialog,
						bool lax_read_mode);

	/// @}

} // end of namespace

#endif
