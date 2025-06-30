/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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

#include "../my_config.h"

extern "C"
{
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
} // end extern "C"

#include <map>
#include "filtre.hpp"
#include "user_interaction.hpp"
#include "erreurs_ext.hpp"
#include "filesystem_backup.hpp"
#include "filesystem_restore.hpp"
#include "filesystem_diff.hpp"
#include "ea.hpp"
#include "defile.hpp"
#include "null_file.hpp"
#include "thread_cancellation.hpp"
#include "compressor.hpp"
#include "sparse_file.hpp"
#include "semaphore.hpp"
#include "deci.hpp"
#include "cat_all_entrees.hpp"
#include "null_file.hpp"
#include "cat_signature.hpp"
#include "tools.hpp"
#include "op_tools.hpp"
#include "fichier_global.hpp"
#include "capabilities.hpp"

using namespace std;

#define SKIPPED "Skipping file: "
#define SQUEEZED "Ignoring empty directory: "

namespace libdar
{

	// returns false if file has changed during backup (inode is saved however, but the saved data may be invalid)
	// return true if file has not change, false if file need not resaving or does not add wasted bytes in archive
	// throw exceptions in case of error
    static bool save_inode(const shared_ptr<user_interaction> & dialog,//< how to report to user
			   const string &info_quoi,  ///< full path name of the file to save (including its name)
			   cat_entree * & e,         ///< cat_entree to save into the archive
			   const cat_entree * ref,   ///< reference object if it exists (to grab CRC and delta signature from if present and necessary)
			   const pile_descriptor & pdesc,//< where to write to
			   bool info_details,        ///< verbose output to user
			   bool display_treated,     ///< add an information line before treating a file
			   bool alter_atime,         ///< whether to set back atime of filesystem
			   bool check_change,        ///< whether to check file change during backup
			   bool compute_crc,         ///< whether to recompute the CRC
			   cat_file::get_data_mode keep_mode, ///< whether to copy compressed data (same compression algo), uncompress but keep hole structure (change compression algo) or uncompress and fill data holes (redetect holes in file)
			   const catalogue & cat,    ///< catalogue to update for escape sequence mark
			   const infinint & repeat_count, ///< how much time to retry saving the file if it has changed during the backup
			   const infinint & repeat_byte, ///< how much byte remains to waste for saving again a changing file
			   const infinint & hole_size,   ///< the threshold for hole detection, set to zero completely disable the sparse file detection mechanism
			   semaphore * sem,
			   bool delta_signature,  ///< if set, lead to delta signature to be computed, if false and delta signature already exists it is saved (which in case of merging leads to transfering it in the resulting archive)
			   bool make_delta_diff,         ///< whether delta diff is allowed
			   infinint & new_wasted_bytes,  ///< new amount of wasted bytes to return to the caller.
			   set<string> ignored_as_symlink, ///< list of file to ignore as symlink and fetch the proper mtime
			   bool repair_mode,         ///< if set, try to fix CRC and size problem flagging such fixed files as dirty
			   U_I signature_block_size, ///< block size of delta signatures
			   bool never_resave_uncompressed);

    static bool save_ea(const shared_ptr<user_interaction> & dialog,
			const string & info_quoi,
			cat_inode * & ino,
			const pile_descriptor & pdesc,
			bool display_treated,
			bool repair_mode);

    static void restore_atime(const string & chemin, const cat_inode * & ptr);

    static bool save_fsa(const shared_ptr<user_interaction> & dialog,
			 const string & info_quoi,
			 cat_inode * & ino,
			 const pile_descriptor & pdesc,
			 bool display_treated,
			 bool repair_mode);

	/// merge two sets of EA

	/// \param[in] ref1 is the first EA set
	/// \param[in] ref2 is the second EA set
	/// \param[in,out] res is the EA set result of the merging operation
	/// \note result is the set of EA of ref1 to which those of ref2 are added if not already present in ref1
    static void merge_ea(const ea_attributs & ref1, const ea_attributs & ref2, ea_attributs  &res);

	/// to clone an "cat_entree" taking hard links into account

	/// \param[in] ref is the named entry to be cloned
	/// \param[in,out] hard_link_base is the datastructure that gather/maps hard_links information
	/// \param[in] etiquette_offset is the offset to apply to etiquette (to not mix several hard-link sets using the same etiquette number in different archives)
	/// \return a pointer to the new allocated clone object (to be deleted by the delete operator by the caller)
    static cat_entree *make_clone(const cat_nomme *ref,
				  map<infinint, cat_etoile*> & hard_link_base,
				  const infinint & etiquette_offset);

	/// remove an entry hardlink from a given hard_link database

	/// \param[in] mir is a pointer to the cat_mirage object to delete
	/// \param[in,out] hard_link_base is the datastructure used to gather/map hard_links information
	/// \note if the cat_mirage object is the last one pointing to a given "cat_etoile" object
	/// deleting this cat_mirage will delete the "cat_etoile" object automatically (see destructor implementation of these classes).
	/// However, one need to remove from the database the reference to this "cat_etoile *" that is about to me removed by the caller
    static void clean_hard_link_base_from(const cat_mirage *mir, map<infinint, cat_etoile *> & hard_link_base);


	/// modify the hard_link_base to avoid hole in the numbering of etiquettes (map size == highest etiquette number)
    static void normalize_link_base(map<infinint, cat_etoile *> & hard_link_base);

	/// transfer EA from one cat_inode to another as defined by the given action

	/// \param[in] dialog for user interaction
	/// \param[in] action is the action to perform with EA and FSA
	/// \param[in,out] in_place is the "in place" cat_inode, the resulting EA/FSA operation is placed as EA/FSA of this object, argument may be nullptr
	/// \param[in] to_add is the "to be added" cat_inode
	/// \note actions set to EA_preserve EA_preserve_mark_already_saved and EA_clear are left intentionnaly unimplemented!
	/// \note the nullptr given as argument means that the object is not an cat_inode

    static void do_EFSA_transfert(const shared_ptr<user_interaction> &dialog,
				  over_action_ea action,
				  cat_inode *in_place,
				  const cat_inode *to_add);


	/// overwriting policy when only restoring detruit objects
    static const crit_action *make_overwriting_for_only_deleted();

	/// write down delta signature for unsaved files taking it from entry of reference or computing from filesystem
    static void save_delta_signature(const shared_ptr<user_interaction> & dialog,
				     const string & info_quoi,
				     cat_file * e_file,
				     const cat_file * ref_file,
				     const pile_descriptor & pdesc,
				     U_I signature_block_size,
				     bool info_details,
				     bool display_treated,
				     const catalogue & cat);

    static bool furtive_check(bool furtive,
			      const shared_ptr<user_interaction> & dialog,
			      bool verbose);

	/// try to modify the to_be_added for it provides the patched data based on in_place instead of providing (get_data()) the patch data itself

	/// \param[in] in_place inode to base the patch on (or nullptr or anything else)
	/// \param|in] to_be_added inode with binary patch to apply (or nullptr or anything else)
	/// \return true upon success, get_data() will then provide the batched version of the file, return false else (not a cat_file, no patch data...)
    static bool merge_applying_patch_possible(const cat_entree* in_place,
					      cat_entree* to_be_added);

    void filtre_restore(const shared_ptr<user_interaction> & dialog,
			const mask & filtre,
			const mask & subtree,
			const catalogue & cat,
			const path & fs_racine,
			bool fs_warn_overwrite,
			bool info_details,
			bool display_treated,
			bool display_treated_only_dir,
			bool display_skipped,
			statistics & st,
			const mask & ea_mask,
			bool flat,
			comparison_fields what_to_check,
			bool warn_remove_no_match,
			bool empty,
			bool empty_dir,
			const crit_action & x_overwrite,
			archive_options_extract::t_dirty dirty,
			bool only_deleted,
			bool not_deleted,
			const fsa_scope & scope,
			bool ignore_unix_sockets)
    {
	defile juillet = fs_racine; // 'juillet' is in reference to 14th of July ;-) when takes place the "defile'" on the Champs-Elysees.
	const cat_eod tmp_eod;
	const cat_entree *e;
	thread_cancellation thr_cancel;
	const crit_action * when_only_deleted = only_deleted ? make_overwriting_for_only_deleted() : nullptr;
	const crit_action & overwrite = only_deleted ? *when_only_deleted : x_overwrite;

	if(!dialog)
	    throw SRC_BUG; // dialog points to nothing

	if(display_treated_only_dir && display_treated)
	    display_treated = false;
	    // avoid having filesystem to report action performed for each entry
	    // specific code in this function will show instead the current directory
	    // under which file are processed
	else
	    display_treated_only_dir = false; // avoid incoherence

	try
	{
	    filesystem_restore fs(dialog,
				  fs_racine,
				  fs_warn_overwrite,
				  display_treated,
				  ea_mask,
				  what_to_check,
				  warn_remove_no_match,
				  empty,
				  &overwrite,
				  only_deleted,
				  scope);
		// if only_deleted, we set the filesystem to only overwrite mode (no creatation if not existing)
		// we also filter to only restore directories and detruit objects.

	    st.clear();
	    cat.reset_read();

	    if(!empty_dir)
		cat.launch_recursive_has_changed_update();

	    while(cat.read(e))
	    {
		const cat_nomme *e_nom = dynamic_cast<const cat_nomme *>(e);
		const cat_directory *e_dir = dynamic_cast<const cat_directory *>(e);
		const cat_mirage *e_mir = dynamic_cast<const cat_mirage *>(e);
		const cat_inode *e_ino = dynamic_cast<const cat_inode *>(e);
		const cat_file *e_file = dynamic_cast<const cat_file *>(e);
		const cat_detruit *e_det = dynamic_cast<const cat_detruit *>(e);
		const cat_prise *e_pri = dynamic_cast<const cat_prise *>(e);

		if(e_mir != nullptr)
		{
		    e_ino = e_mir->get_inode();
		    if(e_ino == nullptr)
			throw SRC_BUG; // !?! how is this possible ?
		    e_mir->get_inode()->change_name(e_mir->get_name()); // temporarily changing the inode name to the one of the cat_mirage
		    e_file = dynamic_cast<const cat_file *>(e_ino);
		}

		if(e_det != nullptr && not_deleted)
		    continue; // skip this cat_detruit object

		juillet.enfile(e);
		thr_cancel.check_self_cancellation();
		if(display_treated_only_dir)
		{
		    if(e_dir != nullptr)
			dialog->message(string(gettext("Inspecting directory ")) + juillet.get_string());
		}

		if(e_nom != nullptr)
		{
		    try
		    {
			bool path_covered = subtree.is_covered(juillet.get_path());   // is current entry covered by filters (path)
			bool name_covered = e_dir != nullptr || filtre.is_covered(e_nom->get_name()); //  is current entry's filename covered (not applied to directories)
			bool dirty_covered = e_file == nullptr || !e_file->is_dirty() || dirty != archive_options_extract::dirty_ignore;  // checking against dirty files
			bool empty_dir_covered = e_dir == nullptr || empty_dir || e_dir->get_recursive_has_changed(); // checking whether this is not a directory without any file to restore in it
			bool flat_covered = e_dir == nullptr || !flat; // we do not restore directories in flat mode
			bool only_deleted_covered = !only_deleted || e_dir != nullptr || e_det != nullptr; // we do not restore other thing than directories and cat_detruits when in "only_deleted" mode
			bool socket_covered = ! ignore_unix_sockets || e_pri == nullptr;

			if(path_covered
			   && name_covered
			   && dirty_covered
			   && empty_dir_covered
			   && flat_covered
			   && only_deleted_covered
			   && socket_covered)
			{
			    filesystem_restore::action_done_for_data data_restored = filesystem_restore::done_no_change_no_data; // will be true if file's data have been restored (depending on overwriting policy)
			    bool ea_restored = false; // will be true if file's EA have been restored (depending on overwriting policy)
			    bool hard_link = false; // will be true if data_restored is true and only lead to a hard link creation or file replacement by a hard link to an already existing inode
			    bool fsa_restored = false; // will be true if file's FSA have been restored (depending on overwriting policy)
			    bool first_time = true; // if quite dirty file (saved several time), need to record in sequential reading whether this is the first time we restore it (not used in direct access reading).
			    bool created_retry; // when restoring several dirty copies (in sequential read), we not forget whether the file
				// has been created or not the first time (which is kept in "created"), for each try, we thus use created_retry
				// to carry the info whether the file has been created at each try

			    struct cached_Erange
			    {
				bool active;
				string source;
				string message;
				cached_Erange() { active = false; };
			    } tmp_exc;    // used to hold exception information, when restoring in sequential reading in the hope another copy is available for the file

			    if(dirty == archive_options_extract::dirty_warn && e_file != nullptr && e_file->is_dirty())
			    {
				string tmp = juillet.get_string();
				dialog->pause(tools_printf(gettext("File %S has changed during backup and is probably not saved in a valid state (\"dirty file\"), do you want to consider it for restoration anyway?"), &tmp));
			    }

			    do
			    {
				if(!first_time) // a second time only occures in sequential read mode
				{
				    const cat_file *e_file = dynamic_cast<const cat_file *>(e_ino);

				    if(info_details)
					dialog->message(string(gettext("File had changed during backup and had been copied another time, restoring the next copy of file: ")) + juillet.get_string());

					// we must let the filesystem object forget that
					// this hard linked inode has already been seen
				    if(e_mir != nullptr)
					fs.clear_corres_if_pointing_to(e_mir->get_etiquette(), juillet.get_string());

				    if(e_file != nullptr)
				    {
					cat_file *me_file = const_cast<cat_file *>(e_file);

					if(me_file == nullptr)
					    throw SRC_BUG;

					me_file->drop_crc();
					me_file->set_storage_size(0);
					if(cat.get_escape_layer() == nullptr)
					    throw SRC_BUG;
					me_file->set_offset(cat.get_escape_layer()->get_position());
				    }

				    fs.ignore_overwrite_restrictions_for_next_write();
				}

				try
				{
				    fs.write(e, data_restored, ea_restored, created_retry, hard_link, fsa_restored); // e and not e_ino, it may be a hard link
				    if(tmp_exc.active)
				    { // restoration succeeded so we drop any pending exception
					tmp_exc.active = false;
				    }
				}
				catch(Erange & e)
				{
					// we do not throw the exception right now, but
					// we let a chance to read a new copy (retry upon change)
					// of that file. Only if there is no more copy available
					// we will throw this exception (which we cannot not
					// at this time if following sequential reading mode.
				    tmp_exc.active = true;
				    tmp_exc.source = e.get_source();
				    tmp_exc.message = e.get_message();
				}

				if(first_time)
				    first_time = false;
				    // Now checking whether there is not a second copy
				    // of the file to use instead, due to file change
				    // while being read for backup. The last copy is the one
				    // to use for restoration.
			    }
			    while(cat.get_escape_layer() != nullptr
				  && cat.get_escape_layer()->skip_to_next_mark(escape::seqt_changed, false)
				  && data_restored == filesystem_restore::done_data_restored);

			    if(tmp_exc.active)
				throw Erange(tmp_exc.source, tmp_exc.message);

			    if(cat.get_escape_layer() != nullptr && cat.get_escape_layer()->skip_to_next_mark(escape::seqt_dirty, false))
			    {
				string tmp = juillet.get_string();
				    // file is dirty and we are reading archive sequentially, thus this is only now once we have restored the file that
				    // we can inform the user about the dirtyness of the file.

				if(e_nom == nullptr)
				    throw SRC_BUG;
				cat_detruit killer = *e_nom;
				bool  tmp_ea_restored, tmp_created_retry, tmp_hard_link, tmp_fsa_restored;
				filesystem_restore::action_done_for_data tmp_data_restored;

				switch(dirty)
				{
				case archive_options_extract::dirty_warn:
				    dialog->pause(tools_printf(gettext("The just restored file %S has been marked as dirty (sequential reading can only detect the dirty status after restoration), do we remove this just restored dirty file?"), &tmp));
					// NO BREAK HERE !!! This is intended.
				case archive_options_extract::dirty_ignore:
					// we must remove the file
				    if(info_details)
				    {
					if(dirty == archive_options_extract::dirty_ignore)
					    dialog->message(tools_printf(gettext("The just restored file %S has been marked as dirty (sequential reading can only detect the dirty status after restoration), removing the just restored dirty file as it is asked to ignore this type of file"), &tmp));
					else
					    dialog->message(tools_printf(gettext("Removing the dirty file %S"), &tmp));
				    }
				    fs.ignore_overwrite_restrictions_for_next_write();
				    fs.write(&killer, tmp_data_restored, tmp_ea_restored, tmp_created_retry, tmp_hard_link, tmp_fsa_restored);
				    break;
				case archive_options_extract::dirty_ok:
				    break;
				default:
				    throw SRC_BUG;
				}
			    }

				// Now updating the statistics counters

			    if(hard_link)
				st.incr_hard_links();

			    switch(data_restored)
			    {
			    case filesystem_restore::done_data_restored:
				if(e_dir == nullptr || !cat.read_second_time_dir())
				    st.incr_treated();
				break;
			    case filesystem_restore::done_no_change_no_data:
				st.incr_skipped();
				break;
			    case filesystem_restore::done_no_change_policy:
				st.incr_tooold();
				break;
			    case filesystem_restore::done_data_removed:
				st.incr_deleted();
				break;
			    default:
				throw SRC_BUG;
			    }

			    if(ea_restored)
				st.incr_ea_treated();

			    if(fsa_restored)
				st.incr_fsa_treated();
			}
			else // object not covered by filters
			{
			    if(display_skipped)
			    {
				if(!path_covered || !name_covered || !dirty_covered || !socket_covered)
				    dialog->message(string(gettext(SKIPPED)) + juillet.get_string());
				else
				    dialog->message(string(gettext(SQUEEZED)) + juillet.get_string());
			    }

			    if(e_dir == nullptr || !cat.read_second_time_dir())
				st.incr_ignored();
			    if(e_dir != nullptr)
			    {
				if(!path_covered || !empty_dir_covered)
				{
					// this directory has been excluded by path_covered
					// or empty_dir_covered. We must not recurse in it
					// (this is not a flat restoration for example)
				    cat.skip_read_to_parent_dir();
				    juillet.enfile(&tmp_eod);
				}
			    }
			}
		    }
		    catch(Ebug & e)
		    {
			throw;
		    }
		    catch(Euser_abort & e)
		    {
			dialog->message(juillet.get_string() + gettext(" not restored (user choice)"));

			if(e_dir != nullptr && !flat)
			{
			    dialog->message(gettext("No file in this directory will be restored."));
			    cat.skip_read_to_parent_dir();
			    juillet.enfile(&tmp_eod);
			}
			if(e_dir == nullptr || !cat.read_second_time_dir())
			    st.incr_ignored();
		    }
		    catch(Ethread_cancel & e)
		    {
			throw;
		    }
		    catch(Escript & e)
		    {
			throw;
		    }
		    catch(Egeneric & e)
		    {
			if(!only_deleted || e_dir == nullptr)
			    dialog->message(string(gettext("Error while restoring ")) + juillet.get_string() + " : " + e.get_message());

			if(e_dir != nullptr && !flat)
			{
			    if(!only_deleted)
				dialog->message(string(gettext("Warning! No file in that directory will be restored: ")) + juillet.get_string());
			    cat.skip_read_to_parent_dir();
			    juillet.enfile(&tmp_eod);
			}
			if(e_dir == nullptr || !cat.read_second_time_dir())
			    if(!only_deleted)
				st.incr_errored();
		    }

			// avoiding keeping in memory the delta signature
			// is is loaded in any case when in sequential read mode
			// it may be loaded when needed in direct access mode
			// it is not stored in a catalogue, thus dropping it now
			// still allows on-fly isolation later on with that catalogue
		    if(e_file != nullptr)
			e_file->drop_delta_signature_data();
		}
		else // e_nom == nullptr : this should be a CAT_EOD
		{
		    const cat_eod *e_eod = dynamic_cast<const cat_eod *>(e);

		    if(e_eod == nullptr)
			throw SRC_BUG; // not an class cat_eod object, nor a class cat_nomme object ???

		    if(!flat)
		    {
			bool notusedhere;
			filesystem_restore::action_done_for_data tmp;
			fs.write(e, tmp, notusedhere, notusedhere, notusedhere, notusedhere); // cat_eod; don't care returned value
		    }
		}
	    }
	}
	catch(...)
	{
	    if(when_only_deleted != nullptr)
		delete when_only_deleted;
	    throw;
	}
	if(when_only_deleted != nullptr)
	    delete when_only_deleted;
    }

    void filtre_sauvegarde(const shared_ptr<user_interaction> & dialog,
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
			   bool alter_atime,
			   bool furtive_read_mode,
			   const filesystem_ids & same_fs,
			   comparison_fields what_to_check,
			   bool snapshot,
			   bool cache_directory_tagging,
			   bool security_check,
			   const infinint & repeat_count,
			   const infinint & repeat_byte,
			   const infinint & fixed_date,
			   const infinint & sparse_file_min_size,
			   const string & backup_hook_file_execute,
			   const mask & backup_hook_file_mask,
			   bool ignore_unknown,
			   const fsa_scope & scope,
			   const string & exclude_by_ea,
			   bool delta_signature,
			   const infinint & delta_sig_min_size,
			   const mask & delta_mask,
			   bool delta_diff,
			   bool auto_zeroing_neg_dates,
			   const set<string> & ignored_symlinks,
			   modified_data_detection mod_data_detect,
			   const delta_sig_block_size & delta_sig_block_len,
			   bool never_resave_uncompressed)
    {
	if(!dialog)
	    throw SRC_BUG; // dialog points to nothing

	cat_entree *e = nullptr;
        const cat_entree *f = nullptr;
        defile juillet = fs_racine;
        const cat_eod tmp_eod;
        compression stock_algo;
	semaphore sem = semaphore(dialog, backup_hook_file_execute, backup_hook_file_mask);

	if(display_treated_only_dir && display_treated)
	    display_treated = false;
	    // avoid having save_inode/save_ea to report action performed for each entry
	    // specific code in this function will show instead the current directory
	    // under which file are processed
	else
	    display_treated_only_dir = false; // avoid incoherence


	stock_algo = pdesc.compr->get_algo();
	infinint root_fs_device;
        filesystem_backup fs(dialog,
			     fs_racine,
			     info_details,
			     ea_mask,
			     nodump,
			     alter_atime,
			     furtive_check(furtive_read_mode,
					   dialog,
					   info_details),
			     cache_directory_tagging,
			     root_fs_device,
			     ignore_unknown,
			     scope);
	thread_cancellation thr_cancel;
	infinint skipped_dump, fs_errors;
	infinint wasted_bytes = 0;

	if(auto_zeroing_neg_dates)
	    fs.zeroing_negative_dates_without_asking();

        st.clear();
        cat.reset_add();
        ref.reset_compare();
	fs.set_ignored_symlinks_list(ignored_symlinks);

	try
	{
	    try
	    {
		while(fs.read(e, fs_errors, skipped_dump))
		{
		    cat_nomme *nom = dynamic_cast<cat_nomme *>(e);
		    cat_directory *dir = dynamic_cast<cat_directory *>(e);
		    cat_inode *e_ino = dynamic_cast<cat_inode *>(e);
		    cat_file *e_file = dynamic_cast<cat_file *>(e);
		    cat_mirage *e_mir = dynamic_cast<cat_mirage *>(e);
		    bool known_hard_link = false;

		    st.add_to_ignored(skipped_dump);
		    st.add_to_errored(fs_errors);

		    juillet.enfile(e);
		    thr_cancel.check_self_cancellation();
		    if(display_treated_only_dir)
		    {
			if(dir != nullptr)
			    dialog->message(string(gettext("Inspecting directory ")) + juillet.get_string());
		    }

		    if(e_mir != nullptr)
		    {
			known_hard_link = e_mir->is_inode_wrote();
			if(!known_hard_link)
			{
			    e_ino = dynamic_cast<cat_inode *>(e_mir->get_inode());
			    e_file = dynamic_cast<cat_file *>(e_mir->get_inode());
			    if(e_ino == nullptr)
				throw SRC_BUG;
			    e_ino->change_name(e_mir->get_name());
			}
		    }

		    if(nom != nullptr)
		    {
			try
			{
			    string tmp_val;

			    if(subtree.is_covered(juillet.get_path())
			       && (dir != nullptr || filtre.is_covered(nom->get_name()))
			       && (e_ino == nullptr || same_fs.is_covered(e_ino->get_device()))
			       && (e_ino == nullptr || exclude_by_ea == "" || e_ino->ea_get_saved_status() != ea_saved_status::full || e_ino->get_ea() == nullptr || !e_ino->get_ea()->find(exclude_by_ea, tmp_val)))
			    {
				if(known_hard_link)
				{
					// no need to update the semaphore here as no data is read and no directory is expected here
				    cat.pre_add(e); // if cat is a escape_catalogue, this adds an escape sequence and entry info in the archive
				    cat.add(e);
				    e = nullptr;
				    st.incr_hard_links();
				    st.incr_treated();
				    if(e_mir != nullptr)
				    {
					if(e_mir->get_inode()->get_saved_status() == saved_status::saved
					   || e_mir->get_inode()->get_saved_status() == saved_status::delta
					   || e_mir->get_inode()->ea_get_saved_status() == ea_saved_status::full)
					    if(display_treated)
						dialog->message(string(gettext("Recording hard link into the archive: "))+juillet.get_string());
				    }
				    else
					throw SRC_BUG; // known_hard_link is true and e_mir == nullptr !???
				}
				else // not a hard link nor a known hard linked inode
				{
				    const cat_inode *f_ino = nullptr;
				    const cat_file *f_file = nullptr;
				    const cat_mirage *f_mir = nullptr;

				    if(e_ino == nullptr)
					throw SRC_BUG; // if not a known hard link, e_ino should still either point to a real cat_inode
					// or to the hard linked new cat_inode.

				    if(fixed_date.is_zero())
				    {
					bool conflict = ref.compare(e, f);

					if(!conflict)
					{
					    f = nullptr;
					    f_ino = nullptr;
					    f_mir = nullptr;
					}
					else // inode was already present in filesystem at the time the archive of reference was made
					{
					    f_ino = dynamic_cast<const cat_inode *>(f);
					    f_file = dynamic_cast<const cat_file *>(f);
					    f_mir = dynamic_cast<const cat_mirage *>(f);

					    if(f_mir != nullptr)
					    {
						f_ino = f_mir->get_inode();
						f_file = dynamic_cast<const cat_file *>(f_ino);
					    }

						// Now checking for filesystem dissimulated modifications

					    if(security_check)
					    {
						if(f_ino != nullptr && e_ino != nullptr)
						{
							// both are inodes

						    if(cat_signature::compatible_signature(f_ino->signature(), e_ino->signature())
							   // both are of the same type of inode
						       && f_file != nullptr)
							    // both are plain files (no warning issued for other inode types)
						    {
							    // both are plain file or hard-linked plain files

							if(f_ino->get_uid() == e_ino->get_uid()
							   && f_ino->get_gid() == e_ino->get_gid()
							   && f_ino->get_perm() == e_ino->get_perm()
							   && f_ino->get_last_modif() == e_ino->get_last_modif()) // no datetime::loose_equal() here!
							{
								// same inode information

							    if(f_ino->has_last_change() && e_ino->has_last_change())
							    {
								    // both inode ctime has been recorded

								if(!f_ino->get_last_change().loose_equal(e_ino->get_last_change()))
								{
								    string tmp = juillet.get_string();

								    dialog->printf(gettext("SECURITY WARNING! SUSPICIOUS FILE %S: ctime changed since archive of reference was done, while no other inode information changed"), &tmp);
								}
							    }
							}
						    }
						}
					    }
					}
				    }
				    else
					f = nullptr;

				    try
				    {
					U_I sig_bl; // stores the delta sig block len for this file

					f_ino = snapshot ? nullptr : f_ino;
					f_file = snapshot ? nullptr : f_file;

					    // EVALUATING THE ACTION TO PERFORM

					bool change_to_remove_ea =
					    e_ino != nullptr && e_ino->ea_get_saved_status() == ea_saved_status::none
						// current inode to backup does not have any EA
					    && f_ino != nullptr && f_ino->ea_get_saved_status() != ea_saved_status::none
					    && f_ino->ea_get_saved_status() != ea_saved_status::removed;
					    // and reference was an inode with EA

					bool avoid_saving_inode =
					    snapshot
						// don't backup if doing a snapshot
					    || (!fixed_date.is_zero() && e_ino != nullptr && e_ino->get_last_modif() < fixed_date)
						// don't backup if older than given date (if reference date given)
					    || (fixed_date.is_zero() && e_ino != nullptr && f_ino != nullptr && !e_ino->has_changed_since(*f_ino, hourshift, what_to_check)
						&& (f_file == nullptr || !f_file->is_dirty()))
						// don't backup if doing differential backup and entry is the same as the one in the archive of reference
						// and if the reference is a plain file, it was not saved as dirty
					    ;

					bool make_delta_diff =
					    !avoid_saving_inode
					    && delta_diff
					    && e_file != nullptr
					    && f_file != nullptr
					    && f_file->has_delta_signature_available(); // will fail in sequential read mode as the delta_sig of f_file is not been loaded at that time

					bool avoid_saving_ea =
					    snapshot
						// don't backup if doing a snapshot
					    || (!fixed_date.is_zero() && e_ino != nullptr &&  e_ino->ea_get_saved_status() != ea_saved_status::none && e_ino->get_last_change() < fixed_date)
						// don't backup if older than given date (if reference date given)
					    || (fixed_date.is_zero() && e_ino != nullptr && e_ino->ea_get_saved_status() == ea_saved_status::full && f_ino != nullptr && f_ino->ea_get_saved_status() != ea_saved_status::none && (e_ino->get_last_change() <= f_ino->get_last_change() || e_ino->get_last_change().loose_equal(f_ino->get_last_change())) )
						// don't backup if doing differential backup and entry is the same as the one in the archive of reference
					    ;

					bool avoid_saving_fsa =
					    snapshot
						// don't backup if doing a snapshot
					    || (!fixed_date.is_zero() && e_ino != nullptr && e_ino->fsa_get_saved_status() != fsa_saved_status::none && e_ino->get_last_change() < fixed_date)
						// don't backup if older than given date (if reference date given)
						|| (fixed_date.is_zero() && e_ino != nullptr && e_ino->fsa_get_saved_status() == fsa_saved_status::full && f_ino != nullptr && f_ino->fsa_get_saved_status() != fsa_saved_status::none && (e_ino->get_last_change() <= f_ino->get_last_change() || e_ino->get_last_change().loose_equal(f_ino->get_last_change())) )
						// don't backup if doing differential backup and entry is the same as the one in the archive of reference
					    ;

					bool sparse_file_detection =
					    e_file != nullptr
					    && e_file->get_size() > sparse_file_min_size
					    && !sparse_file_min_size.is_zero();

					    // MODIFIYING INODE IF NECESSARY

					if(e_ino->get_saved_status() != saved_status::saved)
					    throw SRC_BUG; // filsystem should always provide "saved" "cat_entree"

					if(avoid_saving_inode)
					{
					    e_ino->set_saved_status(saved_status::not_saved);
					    st.incr_skipped();
					}
					else // checking whether we can leverage the inode_only status
					{
					    if(f_file != nullptr && e_file != nullptr)
					    {
						bool same_data = false;

						    // the whole inode is about to be resaved
						    // checking whether data has really changed
						    // or whether the change only concerns metadata
						switch(mod_data_detect)
						{
						case modified_data_detection::any_inode_change:
						    break;
						case modified_data_detection::mtime_size:
						    same_data = e_file->same_data_as(*f_file, false, hourshift);
						    break;
						default:
						    throw SRC_BUG;
						}

						if(same_data)
						{
						    e_file->set_saved_status(saved_status::inode_only);
						    make_delta_diff = false;
						    avoid_saving_inode = true;
						    st.incr_inode_only();
						}
					    }
						// nothing to do else
					}

					if(make_delta_diff)
					    e_ino->set_saved_status(saved_status::delta);

					if(avoid_saving_ea)
					{
					    if(e_ino->ea_get_saved_status() == ea_saved_status::full)
						e_ino->ea_set_saved_status(ea_saved_status::partial);
					}

					if(avoid_saving_fsa)
					{
					    if(e_ino->fsa_get_saved_status() == fsa_saved_status::full)
						e_ino->fsa_set_saved_status(fsa_saved_status::partial);
					}

					if(change_to_remove_ea)
					    e_ino->ea_set_saved_status(ea_saved_status::removed);

					if(e_file != nullptr)
					{
					    if(make_delta_diff)
						    // sparse_file layer is not useful
						    // when performing a delta patch
						e_file->set_sparse_file_detection_write(false);
					    else
						e_file->set_sparse_file_detection_write(sparse_file_detection);
					}

					    // DECIDING WHETHER FILE DATA WILL BE COMPRESSED OR NOT

					if(e_file != nullptr)
					{
					    if(compr_mask.is_covered(nom->get_name()) && e_file->get_size() >= min_compr_size)
						    // e_nom not e_file because "e"
						    // may be a hard link, in which case its name is not carried by e_ino nor e_file
						e_file->change_compression_algo_write(stock_algo);
					    else
						e_file->change_compression_algo_write(compression::none);
					}

					    // DECIDING WHETHER DELTA SIGNATURE WILL BE CALCULATED

					if(e_file != nullptr
					   && delta_signature
					   && delta_mask.is_covered(juillet.get_string())
					   && e_file->get_size() >= delta_sig_min_size)
					{
					    e_file->will_have_delta_signature_available();
						// during small inode dump for that file, the flag telling a delta_sig is present will be set


						// CALCULATING THE SIGNATURE BLOCK SIZE

					    if(e_file != nullptr)
						sig_bl = delta_sig_block_len.calculate(e_file->get_size());
					    else
						sig_bl = 0;
					}
					else
					    sig_bl = 0;


					    // PERFORMING ACTION FOR ENTRY (cat_entree dump, eventually data dump)

					if(!save_inode(dialog,
						       juillet.get_string(),
						       e,
						       f,
						       pdesc,
						       info_details,
						       display_treated,
						       alter_atime,
						       true,   // check_change
						       true,   // compute_crc
						       cat_file::normal, // keep_mode
						       cat,
						       repeat_count,
						       repeat_byte,
						       sparse_file_min_size,
						       &sem,
						       e_file == nullptr ? false : e_file->has_delta_signature_available(),
						       make_delta_diff,
						       wasted_bytes,
						       ignored_symlinks,
						       false,
						       sig_bl,
						       never_resave_uncompressed))
					    st.incr_tooold(); // counting a new dirty file in archive

					st.set_byte_amount(wasted_bytes);

					if(!avoid_saving_inode)
					    st.incr_treated();


					    // PERFORMING ACTION FOR EA

					if(e_ino->ea_get_saved_status() != ea_saved_status::removed)
					{
					    if(e_ino->ea_get_saved_status() == ea_saved_status::full)
						cat.pre_add_ea(e);
					    if(save_ea(dialog, juillet.get_string(), e_ino, pdesc, display_treated, false))
						st.incr_ea_treated();
					    cat.pre_add_ea_crc(e);
					}

					    // PERFORMING ACTION FOR FSA

					if(e_ino->fsa_get_saved_status() == fsa_saved_status::full)
					{
					    cat.pre_add_fsa(e);
					    if(save_fsa(dialog, juillet.get_string(), e_ino, pdesc, display_treated, false))
						st.incr_fsa_treated();
					    cat.pre_add_fsa_crc(e);
					}

					    // UPDATING HARD LINKS

					if(e_mir != nullptr)
					    e_mir->set_inode_wrote(true); // record that this inode has been saved (it is a "known_hard_link" now)

					    // ADDING ENTRY TO CATALOGUE

					cat.add(e);
					e = nullptr;
				    }
				    catch(...)
				    {
					if(dir != nullptr && fixed_date.is_zero())
					    ref.compare(&tmp_eod, f);
					throw;
				    }
				}

				    //  the object is kept in the catalogue, reducing memory footprint
				    // avoiding keeping in memory the delta signature data
				if(e_file != nullptr)
				    e_file->drop_delta_signature_data();
			    }
			    else // inode not covered
			    {
				cat_nomme *ig = nullptr;
				cat_inode *ignode = nullptr;
				sem.raise(juillet.get_string(), e, false);

				if(display_skipped)
				    dialog->message(string(gettext(SKIPPED)) + juillet.get_string());

				if(dir != nullptr && make_empty_dir)
				    ig = ignode = new (nothrow) cat_ignored_dir(*dir);
				else
				    ig = new (nothrow) cat_ignored(nom->get_name());
				    // necessary to not record deleted files at comparison
				    // time in case files are just not covered by filters
				st.incr_ignored();

				if(ig == nullptr)
				    throw Ememory("filtre_sauvegarde");
				else
				    cat.add(ig);

				if(dir != nullptr)
				{
				    if(make_empty_dir)
				    {
					bool known;

					if(fixed_date.is_zero())
					    known = ref.compare(dir, f);
					else
					    known = false;

					try
					{
					    const cat_inode *f_ino = known ? dynamic_cast<const cat_inode *>(f) : nullptr;
					    bool tosave = false;

					    if(known)
						if(f_ino != nullptr)
						    tosave = dir->has_changed_since(*f_ino, hourshift, what_to_check);
						else
						    throw SRC_BUG;
						// catalogue::compare() with a directory should return false or give a directory as
						// second argument or here f is not an inode (f_ino == nullptr) !
						// and known == true
					    else
						tosave = true;

					    if(ignode == nullptr)
						throw SRC_BUG;

					    ignode->set_saved_status(tosave && !snapshot ? saved_status::saved : saved_status::not_saved);

						// this creates the escape sequence mark
					    cat.pre_add(ignode);
					    cat.pre_add(&tmp_eod);
					}
					catch(...)
					{
					    if(fixed_date.is_zero())
						ref.compare(&tmp_eod, f);
					    throw;
					}
					if(fixed_date.is_zero())
					    ref.compare(&tmp_eod, f);
				    }
				    fs.skip_read_to_parent_dir();
				    juillet.enfile(&tmp_eod);
				}

				delete e; // we don't keep this object in the catalogue so we must destroy it
				e = nullptr;
			    }
			}
			catch(Ebug & e)
			{
			    throw;
			}
			catch(Euser_abort & e)
			{
			    throw;
			}
			catch(Escript & e)
			{
			    throw;
			}
			catch(Ethread_cancel & e)
			{
			    throw;
			}
			catch(Elimitint & e)
			{
			    throw;
			}
			catch(Egeneric & ex)
			{
			    const string & how = ex.find_object("generic_file::copy_to");

			    if(how != "write") // error did not occured while adding data to the archive
			    {
				cat_nomme *tmp = new (nothrow) cat_ignored(nom->get_name());
				dialog->message(string(gettext("Error while saving ")) + juillet.get_string() + ": " + ex.get_message());
				st.incr_errored();

				    // now we can destroy the object
				delete e;
				e = nullptr;

				if(tmp == nullptr)
				    throw Ememory("fitre_sauvegarde");
				cat.add(tmp);

				if(dir != nullptr)
				{
				    fs.skip_read_to_parent_dir();
				    juillet.enfile(&tmp_eod);
				    dialog->message(gettext("NO FILE IN THAT DIRECTORY CAN BE SAVED."));
				}
			    }
			    else
			    {
				ex.prepend_message(gettext("Cannot write down the archive: "));
				throw; // error occured while adding data to the archive, archive cannot be generated properly
			    }
			}
		    }
		    else // cat_eod
		    {
			sem.raise(juillet.get_string(), e, true);
			sem.lower();
			if(fixed_date.is_zero())
			    ref.compare(e, f); // makes the comparison in the reference catalogue go to parent directory
			cat.pre_add(e); // adding a mark and dropping CAT_EOD entry in the archive if cat is an escape_catalogue object (else, does nothing)
			if(display_finished)
			{
			    const cat_directory & cur = cat.get_current_add_dir();
			    string what = juillet.get_string();
			    string size = tools_display_integer_in_metric_system(cur.get_size(), "o", true);
			    string ratio = gettext(", compression ratio ");

			    if(!cur.get_storage_size().is_zero())
				ratio += tools_get_compression_ratio(cur.get_storage_size(), cur.get_size(), true);
			    else
				ratio = "";
			    dialog->printf(gettext("Finished Inspecting directory %S , saved %S%S"),
					  &what,
					  &size,
					  &ratio);
			}
			cat.add(e);

		    }
		}
	    }
	    catch(Ethread_cancel & e)
	    {
		if(!e.immediate_cancel())
		{
		    if(pdesc.compr->is_compression_suspended())
		    {
			pdesc.stack->sync_write_above(pdesc.compr);
			pdesc.compr->resume_compression();
		    }
		}
		throw Ethread_cancel_with_attr(e.immediate_cancel(), e.get_flag(), fs.get_last_etoile_ref());
	    }
	}
	catch(...)
	{
	    if(e != nullptr)
		delete e;
	    throw;
	}

	if(pdesc.compr->is_compression_suspended())
	{
	    pdesc.stack->sync_write_above(pdesc.compr);
	    pdesc.compr->resume_compression();
	}
    }

    void filtre_difference(const shared_ptr<user_interaction> & dialog,
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
			   bool alter_atime,
			   bool furtive_read_mode,
			   comparison_fields what_to_check,
			   const infinint & hourshift,
			   bool compare_symlink_date,
			   const fsa_scope & scope,
			   bool isolated_mode,
			   bool auto_zeroing_neg_dates)
    {
	if(!dialog)
	    throw SRC_BUG; // dialog points to nothing

	const cat_entree *e;
        defile juillet = fs_racine;
        const cat_eod tmp_eod;
        filesystem_diff fs(dialog,
			   fs_racine,
			   info_details,
			   ea_mask,
			   alter_atime,
			   furtive_check(furtive_read_mode,
					 dialog,
					 info_details),
			   scope);
	thread_cancellation thr_cancel;

	if(auto_zeroing_neg_dates)
	    fs.zeroing_negative_dates_without_asking();

	if(display_treated_only_dir && display_treated)
	    display_treated = false;
	    // avoiding the report of action performed for each entry
	    // specific code in this function will show instead the current directory
	    // under which file are processed
	else
	    display_treated_only_dir = false; // avoid incoherence

        st.clear();
        cat.reset_read();

	while(cat.read(e))
	{
	    const cat_directory *e_dir = dynamic_cast<const cat_directory *>(e);
	    const cat_nomme *e_nom = dynamic_cast<const cat_nomme *>(e);
	    const cat_inode *e_ino = dynamic_cast<const cat_inode *>(e);
	    const cat_mirage *e_mir = dynamic_cast<const cat_mirage *>(e);

	    if(e_mir != nullptr)
	    {
		const cat_file *e_file = dynamic_cast<const cat_file *>(e_mir->get_inode());

		if(e_file == nullptr || e_mir->get_etoile_ref_count() == 1 || cat.get_escape_layer() == nullptr)
		{
		    e_ino = e_mir->get_inode();
		    e_mir->get_inode()->change_name(e_mir->get_name());
		}
		else
		    dialog->message(gettext("SKIPPED (hard link in sequential read mode): ") + e_mir->get_name());
	    }

	    juillet.enfile(e);
	    thr_cancel.check_self_cancellation();
	    if(display_treated_only_dir)
	    {
		if(e_dir != nullptr)
		    dialog->message(string(gettext("Inspecting directory ")) + juillet.get_string());
	    }

	    try
	    {
		if(e_nom != nullptr)
		{
		    if(subtree.is_covered(juillet.get_path()) && (e_dir != nullptr || filtre.is_covered(e_nom->get_name())))
		    {
			cat_nomme *exists_nom = nullptr;

			if(e_ino != nullptr)
			{
			    if(e_nom == nullptr)
				throw SRC_BUG;
			    if(fs.read_filename(e_nom->get_name(), exists_nom))
			    {
				try
				{
				    cat_inode *exists = dynamic_cast<cat_inode *>(exists_nom);
				    cat_directory *exists_dir = dynamic_cast<cat_directory *>(exists_nom);

				    if(exists != nullptr)
				    {
					try
					{
					    e_ino->compare(*exists, ea_mask, what_to_check, hourshift, compare_symlink_date, scope, isolated_mode);
					    if(display_treated)
						dialog->message(string(gettext("OK   "))+juillet.get_string());
					    if(e_dir == nullptr || !cat.read_second_time_dir())
						st.incr_treated();
					    if(!alter_atime)
					    {
						const cat_inode * tmp_exists = exists;
						restore_atime(juillet.get_string(), tmp_exists);
					    }
					}
					catch(Erange & e)
					{
					    dialog->message(string(gettext("DIFF "))+juillet.get_string()+": "+ e.get_message());
					    if(e_dir == nullptr && exists_dir != nullptr)
						fs.skip_read_filename_in_parent_dir();
					    if(e_dir != nullptr && exists_dir == nullptr)
					    {
						cat.skip_read_to_parent_dir();
						juillet.enfile(&tmp_eod);
					    }

					    if(e_dir == nullptr || !cat.read_second_time_dir())
						st.incr_errored();
					    if(!alter_atime)
					    {
						const cat_inode * tmp_exists = exists;
						restore_atime(juillet.get_string(), tmp_exists);
					    }
					}
				    }
				    else // existing file is not an inode
					throw SRC_BUG; // filesystem, should always return inode with read_filename()
				}
				catch(...)
				{
				    delete exists_nom;
				    exists_nom = nullptr;
				    throw;
				}
				delete exists_nom;
				exists_nom = nullptr;
			    }
			    else // can't compare, nothing of that name in filesystem
			    {
				dialog->message(string(gettext("DIFF "))+ juillet.get_string() + gettext(": file not present in filesystem"));
				if(e_dir != nullptr)
				{
				    cat.skip_read_to_parent_dir();
				    juillet.enfile(&tmp_eod);
				}

				if(e_dir == nullptr || !cat.read_second_time_dir())
				    st.incr_errored();
			    }
			}
			else // not an inode (for example a cat_detruit, hard_link etc...), nothing to do
			    st.incr_treated();
		    }
		    else // not covered by filters
		    {
			if(display_skipped)
			    dialog->message(string(gettext(SKIPPED)) + juillet.get_string());

			if(e_dir == nullptr || !cat.read_second_time_dir())
			    st.incr_ignored();
			if(e_dir != nullptr)
			{
			    cat.skip_read_to_parent_dir();
			    juillet.enfile(&tmp_eod);
			}
		    }
		}
		else // cat_eod ?
		    if(dynamic_cast<const cat_eod *>(e) != nullptr) // yes cat_eod
			fs.skip_read_filename_in_parent_dir();
		    else // no ?!?
			throw SRC_BUG; // not cat_nomme neither cat_eod ! what's that ?
	    }
	    catch(Euser_abort &e)
	    {
		throw;
	    }
	    catch(Ebug &e)
	    {
		throw;
	    }
	    catch(Escript & e)
	    {
		throw;
	    }
	    catch(Ethread_cancel & e)
	    {
		throw;
	    }
	    catch(Egeneric & e)
	    {
		dialog->message(string(gettext("ERR  ")) + juillet.get_string() + " : " + e.get_message());
		if(e_dir == nullptr || !cat.read_second_time_dir())
		    st.incr_errored();
	    }
	}
        fs.skip_read_filename_in_parent_dir();
            // this call here only to restore dates of the root (-R option) directory
    }

    void filtre_test(const shared_ptr<user_interaction> & dialog,
		     const mask &filtre,
                     const mask &subtree,
                     const catalogue & cat,
                     bool info_details,
		     bool display_treated,
		     bool display_treated_only_dir,
		     bool display_skipped,
		     bool empty,
		     bool repairing,
                     statistics & st)
    {
        const cat_entree *e;
        defile juillet = FAKE_ROOT;
        null_file black_hole = null_file(gf_write_only);
        infinint offset;
        const cat_eod tmp_eod;
	thread_cancellation thr_cancel;
	string perimeter;
	shared_ptr<memory_file> delta_sig;
	U_I sig_block_len;

	if(!dialog)
	    throw SRC_BUG; // dialog points to nothing

	if(display_treated_only_dir && display_treated)
	    display_treated = false;
	    // avoid having save_inode/save_ea to report action performed for each entry
	    // specific code in this function will show instead the current directory
	    // under which file are processed
	else
	    display_treated_only_dir = false; // avoid incoherence


        st.clear();
	cat.set_all_mirage_s_inode_wrote_field_to(false);
        cat.reset_read();
	try
	{
	    while(cat.read(e))
	    {
		const cat_file *e_file = dynamic_cast<const cat_file *>(e);
		const cat_inode *e_ino = dynamic_cast<const cat_inode *>(e);
		const cat_directory *e_dir = dynamic_cast<const cat_directory *>(e);
		const cat_nomme *e_nom = dynamic_cast<const cat_nomme *>(e);
		const cat_mirage *e_mir = dynamic_cast<const cat_mirage *>(e);

		juillet.enfile(e);
		thr_cancel.check_self_cancellation();
		if(display_treated_only_dir)
		{
		    if(e_dir != nullptr)
			dialog->message(string(gettext("Inspecting directory ")) + juillet.get_string());
		}

		perimeter = "";
		try
		{
		    if(e_mir != nullptr)
		    {
			if(!e_mir->is_inode_wrote())
			{
			    e_file = dynamic_cast<const cat_file *>(e_mir->get_inode());
			    e_ino = e_mir->get_inode();
			}
		    }

		    if(e_nom != nullptr)
		    {
			if(subtree.is_covered(juillet.get_path()) && (e_dir != nullptr || filtre.is_covered(e_nom->get_name())))
			{
				// checking data file if any

			    if(e_file != nullptr &&
			       (e_file->get_saved_status() == saved_status::saved
				|| e_file->get_saved_status() == saved_status::delta))
			    {
				perimeter = gettext("Data");
				if(!empty)
				{
				    bool dirty_file;

				    do
				    {
					generic_file *dat = e_file->get_data(cat_file::normal, nullptr, 0, nullptr);
					if(dat == nullptr)
					    throw Erange("filtre_test", gettext("Can't read saved data."));

					dirty_file = false;

					try
					{
					    infinint crc_size;
					    crc *check = nullptr;
					    const crc *original = nullptr;

					    if(!e_file->get_crc_size(crc_size))
						crc_size = tools_file_size_to_crc_size(e_file->get_size());

					    dat->skip(0);
						// in sequential read mode, storage_size is zero
						// which leads to ask an endless read_ahead (up to eof)
						// thus the read_ahaead will be bounded by the escape
						// layer up to the next tape mark, as expected
					    dat->read_ahead(e_file->get_storage_size());
					    try
					    {
						dat->copy_to(black_hole, crc_size, check);
					    }
					    catch(...)
					    {
						    // in sequential read mode we must
						    // try to read the CRC for the object
						    // be completed and not generating an
						    // error due to absence of CRC later on
						e_file->get_crc(original);
						throw;
					    }
					    if(check == nullptr)
						throw SRC_BUG;

					    try
					    {
						    // due to possible sequential reading mode, the CRC
						    // must not be fetched before the data has been copied
						if(e_file->get_crc(original))
						{
						    if(original == nullptr)
							throw SRC_BUG;
						    if(typeid(*check) != typeid(*original))
							throw SRC_BUG;
						    if(*check != *original)
							throw Erange("fitre_test", gettext("CRC error: data corruption."));
						}
					    }
					    catch(...)
					    {
						delete check;
						throw;
					    }
					    delete check;
					}
					catch(...)
					{
					    delete dat;
					    e_file->clean_data();
					    throw;
					}
					delete dat;
					e_file->clean_data();

					if(cat.get_escape_layer() != nullptr
					   && cat.get_escape_layer()->skip_to_next_mark(escape::seqt_changed, false))
					{
					    dirty_file = true;
					    cat_file *modif_e_file = const_cast<cat_file *>(e_file);
					    if(modif_e_file == nullptr)
						throw SRC_BUG;
					    modif_e_file->drop_crc();
					    modif_e_file->set_storage_size(0);
					    modif_e_file->set_offset(cat.get_escape_layer()->get_position());
					}
				    }
				    while(dirty_file);
				}
			    }


				// checking delta signature if any

			    if(e_file != nullptr && e_file->has_delta_signature_structure())
			    {
				try
				{
					// reading the delta signature

				    e_file->read_delta_signature(delta_sig, sig_block_len);
				    if(delta_sig)
					delta_sig.reset();

				    if(perimeter == "")
					perimeter = "Delta sig";
				    else
					perimeter += " + Delta sig";
				}
				catch(...)
				{
				    e_file->drop_delta_signature_data();
				    throw;
				}
				e_file->drop_delta_signature_data();
			    }


				// checking inode EA if any

			    if(e_ino != nullptr && e_ino->ea_get_saved_status() == ea_saved_status::full && !cat.read_second_time_dir())
			    {
				if(perimeter == "")
				    perimeter = "EA";
				else
				    perimeter += " + EA";
				if(!empty)
				{
				    ea_attributs tmp = *(e_ino->get_ea());
				    perimeter += "(" + deci(tmp.size()).human() +")";
				    e_ino->ea_detach();
				}
			    }

				// checking FSA if any
			    if(e_ino != nullptr && e_ino->fsa_get_saved_status() == fsa_saved_status::full && !cat.read_second_time_dir())
			    {
				if(perimeter == "")
				    perimeter = "FSA";
				else
				    perimeter += " + FSA";
				if(!empty)
				{
				    const filesystem_specific_attribute_list *tmp = e_ino->get_fsa();
				    if(tmp == nullptr)
					throw SRC_BUG;
				    perimeter += "(" + deci(tmp->size()).human() + ")";
				    e_ino->fsa_detach();
				}
			    }

			    if(e_dir == nullptr || !cat.read_second_time_dir())
				st.incr_treated();

			    if(e_mir != nullptr)
				e_mir->set_inode_wrote(true);

				// still no exception raised, this all is fine
			    if(display_treated)
				dialog->message(string(gettext("OK  ")) + juillet.get_string() + "  " + perimeter);
			}
			else // excluded by filter
			{
			    if(display_skipped)
				dialog->message(string(gettext(SKIPPED)) + juillet.get_string());

			    if(e_dir != nullptr)
			    {
				juillet.enfile(&tmp_eod);
				cat.skip_read_to_parent_dir();
			    }
			    if(e_dir == nullptr || !cat.read_second_time_dir())
				st.incr_skipped();
			}
		    }
		}
		catch(Ebug & e)
		{
		    throw;
		}
		catch(Escript & e)
		{
		    throw;
		}
		catch(Ethread_cancel & e)
		{
		    throw;
		}
		catch(Egeneric & e)
		{
		    Euser_abort *eabort = dynamic_cast<Euser_abort*>(&e);

		    if(eabort != nullptr && ! repairing)
			throw;

		    dialog->message(string(gettext("ERR ")) + juillet.get_string() + " : " + e.get_message());
		    if(e_dir == nullptr || !cat.read_second_time_dir())
			st.incr_errored();
			// last read inode may have failed reading EA and FSA due to data missing
			// we completely remove it from the catalog as this part of the code is used
			// also for archive isolation (in sequential read mode) and here better
			// drop the inode for it can be resaved later, while for test operation this
			// has no impact at all.
		    catalogue* ncat = const_cast<catalogue*>(&cat);
		    if(ncat == nullptr)
			throw SRC_BUG;
		    ncat->remove_last_read();
		}
	    }
	}
	catch(Egeneric & e)
	{
	    Euser_abort *eabort = dynamic_cast<Euser_abort*>(&e);

	    if(eabort != nullptr && ! repairing)
		throw;
	}
    }

    void filtre_merge(const shared_ptr<user_interaction> & dialog,
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
		      const crit_action & over_action,
		      bool warn_overwrite,
		      bool decremental_mode,
		      const infinint & sparse_file_min_size,
		      const fsa_scope & scope,
		      bool delta_signature,
		      bool build_delta_sig,
		      const infinint & delta_sig_min_size,
		      const mask & delta_mask,
		      const delta_sig_block_size & signature_block_size,
		      bool never_resave_uncompressed)
    {
	crit_action *decr = nullptr; // will point to a locally allocated crit_action
	    // for decremental backup (argument overwrite is ignored)
	const crit_action *overwrite = &over_action; // may point to &decr if
	    // decremental backup is asked
	bool abort = false;
	thread_cancellation thr_cancel;

	if(!dialog)
	    throw SRC_BUG; // dialog points to nothing

	if(display_treated_only_dir && display_treated)
	    display_treated = false;
	    // avoiding the report of action performed for each entry
	    // specific code in this function will show instead the current directory
	    // under which file are processed
	else
	    display_treated_only_dir = false; // avoid incoherence

	filtre_merge_step0(dialog,
			   ref1,
			   ref2,
			   st,
			   decremental_mode,
			   decr,
			   overwrite,
			   abort,
			   thr_cancel);

	filtre_merge_step1(dialog,
			   filtre,
			   subtree,
			   cat,
			   ref1,
			   ref2,
			   info_details,
			   display_treated,
			   display_skipped,
			   st,
			   make_empty_dir,
			   warn_overwrite,
			   decremental_mode,
			   decr,
			   overwrite,
			   abort,
			   thr_cancel);

	filtre_merge_step2(dialog,
			   pdesc,
			   cat,
			   info_details,
			   display_treated,
			   display_treated_only_dir,
			   compr_mask,
			   min_compr_size,
			   keep_compressed,
			   sparse_file_min_size,
			   delta_signature,
			   build_delta_sig,
			   delta_sig_min_size,
			   delta_mask,
			   abort,
			   thr_cancel,
			   false,
			   signature_block_size,
			   never_resave_uncompressed);
    }

    void filtre_merge_step0(const shared_ptr<user_interaction> & dialog,
			    const catalogue * ref1,
			    const catalogue * ref2,
			    statistics & st,
			    bool decremental_mode,
			    crit_action* & decr,
			    const crit_action* & overwrite,
			    bool & abort,
			    thread_cancellation & thr_cancel)
    {
	if(!dialog)
	    throw SRC_BUG; // dialog points to nothing

	    // STEP 0: Getting ready

	st.clear();

	if(decremental_mode)
	{
	    if(ref1 == nullptr || ref2 == nullptr)
	    {
		dialog->pause(gettext("Decremental mode is useless when merging is not applied to both an archive of reference and an auxiliary archive of reference. Ignore decremental mode and continue?"));
		decremental_mode = false;
	    }
	    else
	    {
		    // allocating decr to "{E&R&~R&(A|!H)}[S*] P* ; {(e&~e&r&~r)|(!e&!~e)}[*s] *p"
		    //
		    // which means to record just data presence (S*) when:
		    //   both entries are of the same type and share the same inode data (E)
		    //   and both have the same modification date (R&~R)
		    //   and this is the first time we meet this hard linked inode, or this is not a hard linked inode (A|!H)
		    // else data is taken as is (P*) from the "in place" archive
		    // EA and FSA are recorded as present when:
		    //   both entries have EA/FSA present (e&~e)
		    //   and both EA/FSA set have the same date (r&~r)
		    // OR
		    //  none entry has EA/FSA present
		    // else the EA/FSA (or the absence of EA/FSA) is taken from the "in place" archive

		try
		{
		    crit_chain *decr_crit_chain = new (nothrow) crit_chain();
		    if(decr_crit_chain == nullptr)
			throw Ememory("filtre_merge");
		    decr = decr_crit_chain;
		    crit_and c_and = crit_and();
		    crit_or  c_or = crit_or();

		    c_and.clear();
		    c_or.clear();
		    c_and.add_crit(crit_same_inode_data());
		    c_and.add_crit(crit_in_place_data_more_recent());
		    c_and.add_crit(crit_invert(crit_in_place_data_more_recent()));
		    c_or.add_crit(crit_in_place_is_new_hardlinked_inode());
		    c_or.add_crit(crit_not(crit_in_place_is_hardlinked_inode()));
		    c_and.add_crit(c_or);

		    decr_crit_chain->add(testing(
					     c_and,
					     crit_constant_action(data_preserve_mark_already_saved, EA_undefined),
					     crit_constant_action(data_preserve, EA_undefined)
					     ));

		    c_and.clear();
		    c_or.clear();
		    c_and.add_crit(crit_in_place_EA_present());
		    c_and.add_crit(crit_invert(crit_in_place_EA_present()));
		    c_and.add_crit(crit_in_place_EA_more_recent());
		    c_and.add_crit(crit_invert(crit_in_place_EA_more_recent()));
		    c_or.add_crit(c_and);
		    c_and.clear();
		    c_and.add_crit(crit_not(crit_in_place_EA_present()));
		    c_and.add_crit(crit_not(crit_invert(crit_in_place_EA_present())));
		    c_or.add_crit(c_and);

		    decr_crit_chain->add(testing(
					     c_or,
					     crit_constant_action(data_undefined, EA_preserve_mark_already_saved),
					     crit_constant_action(data_undefined, EA_preserve)
					     ));
		}
		catch(...)
		{
		    if(decr != nullptr)
		    {
			delete decr;
			decr = nullptr;
		    }
		    throw;
		}
		overwrite = decr;
	    }
	}

	    /// End of Step 0
    }

    void filtre_merge_step1(const shared_ptr<user_interaction> & dialog,
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
			    thread_cancellation & thr_cancel)
    {
	const catalogue *ref_tab[] = { ref1, ref2, nullptr };
	const cat_entree *e = nullptr;
	U_I index = 0;
	defile juillet = FAKE_ROOT;
	map <infinint, cat_etoile *> corres_copy;
	infinint etiquette_offset = 0;
	const cat_eod tmp_eod;

	try
	{

	    if(overwrite == nullptr)
		throw SRC_BUG;

	    if(!dialog)
		throw SRC_BUG; // dialog points to nothing


		// STEP 1:
		// we merge catalogues (pointed to by ref_tab[]) of each archive and produce a resulting catalogue 'cat'
		// the merging resolves overwriting conflicts following the "criterium" rule
		// each object of the catalogue is cloned and stored in 'cat', these clones get dump to archive at step 2

	    try
	    {

		while(ref_tab[index] != nullptr) // for each catalogue of reference (ref. and eventually auxiliary ref.) do:
		{
		    juillet = FAKE_ROOT;
		    cat.reset_add();
		    cat.reset_read();
		    ref_tab[index]->reset_read();

		    if(info_details)
		    {
			const char *ptr;
			switch(index)
			{
			case 0:
			    ptr = gettext("first");
			    break;
			case 1:
			    ptr = gettext("second");
			    break;
			default:
			    ptr = gettext("next"); // not yet used, but room is made for future evolutions
			    break;
			}
			dialog->printf(gettext("Merging/filtering files from the %s archive..."), ptr);
		    }

		    while(ref_tab[index]->read(e)) // examining the content of the current archive of reference, each entry one by one
		    {
			const cat_nomme *e_nom = dynamic_cast<const cat_nomme *>(e);
			const cat_mirage *e_mir = dynamic_cast<const cat_mirage *>(e);
			const cat_directory *e_dir = dynamic_cast<const cat_directory *>(e);
			const cat_detruit *e_detruit = dynamic_cast<const cat_detruit *>(e);

			juillet.enfile(e);
			thr_cancel.check_self_cancellation();
			if(e_nom != nullptr)
			{
			    try
			    {
				if(subtree.is_covered(juillet.get_path()) && (e_dir != nullptr || filtre.is_covered(e_nom->get_name())))
				{
				    cat_entree *dolly = make_clone(e_nom, corres_copy, etiquette_offset);

					// now that we have a clone object we must add the copied object to the catalogue, respecting the overwriting constaints

				    try
				    {
					string the_name = e_nom->get_name();
					const cat_nomme *already_here = nullptr;  // may receive an address when an object of that name already exists in the resultant catalogue

					    // some different types of pointer to the "dolly" object

					cat_nomme *dolly_nom = dynamic_cast<cat_nomme *>(dolly);
					cat_directory *dolly_dir = dynamic_cast<cat_directory *>(dolly);
					cat_mirage *dolly_mir = dynamic_cast<cat_mirage *>(dolly);
					cat_inode *dolly_ino = dynamic_cast<cat_inode *>(dolly);

					if(dolly_mir != nullptr)
					    dolly_ino = dolly_mir->get_inode();

					if(cat.read_if_present(& the_name, already_here)) // An entry of that name already exists in the resulting catalogue
					{

						// some different types of pointer to the "already_here" object (aka 'inplace" object)
					    const cat_mirage *al_mir = dynamic_cast<const cat_mirage *>(already_here);
					    const cat_detruit *al_det = dynamic_cast<const cat_detruit *>(already_here);
					    const cat_ignored *al_ign = dynamic_cast<const cat_ignored *>(already_here);
					    const cat_ignored_dir *al_igndir = dynamic_cast<const cat_ignored_dir *>(already_here);
					    const cat_inode *al_ino = dynamic_cast<const cat_inode *>(already_here);
					    const cat_directory *al_dir = dynamic_cast<const cat_directory *>(already_here);
					    const string full_name = juillet.get_string();

					    over_action_data act_data;
					    over_action_ea act_ea;

					    if(al_mir != nullptr)
						al_ino = al_mir->get_inode();

					    if(dolly_nom == nullptr)
						throw SRC_BUG; // dolly should be the clone of a cat_nomme object which is not the case here

						// evaluating the overwriting policy

					    overwrite->get_action(*already_here, *dolly_nom, act_data, act_ea);

					    if(act_data == data_ask)
						act_data = op_tools_crit_ask_user_for_data_action(*dialog, full_name, already_here, dolly);

						// possibly modifying the resulting action when warning is requested

					    if(warn_overwrite)
					    {
						switch(act_data)
						{
						case data_overwrite:
						case data_overwrite_mark_already_saved:
						case data_remove:
						case data_preserve_mark_already_saved:
						    try
						    {
							string action;

							switch(act_data)
							{
							case data_overwrite:
							    action = gettext("overwritten");
							    break;
							case data_overwrite_mark_already_saved:
							case data_preserve_mark_already_saved:
							    action = gettext("dropped from the archive and marked as already saved");
							    break;
							case data_remove:
							    action = gettext("removed");
							    break;
							default:
							    throw SRC_BUG;
							}
							dialog->pause(tools_printf(gettext("Data of file %S is about to be %S, proceed?"), &full_name, &action));
						    }
						    catch(Euser_abort & e)
						    {
							act_data = data_preserve;
						    }
						    break;
						case data_preserve:
						case data_undefined:
						case data_ask:
						    break;
						default:
						    throw SRC_BUG;
						}

						switch(act_ea)
						{
						case EA_overwrite:
						case EA_clear:
						case EA_preserve_mark_already_saved:
						case EA_overwrite_mark_already_saved:
						case EA_merge_overwrite:
						    try
						    {
							string action;

							switch(act_ea)
							{
							case EA_overwrite:
							    action = gettext("replaced");
							    break;
							case EA_clear:
							    action = gettext("removed from the archive");
							    break;
							case EA_preserve_mark_already_saved:
							case EA_overwrite_mark_already_saved:
							    action = gettext("dropped from the archive and marked as already saved");
							    break;
							case EA_merge_overwrite:
							    action = gettext("merged with possible overwriting");
							    break;
							default:
							    throw SRC_BUG;
							}
							dialog->pause(tools_printf(gettext("EA and FSA of file %S are about to be %S, proceed?"), &full_name, &action));
						    }
						    catch(Euser_abort & e)
						    {
							act_data = data_preserve;
						    }

						    break;
						case EA_preserve:
						case EA_merge_preserve:
						case EA_ask:
						case EA_undefined:
						    break;
						default:
						    throw SRC_BUG;
						}
					    }




					    switch(act_data)
					    {
						    /////////////////////////// FIRST ACTIONS CATEGORY for DATA /////
					    case data_preserve:
					    case data_preserve_mark_already_saved:
					    case data_undefined: // remaining data_undefined at the end of the evaluation defaults to data_preserve

						    // drop data if necessary

						if(act_data == data_preserve_mark_already_saved
						   && al_ino != nullptr)
						{
						    cat_inode *tmp = const_cast<cat_inode *>(al_ino);

						    if(tmp->get_saved_status() == saved_status::saved
						       || tmp->get_saved_status() == saved_status::inode_only)
							tmp->set_saved_status(saved_status::not_saved); // dropping data
						}

						    // EA consideration

						if(act_ea == EA_ask)
						{
						    if(dolly_ino != nullptr && al_ino != nullptr
						       && (dolly_ino->ea_get_saved_status() != ea_saved_status::none
							   || al_ino->ea_get_saved_status() != ea_saved_status::none
							   || dolly_ino->fsa_get_saved_status() != fsa_saved_status::none
							   || al_ino->fsa_get_saved_status() != fsa_saved_status::none)
							)
							act_ea = op_tools_crit_ask_user_for_EA_action(*dialog, full_name, already_here, dolly);
						    else
							act_ea = EA_preserve; // whatever what we want is, as no EA exist for both inplace and to be added objects, there is just no need to ask for that.
						}

						switch(act_ea)
						{
						case EA_preserve:
						case EA_undefined: // remaining ea_undefined at the end of the evaluation defaults to ea_preserve
							// nothing to do
						    break;
						case EA_overwrite:
						case EA_overwrite_mark_already_saved:
						case EA_merge_preserve:
						case EA_merge_overwrite:
						    if(display_treated)
							dialog->message(tools_printf(gettext("EA and FSA of file %S from first archive have been updated with those of same named file of the auxiliary archive"), &full_name));
						    do_EFSA_transfert(dialog, act_ea, const_cast<cat_inode *>(al_ino), dolly_ino);
						    break;

						case EA_preserve_mark_already_saved:

						    if(al_ino != nullptr && al_ino->ea_get_saved_status() == ea_saved_status::full)
						    {
							const_cast<cat_inode *>(al_ino)->ea_set_saved_status(ea_saved_status::partial);
							if(display_treated)
							    dialog->message(tools_printf(gettext("EA of file %S from first archive have been dropped and marked as already saved"), &full_name));
						    }
						    if(al_ino != nullptr && al_ino->fsa_get_saved_status() == fsa_saved_status::full)
						    {
							const_cast<cat_inode *>(al_ino)->fsa_set_saved_status(fsa_saved_status::partial);
							if(display_treated)
							    dialog->message(tools_printf(gettext("FSA of file %S from first archive have been dropped and marked as already saved"), &full_name));
						    }
						    break;

						case EA_clear:
						    if(al_ino != nullptr && al_ino->ea_get_saved_status() != ea_saved_status::none)
						    {
							if(al_ino->ea_get_saved_status() == ea_saved_status::full)
							    st.decr_ea_treated();
							if(display_treated)
							    dialog->message(tools_printf(gettext("EA of file %S from first archive have been removed"), &full_name));
							const_cast<cat_inode *>(al_ino)->ea_set_saved_status(ea_saved_status::none);
						    }
						    if(al_ino != nullptr && al_ino->fsa_get_saved_status() != fsa_saved_status::none)
						    {
							if(al_ino->fsa_get_saved_status() == fsa_saved_status::full)
							    st.decr_fsa_treated();
							if(display_treated)
							    dialog->message(tools_printf(gettext("FSA of file %S from first archive have been removed"), &full_name));
							const_cast<cat_inode *>(al_ino)->fsa_set_saved_status(fsa_saved_status::none);
						    }

						    break;

						default:
						    throw SRC_BUG;
						}


						    // we must keep the existing entry in the catalogue

						if(display_skipped && (dolly_dir == nullptr || al_dir == nullptr))
						    dialog->message(tools_printf(gettext("Data of file %S from first archive has been preserved from overwriting"), &full_name));

						if(al_dir != nullptr && dolly_dir != nullptr)
						{
							// we can recurse in the directory in both ref and current catalogue because both entries are directories
						    try
						    {
							cat.re_add_in(al_dir->get_name()); // trying to update the add cursor of cat
						    }
						    catch(Erange & e)
						    {
							ref_tab[index]->skip_read_to_parent_dir(); // updates back the read cursor of catalogue of reference
							juillet.enfile(&tmp_eod);       // updates back the read cursor of juillet
							cat.skip_read_to_parent_dir(); // updates back the read cursor of cat
							throw;
						    }
						}
						else // there is not the possibility to recurse in directory in both reference catalogue and catalogue under construction
						{
						    if(al_dir != nullptr)
							cat.skip_read_to_parent_dir();
						    if(dolly_dir != nullptr)
						    {
							ref_tab[index]->skip_read_to_parent_dir();
							juillet.enfile(&tmp_eod);
						    }
						}

						    // we may need to clean the corres_copy map

						if(dolly_mir != nullptr)
						    clean_hard_link_base_from(dolly_mir, corres_copy);

						    // now we can safely destroy the clone object

						delete dolly;
						dolly = nullptr;
						break;

						    /////////////////////////// SECOND ACTIONS CATEGORY for DATA /////
					    case data_overwrite:
					    case data_overwrite_mark_already_saved:
					    case data_remove:

						    // drop data if necessary

						if(display_treated)
						{
						    switch(act_data)
						    {
						    case data_remove:
							dialog->message(tools_printf(gettext("Data of file %S taken from the first archive of reference has been removed"), &full_name));
							break;
						    case data_overwrite:
							if(merge_applying_patch_possible(already_here, dolly))
							{
							    dialog->message(tools_printf(gettext("Data of file %S taken from the first archive will be patched using the binary delta found in the auxiliary archive"), &full_name));
							    break;
							}
							    /* no break */
						    default:
							dialog->message(tools_printf(gettext("Data of file %S taken from the first archive of reference has been overwritten"), &full_name));
						    }
						}
						else
						    (void) merge_applying_patch_possible(already_here, dolly);
						    // we still need to check and update dolly for patching when no display is requested by the user

						if(act_data == data_overwrite_mark_already_saved && dolly_ino != nullptr)
						{
						    if(dolly_ino->get_saved_status() == saved_status::saved
						       || dolly_ino->get_saved_status() == saved_status::inode_only)
							dolly_ino->set_saved_status(saved_status::not_saved); // dropping data
						}

						    // EA consideration

						if(act_ea == EA_ask && act_data != data_remove)
						{
						    if(dolly_ino != nullptr && al_ino != nullptr &&
						       (dolly_ino->ea_get_saved_status() != ea_saved_status::none
							|| al_ino->ea_get_saved_status() != ea_saved_status::none
							|| dolly_ino->fsa_get_saved_status() != fsa_saved_status::none
							|| al_ino->fsa_get_saved_status() != fsa_saved_status::none))
							act_ea = op_tools_crit_ask_user_for_EA_action(*dialog, full_name, already_here, dolly);
						    else
							act_ea = EA_overwrite; // no need to ask here neither as both entries have no EA.
						}

						if(act_data != data_remove)
						{
						    switch(act_ea)
						    {
						    case EA_preserve:
						    case EA_undefined: // remaining ea_undefined at the end of the evaluation defaults to ea_preserve
							do_EFSA_transfert(dialog, EA_overwrite, dolly_ino, al_ino);
							break;
						    case EA_overwrite:
							if(display_treated)
							    dialog->message(tools_printf(gettext("EA of file %S has been overwritten"), &full_name));
							break; // nothing to do
						    case EA_overwrite_mark_already_saved:
							if(display_treated)
							    dialog->message(tools_printf(gettext("EA of file %S has been overwritten and marked as already saved"), &full_name));
							if(dolly_ino != nullptr && dolly_ino->ea_get_saved_status() == ea_saved_status::full)
							    dolly_ino->ea_set_saved_status(ea_saved_status::partial);
							break;
						    case EA_merge_preserve:
							if(display_treated)
							    dialog->message(tools_printf(gettext("EA of file %S from first archive have been updated with those of the same named file of the auxiliary archive"), &full_name));
							do_EFSA_transfert(dialog, EA_merge_overwrite, dolly_ino, al_ino);
							break;
						    case EA_merge_overwrite:
							if(display_treated)
							    dialog->message(tools_printf(gettext("EA of file %S from first archive have been updated with those of the same named file of the auxiliary archive"), &full_name));
							do_EFSA_transfert(dialog, EA_merge_preserve, dolly_ino, al_ino);
							break;
						    case EA_preserve_mark_already_saved:
							if(display_treated)
							    dialog->message(tools_printf(gettext("EA of file %S has been overwritten and marked as already saved"), &full_name));
							do_EFSA_transfert(dialog, EA_overwrite_mark_already_saved, dolly_ino, al_ino);
							break;
						    case EA_clear:
							if(al_ino->ea_get_saved_status() != ea_saved_status::none)
							{
							    if(display_treated)
								dialog->message(tools_printf(gettext("EA of file %S from first archive have been removed"), &full_name));
							    dolly_ino->ea_set_saved_status(ea_saved_status::none);
							}
							break;
						    default:
							throw SRC_BUG;
						    }
						}
						else // data_remove
						{
							// we remove both objects in overwriting conflict: here for now the dolly clone of the to be added

						    if(dolly_mir != nullptr)
							clean_hard_link_base_from(dolly_mir, corres_copy);

						    if(dolly_dir != nullptr)
						    {
							juillet.enfile(&tmp_eod);
							ref_tab[index]->skip_read_to_parent_dir();
							dolly_dir = nullptr;
						    }

							// now we can safely destroy the clone object

						    delete dolly;
						    dolly = nullptr;
						}


						    // we must remove the existing entry present in the catalogue to make room for the new entry to be added

						if(dolly_dir == nullptr || al_dir == nullptr || act_data == data_remove) // one or both are not directory we effectively must remove the entry from the catalogue
						{
						    cat_ignored_dir *if_removed = nullptr;

							// to known which counter to decrement

						    st.decr_treated();

						    if(al_ino != nullptr)
							if(al_ino->ea_get_saved_status() == ea_saved_status::full)
							    st.decr_ea_treated();

							// hard link specific actions

						    if(al_mir != nullptr)
						    {
							    // update back hard link counter
							st.decr_hard_links();

							    // cleaning the corres_copy map from the pointed to cat_etoile object reference if necessary
							clean_hard_link_base_from(al_mir, corres_copy);
						    }


						    if(al_det != nullptr)
							st.decr_deleted();

						    if(al_ign != nullptr || al_igndir != nullptr)
							st.decr_ignored();

						    if(act_data == data_remove)
							st.incr_ignored();

							// remove the current entry from the catalogue
						    if(al_dir != nullptr)
						    {
							infinint tree_size = al_dir->get_tree_size();
							map<infinint, infinint> tiquettes;
							map<infinint, infinint>::iterator ut;

							st.add_to_ignored(tree_size);
							st.sub_from_treated(tree_size);
							st.sub_from_ea_treated(al_dir->get_tree_ea_num());
							st.sub_from_hard_links(al_dir->get_tree_mirage_num());

							cat.skip_read_to_parent_dir();

							    // updating corres_copy with hard_link that will be destroyed due to directory deletion
							tiquettes.clear();
							al_dir->get_etiquettes_found_in_tree(tiquettes);
							ut = tiquettes.begin();
							while(ut != tiquettes.end())
							{
							    map<infinint, cat_etoile *>::iterator it = corres_copy.find(ut->first);

							    if(it == corres_copy.end())
								throw SRC_BUG; // unknown etiquettes found in directory tree

							    if(it->second->get_ref_count() < ut->second)
								throw SRC_BUG;
								// more reference found in directory tree toward this cat_etoile than
								// this cat_etoile is aware of !

							    if(it->second->get_ref_count() == ut->second)
								    // this cat_etoile will disapear because all its reference are located
								    // in the directory tree we are about to remove, we must clean this
								    // entry from corres_copy
								corres_copy.erase(it);

							    ++ut;
							}


							if(make_empty_dir)
							{
							    if_removed = new (nothrow) cat_ignored_dir(*al_dir);
							    if(if_removed == nullptr)
								throw Ememory("filtre_merge");
							}
						    }

						    try
						    {

							    // we can now remove the entry from the catalogue
							cat.remove_read_entry(the_name);

							    // now that the ancient directory has been removed we can replace it by an cat_ignored_dir entry if required
							if(if_removed != nullptr)
							    cat.add(if_removed);

						    }
						    catch(...)
						    {
							if(if_removed != nullptr)
							{
							    delete if_removed;
							    if_removed = nullptr;
							}
							throw;
						    }

						}
						else // both existing and the one to add are directories we can proceed to the merging of their contents
						{
						    cat.re_add_in_replace(*dolly_dir);
						    delete dolly;
						    dolly = nullptr;
						}
						break;

					    default:                          /////////////////////////// THIRD ACTIONS CATEGORY for DATA /////
						throw SRC_BUG; // unknown data action !
					    }


					} // end of overwiting considerations
					else
					    if(index >= 1 && decremental_mode)
					    {
						unsigned char firm;

						    // this entry only exists in the auxilliary archive of reference, we must thus replace it by a "cat_detruit
						    // because it will have to be removed when restoring the decremental backup.


						    // we may need to clean the corres_copy map
						if(dolly_mir != nullptr)
						{
						    clean_hard_link_base_from(dolly_mir, corres_copy);
						    dolly_mir = nullptr;
						}

						    // then taking care or directory hierarchy
						if(dolly_dir != nullptr)
						{
						    juillet.enfile(&tmp_eod);
						    ref_tab[index]->skip_read_to_parent_dir();
						    dolly_dir = nullptr;
						}

						if(dolly != nullptr)
						{
						    delete dolly;
						    dolly = nullptr;
						}
						else
						    throw SRC_BUG;
						dolly_ino = nullptr;

						if(e_mir != nullptr)
						    firm = e_mir->get_inode()->signature();
						else
						    firm = e->signature();

						dolly = new (nothrow) cat_detruit(the_name, firm, ref_tab[index]->get_current_reading_dir().get_last_modif());
						if(dolly == nullptr)
						    throw Ememory("filtre_merge");
						dolly_nom = dynamic_cast<cat_nomme *>(dolly);
					    }

					if(dolly != nullptr)
					{
					    const cat_inode *e_ino = dynamic_cast<const cat_inode *>(e);
					    cat_file *e_file = dynamic_cast<cat_file *>(dolly);

					    cat.add(dolly);
					    if(e_file == nullptr || ! e_file->applying_binary_patch())
						st.incr_treated();

					    if(e_mir != nullptr)
					    {
						st.incr_hard_links();
						e_ino = e_mir->get_inode();
					    }

					    if(e_detruit != nullptr)
						st.incr_deleted();


					    if(e_ino != nullptr)
					    {
						if(e_ino->ea_get_saved_status() == ea_saved_status::full)
						    st.incr_ea_treated();

						if(e_ino->fsa_get_saved_status() == fsa_saved_status::full)
						    st.incr_fsa_treated();
					    }

					    if(e_dir != nullptr) // we must chdir also for the *reading* method of this catalogue object
					    {
						if(!cat.read_if_present(&the_name, already_here))
						    throw SRC_BUG;
					    }
					}
				    }
				    catch(...)
				    {
					if(dolly != nullptr)
					{
					    delete dolly;
					    dolly = nullptr;
					}
					throw;
				    }
				}
				else // excluded by filters
				{
				    if(e_dir != nullptr && make_empty_dir)
				    {
					cat_ignored_dir *igndir = new (nothrow) cat_ignored_dir(*e_dir);
					if(igndir == nullptr)
					    throw Ememory("filtre_merge");
					else
					    cat.add(igndir);
				    }

				    if(display_skipped)
					dialog->message(string(gettext(SKIPPED)) + juillet.get_string());

				    st.incr_ignored();
				    if(e_dir != nullptr)
				    {
					ref_tab[index]->skip_read_to_parent_dir();
					juillet.enfile(&tmp_eod);
				    }
				}
			    }
			    catch(Ebug & e)
			    {
				throw;
			    }
			    catch(Euser_abort & e)
			    {
				dialog->message(juillet.get_string() + gettext(" not merged (user choice)"));

				if(e_dir != nullptr)
				{
				    dialog->message(gettext("No file in this directory will be considered for merging."));
				    ref_tab[index]->skip_read_to_parent_dir();
				    juillet.enfile(&tmp_eod);
				}
				st.incr_errored();
			    }
			    catch(Ethread_cancel & e)
			    {
				throw;
			    }
			    catch(Escript & e)
			    {
				throw;
			    }
			    catch(Elimitint & e)
			    {
				throw;
			    }
			    catch(Egeneric & e)
			    {
				dialog->message(string(gettext("Error while considering file ")) + juillet.get_string() + " : " + e.get_message());

				if(e_dir != nullptr)
				{
				    dialog->message(string(gettext("Warning! No file in this directory will be considered for merging: ")) + juillet.get_string());
				    ref_tab[index]->skip_read_to_parent_dir();
				    juillet.enfile(&tmp_eod);
				}
				st.incr_errored();
			    }
			}
			else  // cat_entree is not a cat_nomme object (this is a "cat_eod")
			{
			    cat_entree *tmp = e->clone();
			    try
			    {
				const cat_nomme *already = nullptr;

				if(tmp == nullptr)
				    throw Ememory("filtre_merge");
				if(dynamic_cast<cat_eod *>(tmp) == nullptr)
				    throw SRC_BUG;
				cat.add(tmp); // add cat_eod to catalogue (add cursor)
				cat.read_if_present(nullptr, already); // equivalent to cat_eod for the reading methods
			    }
			    catch(...)
			    {
				if(tmp != nullptr)
				    delete tmp;
				throw;
			    }
			}
		    }
		    normalize_link_base(corres_copy);
		    index++;  // next archive
		    etiquette_offset = corres_copy.size();
		}
	    }
	    catch(Ethread_cancel & e)
	    {
		abort = true;
		dialog->message(gettext("File selection has been aborted. Now building the resulting archive with the already selected files"));
		if(e.immediate_cancel())
		    throw;
	    }
	}
	catch(...)
	{
	    if(decr != nullptr)
	    {
		delete decr;
		decr = nullptr;
		overwrite = nullptr;
	    }
	    throw;
	}

	if(decr != nullptr)
	{
	    delete decr;
	    decr = nullptr;
	    overwrite = nullptr;
	}

    }

    void filtre_merge_step2(const shared_ptr<user_interaction> & dialog,
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
			    bool repair_mode,
			    const delta_sig_block_size & signature_block_size,
			    bool never_resave_uncompressed)
    {
	compression stock_algo = pdesc.compr->get_algo();
	defile juillet = FAKE_ROOT;
	const cat_entree *e = nullptr;
	infinint fake_repeat = 0;

	if(!dialog)
	    throw SRC_BUG; // dialog points to nothing

	    // STEP 2:
	    // 'cat' is now completed
	    // we must copy the file's data, delta sig, EA and FSA to the archive


	if(info_details)
	    dialog->message("Copying filtered files to the resulting archive...");

	cat.set_all_mirage_s_inode_wrote_field_to(false);
	cat.reset_read();

	try
	{
	    thr_cancel.block_delayed_cancellation(true);

	    while(cat.read(e))
	    {
		cat_entree *e_var = const_cast<cat_entree *>(e);
		cat_nomme *e_nom = dynamic_cast<cat_nomme *>(e_var);
		cat_inode *e_ino = dynamic_cast<cat_inode *>(e_var);
		cat_file *e_file = dynamic_cast<cat_file *>(e_var);
		cat_mirage *e_mir = dynamic_cast<cat_mirage *>(e_var);
		cat_directory *e_dir = dynamic_cast<cat_directory *>(e_var);

		if(e_var == nullptr)
		    throw SRC_BUG;

		cat_file::get_data_mode keep_mode = keep_compressed ? cat_file::keep_compressed : cat_file::keep_hole;

		if(e_mir != nullptr)
		    if(!e_mir->is_inode_wrote()) // we store only once the inode pointed by a set of hard links
		    {
			e_ino = e_mir->get_inode();
			e_file = dynamic_cast<cat_file *>(e_ino);
			if(repair_mode)
			    e_mir->disable_reduction_to_normal_inode();
		    }

		juillet.enfile(e);
		thr_cancel.check_self_cancellation();
		if(display_treated_only_dir)
		{
		    if(e_dir != nullptr)
			dialog->message(string(gettext("Inspecting directory ")) + juillet.get_string());
		}

		if(e_ino != nullptr) // inode
		{
		    bool compute_file_crc = false;

		    if(e_file != nullptr && !repair_mode)
		    {
			const crc *val = nullptr;

			if(!e_file->get_crc(val)
			   || e_file->applying_binary_patch()) // this can occur when reading an old archive format
			    compute_file_crc = true;
		    }

			// deciding whether the file will be compressed or not

		    if(e_file != nullptr)
		    {
			if(e_file->applying_binary_patch())
			    keep_mode = cat_file::normal;

			if(keep_mode != cat_file::keep_compressed)
			    if(compr_mask.is_covered(e_nom->get_name()) && e_file->get_size() >= min_compr_size)
				e_file->change_compression_algo_write(stock_algo);
			    else
				e_file->change_compression_algo_write(compression::none);
			else // keep compressed:
			    e_file->change_compression_algo_write(e_file->get_compression_algo_read());
		    }

			// deciding whether the sparse file detection mechanism will be enabled or not
			// the sparse file detection mechanism is faked active in keep_compressed mode
			// because we need to record that sparse file datastructure is used as compressed data

		    if(e_file != nullptr)
		    {
			if(!sparse_file_min_size.is_zero() && keep_mode != cat_file::keep_compressed) // sparse_file detection is activated
			{
			    if(e_file->get_size() > sparse_file_min_size)
			    {
				e_file->set_sparse_file_detection_write(true);
				keep_mode = cat_file::normal;
			    }
			    else
				if(e_file->get_sparse_file_detection_read())
				{
				    e_file->set_sparse_file_detection_write(false);
				    keep_mode = cat_file::normal;
				}
			}
			else // sparse file layer or absence of layer is unchanged
			    e_file->set_sparse_file_detection_write(e_file->get_sparse_file_detection_read());
		    }

			// deciding whether we calculate (not just transfer) delta signature

		    bool calculate_delta_signature = false;
		    U_I sig_bl = 0; // stores the signature block len

		    if(e_file != nullptr)
		    {
			if(e_file->applying_binary_patch())
			    e_file->clear_delta_signature_only();

			if(!delta_signature) // instructed to remove all delta signature information
			{
			    if(e_file->has_delta_signature_available())
				e_file->clear_delta_signature_only();
			}
			else // delta signature asked for transfer or calculation
			{
			    if(build_delta_sig) // delta signature need to be recalculated if absent
			    {
				if(delta_mask.is_covered(juillet.get_string())
				   && e_file->get_size() >= delta_sig_min_size)
				{
				    if(!e_file->has_delta_signature_available())
				    {
					switch(keep_mode)
					{
					case cat_file::keep_compressed:
						// reading a compressed file and keep compressed mode asked,
						// situation that should be filtered out by sanity checks
					    throw SRC_BUG;
					case cat_file::keep_hole:
					    if(e_file->get_sparse_file_detection_read())
					    {
						string tmp = juillet.get_string();

						dialog->message(tools_printf(gettext("Need to activate sparse file detection in order to calculate delta signature for sparse file %S"), &tmp));
					    }
					    else
					    {
						e_file->will_have_delta_signature_available();
						calculate_delta_signature = true;
						sig_bl = signature_block_size.calculate(e_file->get_size());
					    }
					    break;
					case cat_file::normal:
					case cat_file::plain:
					    e_file->will_have_delta_signature_available();
					    calculate_delta_signature = true;
					    sig_bl = signature_block_size.calculate(e_file->get_size());
					    break;
					default:
					    throw SRC_BUG;
					}
				    }
				}
				else
				{
				    if(e_file->has_delta_signature_available())
					e_file->clear_delta_signature_only();
				}
			    }

				// else nothing to do, save_inode will transfer existing delta signatures
			}
		    }

			// saving inode's data

		    if(!save_inode(dialog,
				   juillet.get_string(),
				   e_var,
				   nullptr,   // ref
				   pdesc,
				   info_details,
				   display_treated,
				   true,       // alter_atime
				   false,      // check_change
				   compute_file_crc,
				   keep_mode,
				   cat,
				   0,     // repeat_count
				   0,     // repeat_byte
				   sparse_file_min_size,
				   nullptr,  // semaphore
				   calculate_delta_signature, // delta_signature
				   false,    // delta_diff
				   fake_repeat,   // current_wasted_bytes
				   set<string>(), // empty list for ignored_as_symlink
				   repair_mode,
				   sig_bl,
				   never_resave_uncompressed))

			throw SRC_BUG;
		    else // succeeded saving
		    {
			if(e_mir != nullptr)
			    e_mir->set_inode_wrote(true);
		    }

			// saving inode's EA

		    if(e_ino->ea_get_saved_status() == ea_saved_status::full)
		    {
			cat.pre_add_ea(e, &pdesc);
			    // ignoring the return value of save_ea, exceptions may still propagate
			(void)save_ea(dialog, juillet.get_string(), e_ino, pdesc, display_treated, repair_mode);
			cat.pre_add_ea_crc(e,&pdesc);
		    }

			// saving inode's FSA
		    if(e_ino->fsa_get_saved_status() == fsa_saved_status::full)
		    {
			cat.pre_add_fsa(e, &pdesc);
			    // ignoring the return value of save_fsa, exceptions may still propagate
			(void)save_fsa(dialog, juillet.get_string(), e_ino, pdesc, display_treated, repair_mode);
			cat.pre_add_fsa_crc(e, &pdesc);
		    }
		}
		else // not an inode
		{
		    cat.pre_add(e, &pdesc);
		    if(e_mir != nullptr && (e_mir->get_inode()->get_saved_status() == saved_status::saved || e_mir->get_inode()->ea_get_saved_status() == ea_saved_status::full))
			if(display_treated)
			    dialog->message(string(gettext("Adding Hard link to archive: "))+juillet.get_string());
		}

		    // we can now check for interrution requests
		thr_cancel.block_delayed_cancellation(false);
		thr_cancel.block_delayed_cancellation(true);
	    }
	}
	catch(Egeneric & e)
	{
	    Ethread_cancel* e_thread = dynamic_cast<Ethread_cancel*>(&e);
	    Erange* e_range = dynamic_cast<Erange*>(&e);
	    Euser_abort *e_abort = dynamic_cast<Euser_abort*>(&e);

	    if(info_details)
	    {
		string msg = e.get_message();
		dialog->message(tools_printf(gettext("error met while creating archive: %S"), & msg));
	    }
	    if(e_thread == nullptr && e_range == nullptr && ! repair_mode)
		throw;

	    if(!repair_mode)
		cat.tail_catalogue_to_current_read(false);

	    cat.change_location(pdesc);
	    if(pdesc.compr->is_compression_suspended())
	    {
		pdesc.stack->sync_write_above(pdesc.compr);
		pdesc.compr->resume_compression();
	    }
	    thr_cancel.block_delayed_cancellation(false);

	    if(repair_mode && e_abort != nullptr)
		    // we assume the abortion came from the
		    // fact user could not produce a missing
		    // slice, thus we replace the exception
		    // by a Erange one.
		throw Erange("filtre_merge_step2", e.get_message());
	    else
		throw;
	}

	cat.change_location(pdesc);
	if(pdesc.compr->is_compression_suspended())
	{
	    pdesc.stack->sync_write_above(pdesc.compr);
	    pdesc.compr->resume_compression();
	}
	thr_cancel.block_delayed_cancellation(false);

	if(abort)
	    throw Ethread_cancel(false, 0);
    }

    void filtre_sequentially_read_all_catalogue(catalogue & cat,
						const shared_ptr<user_interaction> & dialog,
						bool lax_read_mode)
    {
	const cat_entree *e;
	thread_cancellation thr_cancel;
	defile juillet = FAKE_ROOT;

	cat.set_all_mirage_s_inode_wrote_field_to(false);
	cat.reset_read();

	try
	{
	    while(cat.read(e))
	    {
		const cat_file *e_file = dynamic_cast<const cat_file *>(e);
		const cat_inode *e_ino = dynamic_cast<const cat_inode *>(e);
		const cat_mirage *e_mir = dynamic_cast<const cat_mirage *>(e);
		const crc *check = nullptr;

		juillet.enfile(e);

		thr_cancel.check_self_cancellation();
		if(e_mir != nullptr)
		{
		    if(!e_mir->is_inode_wrote())
		    {
			e_file = dynamic_cast<const cat_file *>(e_mir->get_inode());
			e_ino = e_mir->get_inode();
		    }
		}

		try
		{
		    if(e_file != nullptr)
			(void)e_file->get_crc(check);
		}
		catch(Erange & e)
		{
		    string msg = string(gettext("failed reading CRC from file: ")) + juillet.get_string();
		    if(lax_read_mode)
			dialog->message(msg);
		    else
			throw Edata(msg);
		}

		if(e_mir != nullptr && (e_ino != nullptr || e_file != nullptr))
		    e_mir->set_inode_wrote(true);

		try
		{
		    if(e_ino != nullptr)
		    {
			if(e_ino->ea_get_saved_status() == ea_saved_status::full)
			{
			    (void)e_ino->get_ea();
			    e_ino->ea_get_crc(check);
			}
			if(e_ino->fsa_get_saved_status() == fsa_saved_status::full)
			{
			    (void)e_ino->get_fsa();
			    e_ino->fsa_get_crc(check);
			}
		    }
		}
		catch(Erange & e)
		{
		    string msg = string(gettext("Failed reading CRC for EA and FSA: ")) + juillet.get_string();

		    if(lax_read_mode)
			dialog->message(msg);
		    else
			throw Edata(msg);
		}
	    }
	}
	catch(Erange & e)
	{
	    dialog->message(string(gettext("Error met while reading next entry: ")) + juillet.get_string());
	}
    }



    static bool save_inode(const shared_ptr<user_interaction> & dialog,
			   const string & info_quoi,
			   cat_entree * & e,
			   const cat_entree * ref,
			   const pile_descriptor & pdesc,
			   bool info_details,
			   bool display_treated,
			   bool alter_atime,
			   bool check_change,
			   bool compute_crc,
			   cat_file::get_data_mode keep_mode,
			   const catalogue & cat,
			   const infinint & repeat_count,
			   const infinint & repeat_byte,
			   const infinint & hole_size,
			   semaphore *sem,
			   bool delta_signature,
			   bool delta_diff,
			   infinint & current_wasted_bytes,
			   set<string> ignored_as_symlink,
			   bool repair_mode,
			   U_I signature_block_size,
			   bool never_resave_uncompressed)
    {
	bool ret = true;
	infinint current_repeat_count = 0;
	infinint storage_size;
	bool loop;
	cat_mirage *mir = dynamic_cast<cat_mirage *>(e);
	cat_inode *ino = dynamic_cast<cat_inode *>(e);
	cat_file *fic = dynamic_cast<cat_file *>(ino);
	const cat_file *ref_fic = dynamic_cast<const cat_file *>(ref);
	bool resave_uncompressed = false; // per file status (while never_resave_uncompressed arg is global to the whole operation)
	infinint rewinder; // we skip back here if data must be saved uncompressed
	shared_ptr<memory_file> delta_sig_ref; // holds the delta_signature that will be used as reference for delta patch later on
	U_I sig_ref_block_len;
	const crc *result_crc = nullptr;

	if(pdesc.compr == nullptr)
	    throw SRC_BUG;

	    // recording the position where to skip back upon
	    // poor compression or if file changed while we were
	    // saving it:
	pdesc.stack->sync_write_above(pdesc.compr);
	pdesc.compr->sync_write();
	rewinder = pdesc.stack->get_position();

	if(mir != nullptr)
	{
	    ino = mir->get_inode();
	    if(ino == nullptr)
		throw SRC_BUG;
	    fic = dynamic_cast<cat_file *>(ino);
	}

	try // to be able to release ref_fic's delta signature in any case
	{
	    if(fic != nullptr && fic->get_saved_status() == saved_status::delta)
	    {
		if(delta_diff) // not a merging operation
		{
		    if(ref_fic == nullptr || !ref_fic->has_delta_signature_available())
			throw SRC_BUG;

			// fetching the delta signature to base the patch on

		    ref_fic->read_delta_signature(delta_sig_ref, sig_ref_block_len);
		    if(!delta_sig_ref)
			throw SRC_BUG;

			// need to store at least the base CRC and result CRC even if not delta signature is computed:
		    fic->will_have_delta_signature_structure();

		    if(ref_fic->has_patch_result_crc())
		    {
			const crc *tmp = nullptr;

			ref_fic->get_patch_result_crc(tmp);
			if(tmp == nullptr)
			    throw SRC_BUG;
			fic->set_patch_base_crc(*tmp);
		    }
		    else
			throw SRC_BUG;
		}
	    }

	    do // loop if resave_uncompressed is set, this is the OUTER LOOP
	    {
		    // PRE RECORDING THE INODE (for sequential reading)

		cat.pre_add(e, &pdesc);

		    // EXITING FOR NON INODE ENTRIES

		if(ino == nullptr)
		    break; // exit the OUTLER LOOP

		    // WRITING DOWN DELTA SIG FOR NO SAVED FILES

		if(fic != nullptr
		   && fic->has_delta_signature_structure()
		   && fic->get_saved_status() != saved_status::saved
		   && fic->get_saved_status() != saved_status::delta)
		{
		    save_delta_signature(dialog,
					 info_quoi,
					 fic,
					 ref_fic,
					 pdesc,
					 signature_block_size,
					 info_details,
					 display_treated,
					 cat);
		}

		    // EXITING FOR INODE ENTRY WITHOUT DATA TO SAVE

		if(ino->get_saved_status() != saved_status::saved
		   && ino->get_saved_status() != saved_status::delta)
		{
		    if(sem != nullptr)
			sem->raise(info_quoi, ino, false);
		    if(ino->get_saved_status() == saved_status::inode_only && display_treated)
			dialog->message(tools_printf(gettext("Adding only inode metadata to archive: %S"), &info_quoi));
		    break; // exiting the OUTER LOOP
		}

		if(compute_crc && (keep_mode != cat_file::normal && keep_mode != cat_file::plain))
		    throw SRC_BUG; // cannot compute crc if data is compressed or hole datastructure not interpreted

		    // TREATING INODE THAT NEED DATA SAVING

		if(display_treated)
		{
		    if(resave_uncompressed)
			dialog->message(string(gettext("Resaving file without compression: ")) + info_quoi);
		    else
		    {
			if(delta_sig_ref)
			    dialog->message(string(gettext("Delta saving file to archive: ")) + info_quoi);
			else if(fic != nullptr && fic->applying_binary_patch())
			    dialog->message(tools_printf(gettext("Merging by applying binary patch to %S"), &info_quoi));
			else
			{
			    string i_type = ino->get_description();
			    dialog->message(tools_printf(gettext("Adding %S to archive: %S"), &i_type, &info_quoi));
			}
		    }
		}

		if(fic != nullptr)
		{
		    shared_ptr<memory_file> delta_sig;

		    if(sem != nullptr)
			sem->raise(info_quoi, ino, true);

		    try // protecting delta_sig and putting "sem" in proper status in any case
		    {
			do // while "loop" is true, this is the INNER LOOP
			{
			    loop = false;
			    infinint start;
			    generic_file *source = nullptr;

				//////////////////////////////
				// preparing the source

			    try
			    {
				switch(keep_mode)
				{
				case cat_file::keep_compressed:
				    if(fic->get_compression_algo_read() != fic->get_compression_algo_write())
					keep_mode = cat_file::keep_hole;
				    source = fic->get_data(keep_mode, nullptr, signature_block_size, nullptr);
				    break;
				case cat_file::keep_hole:
				    if(delta_signature && !fic->get_sparse_file_detection_read())
				    {
					delta_sig.reset(new (nothrow) memory_file());
					if(!delta_sig)
					    throw Ememory("saved_inode");
					source = fic->get_data(cat_file::normal, delta_sig, signature_block_size, shared_ptr<memory_file>());
					if(display_treated)
					    dialog->message(tools_printf(gettext("building delta signature with block size of %d bytes"), signature_block_size));
				    }
				    else
					source = fic->get_data(keep_mode, nullptr, signature_block_size, nullptr);
				    break;
				case cat_file::normal:
				    if(delta_signature)
				    {
					delta_sig.reset(new (nothrow) memory_file());
					if(!delta_sig)
					    throw Ememory("save_inode");
					if(display_treated)
					    dialog->message(tools_printf(gettext("building delta signature with block size of %d bytes"), signature_block_size));
				    }

				    if(fic->get_sparse_file_detection_read()) // source file already holds a sparse_file structure
					source = fic->get_data(cat_file::plain, delta_sig, signature_block_size, delta_sig_ref, & result_crc);
					// we must hide the holes for it can be redetected
				    else
					source = fic->get_data(cat_file::normal, delta_sig, signature_block_size, delta_sig_ref, & result_crc);
				    break;
				case cat_file::plain:
				    throw SRC_BUG; // save_inode must never be called with this value
				default:
				    throw SRC_BUG; // unknown value for keep_mode
				}
			    }
			    catch(...)
			    {
				cat.pre_add_failed_mark(&pdesc);
				throw;
			    }



				//////////////////////////////
				// preparing the destination


			    if(source != nullptr)
			    {
				try
				{
				    sparse_file *dst_hole = nullptr;
				    infinint crc_size = tools_file_size_to_crc_size(fic->get_size());
				    crc * val = nullptr;
				    const crc * original = nullptr;
				    bool crc_available = false;
				    bool set_storage_size_to_zero = false; // only used in repair mode in case of missing CRC and impossibility to skip backward

				    source->skip(0);
				    source->read_ahead(0);

				    pdesc.stack->sync_write_above(pdesc.compr);
				    pdesc.compr->sync_write(); // necessary in any case to reset the compression engine to be able at later time to decompress starting at that position
				    if(keep_mode == cat_file::keep_compressed || fic->get_compression_algo_write() == compression::none)
					pdesc.compr->suspend_compression();
				    else
					pdesc.compr->resume_compression();

					// now that compression has reset we can fetch the location where the data will be dropped:
				    start = pdesc.stack->get_position();

				    try
				    {
					if(fic->get_sparse_file_detection_write()
					   && keep_mode != cat_file::keep_compressed
					   && keep_mode != cat_file::keep_hole
					   && !delta_diff)
					{
						// creating the sparse_file to copy data to destination

					    dst_hole = new (nothrow) sparse_file(pdesc.stack->top(), hole_size);
					    if(dst_hole == nullptr)
						throw Ememory("save_inode");
					    pdesc.stack->push(dst_hole);
					}

					try
					{

						//////////////////////////////
						// proceeding to file's data backup

					    source->copy_to(*pdesc.stack, crc_size, val);
					    if(val == nullptr)
						throw SRC_BUG;

						// crc must be read after the data has been copied
						// case of repairing, we use sequential reading

					    if(!compute_crc)
						crc_available = fic->get_crc(original);
					    else
						crc_available = false;
					}
					catch(...)
					{
					    bool fixed = false;
					    bool try_skip = false;

					    if(!repair_mode)
						throw;

					    dialog->printf(gettext("Failed reading data CRC for %S, file may be damaged and will be marked dirty"),
							  &info_quoi);
					    fic->set_dirty(true);

						// in the following we try to skip backward to set the storage size to zero
						// in both sequential data and ending generated catalog

					    if(pdesc.stack->truncatable(start))
					    {
						try
						{
						    pdesc.stack->truncate(start);
						    fixed = true;
						}
						catch(Ebug & e)
						{
						    throw;
						}
						catch(Egeneric & e)
						{
						    try_skip = true;
						}
					    }
					    else
						try_skip = true;

					    if(try_skip)
					    {
						infinint current_pos = pdesc.stack->get_position();
						if(pdesc.stack->skippable(generic_file::skip_backward, current_pos - start))
						{
						    if(!pdesc.stack->skip(start))
							pdesc.stack->skip(current_pos);
						    else
							fixed = true;
						}
					    }

					    val->clear();
					    fic->set_crc(*val);

					    if(!fixed)
					    {
						dialog->printf(gettext("Failed setting storage size to zero for this file with missing data CRC, CRC error will be reported for that file while reading the repaired archive"));
						set_storage_size_to_zero = true;
					    }

					    if(fic->has_delta_signature_structure())
						fic->clear_delta_signature_only();
					}


					    //////////////////////////////
					    // checking crc value and storing it in catalogue

					if(compute_crc)
					    fic->set_crc(*val);
					else
					{
					    if(keep_mode == cat_file::normal
					       && crc_available
					       && !fic->get_sparse_file_detection_read())
						    // crc is calculated based on the stored uncompressed data
						    // we can compare only if the data is uncompressed (not
						    // keep_compressed and hole structure inlined in data
						    // is kept as is (not in nomal mode when the source has
						    // a sparse_file layer
					    {
						if(original == nullptr)
						    throw SRC_BUG;
						if(typeid(*original) != typeid(*val))
						    throw SRC_BUG;
						if(*original != *val)
						    throw Erange("save_inode", gettext("Copied data does not match CRC"));
					    }
					}

					    //////////////////////////////
					    // checking whether saved files used sparse_file datastructure

					if(dst_hole != nullptr)
					{
					    pdesc.stack->sync_write_above(dst_hole);
					    dst_hole->sync_write();
					    if(!dst_hole->has_seen_hole() && !dst_hole->has_escaped_data())
						fic->set_sparse_file_detection_write(false);
						// here we drop the sparse_file datastructure as no hole
						// could be read. This will speed up extraction when used
						// normally (not with sequential reading, as the inode info
						// is already written to file and cannot be changed.
						// Reading as sparse_file datastructure a plain normal data
						// is possible while there is no data to escape, this is just
						// a bit more slower.).
					}
					fichier_global *s_fic = dynamic_cast<fichier_global *>(source);
					try
					{
					    if(s_fic != nullptr)
						s_fic->fadvise(fichier_global::advise_dontneed);
					}
					catch(Erange & e)
					{
					    dialog->message(tools_printf(gettext("Failed to set fadvise() for file %S, this is not crucial, no problem"), &info_quoi));
					}

					source->terminate();

					    // we need this for amount of data written to be properly calculated
					pdesc.stack->sync_write_above(pdesc.compr);
					pdesc.compr->sync_write();
				    }
				    catch(...)
				    {
					if(val != nullptr)
					{
					    delete val;
					    val = nullptr;
					}

					if(dst_hole != nullptr)
					{
					    if(pdesc.stack->pop() != dst_hole)
						throw SRC_BUG;
					    delete dst_hole;
					    dst_hole = nullptr;
					}

					throw;
				    }

				    if(val != nullptr)
				    {
					delete val;
					val = nullptr;
				    }

				    if(dst_hole != nullptr)
				    {
					dst_hole->terminate();
					if(pdesc.stack->pop() != dst_hole)
					    throw SRC_BUG;
					delete dst_hole;
					dst_hole = nullptr;
				    }

				    if(pdesc.stack->get_position() >= start || set_storage_size_to_zero)
				    {
					if(set_storage_size_to_zero)
					    storage_size = 0;
					else
					    storage_size = pdesc.stack->get_position() - start;
				    }
				    else
					throw SRC_BUG;
				}
				catch(...)
				{
				    delete source;
				    fic->clean_data();
				    source = nullptr;

					// restore atime of source
				    if(!alter_atime)
					tools_noexcept_make_date(info_quoi, false, ino->get_last_access(), ino->get_last_modif(), ino->get_last_modif());
				    throw;
				}
				delete source;
				fic->clean_data();
				source = nullptr;

				    //////////////////////////////
				    // adding the data CRC if escape marks are used

				cat.pre_add_crc(ino, &pdesc);

				    //////////////////////////////
				    // checking if compressed data is smaller than uncompressed one

				if(! never_resave_uncompressed
				   && fic->get_size() <= storage_size
				   && keep_mode != cat_file::keep_compressed
				   && fic->get_compression_algo_write() != compression::none
				   && !repair_mode)
				{
				    infinint current_pos_tmp = pdesc.stack->get_position();
				    bool try_skip = false;

				    if(current_pos_tmp < rewinder)
					throw SRC_BUG; // we are positionned before the start of the current inode dump!

				    if(pdesc.stack->truncatable(rewinder))
				    {
					try
					{
					    pdesc.stack->truncate(rewinder);
					    if(!resave_uncompressed)
						resave_uncompressed = true;
					    else
						throw SRC_BUG; // should only be tried once per inode
					    fic->change_compression_algo_write(compression::none);
					    break; // stop the inner loop
					}
					catch(Ebug & e)
					{
					    throw;
					}
					catch(...)
					{
					    try_skip = true;
					}
				    }
				    else
					try_skip = true;

				    if(try_skip && pdesc.stack->skippable(generic_file::skip_backward, current_pos_tmp - rewinder))
				    {
					try
					{
					    if(!pdesc.stack->skip(rewinder))
						throw Erange("save_inode","skipping was possible in theory but not in reality");
						// this situation may arise when some data
						// is pending to be written (cache layer) and
						// before skipping back leads the cache to flush
						// his data to the underlying layer (sar for example)
						// which may leads to the creation to a new slice and
						// then skipping back is no more possible as it was before
					    if(!resave_uncompressed)
						resave_uncompressed = true;
					    else
						throw SRC_BUG; // should only be tried once per inode
					    fic->change_compression_algo_write(compression::none);
					    break; // stop the inner loop
					}
					catch(Ebug & e)
					{
					    throw;
					}
					catch(...)
					{
					    if(info_details)
						dialog->message(info_quoi + gettext(" : Failed resaving uncompressed the inode data"));
						// ignoring the error and continuing
					    resave_uncompressed = false;
					    if(pdesc.stack->get_position() != current_pos_tmp)
						throw SRC_BUG;
					}
				    }
				    else
				    {
					if(info_details)
					    dialog->message(info_quoi  + gettext(" : Resaving uncompressed the inode data to gain space is not possible, keeping data compressed"));
				    }
				}
				else
				    resave_uncompressed = false;


				if(!resave_uncompressed)
				{
					// in repair_mode, the file offset points to the data
					// in the damaged archive, it must not be set before the
					// data is read at which time it records the offset in the
					// repaired archive, the offset may change espetially when
					// encryption is used, due to variable sized elastic buffers
					//
					// another situation is while merging an recompressing
					// data, if the new compression gives less good result than
					// the actual, resave_uncompressed will be tried and
					// need to set back the source data (fic->get_offset())
					// to the position in the source archive so we must
					// not overwrite this value until we know we won't need it
					// anymore
				    fic->set_offset(start);
				    fic->set_storage_size(storage_size);
				}


				    //////////////////////////////
				    // checking for last_modification date change

				if(check_change)
				{
				    bool changed = false;

				    try
				    {
					changed = (fic->get_last_modif() != tools_get_mtime(*dialog,
											    info_quoi,
											    true,
											    true, // silently set to zero negative dates
											    ignored_as_symlink));
				    }
				    catch(Erange & e)
				    {
					dialog->message(tools_printf(gettext("File has disappeared while we were reading it, cannot check whether it has changed during its backup: %S"), &info_quoi));
					changed = false;
				    }

				    if(changed)
				    {
					if(current_repeat_count < repeat_count)
					{
					    current_repeat_count++;
					    infinint current_pos_tmp = pdesc.stack->get_position();

					    try
					    {
						bool try_skip = false;

						if(pdesc.stack->truncatable(start))
						{
						    try
						    {
							pdesc.stack->truncate(start);
						    }
						    catch(Ebug & e)
						    {
							throw;
						    }
						    catch(...)
						    {
							try_skip = true;
						    }
						}
						else
						    try_skip = true;

						if(try_skip)
						{
						    if(pdesc.stack->skippable(generic_file::skip_backward, storage_size))
						    {
							if(!pdesc.stack->skip(start))
							{
							    if(!pdesc.stack->skip(start))
								throw SRC_BUG;
							    throw Erange("",""); // used locally
							}
						    }
						    else
							throw Erange("",""); // used locally, not propagated over this try / catch block
						}
					    }
					    catch(...)
					    {
						current_wasted_bytes += current_pos_tmp - start;
						if(pdesc.stack->get_position() != current_pos_tmp)
						    throw SRC_BUG;
					    }

					    if(repeat_byte.is_zero() || (current_wasted_bytes < repeat_byte))
					    {
						if(info_details)
						    dialog->message(tools_printf(gettext("WARNING! File modified while reading it for backup. Performing retry %i of %i"), &current_repeat_count, &repeat_count));
						if(pdesc.stack->get_position() != start)
						    cat.pre_add_waste_mark(&pdesc);
						loop = true;

						    // updating the last modification date of file
						fic->set_last_modif(tools_get_mtime(*dialog,
										    info_quoi,
										    true,
										    true, // silently set to zero negative date
										    ignored_as_symlink));

						    // updating the size of the file
						fic->change_size(tools_get_size(info_quoi));
					    }
					    else
					    {
						dialog->message(string(gettext("WARNING! File modified while reading it for backup. No more retry for that file to not exceed the wasted byte limit. File is ")) + info_quoi);
						fic->set_dirty(true);
						ret = false;
					    }
					}
					else
					{
					    dialog->message(string(gettext("WARNING! File modified while reading it for backup, but no more retry allowed: ")) + info_quoi);
					    fic->set_dirty(true);
					    cat.pre_add_dirty(&pdesc); // when in sequential reading
					    ret = false;
					}
				    }
				}

				    //////////////////////////////
				    // restore atime of source

				if(!alter_atime)
				    tools_noexcept_make_date(info_quoi, false, ino->get_last_access(), ino->get_last_modif(), ino->get_last_modif());

				    //////////////////////////////
				    // dumping delta signature if present or just calculated

				if(fic->has_delta_signature_structure() && !loop)
				{
				    U_I block_size = signature_block_size; // by default signature has been calculated with this given block size

				    if(display_treated)
					dialog->message(string(gettext("Dumping delta signature structure for saved file: ")) + info_quoi);

				    if(!delta_sig) // no delta_sig got calculated during this save_inode() execution
				    {
					if(!delta_diff)
					{
						// merging context, signature not calculated here but already existing: we need to transfer it
					    if(fic->has_delta_signature_available() || repair_mode)
						fic->read_delta_signature(delta_sig, block_size);
						// overriden block_size by the value of the signature
						// we have just read. As we won't recalculate it, in the present
						// case, we have to properly record its block size
					}
					    // else delta diff without delta signature, storing en empty zero length signature
					    // delta_sig stays equal to nullptr
				    }

				    if(!fic->has_patch_result_crc())
				    {
					const crc *tmp = nullptr;

					switch(fic->get_saved_status())
					{
					case saved_status::saved:
					    fic->get_crc(tmp);
					    break;
					case saved_status::fake:
					    throw SRC_BUG;
					case saved_status::not_saved:
					    if(ref_fic == nullptr)
						throw SRC_BUG;
					    if(!ref_fic->has_patch_result_crc())
						throw SRC_BUG;
					    ref_fic->get_patch_result_crc(tmp);
					    break;
					case saved_status::delta:
					    if(result_crc == nullptr)
						throw SRC_BUG;
					    tmp = result_crc;
					    break;
					default:
					    throw SRC_BUG;
					}

					if(tmp == nullptr)
					    throw SRC_BUG;
					fic->set_patch_result_crc(*tmp);
				    }

					// adding a tape mark when in sequential read mode
				    cat.pre_add_delta_sig(&pdesc);
				    pdesc.compr->suspend_compression();

					// dropping the data to the archive and recording its location in the cat_file object
				    try
				    {
					if(delta_sig)
					    fic->dump_delta_signature(delta_sig, block_size, *(pdesc.compr), pdesc.esc != nullptr);
					else
					    fic->dump_delta_signature(*(pdesc.compr), pdesc.esc != nullptr);
				    }
				    catch(...)
				    {
					fic->drop_delta_signature_data();
					throw;
				    }
				    fic->drop_delta_signature_data();
				}
			    }
			    else
				throw SRC_BUG; // saved_status == saved_status::saved, but no data available, and no exception raised;
			}
			while(loop); // INNER LOOP
		    }
		    catch(...)
		    {
			if(sem != nullptr)
			    sem->lower();
			fic->drop_delta_signature_data(); // no more needed delta signature data if present
			throw;
		    }
		    if(sem != nullptr)
			sem->lower();
		    fic->drop_delta_signature_data(); // no more needed delta signature data if present
		}
		else // fic == nullptr
		    if(sem != nullptr)
		    {
			sem->raise(info_quoi, ino, true);
			sem->lower();
		    }
	    }
	    while(resave_uncompressed); // OUTER LOOP

	}
	catch(...)
	{
	    if(ref_fic != nullptr)
		ref_fic->drop_delta_signature_data();
	    throw;
	}
	if(ref_fic != nullptr)
	    ref_fic->drop_delta_signature_data();

	return ret;
    }

    static bool save_ea(const shared_ptr<user_interaction> & dialog,
			const string & info_quoi,
			cat_inode * & ino,
			const pile_descriptor & pdesc,
			bool display_treated,
			bool repair_mode)
    {
        bool ret = false;
        try
        {
            switch(ino->ea_get_saved_status())
            {
            case ea_saved_status::full: // if there is something to save
		if(ino->get_ea() != nullptr)
		{
		    crc * val = nullptr;
		    infinint start;

		    try
		    {
			if(display_treated)
			    dialog->message(string(gettext("Saving Extended Attributes for ")) + info_quoi);
			if(pdesc.compr->is_compression_suspended())
			{
			    pdesc.stack->sync_write_above(pdesc.compr);
			    pdesc.compr->resume_compression();  // always compress EA (no size or filename consideration)
			}
			else
			{
			    pdesc.stack->sync_write_above(pdesc.compr);
			    pdesc.compr->sync_write(); // reset the compression engine to be able to decompress from that point later
			}

			start = pdesc.stack->get_position();
			pdesc.stack->reset_crc(tools_file_size_to_crc_size(ino->ea_get_size())); // start computing CRC for any read/write on stack
			try
			{
			    ino->get_ea()->dump(*pdesc.stack);
			}
			catch(...)
			{
			    val = pdesc.stack->get_crc(); // this keeps "stack" in a coherent status
			    throw;
			}

			ino->ea_set_offset(start);
			val = pdesc.stack->get_crc();
			if(repair_mode)
			{
			    const crc *tmp = nullptr;

			    ino->ea_get_crc(tmp);
			    if(tmp == nullptr)
				throw SRC_BUG;
			    if(*tmp != *val)
			    {
				dialog->printf(gettext("Computed EA CRC for file %S differs from what was stored in the archive, this file's EA may have been corrupted"),
					      &info_quoi);
				ino->ea_set_crc(*val);
			    }
			}
			else
			    ino->ea_set_crc(*val);
			ino->ea_detach();
			ret = true;
		    }
		    catch(...)
		    {
			if(val != nullptr)
			    delete val;
			throw;
		    }
		    if(val != nullptr)
			delete val;
		}
		else
		    throw SRC_BUG;
		break;
            case ea_saved_status::partial:
	    case ea_saved_status::none:
		break;
	    case ea_saved_status::fake:
                throw SRC_BUG; //filesystem, must not provide inode in such a status
	    case ea_saved_status::removed:
		throw SRC_BUG; //filesystem, must not provide inode in such a status
            default:
                throw SRC_BUG;
            }
        }
        catch(Ebug & e)
        {
            throw;
        }
	catch(Ethread_cancel & e)
	{
	    throw;
	}
        catch(Egeneric & e)
        {
	    Euser_abort* e_abort = dynamic_cast<Euser_abort*>(&e);
	    if(e_abort != nullptr && ! repair_mode)
		throw;

            dialog->message(string(gettext("Error saving Extended Attributes for ")) + info_quoi + ": " + e.get_message());
	    if(repair_mode)
	    {
		ino->ea_set_saved_status(ea_saved_status::none);
		dialog->message(gettext("be advised that a CRC error will be reported for the EA of that file while sequentially reading the repaired archive"));
	    }
        }
        return ret;
    }

    static void restore_atime(const string & chemin, const cat_inode * & ptr)
    {
	const cat_file * ptr_f = dynamic_cast<const cat_file *>(ptr);
	if(ptr_f != nullptr)
	    tools_noexcept_make_date(chemin, false, ptr_f->get_last_access(), ptr_f->get_last_modif(), ptr_f->get_last_modif());
    }

    static bool save_fsa(const shared_ptr<user_interaction> & dialog,
			 const string & info_quoi,
			 cat_inode * & ino,
			 const pile_descriptor & pdesc,
			 bool display_treated,
			 bool repair_mode)
    {
        bool ret = false;
        try
        {
            switch(ino->fsa_get_saved_status())
            {
            case fsa_saved_status::full: // if there is something to save
		if(ino->get_fsa() != nullptr)
		{
		    crc * val = nullptr;
		    infinint start;

		    try
		    {
			if(display_treated)
			    dialog->message(string(gettext("Saving Filesystem Specific Attributes for ")) + info_quoi);
			if(pdesc.compr->get_algo() != compression::none)
			{
			    pdesc.stack->sync_write_above(pdesc.compr);
			    pdesc.compr->suspend_compression(); // never compress FSA (no size or filename consideration)
			}

			start = pdesc.stack->get_position();
			pdesc.stack->reset_crc(tools_file_size_to_crc_size(ino->fsa_get_size())); // start computing CRC for any read/write on stack
			try
			{
			    ino->get_fsa()->write(*pdesc.stack);
			}
			catch(...)
			{
			    val = pdesc.stack->get_crc(); // this keeps "" in a coherent status
			    ino->fsa_set_crc(*val);
			    ino->fsa_set_offset(start);
				// and we also keep ino in a coherent status
			    throw;
			}

			ino->fsa_set_offset(start);
			val = pdesc.stack->get_crc();
			if(repair_mode)
			{
			    const crc *tmp = nullptr;

			    ino->fsa_get_crc(tmp);
			    if(tmp == nullptr)
				throw SRC_BUG;
			    if(*tmp != *val)
			    {
				dialog->printf(gettext("Computed FSA CRC for file %S differs from what was stored in the archive, this file's EA may have been corrupted"),
					      &info_quoi);
				ino->fsa_set_crc(*val);
			    }
			}
			else
			    ino->fsa_set_crc(*val);
			ino->fsa_detach();
			ret = true;

			    // compression is left suspended, save_inode, save_ea, will change or catalogue dump will change it if necessary
		    }
		    catch(...)
		    {
			if(val != nullptr)
			    delete val;
			throw;
		    }
		    if(val != nullptr)
			delete val;
		}
		else
		    throw SRC_BUG;
		break;
            case fsa_saved_status::partial:
	    case fsa_saved_status::none:
		break;
            default:
                throw SRC_BUG;
            }
        }
        catch(Ebug & e)
        {
            throw;
        }
	catch(Ethread_cancel & e)
	{
	    throw;
	}
        catch(Egeneric & e)
        {
	    Euser_abort* e_abort = dynamic_cast<Euser_abort*>(&e);
	    if(e_abort != nullptr && ! repair_mode)
		throw;

            dialog->message(string(gettext("Error saving Filesystem Specific Attributes for ")) + info_quoi + ": " + e.get_message());
	    if(repair_mode)
	    {
		ino->fsa_set_saved_status(fsa_saved_status::none);
		dialog->message(gettext("be advised that a CRC error will be reported for the FSA of that file while sequentially reading the repaired archive"));
	    }
        }
        return ret;
    }

    static void do_EFSA_transfert(const shared_ptr<user_interaction> &dialog,
				  over_action_ea action,
				  cat_inode *place_ino,
				  const cat_inode *add_ino)
    {
	ea_attributs *tmp_ea = nullptr;
	filesystem_specific_attribute_list *tmp_fsa = nullptr;

	switch(action)
	{
	case EA_overwrite:
	case EA_overwrite_mark_already_saved:
	case EA_merge_preserve:
	case EA_merge_overwrite:
	    break;
	case EA_preserve:
	case EA_preserve_mark_already_saved:
	case EA_clear:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}

	if(add_ino == nullptr) // to_add is not an inode thus cannot have any EA
	    return;         // we do nothing in any case as there is not different EA set in conflict

	if(place_ino == nullptr) // in_place is not an inode thus cannot have any EA
	    return;           // nothing can be done neither here as the resulting object (in_place) cannot handle EA

	    // in the following we know that both in_place and to_add are inode,
	    //we manipulate them thanks to their cat_inode * pointers (place_ino and add_ino)

	switch(action) // action concerns both EA and FSA in spite of the name of its values
	{
	case EA_overwrite:

		// overwriting last change date

	    if(add_ino->has_last_change())
		place_ino->set_last_change(add_ino->get_last_change());

		// EA Consideration (for overwriting)

	    switch(add_ino->ea_get_saved_status())
	    {
	    case ea_saved_status::none:
	    case ea_saved_status::removed:
		place_ino->ea_set_saved_status(ea_saved_status::none);
		break;
	    case ea_saved_status::partial:
	    case ea_saved_status::fake:
		place_ino->ea_set_saved_status(ea_saved_status::partial);
		break;
	    case ea_saved_status::full:
		tmp_ea = new (nothrow) ea_attributs(*add_ino->get_ea()); // we clone the EA of add_ino
		if(tmp_ea == nullptr)
		    throw Ememory("filtre::do_EFSA_transfert");
		try
		{
		    if(place_ino->ea_get_saved_status() == ea_saved_status::full) // then we must drop the old EA:
			place_ino->ea_detach();
		    else
			place_ino->ea_set_saved_status(ea_saved_status::full);
		    place_ino->ea_attach(tmp_ea);
		    tmp_ea = nullptr;
		}
		catch(...)
		{
		    if(tmp_ea != nullptr)
		    {
			delete tmp_ea;
			tmp_ea = nullptr;
		    }
		    throw;
		}
		break;
	    default:
		throw SRC_BUG;
	    }

		// FSA Considerations (for overwriting)

	    switch(add_ino->fsa_get_saved_status())
	    {
	    case fsa_saved_status::none:
		place_ino->fsa_set_saved_status(fsa_saved_status::none);
		break;
	    case fsa_saved_status::partial:
		place_ino->fsa_set_saved_status(fsa_saved_status::partial);
		place_ino->fsa_partial_attach(add_ino->fsa_get_families());
		break;
	    case fsa_saved_status::full:
		tmp_fsa = new (nothrow) filesystem_specific_attribute_list(*add_ino->get_fsa()); // we clone the FSA of add_ino
		if(tmp_fsa == nullptr)
		    throw Ememory("filtre::do_EFSA_transfer");
		try
		{
		    if(place_ino->fsa_get_saved_status() == fsa_saved_status::full) // we must drop the old FSA
			place_ino->fsa_detach();
		    else
			place_ino->fsa_set_saved_status(fsa_saved_status::full);
		    place_ino->fsa_attach(tmp_fsa);
		    tmp_fsa = nullptr;
		}
		catch(...)
		{
		    if(tmp_fsa != nullptr)
		    {
			delete tmp_fsa;
			tmp_fsa = nullptr;
		    }
		    throw;
		}
		break;
	    default:
		throw SRC_BUG;
	    }
	    break; // end of case EA_FSA_overwrite for action

	case EA_overwrite_mark_already_saved:

		// Overwriting Date

	    if(add_ino->has_last_change())
		place_ino->set_last_change(add_ino->get_last_change());

		// EA considerations

	    place_ino->ea_set_saved_status(add_ino->ea_get_saved_status()); // at this step, ea_full may be set, it will be changed to ea_partial below.
	    if(place_ino->ea_get_saved_status() == ea_saved_status::full || place_ino->ea_get_saved_status() == ea_saved_status::fake)
		place_ino->ea_set_saved_status(ea_saved_status::partial);

		// FSA considerations

	    place_ino->fsa_set_saved_status(add_ino->fsa_get_saved_status()); // at this step fsa_full may be set, will be changed to fsa_partial below
	    if(place_ino->fsa_get_saved_status() == fsa_saved_status::full)
		place_ino->fsa_set_saved_status(fsa_saved_status::partial);

	    break;

	case EA_merge_preserve:

		// no last change date modification (preserve context)

		// EA considerations

	    if(place_ino->ea_get_saved_status() == ea_saved_status::full && add_ino->ea_get_saved_status() == ea_saved_status::full) // we have something to merge
	    {
		tmp_ea = new (nothrow) ea_attributs();
		if(tmp_ea == nullptr)
		    throw Ememory("filtre.cpp:do_EFSA_transfert");
		try
		{
		    merge_ea(*place_ino->get_ea(), *add_ino->get_ea(), *tmp_ea);
		    place_ino->ea_detach();
		    place_ino->ea_attach(tmp_ea);
		    tmp_ea = nullptr;
		}
		catch(...)
		{
		    if(tmp_ea != nullptr)
		    {
			delete tmp_ea;
			tmp_ea = nullptr;
		    }
		    throw;
		}
	    }
	    else
		if(add_ino->ea_get_saved_status() == ea_saved_status::full)
		{
		    place_ino->ea_set_saved_status(ea_saved_status::full); // it was not the case else we would have executed the above block
		    tmp_ea = new (nothrow) ea_attributs(*add_ino->get_ea());   // we clone the EA set of to_add
		    if(tmp_ea == nullptr)
			throw Ememory("filtre.cpp:do_EFSA_transfert");
		    try
		    {
			place_ino->ea_attach(tmp_ea);
			tmp_ea = nullptr;
		    }
		    catch(...)
		    {
			if(tmp_ea != nullptr)
			{
			    delete tmp_ea;
			    tmp_ea = nullptr;
			}
			throw;
		    }
		}
		// else nothing is done: either res_ino has full EA but ref_ino has not
		// or res_ino has not full EA nor do has ref_ino and nothing can be done neither


		// FSA considerations

	    if(place_ino->fsa_get_saved_status() == fsa_saved_status::full && add_ino->fsa_get_saved_status() == fsa_saved_status::full) // we have something to merge
	    {
		tmp_fsa = new (nothrow) filesystem_specific_attribute_list();
		if(tmp_fsa == nullptr)
		    throw Ememory("filtre.cpp::do_EFSA_transfer");

		try
		{
		    *tmp_fsa = *add_ino->get_fsa() + *place_ino->get_fsa(); // overwriting add_ino with place_ino's FSA
		    place_ino->fsa_detach();
		    place_ino->fsa_attach(tmp_fsa);
		    tmp_fsa = nullptr;
		}
		catch(...)
		{
		    if(tmp_fsa != nullptr)
		    {
			delete tmp_fsa;
			tmp_fsa = nullptr;
		    }
		    throw;
		}
	    }
	    else
	    {
		if(add_ino->fsa_get_saved_status() == fsa_saved_status::full)
		{
		    place_ino->fsa_set_saved_status(fsa_saved_status::full);
		    tmp_fsa = new (nothrow) filesystem_specific_attribute_list(*add_ino->get_fsa());
		    if(tmp_fsa == nullptr)
			throw Ememory("filtre.cpp:do_EFSA_transfert");
		    try
		    {
			place_ino->fsa_attach(tmp_fsa);
			tmp_fsa = nullptr;
		    }
		    catch(...)
		    {
			if(tmp_fsa != nullptr)
			{
			    delete tmp_fsa;
			    tmp_fsa = nullptr;
			}
			throw;
		    }
		}
		    // else nothing to be done (in_place eventually has FSA and add_ino does nothing to add)
	    }

	    break;

	case EA_merge_overwrite:

		// last change date transfert

	    if(add_ino->has_last_change())
		place_ino->set_last_change(add_ino->get_last_change());

		// EA considerations

	    if(place_ino->ea_get_saved_status() == ea_saved_status::full && add_ino->ea_get_saved_status() == ea_saved_status::full)
	    {
		tmp_ea = new (nothrow) ea_attributs();
		if(tmp_ea == nullptr)
		    throw Ememory("filtre.cpp:do_EFSA_transfert");
		try
		{
		    merge_ea(*add_ino->get_ea(), *place_ino->get_ea(), *tmp_ea);
		    place_ino->ea_detach();
		    place_ino->ea_attach(tmp_ea);
		    tmp_ea = nullptr;
		}
		catch(...)
		{
		    if(tmp_ea != nullptr)
		    {
			delete tmp_ea;
			tmp_ea = nullptr;
		    }
		    throw;
		}
	    }
	    else
		if(add_ino->ea_get_saved_status() == ea_saved_status::full)
		{
		    place_ino->ea_set_saved_status(ea_saved_status::full); // it was not the case else we would have executed the above block
		    tmp_ea = new (nothrow) ea_attributs(*add_ino->get_ea());
		    if(tmp_ea == nullptr)
			throw Ememory("filtre.cpp:do_EFSA_transfert");
		    try
		    {
			place_ino->ea_attach(tmp_ea);
			tmp_ea = nullptr;
		    }
		    catch(...)
		    {
			if(tmp_ea != nullptr)
			{
			    delete tmp_ea;
			    tmp_ea = nullptr;
			}
			throw;
		    }
		}
		// else nothing is done: either res_ino has full EA but ref_ino has not
		// or res_ino has not full EA nor do has ref_ino and nothing can be done neither

		// FSA considerations

	    if(place_ino->fsa_get_saved_status() == fsa_saved_status::full && add_ino->fsa_get_saved_status() == fsa_saved_status::full) // we have something to merge
	    {
		tmp_fsa = new (nothrow) filesystem_specific_attribute_list();
		if(tmp_fsa == nullptr)
		    throw Ememory("filtre.cpp::do_EFSA_transfer");

		try
		{
		    *tmp_fsa = *place_ino->get_fsa() + *add_ino->get_fsa(); // overwriting place_ino with add_ino's FSA
		    place_ino->fsa_detach();
		    place_ino->fsa_attach(tmp_fsa);
		    tmp_fsa = nullptr;
		}
		catch(...)
		{
		    if(tmp_fsa != nullptr)
		    {
			delete tmp_fsa;
			tmp_fsa = nullptr;
		    }
		    throw;
		}
	    }
	    else
	    {
		if(add_ino->fsa_get_saved_status() == fsa_saved_status::full)
		{
		    place_ino->fsa_set_saved_status(fsa_saved_status::full);
		    tmp_fsa = new (nothrow) filesystem_specific_attribute_list(*add_ino->get_fsa());
		    if(tmp_fsa == nullptr)
			throw Ememory("filtre.cpp:do_EFSA_transfert");
		    try
		    {
			place_ino->fsa_attach(tmp_fsa);
			tmp_fsa = nullptr;
		    }
		    catch(...)
		    {
			if(tmp_fsa != nullptr)
			{
			    delete tmp_fsa;
			    tmp_fsa = nullptr;
			}
			throw;
		    }
		}
		    // else nothing to be done (in_place eventually has FSA and add_ino does nothing to add)
	    }


	    break;

	default:
	    throw SRC_BUG;
	}
    }

    static void merge_ea(const ea_attributs & ref1, const ea_attributs & ref2, ea_attributs &res)
    {
	string ent_key, ent_val;
	string val;

	res = ref1; // assignment operator
	res.reset_read();

	ref2.reset_read();
	while(ref2.read(ent_key, ent_val))
	    if(!res.find(ent_key, val))
		res.add(ent_key, ent_val);
    }


    static cat_entree *make_clone(const cat_nomme *ref,
				  map<infinint, cat_etoile*> & hard_link_base,
				  const infinint & etiquette_offset)
    {
	cat_entree *dolly = nullptr; // will be the address of the cloned object
	string the_name;
	const cat_mirage *ref_mir = dynamic_cast<const cat_mirage *>(ref);

	if(ref == nullptr)
	    throw SRC_BUG;

	the_name = ref->get_name();


	if(ref_mir != nullptr) // this is hard linked inode
	{
		// check whether this is the first time we see this file (in list of file covered by the file masks)
	    map <infinint, cat_etoile *>::iterator it = hard_link_base.find(ref_mir->get_etiquette() + etiquette_offset);
	    if(it == hard_link_base.end()) // this inode has not been yet recorded in the resulting archive
	    {
		cat_etoile *filante = nullptr;
		dolly = ref_mir->get_inode()->clone(); // we must clone the attached inode
		try
		{
		    cat_inode *dollinode = dynamic_cast<cat_inode *>(dolly);

		    if(dollinode == nullptr)
			throw Ememory("filtre:make_clone");

		    infinint shift_etiquette = ref_mir->get_etiquette() + etiquette_offset;
		    filante = new (nothrow) cat_etoile(dollinode, shift_etiquette);
		    if(filante == nullptr)
			throw Ememory("make_clone");
		    try
		    {
			dolly = nullptr; // the inode is now managed by filante
			dolly = new (nothrow) cat_mirage(the_name, filante);
			if(dolly == nullptr)
			    throw Ememory("make_clone");
			try
			{
			    hard_link_base[shift_etiquette] = filante; // we now record this file_etiquette in the map of already enrolled hard_link sets
			}
			catch(...)
			{
			    filante = nullptr; // now managed by the cat_mirage pointed to by dolly
			    throw;
			}
		    }
		    catch(...)
		    {
			if(filante != nullptr)
			{
			    delete filante;
			    filante = nullptr;
			}
			throw;
		    }
		}
		catch(...)
		{
		    if(dolly != nullptr)
		    {
			delete dolly;
			dolly = nullptr;
		    }
		    throw;
		}
	    }
	    else // already added to archive
		dolly = new (nothrow) cat_mirage(the_name, it->second); // we make a new cat_mirage pointing to the cat_etoile already involved in the catalogue under construction
	}
	else // not a hard_link file
	    dolly = ref->clone();  // we just clone the entry

	if(dolly == nullptr)
	    throw Ememory("make_clone");

	return dolly;
    }


    static void clean_hard_link_base_from(const cat_mirage *mir, map<infinint, cat_etoile *> & hard_link_base)
    {
	if(mir->get_etoile_ref_count().is_zero())
	    throw SRC_BUG; // count should be >= 1

	if(mir->get_etoile_ref_count() == 1)
	{
	    map<infinint, cat_etoile *>::iterator it = hard_link_base.find(mir->get_etiquette());
	    const cat_inode *al_ptr_ino = mir->get_inode();
	    if(al_ptr_ino == nullptr)
		throw SRC_BUG;
	    if(it == hard_link_base.end())
		throw SRC_BUG; // the cat_etoile object pointed to by dolly_mir should be known by corres_copy
	    hard_link_base.erase(it);
	}
    }

    static void normalize_link_base(map<infinint, cat_etoile *> & hard_link_base)
    {
	infinint max_val = 0;
	map<infinint, cat_etoile *>::iterator it, ut;
	infinint num_etoile = hard_link_base.size();
	infinint search = 0;

	    // first pass, looking highest etiquette number

	for(it = hard_link_base.begin(); it != hard_link_base.end(); ++it)
	{
	    if(it->first > max_val)
		max_val = it->first;
	}

	    // second pass, looking for holes and moving highest value to fill the gap in

	while(search < num_etoile)
	{
	    it = hard_link_base.find(search);
	    if(it == hard_link_base.end())
	    {
		    // unused etiquette value, looking for the higest value
		do
		{
		    if(max_val <= search)
			throw SRC_BUG; // num_etoile > 0, but we cannot find a cat_etoile to move into the hole
		    ut = hard_link_base.find(max_val);
		    --max_val;
		}
		while(ut == hard_link_base.end());

		    // we can now renumber cat_etoile tmp from max_val to search

		cat_etoile *tmp = ut->second;
		if(tmp == nullptr)
		    throw SRC_BUG;
		tmp->change_etiquette(search);
		hard_link_base.erase(ut);
		hard_link_base[search] = tmp;
	    }
	    ++search;
	}
    }

    static const crit_action *make_overwriting_for_only_deleted()
    {
	const crit_action *ret = new (nothrow) testing(crit_invert(crit_in_place_is_inode()), crit_constant_action(data_preserve, EA_preserve), crit_constant_action(data_overwrite, EA_overwrite));
	if(ret == nullptr)
	    throw Ememory("make_overwriting_fir_only_deleted");

	return ret;
    }

    static void save_delta_signature(const shared_ptr<user_interaction> & dialog,
				     const string & info_quoi,
				     cat_file * e_file,
				     const cat_file * ref_file,
				     const pile_descriptor & pdesc,
				     U_I signature_block_size,
				     bool info_details,
				     bool display_treated,
				     const catalogue & cat)
    {
	if(e_file != nullptr
	   && e_file->has_delta_signature_structure())
	{
	    shared_ptr<memory_file> sig;
	    U_I block_size = signature_block_size;

	    try
	    {
		if(ref_file != nullptr
		   && ref_file->has_delta_signature_structure())
		{
		    if(display_treated)
			dialog->message(string(gettext("Copying delta signature structure from the archive of reference: ")) + info_quoi);

		    if(ref_file->has_delta_signature_available())
		    {
			ref_file->read_delta_signature(sig, block_size);
			    // overwriting block size to fit the block
			    // size used to create this signature
			if(!sig)
			    throw SRC_BUG;
		    }

		    if(ref_file->has_patch_result_crc())
		    {
			const crc *tmp;
			ref_file->get_patch_result_crc(tmp);
			e_file->set_patch_result_crc(*tmp);
		    }
		    else
			throw SRC_BUG;

		}
		else // no ref_file inode, trying to calculate the delta signature from data
		{
		    if(e_file->can_get_data()
		       && e_file->get_saved_status() != saved_status::delta) // provided data is not a delta signature
		    {
			null_file trou_noir(gf_write_only);
			generic_file *data = nullptr;

			if(display_treated)
			    dialog->message(string(gettext("Calculating delta signature from filesystem: "))
					    + info_quoi);

			try // protecting data
			{
			    infinint crc_size = tools_file_size_to_crc_size(e_file->get_size());
			    crc *patch_sig_crc = nullptr;

			    try // protecting patch_sig_crc
			    {
				sig.reset(new (nothrow) memory_file());
				if(!sig)
				    throw Ememory("filtre_sauvegarde");

				data = e_file->get_data(cat_file::normal, sig, block_size, nullptr);
				if(data == nullptr)
				    throw Ememory("filtre_sauvegarde");
				data->copy_to(trou_noir, crc_size, patch_sig_crc);
				if(patch_sig_crc == nullptr)
				    throw SRC_BUG;
				e_file->set_patch_base_crc(*patch_sig_crc);
				e_file->set_patch_result_crc(*patch_sig_crc);
			    }
			    catch(...)
			    {
				if(patch_sig_crc != nullptr)
				    delete patch_sig_crc;
				throw;
			    }
			    if(patch_sig_crc != nullptr)
				delete patch_sig_crc;
			}
			catch(...)
			{
			    if(data != nullptr)
				delete data;
			    e_file->clean_data();
			    throw;
			}
			if(data != nullptr)
			    delete data;
			e_file->clean_data();
		    }
		    else // no data or delta patch is available
		    {
			if(e_file->get_saved_status() == saved_status::delta)
			    throw SRC_BUG;
			e_file->read_delta_signature(sig, block_size);
		    }
		}

		cat.pre_add_delta_sig(&pdesc);
		pdesc.compr->suspend_compression();
		if(sig)
		    e_file->dump_delta_signature(sig, block_size, *(pdesc.compr), pdesc.esc != nullptr);
		else
		    e_file->dump_delta_signature(*(pdesc.compr), pdesc.esc != nullptr);
	    }
	    catch(...)
	    {
		if(ref_file != nullptr)
		    ref_file->drop_delta_signature_data();
		if(e_file != nullptr)
		    e_file->drop_delta_signature_data();
		throw;
	    }
	    if(ref_file != nullptr)
		ref_file->drop_delta_signature_data();
	    if(e_file != nullptr)
		e_file->drop_delta_signature_data();
	}
    }

    static bool furtive_check(bool furtive,
			      const shared_ptr<user_interaction> & dialog,
			      bool verbose)
    {
	if(furtive
	   && capability_FOWNER(*dialog, verbose) != capa_set
	   && getuid() != 0)
	{
	    if(verbose)
		dialog->printf(gettext("Furtive read mode requires either root permission and FOWNER capability, falling back to normal filesystem read"));
	    furtive = false;
	}

	return furtive;
    }

    static bool merge_applying_patch_possible(const cat_entree* in_place, cat_entree* to_be_added)
    {
	const cat_mirage* in_mir = dynamic_cast<const cat_mirage*>(in_place);
	const cat_file* in_file = dynamic_cast<const cat_file*>(in_place);
	if(in_mir != nullptr)
	    in_file = dynamic_cast<const cat_file*>(in_mir->get_inode());
	cat_file* nonconst_in_file = const_cast<cat_file*>(in_file);

	cat_mirage* mir_added = dynamic_cast<cat_mirage*>(to_be_added);
	cat_file* added = dynamic_cast<cat_file*>(to_be_added);
	if(mir_added != nullptr)
	    added = dynamic_cast<cat_file*>(mir_added->get_inode());

	if(nonconst_in_file == nullptr)
	    return false;

	if(added == nullptr)
	    return false;

	return added->set_data_from_binary_patch(nonconst_in_file);
    }


} // end of namespace
