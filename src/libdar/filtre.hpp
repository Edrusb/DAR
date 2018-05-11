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
#include "mask.hpp"
#include "pile.hpp"
#include "catalogue.hpp"
#include "path.hpp"
#include "statistics.hpp"
#include "archive_options.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    extern void filtre_restore(const std::shared_ptr<user_interaction> & dialog, ///< for user interaction
			       const mask &filtre,        ///< which filename to restore
                               const mask & subtree,      ///< which directory and paths to restore
                               const catalogue & cat,     ///< table of content to extract information from
                               const path & fs_racine,    ///< root path under which to restore directiry tree and files
                               bool fs_warn_overwrite,    ///< whether to warn before overwriting (to be replaced by overwriting policy)
                               bool info_details,         ///< whether to display processing messages
			       bool display_treated,      ///< whether to display treated files
			       bool display_treated_only_dir, ///< whether to only display current directory of treated file
			       bool display_skipped,      ///< whether to display skipped files
                               statistics & st,           ///< statistics result about the operation
                               const mask & ea_mask,      ///< defines EA to restore/not restore
                               bool flat,                 ///< if true, directories are not restores, all files are placed directly at in fs_racine directory
			       comparison_fields what_to_check, ///< which file properties to restore
			       bool warn_remove_no_match, ///< wether to warn for file to remove not matching the expected type
			       bool empty,                ///< dry-run execution
			       bool empty_dir,            ///< whether to restore directories that do contain any file to restore
			       const crit_action & x_overwrite, ///< how and whether to overwrite files
 			       archive_options_extract::t_dirty dirty, ///< whether to restore dirty files
			       bool only_deleted,         ///< whether to only consider deleted files
			       bool not_deleted,          ///< wether to consider deleted files
			       const fsa_scope & scope    ///< scope of FSA to take into account
	);

    extern void filtre_sauvegarde(const std::shared_ptr<user_interaction> & dialog,
				  const mask &filtre,
                                  const mask &subtree,
                                  const pile_descriptor & pdesc,
                                  catalogue & cat,
                                  const catalogue & ref,
                                  const path & fs_racine,
                                  bool info_details,
				  bool display_treated,
				  bool display_treated_only_dir,
				  bool display_skipped,
				  bool display_finished,
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
				  comparison_fields what_to_check,
				  bool snapshot,
				  bool cache_directory_tagging,
				  bool security_check,
				  const infinint & repeat_count,
				  const infinint & repeat_byte,
				  const infinint & fixed_date,
				  const infinint & sparse_file_min_size,
				  const std::string & backup_hook_file_execute,
				  const mask & backup_hook_file_mask,
				  bool ignore_unknown,
				  const fsa_scope & scope,
				  const std::string & exclude_by_ea,
				  bool delta_signature,     // whether to compute delta sig file on the saved file
				  const infinint & delta_sig_min_size, // size below which to never calculate delta sig
				  const mask & delta_mask,  // mask defining for which file to calculate delta sig
				  bool delta_diff,          // whether to perform delta diff backup when delta sig is present
				  bool auto_zeroing_neg_dates,
				  const std::set<std::string> & ignored_symlinks,
				  modified_data_detection mod_data_detect);

    extern void filtre_difference(const std::shared_ptr<user_interaction> & dialog,
				  const mask &filtre,
                                  const mask &subtree,
                                  const catalogue & cat,
                                  const path & fs_racine,
                                  bool info_details,
				  bool display_treated,
				  bool display_treated_only_dir,
				  bool display_skipped,
				  statistics & st,
				  const mask & ea_mask,
				  bool alter_time,
				  bool furtive_read_mode,
				  comparison_fields what_to_check,
				  const infinint & hourshift,
				  bool compare_symlink_date,
				  const fsa_scope & scope,
				  bool isolated_mode);

    extern void filtre_test(const std::shared_ptr<user_interaction> & dialog,
			    const mask &filtre,
                            const mask &subtree,
                            const catalogue & cat,
                            bool info_details,
			    bool display_treated,
			    bool display_treated_only_dir,
			    bool display_skipped,
			    bool empty,
                            statistics & st);

    extern void filtre_merge(const std::shared_ptr<user_interaction> & dialog,
			     const mask & filtre,
			     const mask & subtree,
			     const pile_descriptor & pdesc,
			     catalogue & cat,
			     const catalogue * ref1,
			     const catalogue * ref2,
			     bool info_details,
			     bool display_treated,
			     bool display_treated_only_dir,
			     bool display_skipped,
			     statistics & st,
			     bool make_empty_dir,
			     const mask & ea_mask,
			     const mask & compr_mask,
			     const infinint & min_compr_size,
			     bool keep_compressed,
			     const crit_action & overwrite,
			     bool warn_overwrite,
			     bool decremental_mode,
			     const infinint & sparse_file_min_size,
			     const fsa_scope & scope,
			     bool delta_signature,
			     bool build_delta_sig,
			     const infinint & delta_sig_min_size,
			     const mask & delta_mask);


	/// initialize variables used for merging in step1 and step2

	/// \note also used for repairing
    extern void filtre_merge_step0(const std::shared_ptr<user_interaction> & dialog,
				   const catalogue * ref1,
				   const catalogue * ref2,
				   statistics & st,
				   bool decremental_mode,
				   crit_action* & decr,
				   const crit_action* & overwrite,
				   bool & abort,
				   thread_cancellation & thr_cancel);

        /// builds a catalogue from two refs with the given policy and filters

    extern void filtre_merge_step1(const std::shared_ptr<user_interaction> & dialog,
				   const mask & filtre,
				   const mask & subtree,
				   catalogue & cat,
				   const catalogue * ref1,
				   const catalogue * ref2,
				   bool info_details,
				   bool display_treated,
				   bool display_skipped,
				   statistics & st,
				   bool make_empty_dir,
				   bool warn_overwrite,
				   bool decremental_mode,
				   crit_action* & decr,
				   const crit_action* & overwrite,
				   bool & abort,
				   thread_cancellation & thr_cancel);


        /// copies data of "cat" catalogue to the pdesc of a brand new archive

	/// \note also used for repairing
    extern void filtre_merge_step2(const std::shared_ptr<user_interaction> & dialog,
				   const pile_descriptor & pdesc,
				   catalogue & cat,
				   bool info_details,
				   bool display_treated,
				   bool display_treated_only_dir,
				   const mask & compr_mask,
				   const infinint & min_compr_size,
				   bool keep_compressed,
				   const infinint & sparse_file_min_size,
				   bool delta_signature,
				   bool build_delta_sig,
				   const infinint & delta_sig_min_size,
				   const mask & delta_mask,
				   bool & abort,
				   thread_cancellation & thr_cancel,
				   bool repair_mode);


    void filtre_sequentially_read_all_catalogue(catalogue & cat,
						const std::shared_ptr<user_interaction> & dialog,
						bool lax_read_mode);



	/// @}

} // end of namespace

#endif
