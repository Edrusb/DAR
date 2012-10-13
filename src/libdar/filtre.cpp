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
#include "filesystem.hpp"
#include "ea.hpp"
#include "defile.hpp"
#include "null_file.hpp"
#include "thread_cancellation.hpp"
#include "compressor.hpp"
#include "sparse_file.hpp"
#include "semaphore.hpp"

using namespace std;

#define SKIPPED "Skipping file: "
#define SQUEEZED "Ignoring empty directory: "

namespace libdar
{

	// returns false if file has changed during backup (inode is saved however, but the saved data may be invalid)
	// return true if file has not change
	// throw exceptions in case of error
    static bool save_inode(user_interaction & dialog,//< how to report to user
			   const string &info_quoi,  //< full path name of the file to save (including its name)
			   inode * & ino,            //< inode to save to archive
			   compressor *stock,        //< where to write to
			   bool info_details,        //< verbose output to user
			   compression compr_used,   //< compression to use to write data
			   bool alter_atime,         //< whether to set back atime of filesystem
			   bool check_change,        //< whether to check file change during backup
			   bool compute_crc,         //< whether to recompute the CRC
			   file::get_data_mode keep_mode, //< whether to copy compressed data (same compression algo), uncompress but keep hole structure (change compression algo) or uncompress and fill data holes (redetect holes in file)
			   const catalogue & cat,    //< catalogue to update for escape sequence mark
			   const infinint & repeat_count, //< how much time to retry saving the file if it has changed during the backup
			   const infinint & repeat_byte, //< how much byte remains to waste for saving again a changing file
			   const infinint & hole_size,   //< the threshold for hole detection, set to zero completely disable the sparse file detection mechanism
			   semaphore * sem,
			   infinint & new_wasted_bytes); //< new amount of wasted bytes to return to the caller.
    static bool save_ea(user_interaction & dialog,
			const string & info_quoi, inode * & ino, compressor *stock,
			const inode * ref, bool info_details, compression compr_used);
    static void restore_atime(const string & chemin, const inode * & ptr);

	/// merge two sets of EA

	/// \param[in] ref1 is the first EA set
	/// \param[in] ref2 is the second EA set
	/// \param[in,out] res is the EA set result of the merging operation
	/// \note result is the set of EA of ref1 to which those of ref2 are added if not already present in ref1
    static void merge_ea(const ea_attributs & ref1, const ea_attributs & ref2, ea_attributs  &res);

	/// to clone an "entree" taking hard links into account

	/// \param[in] ref is the named entry to be cloned
	/// \param[in,out] hard_link_base is the datastructure that gather/maps hard_links information
	/// \param[in] etiquette_offset is the offset to apply to etiquette (to not mix several hard-link sets using the same etiquette number in different archives)
	/// \return a pointer to the new allocated clone object (to be deleted by the delete operator by the caller)
    static entree *make_clone(const nomme *ref, map<infinint, etoile*> & hard_link_base, const infinint & etiquette_offset);

	/// remove an entry hardlink from a given hard_link database

	/// \param[in] mir is a pointer to the mirage object to delte
	/// \param[in,out] hard_link_base is the datastructure used to gather/map hard_links information
	/// \note if the mirage object is the last one pointing to a given "etoile" object
	/// deleting this mirage will delete the "etoile" object automatically (see destructor implementation of these classes).
	/// However, one need to remove from the database the reference to this "etoile *" that is about to me removed by the caller
    static void clean_hard_link_base_from(const mirage *mir, map<infinint, etoile *> & hard_link_base);



	/// transfer EA from one inode to another as defined by the given action

	/// \param[in] dialog for user interaction
	/// \param[in] action is the action to perform with EA, argument may be NULL
	/// \param[in,out] in_place is the "in place" inode, the resulting EA operation is placed as EA of this object, argument may be NULL
	/// \param[in] to_add is the "to be added" inode
	/// \note EA_preserve EA_preserve_mark_already_saved and EA_clear are left intentionnaly unimplemented!
	/// \note the NULL given as argument means that the object is not an inode

    static void do_EA_transfert(user_interaction &dialog, over_action_ea action, inode *in_place, const inode *to_add);


	/// overwriting policy when only restoring detruit objects
    static const crit_action *make_overwriting_for_only_deleted();

    void filtre_restore(user_interaction & dialog,
			const mask & filtre,
			const mask & subtree,
			catalogue & cat,
			const path & fs_racine,
			bool fs_warn_overwrite,
			bool info_details,
			statistics & st,
			const mask & ea_mask,
			bool flat,
			inode::comparison_fields what_to_check,
			bool warn_remove_no_match,
			bool empty,
			bool display_skipped,
			bool empty_dir,
			const crit_action & x_overwrite,
			archive_options_extract::t_dirty dirty,
			bool only_deleted,
			bool not_deleted)
    {
	defile juillet = fs_racine; // 'juillet' is in reference to 14th of July ;-) when takes place the "defile'" on the Champs-Elysees.
	const eod tmp_eod;
	const entree *e;
	thread_cancellation thr_cancel;
	const crit_action * when_only_deleted = only_deleted ? make_overwriting_for_only_deleted() : NULL;
	const crit_action & overwrite = only_deleted ? *when_only_deleted : x_overwrite;

	try
	{
	    filesystem_restore fs = filesystem_restore(dialog,
						       fs_racine,
						       fs_warn_overwrite,
						       info_details,
						       ea_mask,
						       what_to_check,
						       warn_remove_no_match,
						       empty,
						       &overwrite,
						       only_deleted);
		// if only_deleted, we set the filesystem to only overwrite mode (no creatation if not existing)
		// we also filter to only restore directories and detruit objects.

	    st.clear();
	    cat.reset_read();

	    if(!empty_dir)
		cat.launch_recursive_has_changed_update();

	    while(cat.read(e))
	    {
		const nomme *e_nom = dynamic_cast<const nomme *>(e);
		const directory *e_dir = dynamic_cast<const directory *>(e);
		const mirage *e_mir = dynamic_cast<const mirage *>(e);
		const inode *e_ino = dynamic_cast<const inode *>(e);
		const file *e_file = dynamic_cast<const file *>(e);
		const detruit *e_det = dynamic_cast<const detruit *>(e);

		if(e_mir != NULL)
		{
		    e_ino = const_cast<const inode *>(e_mir->get_inode());
		    if(e_ino == NULL)
			throw SRC_BUG; // !?! how is this possible ?
		    e_mir->get_inode()->change_name(e_mir->get_name()); // temporarily changing the inode name to the one of the mirage
		    e_file = dynamic_cast<const file *>(e_ino);
		}

		if(e_det != NULL && not_deleted)
		    continue; // skip this detruit object

		juillet.enfile(e);
		thr_cancel.check_self_cancellation();
		if(e_nom != NULL)
		{
		    try
		    {
			bool path_covered = subtree.is_covered(juillet.get_path());   // is current entry covered by filters (path)
			bool name_covered = e_dir != NULL || filtre.is_covered(e_nom->get_name()); //  is current entry's filename covered (not applied to directories)
			bool dirty_covered = e_file == NULL || !e_file->is_dirty() || dirty != archive_options_extract::dirty_ignore;  // checking against dirty files
			bool empty_dir_covered = e_dir == NULL || empty_dir || e_dir->get_recursive_has_changed(); // checking whether this is not a directory without any file to restore in it
			bool flat_covered = e_dir == NULL || !flat; // we do not restore directories in flat mode
			bool only_deleted_covered = !only_deleted || e_dir != NULL || e_det != NULL; // we do not restore other thing that directories and detruits when in "only_deleted" mode

			if(path_covered && name_covered && dirty_covered && empty_dir_covered && flat_covered && only_deleted_covered)
			{
			    filesystem_restore::action_done_for_data data_restored = filesystem_restore::done_no_change_no_data; // will be true if file's data have been restored (depending on overwriting policy)
			    bool ea_restored = false; // will be true if file's EA have been restored (depending on overwriting policy)
			    bool hard_link = false; // will be true if data_restored is true and only lead to a hard link creation or file replacement by a hard link to an already existing inode
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

			    if(dirty == archive_options_extract::dirty_warn && e_file != NULL && e_file->is_dirty())
			    {
				string tmp = juillet.get_string();
				dialog.pause(tools_printf(gettext("File %S has changed during backup and is probably not saved in a valid state (\"dirty file\"), do you want to consider it for restoration anyway?"), &tmp));
			    }

			    do
			    {
				if(!first_time) // a second time only occures in sequential read mode
				{
				    const file *e_file = dynamic_cast<const file *>(e_ino);

				    if(info_details)
					dialog.warning(string(gettext("File had changed during backup and had been copied another time, restoring the next copy of file: ")) + juillet.get_string());

					// we must let the filesystem object forget that
					// this hard linked inode has already been seen
				    if(e_mir != NULL)
					fs.clear_corres_if_pointing_to(e_mir->get_etiquette(), juillet.get_string());

				    if(e_file != NULL)
				    {
					file *me_file = const_cast<file *>(e_file);

					if(me_file == NULL)
					    throw SRC_BUG;

					me_file->drop_crc();
					if(cat.get_escape_layer() == NULL)
					    throw SRC_BUG;
					me_file->set_offset(cat.get_escape_layer()->get_position());
				    }

				    fs.ignore_overwrite_restrictions_for_next_write();
				}

				try
				{
				    fs.write(e, data_restored, ea_restored, created_retry, hard_link); // e and not e_ino, it may be a hard link
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
			    while(cat.get_escape_layer() != NULL && cat.get_escape_layer()->skip_to_next_mark(escape::seqt_changed, false) && data_restored == filesystem_restore::done_data_restored);

			    if(tmp_exc.active)
				throw Erange(tmp_exc.source, tmp_exc.message);

			    if(cat.get_escape_layer() != NULL && cat.get_escape_layer()->skip_to_next_mark(escape::seqt_dirty, false))
			    {
				string tmp = juillet.get_string();
				    // file is dirty and we are reading archive sequentially, thus this is only now once we have restored the file that
				    // we can inform the user about the dirtyness of the file.

				if(e_nom == NULL)
				    throw SRC_BUG;
				detruit killer = *e_nom;
				bool  tmp_ea_restored, tmp_created_retry, tmp_hard_link;
				filesystem_restore::action_done_for_data tmp_data_restored;

				switch(dirty)
				{
				case archive_options_extract::dirty_warn:
				    dialog.pause(tools_printf(gettext("The just restored file %S has been marked as dirty (sequential reading can only detect the dirty status after restoration), do we remove this just restored dirty file?"), &tmp));
					// NO BREAK HERE !!! This is intended.
				case archive_options_extract::dirty_ignore:
					// we must remove the file
				    if(info_details)
				    {
					if(dirty == archive_options_extract::dirty_ignore)
					    dialog.warning(tools_printf(gettext("The just restored file %S has been marked as dirty (sequential reading can only detect the dirty status after restoration), removing the just restored dirty file as it is asked to ignore this type of file"), &tmp));
					else
					    dialog.warning(tools_printf(gettext("Removing the dirty file %S"), &tmp));
				    }
				    fs.ignore_overwrite_restrictions_for_next_write();
				    fs.write(&killer, tmp_data_restored, tmp_ea_restored, tmp_created_retry, tmp_hard_link);
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
				if(e_dir == NULL || !cat.read_second_time_dir())
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
			}
			else // oject not covered by filters
			{
			    if(display_skipped)
			    {
				if(!path_covered || !name_covered || !dirty_covered)
				    dialog.warning(string(gettext(SKIPPED)) + juillet.get_string());
				else
				    dialog.warning(string(gettext(SQUEEZED)) + juillet.get_string());
			    }

			    if(e_dir == NULL || !cat.read_second_time_dir())
				st.incr_ignored();
			    if(e_dir != NULL)
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
			dialog.warning(juillet.get_string() + gettext(" not restored (user choice)"));

			if(e_dir != NULL && !flat)
			{
			    dialog.warning(gettext("No file in this directory will be restored."));
			    cat.skip_read_to_parent_dir();
			    juillet.enfile(&tmp_eod);
			}
			if(e_dir == NULL || !cat.read_second_time_dir())
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
			if(!only_deleted || e_dir == NULL)
			    dialog.warning(string(gettext("Error while restoring ")) + juillet.get_string() + " : " + e.get_message());

			if(e_dir != NULL && !flat)
			{
			    if(!only_deleted)
				dialog.warning(string(gettext("Warning! No file in that directory will be restored: ")) + juillet.get_string());
			    cat.skip_read_to_parent_dir();
			    juillet.enfile(&tmp_eod);
			}
			if(e_dir == NULL || !cat.read_second_time_dir())
			    if(!only_deleted)
				st.incr_errored();
		    }
		}
		else // e_nom == NULL : this should be a EOD
		{
		    const eod *e_eod = dynamic_cast<const eod *>(e);

		    if(e_eod == NULL)
			throw SRC_BUG; // not an class eod object, nor a class nomme object ???

		    if(!flat)
		    {
			bool notusedhere;
			filesystem_restore::action_done_for_data tmp;
			fs.write(e, tmp, notusedhere, notusedhere, notusedhere); // eod; don't care returned value
		    }
		}
	    }
	}
	catch(...)
	{
	    if(when_only_deleted != NULL)
		delete when_only_deleted;
	    throw;
	}
	if(when_only_deleted != NULL)
	    delete when_only_deleted;
    }

    void filtre_sauvegarde(user_interaction & dialog,
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
			   bool alter_atime,
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
			   const string & backup_hook_file_execute,
			   const mask & backup_hook_file_mask,
			   bool ignore_unknown)
    {
        entree *e = NULL;
        const entree *f = NULL;
        defile juillet = fs_racine;
        const eod tmp_eod;
	compressor *stockage;
        compression stock_algo;
	semaphore sem = semaphore(dialog, backup_hook_file_execute, backup_hook_file_mask);

	stack.find_first_from_top(stockage);
	if(stockage == NULL)
	    throw SRC_BUG;
	stock_algo = stockage->get_algo();
	infinint root_fs_device;
        filesystem_backup fs = filesystem_backup(dialog,
						 fs_racine,
						 info_details,
						 ea_mask,
						 nodump,
						 alter_atime,
						 furtive_read_mode,
						 cache_directory_tagging,
						 root_fs_device,
						 ignore_unknown);
	thread_cancellation thr_cancel;
	infinint skipped_dump, fs_errors;
	infinint wasted_bytes = 0;

        st.clear();
        cat.reset_add();
        ref.reset_compare();

	try
	{
	    try
	    {
		while(fs.read(e, fs_errors, skipped_dump))
		{
		    nomme *nom = dynamic_cast<nomme *>(e);
		    directory *dir = dynamic_cast<directory *>(e);
		    inode *e_ino = dynamic_cast<inode *>(e);
		    file *e_file = dynamic_cast<file *>(e);
		    mirage *e_mir = dynamic_cast<mirage *>(e);
		    bool known_hard_link = false;

		    st.add_to_ignored(skipped_dump);
		    st.add_to_errored(fs_errors);

		    juillet.enfile(e);
		    thr_cancel.check_self_cancellation();

		    if(e_mir != NULL)
		    {
			known_hard_link = e_mir->is_inode_wrote();
			if(!known_hard_link)
			{
			    e_ino = dynamic_cast<inode *>(e_mir->get_inode());
			    e_file = dynamic_cast<file *>(e_mir->get_inode());
			    if(e_ino == NULL)
				throw SRC_BUG;
			    e_ino->change_name(e_mir->get_name());
			}
		    }

		    if(nom != NULL)
		    {
			try
			{
			    if(subtree.is_covered(juillet.get_path())
			       && (dir != NULL || filtre.is_covered(nom->get_name()))
			       && (! same_fs || e_ino == NULL || e_ino->get_device() == root_fs_device))
			    {
				if(known_hard_link)
				{
					// no need to update the semaphore here as no data is read and no directory is expected here
				    cat.pre_add(e, stockage); // if cat is a escape_catalogue, this adds an escape sequence and entry info in the archive
				    cat.add(e);
				    e = NULL;
				    st.incr_hard_links();
				    if(e_mir != NULL)
				    {
					if(e_mir->get_inode()->get_saved_status() == s_saved || e_mir->get_inode()->ea_get_saved_status() == inode::ea_full)
					    if(info_details)
						dialog.warning(string(gettext("Recording hard link into the archive: "))+juillet.get_string());
				    }
				    else
					throw SRC_BUG; // known_hard_link is true and e_mir == NULL !???
				}
				else
				{
				    const inode *f_ino = NULL;
				    const file *f_file = NULL;
				    const mirage *f_mir = NULL;

				    if(e_ino == NULL)
					throw SRC_BUG; // if not a known hard link, e_ino should still either point to a real inode
					// or to the hard linked new inode.

				    if(fixed_date == 0)
				    {
					bool conflict = ref.compare(e, f);

					if(!conflict)
					{
					    f = NULL;
					    f_ino = NULL;
					    f_mir = NULL;
					}
					else // inode was already present in filesystem at the time the archive of reference was made
					{
					    f_ino = dynamic_cast<const inode*>(f);
					    f_file = dynamic_cast<const file *>(f);
					    f_mir = dynamic_cast<const mirage *>(f);

					    if(f_mir != NULL)
					    {
						f_ino = f_mir->get_inode();
						f_file = dynamic_cast<const file *>(f_ino);
					    }

						// Now checking for filesystem dissimulation

					    if(security_check)
					    {
						if(f_ino != NULL && e_ino != NULL)
						{
							// both are inodes

						    if(f_ino->signature() == e_ino->signature())
						    {
							    // same inode type

							if(f_ino->get_uid() == e_ino->get_uid()
							   && f_ino->get_gid() == e_ino->get_gid()
							   && f_ino->get_perm() == e_ino->get_perm()
							   && f_ino->get_last_modif() == e_ino->get_last_modif())
							{
								// same inode information

							    if(f_file == NULL || e_file == NULL
							       || f_file->get_size() == e_file->get_size())
							    {
								    // file size is unchanged

								if(f_ino->has_last_change() && e_ino->has_last_change())
								{
									// both inode ctime has been recorded

								    if(f_ino->get_last_change() != e_ino->get_last_change())
								    {
									string tmp = juillet.get_string();

									dialog.printf(gettext("SECURITY WARNING! SUSPICIOUS FILE %S: ctime changed since archive of reference was done, while no inode or data changed"), &tmp);
								    }
								}
							    }
							}
						    }
						}
					    }
					}
				    }
				    else
					f = NULL;

				    try
				    {
					f_ino = snapshot ? NULL : f_ino;
					f_file = snapshot ? NULL : f_file;

					    // EVALUATING THE ACTION TO PERFORM

					bool change_to_remove_ea =
					    e_ino != NULL && e_ino->ea_get_saved_status() == inode::ea_none
						// current inode to backup does not have any EA
					    && f_ino != NULL && f_ino->ea_get_saved_status() != inode::ea_none
					    && f_ino->ea_get_saved_status() != inode::ea_removed;
					    // and reference was an inode with EA

					bool avoid_saving_inode =
					    snapshot
						// don't backup if doing a snapshot
					    || (fixed_date > 0 && e_ino != NULL && e_ino->get_last_modif() < fixed_date)
						// don't backup if older than given date (if reference date given)
					    || (fixed_date == 0 && e_ino != NULL && f_ino != NULL && !e_ino->has_changed_since(*f_ino, hourshift, what_to_check)
						&& (f_file == NULL || !f_file->is_dirty()))
						// don't backup if doing differential backup and entry is the same as the one in the archive of reference
						// and if the reference is a plain file, it was not saved as dirty
					    ;

					bool avoid_saving_ea =
					    snapshot
						// don't backup if doing a snapshot
					    || (fixed_date > 0 && e_ino != NULL &&  e_ino->ea_get_saved_status() != inode::ea_none && e_ino->get_last_change() < fixed_date)
						// don't backup if older than given date (if reference date given)
					    || (fixed_date == 0 && e_ino != NULL && e_ino->ea_get_saved_status() == inode::ea_full && f_ino != NULL && f_ino->ea_get_saved_status() != inode::ea_none && e_ino->get_last_change() <= f_ino->get_last_change())
						// don't backup if doing differential backup and entry is the same as the one in the archive of reference
					    ;

					bool sparse_file_detection =
					    e_file != NULL
					    && e_file->get_size() > sparse_file_min_size
					    && sparse_file_min_size != 0;

					    // MODIFIYING INODE IF NECESSARY

					if(e_ino->get_saved_status() != s_saved)
					    throw SRC_BUG; // filsystem should always provide "saved" "entree"

					if(avoid_saving_inode)
					{
					    e_ino->set_saved_status(s_not_saved);
					    st.incr_skipped();
					}

					if(avoid_saving_ea)
					{
					    if(e_ino->ea_get_saved_status() == inode::ea_full)
						e_ino->ea_set_saved_status(inode::ea_partial);
					}

					if(change_to_remove_ea)
					    e_ino->ea_set_saved_status(inode::ea_removed);

					if(e_file != NULL)
					    e_file->set_sparse_file_detection_write(sparse_file_detection);

					    // DECIDING WHETHER FILE DATA WILL BE COMPRESSED OR NOT

					if(e_file != NULL)
					{
					    if(compr_mask.is_covered(nom->get_name()) && e_file->get_size() >= min_compr_size)
						    // e_nom not e_file because "e"
						    // may be a hard link, in which case its name is not carried by e_ino nor e_file
						e_file->change_compression_algo_write(stock_algo);
					    else
						e_file->change_compression_algo_write(none);
					}

					    // PRE RECORDING THE INODE (for sequential reading)

					cat.pre_add(e, stockage);

					    // PERFORMING ACTION FOR INODE

					if(!save_inode(dialog,
						       juillet.get_string(),
						       e_ino,
						       stockage,
						       info_details,
						       stock_algo,
						       alter_atime,
						       true,   // check_change
						       true,   // compute_crc
						       file::normal, // keep_mode
						       cat,
						       repeat_count,
						       repeat_byte,
						       sparse_file_min_size,
						       &sem,
						       wasted_bytes))
					{
					    st.set_byte_amount(wasted_bytes);
					    st.incr_tooold();
					}

					if(!avoid_saving_inode)
					    st.incr_treated();

					    // PERFORMING ACTION FOR EA

					if(e_ino->ea_get_saved_status() != inode::ea_removed)
					{
					    if(e_ino->ea_get_saved_status() == inode::ea_full)
						cat.pre_add_ea(e, stockage);
					    if(save_ea(dialog, juillet.get_string(), e_ino, stockage, NULL, info_details, stock_algo))
						st.incr_ea_treated();
					    cat.pre_add_ea_crc(e, stockage);
					}

					    // CLEANING UP MEMORY FOR PLAIN FILES

					if(e_file != NULL)
					    e_file->clean_data();

					    // UPDATING HARD LINKS

					if(e_mir != NULL)
					    e_mir->set_inode_wrote(true); // record that this inode has been saved (it is a "known_hard_link" now)

					    // ADDING ENTRY TO CATALOGUE

					cat.add(e);
					e = NULL;
				    }
				    catch(...)
				    {
					if(dir != NULL && fixed_date == 0)
					    ref.compare(&tmp_eod, f);
					throw;
				    }
				}
			    }
			    else // inode not covered
			    {
				nomme *ig = NULL;
				inode *ignode = NULL;
				sem.raise(juillet.get_string(), e, false);

				if(display_skipped)
				    dialog.warning(string(gettext(SKIPPED)) + juillet.get_string());

				if(dir != NULL && make_empty_dir)
				    ig = ignode = new (nothrow) ignored_dir(*dir);
				else
				    ig = new (nothrow) ignored(nom->get_name());
				    // necessary to not record deleted files at comparison
				    // time in case files are just not covered by filters
				st.incr_ignored();

				if(ig == NULL)
				    throw Ememory("filtre_sauvegarde");
				else
				    cat.add(ig);

				if(dir != NULL)
				{
				    if(make_empty_dir)
				    {
					bool known;

					if(fixed_date == 0)
					    known = ref.compare(dir, f);
					else
					    known = false;

					try
					{
					    const inode *f_ino = known ? dynamic_cast<const inode *>(f) : NULL;
					    bool tosave = false;

					    if(known)
						if(f_ino != NULL)
						    tosave = dir->has_changed_since(*f_ino, hourshift, what_to_check);
						else
						    throw SRC_BUG;
						// catalogue::compare() with a directory should return false or give a directory as
						// second argument or here f is not an inode (f_ino == NULL) !
						// and known == true
					    else
						tosave = true;

					    ignode->set_saved_status(tosave && !snapshot ? s_saved : s_not_saved);
					}
					catch(...)
					{
					    if(fixed_date == 0)
						ref.compare(&tmp_eod, f);
					    throw;
					}
					if(fixed_date == 0)
					    ref.compare(&tmp_eod, f);
				    }
				    fs.skip_read_to_parent_dir();
				    juillet.enfile(&tmp_eod);
				}

				delete e; // we don't keep this object in the catalogue so we must destroy it
				e = NULL;
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
				nomme *tmp = new (nothrow) ignored(nom->get_name());
				dialog.warning(string(gettext("Error while saving ")) + juillet.get_string() + ": " + ex.get_message());
				st.incr_errored();

				    // now we can destroy the object
				delete e;
				e = NULL;

				if(tmp == NULL)
				    throw Ememory("fitre_sauvegarde");
				cat.add(tmp);

				if(dir != NULL)
				{
				    fs.skip_read_to_parent_dir();
				    juillet.enfile(&tmp_eod);
				    dialog.warning(gettext("NO FILE IN THAT DIRECTORY CAN BE SAVED."));
				}
			    }
			    else
			    {
				ex.prepend_message(gettext("Cannot write down the archive: "));
				throw; // error occured while adding data to the archive, archive cannot be generated properly
			    }
			}
		    }
		    else // eod
		    {
			sem.raise(juillet.get_string(), e, true);
			sem.lower();
			if(fixed_date == 0)
			    ref.compare(e, f); // makes the comparison in the reference catalogue go to parent directory
			cat.pre_add(e, stockage); // adding a mark and dropping EOD entry in the archive if cat is an escape_catalogue object (else, does nothing)
			cat.add(e);
		    }
		}
	    }
	    catch(Ethread_cancel & e)
	    {
		if(!e.immediate_cancel())
		    stockage->change_algo(stock_algo);
		throw Ethread_cancel_with_attr(e.immediate_cancel(), e.get_flag(), fs.get_last_etoile_ref());
	    }
	}
	catch(...)
	{
	    if(e != NULL)
		delete e;
	    throw;
	}

        stockage->change_algo(stock_algo);
    }

    void filtre_difference(user_interaction & dialog,
			   const mask &filtre,
                           const mask &subtree,
                           catalogue & cat,
                           const path & fs_racine,
                           bool info_details, statistics & st,
			   const mask & ea_mask,
			   bool alter_atime,
			   bool furtive_read_mode,
			   inode::comparison_fields what_to_check,
			   bool display_skipped,
			   const infinint & hourshift,
			   bool compare_symlink_date)
    {
        const entree *e;
        defile juillet = fs_racine;
        const eod tmp_eod;
        filesystem_diff fs = filesystem_diff(dialog,
					     fs_racine,
					     info_details,
					     ea_mask,
					     alter_atime,
					     furtive_read_mode);
	thread_cancellation thr_cancel;

        st.clear();
        cat.reset_read();

	while(cat.read(e))
	{
	    const directory *e_dir = dynamic_cast<const directory *>(e);
	    const nomme *e_nom = dynamic_cast<const nomme *>(e);
	    const inode *e_ino = dynamic_cast<const inode *>(e);
	    const mirage *e_mir = dynamic_cast<const mirage *>(e);

	    if(e_mir != NULL)
	    {
		const file *e_file = dynamic_cast<const file *>(e_mir->get_inode());

		if(e_file == NULL || e_mir->get_etoile_ref_count() == 1 || cat.get_escape_layer() == NULL)
		{
		    e_ino = e_mir->get_inode();
		    e_mir->get_inode()->change_name(e_mir->get_name());
		}
		else
		    dialog.warning(gettext("SKIPPED (hard link in sequential read mode): ") + e_mir->get_name());
	    }

	    juillet.enfile(e);
	    thr_cancel.check_self_cancellation();
	    try
	    {
		if(e_nom != NULL)
		{
		    if(subtree.is_covered(juillet.get_path()) && (e_dir != NULL || filtre.is_covered(e_nom->get_name())))
		    {
			nomme *exists_nom = NULL;

			if(e_ino != NULL)
			{
			    if(e_nom == NULL)
				throw SRC_BUG;
			    if(fs.read_filename(e_nom->get_name(), exists_nom))
			    {
				try
				{
				    inode *exists = dynamic_cast<inode *>(exists_nom);
				    directory *exists_dir = dynamic_cast<directory *>(exists_nom);

				    if(exists != NULL)
				    {
					try
					{
					    e_ino->compare(*exists, ea_mask, what_to_check, hourshift, compare_symlink_date);
					    if(info_details)
						dialog.warning(string(gettext("OK   "))+juillet.get_string());
					    if(e_dir == NULL || !cat.read_second_time_dir())
						st.incr_treated();
					    if(!alter_atime)
					    {
						const inode * tmp_exists = const_cast<const inode *>(exists);
						restore_atime(juillet.get_string(), tmp_exists);
					    }
					}
					catch(Erange & e)
					{
					    dialog.warning(string(gettext("DIFF "))+juillet.get_string()+": "+ e.get_message());
					    if(e_dir == NULL && exists_dir != NULL)
						fs.skip_read_filename_in_parent_dir();
					    if(e_dir != NULL && exists_dir == NULL)
					    {
						cat.skip_read_to_parent_dir();
						juillet.enfile(&tmp_eod);
					    }

					    if(e_dir == NULL || !cat.read_second_time_dir())
						st.incr_errored();
					    if(!alter_atime)
					    {
						const inode * tmp_exists = const_cast<const inode *>(exists);
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
				    exists_nom = NULL;
				    throw;
				}
				delete exists_nom;
				exists_nom = NULL;
			    }
			    else // can't compare, nothing of that name in filesystem
			    {
				dialog.warning(string(gettext("DIFF "))+ juillet.get_string() + gettext(": file not present in filesystem"));
				if(e_dir != NULL)
				{
				    cat.skip_read_to_parent_dir();
				    juillet.enfile(&tmp_eod);
				}

				if(e_dir == NULL || !cat.read_second_time_dir())
				    st.incr_errored();
			    }
			}
			else // not an inode (for example a detruit, hard_link etc...), nothing to do
			    st.incr_treated();
		    }
		    else // not covered by filters
		    {
			if(display_skipped)
			    dialog.warning(string(gettext(SKIPPED)) + juillet.get_string());

			if(e_dir == NULL || !cat.read_second_time_dir())
			    st.incr_ignored();
			if(e_dir != NULL)
			{
			    cat.skip_read_to_parent_dir();
			    juillet.enfile(&tmp_eod);
			}
		    }
		}
		else // eod ?
		    if(dynamic_cast<const eod *>(e) != NULL) // yes eod
			fs.skip_read_filename_in_parent_dir();
		    else // no ?!?
			throw SRC_BUG; // not nomme neither eod ! what's that ?
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
		dialog.warning(string(gettext("ERR  ")) + juillet.get_string() + " : " + e.get_message());
		if(e_dir == NULL || !cat.read_second_time_dir())
		    st.incr_errored();
	    }
	}
        fs.skip_read_filename_in_parent_dir();
            // this call here only to restore dates of the root (-R option) directory
    }

    void filtre_test(user_interaction & dialog,
		     const mask &filtre,
                     const mask &subtree,
                     catalogue & cat,
                     bool info_details,
		     bool empty,
                     statistics & st,
		     bool display_skipped)
    {
        const entree *e;
        defile juillet = FAKE_ROOT;
        null_file black_hole = null_file(gf_write_only);
        ea_attributs ea;
        infinint offset;
        const eod tmp_eod;
	thread_cancellation thr_cancel;

        st.clear();
        cat.reset_read();
        while(cat.read(e))
        {
	    const file *e_file = dynamic_cast<const file *>(e);
	    const inode *e_ino = dynamic_cast<const inode *>(e);
	    const directory *e_dir = dynamic_cast<const directory *>(e);
	    const nomme *e_nom = dynamic_cast<const nomme *>(e);
	    const mirage *e_mir = dynamic_cast<const mirage *>(e);

            juillet.enfile(e);
	    thr_cancel.check_self_cancellation();
            try
            {
		if(e_mir != NULL)
		{
		    if(!e_mir->is_inode_wrote())
		    {
			e_file = dynamic_cast<const file *>(e_mir->get_inode());
			e_ino = e_mir->get_inode();
		    }
		}

                if(e_nom != NULL)
                {
                    if(subtree.is_covered(juillet.get_path()) && (e_dir != NULL || filtre.is_covered(e_nom->get_name())))
                    {
                            // checking data file if any
                        if(e_file != NULL && e_file->get_saved_status() == s_saved)
                        {
			    if(!empty)
			    {
				generic_file *dat = e_file->get_data(file::normal);
				if(dat == NULL)
				    throw Erange("filtre_test", gettext("Can't read saved data."));
				try
				{
				    infinint crc_size;
				    crc *check = NULL;
				    const crc *original = NULL;

				    if(!e_file->get_crc_size(crc_size))
					crc_size = tools_file_size_to_crc_size(e_file->get_size());

				    dat->skip(0);
				    dat->copy_to(black_hole, crc_size, check);
				    if(check == NULL)
					throw SRC_BUG;

				    try
				    {
					    // due to possible sequential reading mode, the CRC
					    // must not be fetched before the data has been copied
					if(e_file->get_crc(original))
					{
					    if(original == NULL)
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
				    throw;
				}
				delete dat;
			    }
                        }
                            // checking inode EA if any
                        if(e_ino != NULL && e_ino->ea_get_saved_status() == inode::ea_full)
                        {
			    if(!empty)
			    {
				ea_attributs tmp = *(e_ino->get_ea());
				e_ino->ea_detach();
			    }
                        }

			if(e_dir == NULL || !cat.read_second_time_dir())
			    st.incr_treated();

			if(e_mir != NULL)
			    e_mir->set_inode_wrote(true);

                            // still no exception raised, this all is fine
                        if(info_details)
                            dialog.warning(string(gettext("OK  ")) + juillet.get_string());
                    }
                    else // excluded by filter
                    {
			if(display_skipped)
			    dialog.warning(string(gettext(SKIPPED)) + juillet.get_string());

                        if(e_dir != NULL)
                        {
                            juillet.enfile(&tmp_eod);
                            cat.skip_read_to_parent_dir();
                        }
			if(e_dir == NULL || !cat.read_second_time_dir())
			    st.incr_skipped();
                    }
                }
            }
            catch(Euser_abort & e)
            {
                throw;
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
                dialog.warning(string(gettext("ERR ")) + juillet.get_string() + " : " + e.get_message());
		if(e_dir == NULL || !cat.read_second_time_dir())
		    st.incr_errored();
            }
        }
    }

    void filtre_isolate(user_interaction & dialog,
			catalogue & cat,
                        catalogue & ref,
                        bool info_details)
    {
        const entree *e;
        const eod tmp_eod;
        map<infinint, etoile *> corres;
	thread_cancellation thr_cancel;

        ref.reset_read();
        cat.reset_add();

        if(info_details)
            dialog.warning(gettext("Preparing the archive contents for isolation..."));

	thr_cancel.block_delayed_cancellation(true);
        while(ref.read(e))
        {
            const inode *e_ino = dynamic_cast<const inode *>(e);
	    const mirage *e_mir = dynamic_cast<const mirage *>(e);
	    etoile *alcor = NULL;

	    if(e_mir != NULL)
	    {
		    // looking whether this etiquette has already been treated
		map<infinint, etoile *>::iterator it = corres.find(e_mir->get_etiquette());

		if(it == corres.end()) // not found, thus not treated
		    e_ino = e_mir->get_inode();
		    // and alcor stays equal to NULL
		else  // already treated, we just have to spawn a new mirage object pointing to alcor
		    alcor = it->second;
		    // and e_ino stays equal to NULL
	    }

            if(e_ino != NULL) // specific treatment for inode
            {
                entree *f = e_ino->clone();
                inode *f_ino = dynamic_cast<inode *>(f);

		if(f == NULL)
		    throw Ememory("filtre_isolate");

                try
                {
                    if(f_ino == NULL)
                        throw SRC_BUG; // inode should clone an inode

                        // for hard link we must attach the cloned inode to an new etoile and attached this one to a new mirage that will be added to catalogue (!)
                    if(e_mir != NULL)
                    {
			    // we reached this statement thus e_mir != NULL and e_ino != NULL, thus this is a new etiquette
			etoile *mizar = new (nothrow) etoile(f_ino, e_mir->get_etiquette());
			    // note about etiquette: the cloned object has the same etiquette
			    // and thus each etiquette correspond to two instances
			if(mizar == NULL)
			    throw Ememory("filtre_isolate");
			try
			{
			    f_ino = NULL; // must not be released by the catch stament below as it is now managed by mizar
			    f = new (nothrow) mirage(e_mir->get_name(), mizar);
			    if(f == NULL)
				throw Ememory("filtre_isolate");
			    corres[e_mir->get_etiquette()] = mizar;
			}
			catch(...)
			{
			    if(mizar != NULL)
				delete mizar;
			    throw;
			}
                    }

                    cat.add(f);
                }
                catch(...)
                {
                    if(f != NULL)
                        delete f;
                    throw;
                }
            }
            else // other entree than inode
                if(e != NULL)
                {
                    entree *f = NULL;

		    try
		    {
			if(e_mir != NULL)
			{
			    if(alcor == NULL)
				throw SRC_BUG;
			    f = new (nothrow) mirage(e_mir->get_name(), alcor);
			}
			else
			    f = e->clone();

			if(f == NULL)
			    throw Ememory("filtre_isolate");

			cat.add(f);
		    }
		    catch(...)
		    {
			if(f != NULL)
			    delete f;
                        throw;
                    }
                }
                else
                    throw SRC_BUG; // read provided NULL while returning true
        }
	thr_cancel.block_delayed_cancellation(false);
    }


    void filtre_merge(user_interaction & dialog,
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
		      const crit_action & over_action,
		      bool warn_overwrite,
		      bool decremental_mode,
		      const infinint & sparse_file_min_size)
    {
	compressor *stockage;
	compression stock_algo;

	stack.find_first_from_top(stockage);
	if(stockage == NULL)
	    throw SRC_BUG;
	stock_algo = stockage->get_algo();
	thread_cancellation thr_cancel;
	const eod tmp_eod;
	catalogue *ref_tab[] = { ref1, ref2, NULL };
	infinint etiquette_offset = 0;
	map <infinint, etoile *> corres_copy;
	const entree *e = NULL;
	U_I index = 0;
	defile juillet = FAKE_ROOT;
	infinint fake_repeat = 0;
	bool abort = false;
	crit_action *decr = NULL; // will point to a locally allocated crit_action
	    // for decremental backup (argument overwrite is ignored)
	const crit_action *overwrite = &over_action; // may point to &decr if
	    // decremental backup is asked

	    // STEP 0: Getting ready

	st.clear();

	if(decremental_mode)
	{
	    if(ref1 == NULL || ref2 == NULL)
	    {
		dialog.pause(gettext("Decremental mode is useless when merging is not applied to both an archive of reference and an auxiliary archive of reference. Ignore decremental mode and continue?"));
		decremental_mode = false;
	    }
	    else
	    {
		    // allocating decr to "{T&R&~R&(A|!H)}[S*] P* ; {(e&~e&r&~r)|(!e&!~e)}[*s] *p"
		    //
		    // which means to record just data presence (S*) when:
		    //   both entries are of the same type (T)
		    //   and both have the same modification date (R&~R)
		    //   and this is the first time we meet this hard linked inode, or this is not a hard linked inode (A|!H)
		    // else data is taken as is (P*) from the "in place" archive
		    // EA are recorded as present when:
		    //   both entries have EA present (e&~e)
		    //   and both EA set have the same date (r&~r)
		    // OR
		    //  none entry has EA present
		    // else the EA (or the absence of EA) is taken from the "in place" archive

		try
		{
		    crit_chain *decr_crit_chain = new (nothrow) crit_chain();
		    if(decr_crit_chain == NULL)
			throw Ememory("filtre_merge");
		    decr = decr_crit_chain;
		    crit_and c_and = crit_and();
		    crit_or  c_or = crit_or();

		    c_and.clear();
		    c_or.clear();
		    c_and.add_crit(crit_same_type());
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
		    if(decr != NULL)
		    {
			delete decr;
			decr = NULL;
		    }
		    throw;
		}
		overwrite = decr;
	    }
	}

	try
	{

	    if(overwrite == NULL)
		throw SRC_BUG;


		// STEP 1:
		// we merge catalogues (pointed to by ref_tab[]) of each archive and produce a resulting catalogue 'cat'
		// the merging resolves overwriting conflicts following the "criterium" rule
		// each object of the catalogue is cloned, this is the clones that get added to the resulting catalogue

	    try
	    {

		while(ref_tab[index] != NULL) // for each catalogue of reference (ref. and eventually auxiliary ref.) do:
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
			    ptr = gettext("next"); // not yet used, but room is open for future evolutions
			    break;
			}
			dialog.printf(gettext("Merging/filtering files from the %s archive"), ptr);
		    }

		    while(ref_tab[index]->read(e)) // examining the content of the current archive of reference, each entry one by one
		    {
			const nomme *e_nom = dynamic_cast<const nomme *>(e);
			const mirage *e_mir = dynamic_cast<const mirage *>(e);
			const directory *e_dir = dynamic_cast<const directory *>(e);
			const detruit *e_detruit = dynamic_cast<const detruit *>(e);

			juillet.enfile(e);
			thr_cancel.check_self_cancellation();
			if(e_nom != NULL)
			{
			    try
			    {
				if(subtree.is_covered(juillet.get_path()) && (e_dir != NULL || filtre.is_covered(e_nom->get_name())))
				{
				    entree *dolly = make_clone(e_nom, corres_copy, etiquette_offset);

					// now that we have a clone object we must add the copied object to the catalogue, respecting the overwriting constaints

				    try
				    {
					string the_name = e_nom->get_name();
					const nomme *already_here = NULL;  // may receive an address when an object of that name already exists in the resultant catalogue

					    // some different types of pointer to the "dolly" object

					nomme *dolly_nom = dynamic_cast<nomme *>(dolly);
					directory *dolly_dir = dynamic_cast<directory *>(dolly);
					mirage *dolly_mir = dynamic_cast<mirage *>(dolly);
					inode *dolly_ino = dynamic_cast<inode *>(dolly);

					if(dolly_mir != NULL)
					    dolly_ino = dolly_mir->get_inode();

					if(cat.read_if_present(& the_name, already_here)) // An entry of that name already exists in the resulting catalogue
					{

						// some different types of pointer to the "already_here" object (aka 'inplace" object)
					    const mirage *al_mir = dynamic_cast<const mirage *>(already_here);
					    const detruit *al_det = dynamic_cast<const detruit *>(already_here);
					    const ignored *al_ign = dynamic_cast<const ignored *>(already_here);
					    const ignored_dir *al_igndir = dynamic_cast<const ignored_dir *>(already_here);
					    const inode *al_ino = dynamic_cast<const inode *>(already_here);
					    const directory *al_dir = dynamic_cast<const directory *>(already_here);
					    const string full_name = juillet.get_string();

					    over_action_data act_data;
					    over_action_ea act_ea;

					    if(al_mir != NULL)
						al_ino = al_mir->get_inode();

					    if(dolly_nom == NULL)
						throw SRC_BUG; // dolly should be the clone of a nomme object which is not the case here

						// evaluating the overwriting policy

					    overwrite->get_action(*already_here, *dolly_nom, act_data, act_ea);

					    if(act_data == data_ask)
						act_data = crit_ask_user_for_data_action(dialog, full_name, already_here, dolly);

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
							dialog.pause(tools_printf(gettext("Data of file %S is about to be %S, proceed?"), &full_name, &action));
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
							dialog.pause(tools_printf(gettext("EA of file %S are about to be %S, proceed?"), &full_name, &action));
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
						   && al_ino != NULL)
						{
						    inode *tmp = const_cast<inode *>(al_ino);
						    if(tmp->get_saved_status() == s_saved)
							tmp->set_saved_status(s_not_saved); // dropping data
						}

						    // EA consideration

						if(act_ea == EA_ask)
						{
						    if(dolly_ino != NULL && al_ino != NULL && (dolly_ino->ea_get_saved_status() != inode::ea_none || al_ino->ea_get_saved_status() != inode::ea_none))
							act_ea = crit_ask_user_for_EA_action(dialog, full_name, already_here, dolly);
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
						    if(info_details)
							dialog.warning(tools_printf(gettext("EA of file %S from first archive have been updated with those of same named file of the auxiliary archive"), &full_name));
						    do_EA_transfert(dialog, act_ea, const_cast<inode *>(al_ino), dolly_ino);
						    break;
						case EA_preserve_mark_already_saved:

						    if(al_ino != NULL && al_ino->ea_get_saved_status() == inode::ea_full)
						    {
							const_cast<inode *>(al_ino)->ea_set_saved_status(inode::ea_partial);
							if(info_details)
							    dialog.warning(tools_printf(gettext("EA of file %S from first archive have been dropped and marked as already saved"), &full_name));
						    }
						    break;
						case EA_clear:
						    if(al_ino->ea_get_saved_status() != inode::ea_none)
						    {
							if(al_ino->ea_get_saved_status() == inode::ea_full)
							    st.decr_ea_treated();
							if(info_details)
							    dialog.warning(tools_printf(gettext("EA of file %S from first archive have been removed"), &full_name));
							const_cast<inode *>(al_ino)->ea_set_saved_status(inode::ea_none);
						    }
						    break;
						default:
						    throw SRC_BUG;
						}


						    // we must keep the existing entry in the catalogue

						if(display_skipped && (dolly_dir == NULL || al_dir == NULL))
						    dialog.warning(tools_printf(gettext("Data of file %S from first archive has been preserved from overwriting"), &full_name));

						if(al_dir != NULL && dolly_dir != NULL)
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
						    if(al_dir != NULL)
							cat.skip_read_to_parent_dir();
						    if(dolly_dir != NULL)
						    {
							ref_tab[index]->skip_read_to_parent_dir();
							juillet.enfile(&tmp_eod);
						    }
						}

						    // we may need to clean the corres_copy map

						if(dolly_mir != NULL)
						    clean_hard_link_base_from(dolly_mir, corres_copy);

						    // now we can safely destroy the clone object

						delete dolly;
						dolly = NULL;
						break;

						    /////////////////////////// SECOND ACTIONS CATEGORY for DATA /////
					    case data_overwrite:
					    case data_overwrite_mark_already_saved:
					    case data_remove:

						    // drop data if necessary

						if(info_details)
						{
						    switch(act_data)
						    {
						    case data_remove:
							dialog.warning(tools_printf(gettext("Data of file %S taken from the first archive of reference has been removed"), &full_name));
							break;
						    default:
							dialog.warning(tools_printf(gettext("Data of file %S taken from the first archive of reference has been overwritten"), &full_name));
						    }
						}

						if(act_data == data_overwrite_mark_already_saved && dolly_ino != NULL)
						{
						    if(dolly_ino->get_saved_status() == s_saved)
							dolly_ino->set_saved_status(s_not_saved); // dropping data
						}

						    // EA consideration

						if(act_ea == EA_ask && act_data != data_remove)
						{
						    if(dolly_ino != NULL && al_ino != NULL && (dolly_ino->ea_get_saved_status() != inode::ea_none || al_ino->ea_get_saved_status() != inode::ea_none))
							act_ea = crit_ask_user_for_EA_action(dialog, full_name, already_here, dolly);
						    else
							act_ea = EA_overwrite; // no need to ask here neither as both entries have no EA.
						}

						if(act_data != data_remove)
						{
						    switch(act_ea)
						    {
						    case EA_preserve:
						    case EA_undefined: // remaining ea_undefined at the end of the evaluation defaults to ea_preserve
							do_EA_transfert(dialog, EA_overwrite, dolly_ino, al_ino);
							break;
						    case EA_overwrite:
							if(info_details)
							    dialog.warning(tools_printf(gettext("EA of file %S has been overwritten"), &full_name));
							break; // nothing to do
						    case EA_overwrite_mark_already_saved:
							if(info_details)
							    dialog.warning(tools_printf(gettext("EA of file %S has been overwritten and marked as already saved"), &full_name));
							if(dolly_ino != NULL && dolly_ino->ea_get_saved_status() == inode::ea_full)
							    dolly_ino->ea_set_saved_status(inode::ea_partial);
							break;
						    case EA_merge_preserve:
							if(info_details)
							    dialog.warning(tools_printf(gettext("EA of file %S from first archive have been updated with those of the same named file of the auxiliary archive"), &full_name));
							do_EA_transfert(dialog, EA_merge_overwrite, dolly_ino, al_ino);
							break;
						    case EA_merge_overwrite:
							if(info_details)
							    dialog.warning(tools_printf(gettext("EA of file %S from first archive have been updated with those of the same named file of the auxiliary archive"), &full_name));
							do_EA_transfert(dialog, EA_merge_preserve, dolly_ino, al_ino);
							break;
						    case EA_preserve_mark_already_saved:
							if(info_details)
							    dialog.warning(tools_printf(gettext("EA of file %S has been overwritten and marked as already saved"), &full_name));
							do_EA_transfert(dialog, EA_overwrite_mark_already_saved, dolly_ino, al_ino);
							break;
						    case EA_clear:
							if(al_ino->ea_get_saved_status() != inode::ea_none)
							{
							    if(info_details)
								dialog.warning(tools_printf(gettext("EA of file %S from first archive have been removed"), &full_name));
							    dolly_ino->ea_set_saved_status(inode::ea_none);
							}
							break;
						    default:
							throw SRC_BUG;
						    }
						}
						else // data_remove
						{
							// we remove both objects in overwriting conflict: here for now the dolly clone of the to be added

						    if(dolly_mir != NULL)
							clean_hard_link_base_from(dolly_mir, corres_copy);

						    if(dolly_dir != NULL)
						    {
							juillet.enfile(&tmp_eod);
							ref_tab[index]->skip_read_to_parent_dir();
							dolly_dir = NULL;
						    }

							// now we can safely destroy the clone object

						    delete dolly;
						    dolly = NULL;
						}


						    // we must remove the existing entry present in the catalogue to make room for the new entry to be added

						if(dolly_dir == NULL || al_dir == NULL || act_data == data_remove) // one or both are not directory we effectively must remove the entry from the catalogue
						{
						    ignored_dir *if_removed = NULL;

							// to known which counter to decrement

						    st.decr_treated();

						    if(al_ino != NULL)
							if(al_ino->ea_get_saved_status() == inode::ea_full)
							    st.decr_ea_treated();

							// hard link specific actions

						    if(al_mir != NULL)
						    {
							    // update back hard link counter
							st.decr_hard_links();

							    // updating counter from pointed to inode

							const inode*al_ptr_ino = al_mir->get_inode();
							if(al_ptr_ino == NULL)
							    throw SRC_BUG;
							else
							    if(al_ptr_ino->ea_get_saved_status() == inode::ea_full)
								st.decr_ea_treated();

							    // cleaning the corres_copy map from the pointed to etoile object reference if necessary
							clean_hard_link_base_from(al_mir, corres_copy);
						    }


						    if(al_det != NULL)
							st.decr_deleted();

						    if(al_ign != NULL || al_igndir != NULL)
							st.decr_ignored();

						    if(act_data == data_remove)
							st.incr_ignored();

							// remove the current entry from the catalogue
						    if(al_dir != NULL)
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
							    map<infinint, etoile *>::iterator it = corres_copy.find(ut->first);

							    if(it == corres_copy.end())
								throw SRC_BUG; // unknown etiquettes found in directory tree

							    if(it->second->get_ref_count() < ut->second)
								throw SRC_BUG;
								// more reference found in directory tree toward this etoile than
								// this etoile is aware of !

							    if(it->second->get_ref_count() == ut->second)
								    // this etoile will disapear because all its reference are located
								    // in the directory tree we are about to remove, we must clean this
								    // entry from corres_copy
								corres_copy.erase(it);

							    ++ut;
							}


							if(make_empty_dir)
							{
							    if_removed = new (nothrow) ignored_dir(*al_dir);
							    if(if_removed == NULL)
								throw Ememory("filtre_merge");
							}
						    }

						    try
						    {

							    // we can now remove the entry from the catalogue
							cat.remove_read_entry(the_name);

							    // now that the ancient directory has been removed we can replace it by an ignored_dir entry if required
							if(if_removed != NULL)
							    cat.add(if_removed);

						    }
						    catch(...)
						    {
							if(if_removed != NULL)
							{
							    delete if_removed;
							    if_removed = NULL;
							}
							throw;
						    }

						}
						else // both existing and the one to add are directories we can proceed to the merging of their contents
						{
						    cat.re_add_in_replace(*dolly_dir);
						    delete dolly;
						    dolly = NULL;
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

						    // this entry only exists in the auxilliary archive of reference, we must thus replace it by a "detruit
						    // because it will have to be removed when restoring the decremental backup.


						    // we may need to clean the corres_copy map
						if(dolly_mir != NULL)
						{
						    clean_hard_link_base_from(dolly_mir, corres_copy);
						    dolly_mir = NULL;
						}

						    // then taking care or directory hierarchy
						if(dolly_dir != NULL)
						{
						    juillet.enfile(&tmp_eod);
						    ref_tab[index]->skip_read_to_parent_dir();
						    dolly_dir = NULL;
						}

						if(dolly != NULL)
						{
						    delete dolly;
						    dolly = NULL;
						}
						else
						    throw SRC_BUG;
						dolly_ino = NULL;

						if(e_mir != NULL)
						    firm = e_mir->get_inode()->signature();
						else
						    firm = e->signature();

						dolly = new (nothrow) detruit(the_name, firm, ref_tab[index]->get_current_reading_dir().get_last_modif());
						if(dolly == NULL)
						    throw Ememory("filtre_merge");
						dolly_nom = dynamic_cast<nomme *>(dolly);
					    }

					if(dolly != NULL)
					{
					    const inode *e_ino = dynamic_cast<const inode *>(e);

					    cat.add(dolly);
					    st.incr_treated();

					    if(e_mir != NULL)
					    {
						st.incr_hard_links();
						e_ino = e_mir->get_inode();
					    }

					    if(e_detruit != NULL)
						st.incr_deleted();


					    if(e_ino != NULL && e_ino->ea_get_saved_status() == inode::ea_full)
						st.incr_ea_treated();

					    if(e_dir != NULL) // we must chdir also for the *reading* method of this catalogue object
					    {
						if(!cat.read_if_present(&the_name, already_here))
						    throw SRC_BUG;
					    }
					}
				    }
				    catch(...)
				    {
					if(dolly != NULL)
					{
					    delete dolly;
					    dolly = NULL;
					}
					throw;
				    }
				}
				else // excluded by filters
				{
				    if(e_dir != NULL && make_empty_dir)
				    {
					ignored_dir *igndir = new (nothrow) ignored_dir(*e_dir);
					if(igndir == NULL)
					    throw Ememory("filtre_merge");
					else
					    cat.add(igndir);
				    }

				    if(display_skipped)
					dialog.warning(string(gettext(SKIPPED)) + juillet.get_string());

				    st.incr_ignored();
				    if(e_dir != NULL)
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
				dialog.warning(juillet.get_string() + gettext(" not merged (user choice)"));

				if(e_dir != NULL)
				{
				    dialog.warning(gettext("No file in this directory will be considered for merging."));
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
				dialog.warning(string(gettext("Error while considering file ")) + juillet.get_string() + " : " + e.get_message());

				if(e_dir != NULL)
				{
				    dialog.warning(string(gettext("Warning! No file in this directory will be considered for merging: ")) + juillet.get_string());
				    ref_tab[index]->skip_read_to_parent_dir();
				    juillet.enfile(&tmp_eod);
				}
				st.incr_errored();
			    }
			}
			else  // entree is not a nomme object (this is an "eod")
			{
			    entree *tmp = e->clone();
			    try
			    {
				const nomme *already = NULL;

				if(tmp == NULL)
				    throw Ememory("filtre_merge");
				if(dynamic_cast<eod *>(tmp) == NULL)
				    throw SRC_BUG;
				cat.add(tmp); // add eod to catalogue (add cursor)
				cat.read_if_present(NULL, already); // equivalent to eod for the reading methods
			    }
			    catch(...)
			    {
				if(tmp != NULL)
				    delete tmp;
				throw;
			    }
			}
		    }
		    index++;  // next archive
		    etiquette_offset = corres_copy.size();
		}
	    }
	    catch(Ethread_cancel & e)
	    {
		abort = true;
		dialog.warning(gettext("File selection has been aborted. Now building the resulting archive with the already selected files"));
		if(e.immediate_cancel())
		    throw;
	    }
	}
	catch(...)
	{
	    if(decr != NULL)
	    {
		delete decr;
		decr = NULL;
		overwrite = NULL;
	    }
	    throw;
	}

	if(decr != NULL)
	{
	    delete decr;
	    decr = NULL;
	    overwrite = NULL;
	}

	    // STEP 2:
	    // 'cat' is now completed
	    // we must copy the file's data and EA to the archive


	if(info_details)
	    dialog.warning("Copying filtered files to the resulting archive...");

	cat.reset_read();
	juillet = FAKE_ROOT;

	try
	{
	    thr_cancel.block_delayed_cancellation(true);

	    while(cat.read(e))
	    {
		entree *e_var = const_cast<entree *>(e);
		nomme *e_nom = dynamic_cast<nomme *>(e_var);
		inode *e_ino = dynamic_cast<inode *>(e_var);
		file *e_file = dynamic_cast<file *>(e_var);
		mirage *e_mir = dynamic_cast<mirage *>(e_var);

		file::get_data_mode keep_mode = keep_compressed ? file::keep_compressed : file::keep_hole;

		if(e_mir != NULL)
		    if(!e_mir->is_inode_wrote()) // we store only once the inode pointed by a set of hard links
		    {
			e_ino = e_mir->get_inode();
			e_file = dynamic_cast<file *>(e_ino);
		    }

		juillet.enfile(e);

		if(e_ino != NULL) // inode
		{
		    bool compute_file_crc = false;

		    if(e_file != NULL)
		    {
			const crc *val = NULL;

			if(!e_file->get_crc(val)) // this can occur when reading an old archive format
			    compute_file_crc = true;
		    }

			// deciding whether the file will be compressed or not

		    if(e_file != NULL)
		    {
			if(keep_mode != file::keep_compressed)
			    if(compr_mask.is_covered(e_nom->get_name()) && e_file->get_size() >= min_compr_size)
				e_file->change_compression_algo_write(stock_algo);
			    else
				e_file->change_compression_algo_write(none);
			else // keep compressed:
			    e_file->change_compression_algo_write(e_file->get_compression_algo_read());
		    }

			// deciding whether the sparse file detection mechanism will be enabled or not
			// the sparse file detection mechanism is faked active in keep_compressed mode
			// because we need to record that sparse file datastructure is used as compressed data

		    if(e_file != NULL)
		    {
			if(sparse_file_min_size > 0 && keep_mode != file::keep_compressed) // sparse_file detection is activated
			{
			    if(e_file->get_size() > sparse_file_min_size)
			    {
				e_file->set_sparse_file_detection_write(true);
				keep_mode = file::normal;
			    }
			    else
				if(e_file->get_sparse_file_detection_read())
				{
				    e_file->set_sparse_file_detection_write(false);
				    keep_mode = file::normal;
				}
			}
			else // sparse file layer or absence of layer is unchanged
			    e_file->set_sparse_file_detection_write(e_file->get_sparse_file_detection_read());
		    }

			// saving inode's data

		    cat.pre_add(e, stockage);

		    if(!save_inode(dialog,
				   juillet.get_string(),
				   e_ino,
				   stockage,
				   info_details,
				   stock_algo,
				   true,       // alter_atime
				   false,      // check_change
				   compute_file_crc,
				   keep_mode,
				   cat,
				   0,     // repeat_count
				   0,     // repeat_byte
				   sparse_file_min_size,
				   NULL,  // semaphore
				   fake_repeat))
			throw SRC_BUG;
		    else // succeeded saving
		    {
			if(e_file != NULL)
			    e_file->change_location(stockage);

			if(e_mir != NULL)
			    e_mir->set_inode_wrote(true);
		    }

			// saving inode's EA

		    if(e_ino->ea_get_saved_status() == inode::ea_full)
		    {
			cat.pre_add_ea(e, stockage);
			if(save_ea(dialog, juillet.get_string(), e_ino, stockage, NULL, info_details, stock_algo))
			    e_ino->change_ea_location(stockage);
			cat.pre_add_ea_crc(e, stockage);
		    }
		}
		else // not an inode
		{
		    cat.pre_add(e, stockage);
		    if(e_mir != NULL && (e_mir->get_inode()->get_saved_status() == s_saved || e_mir->get_inode()->ea_get_saved_status() == inode::ea_full))
			if(info_details)
			    dialog.warning(string(gettext("Adding Hard link to archive: "))+juillet.get_string());
		}

		    // we can now check for interrution requests
		thr_cancel.block_delayed_cancellation(false);
		thr_cancel.block_delayed_cancellation(true);
	    }
	}
	catch(Ethread_cancel & e)
	{
	    cat.tail_catalogue_to_current_read();
	    stockage->change_algo(stock_algo);
	    thr_cancel.block_delayed_cancellation(false);
	    throw;
	}

	stockage->change_algo(stock_algo);
	thr_cancel.block_delayed_cancellation(false);

	if(abort)
	    throw Ethread_cancel(false, 0);
    }


    static bool save_inode(user_interaction & dialog,
			   const string & info_quoi,
			   inode * & ino,
			   compressor *stock,
			   bool info_details,
			   compression compr_used,
			   bool alter_atime,
			   bool check_change,
			   bool compute_crc,
			   file::get_data_mode keep_mode,
			   const catalogue &cat,
			   const infinint & repeat_count,
			   const infinint & repeat_byte,
			   const infinint & hole_size,
			   semaphore *sem,
			   infinint & current_wasted_bytes)
    {
	bool ret = true;
	infinint current_repeat_count = 0;
	bool loop;

        if(ino == NULL)
	    return true;
	if(stock == NULL)
            throw SRC_BUG;
        if(ino->get_saved_status() != s_saved)
	{
	    if(sem != NULL)
		sem->raise(info_quoi, ino, false);
            return ret;
	}
	if(compute_crc && (keep_mode != file::normal && keep_mode != file::plain))
	    throw SRC_BUG; // cannot compute crc if data is compressed or hole datastructure not interpreted
        if(info_details)
            dialog.warning(string(gettext("Adding file to archive: ")) + info_quoi);

        file *fic = dynamic_cast<file *>(ino);

        if(fic != NULL)
        {
	    if(sem != NULL)
		sem->raise(info_quoi, ino, true);

	    try
	    {

		do
		{
		    stock->flush_write(); // sanity action

		    loop = false;
		    infinint start = stock->get_position();
		    generic_file *source = NULL;

		    try
		    {
			switch(keep_mode)
			{
			case file::keep_compressed:
			    if(fic->get_compression_algo_read() != fic->get_compression_algo_write())
				keep_mode = file::keep_hole;
			    source = fic->get_data(keep_mode);
			    break;
			case file::keep_hole:
			    source = fic->get_data(keep_mode);
			    break;
			case file::normal:
			    if(fic->get_sparse_file_detection_read()) // source file already holds a sparse_file structure
				source = fic->get_data(file::plain); // we must hide the holes for it can be redetected
			    else
				source = fic->get_data(file::normal);
			    break;
			case file::plain:
			    throw SRC_BUG; // save_inode must never be called with this value
			default:
			    throw SRC_BUG; // unknown value for keep_mode
			}
		    }
		    catch(...)
		    {
			cat.pre_add_failed_mark(stock);
			throw;
		    }

		    if(source != NULL)
		    {
			try
			{
			    generic_file *dest = stock;
			    sparse_file *dst_hole = NULL;
			    infinint crc_size = tools_file_size_to_crc_size(fic->get_size());
			    crc * val = NULL;
			    const crc * original = NULL;
			    bool crc_available = false;

			    fic->set_offset(start);
			    source->skip(0);

			    if(keep_mode == file::keep_compressed)
				stock->change_algo(none);
			    else
				stock->change_algo(fic->get_compression_algo_write());

			    try
			    {
				if(fic->get_sparse_file_detection_write() && keep_mode != file::keep_compressed && keep_mode != file::keep_hole)
				{
					// creating the sparse_file to copy data to destination

				    dst_hole = new (nothrow) sparse_file(stock, hole_size);
				    if(dst_hole == NULL)
					throw Ememory("save_inode");
				    dest = dst_hole;
				}

				if(!compute_crc)
				    crc_available = fic->get_crc(original);
				else
				    crc_available = false;

				source->copy_to(*dest, crc_size, val);
				if(val == NULL)
				    throw SRC_BUG;

				if(compute_crc)
				    fic->set_crc(*val);
				else
				{
				    if(keep_mode == file::normal && crc_available)
				    {
					if(original == NULL)
					    throw SRC_BUG;
					if(typeid(*original) != typeid(*val))
					    throw SRC_BUG;
					if(*original != *val)
					    throw Erange("save_inode", gettext("Copied data does not match CRC"));
				    }
				}

				if(dst_hole != NULL)
				{
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
				source->terminate();
			    }
			    catch(...)
			    {
				if(dst_hole != NULL)
				{
				    delete dst_hole;
				    dst_hole = NULL;
				    dest = NULL;
				}
				if(val != NULL)
				{
				    delete val;
				    val = NULL;
				}
				throw;
			    }

			    if(val != NULL)
			    {
				delete val;
				val = NULL;
			    }

			    if(dst_hole != NULL)
			    {
				dst_hole->terminate();
				delete dst_hole;
				dst_hole = NULL;
				dest = NULL;
			    }

			    stock->sync_write();

			    if(stock->get_position() >= start)
				fic->set_storage_size(stock->get_position() - start);
			    else
				throw SRC_BUG;
			}
			catch(...)
			{
			    delete source;
			    source = NULL;

				// restore atime of source
			    if(!alter_atime)
				tools_noexcept_make_date(info_quoi, ino->get_last_access(), ino->get_last_modif());
			    throw;
			}
			delete source;
			source = NULL;


			    // adding the data CRC if escape marks are used
			cat.pre_add_crc(ino, stock);

			    // checking for last_modification date change
			if(check_change)
			{
			    bool changed = false;

			    try
			    {
				changed = fic->get_last_modif() != tools_get_mtime(info_quoi);
			    }
			    catch(Erange & e)
			    {
				dialog.warning(tools_printf(gettext("File has disappeared while we were reading it, cannot check whether it has changed during its backup: %S"), &info_quoi));
				changed = false;
			    }

			    if(changed)
			    {
				if(current_repeat_count < repeat_count)
				{
				    current_repeat_count++;
				    infinint wasted = stock->get_position() - start;
				    if(repeat_byte == 0 || (current_wasted_bytes + wasted < repeat_byte))
				    {
					if(info_details)
					    dialog.warning(tools_printf(gettext("WARNING! File modified while reading it for backup. Performing retry %i of %i"), &current_repeat_count, &repeat_count));
					current_wasted_bytes += wasted;
					cat.pre_add_waste_mark(stock);
					loop = true;

					    // updating the last modification date of file
					fic->set_last_modif(tools_get_mtime(info_quoi));

					    // updating the size of the file
					fic->change_size(tools_get_size(info_quoi));
				    }
				    else
				    {
					dialog.warning(string(gettext("WARNING! File modified while reading it for backup. No more retry for that file to not exceed the wasted byte limit. File is ")) + info_quoi);
					fic->set_dirty(true);
					ret = false;
				    }
				}
				else
				{
				    dialog.warning(string(gettext("WARNING! File modified while reading it for backup, but no more retry allowed: ")) + info_quoi);
				    fic->set_dirty(true);
				    cat.pre_add_dirty(stock); // when in sequential reading
				    ret = false;
				}
			    }
			}

			    // restore atime of source
			if(!alter_atime)
			    tools_noexcept_make_date(info_quoi, ino->get_last_access(), ino->get_last_modif());
		    }
		    else
			throw SRC_BUG; // saved_status == s_saved, but no data available, and no exception raised;
		}
		while(loop);
	    }
	    catch(...)
	    {
		if(sem != NULL)
		    sem->lower();
		throw;
	    }
	    if(sem != NULL)
		sem->lower();
	}
	else // fil != NULL
	    if(sem != NULL)
	    {
		sem->raise(info_quoi, ino, true);
		sem->lower();
	    }

	return ret;
    }

    static bool save_ea(user_interaction & dialog,
			const string & info_quoi, inode * & ino, compressor *stock,
			const inode * ref, bool info_details, compression compr_used)
    {
        bool ret = false;
        try
        {
            switch(ino->ea_get_saved_status())
            {
            case inode::ea_full: // if there is something to save
		if(ino->get_ea() != NULL)
		{
		    crc * val = NULL;

		    try
		    {
			if(info_details)
			    dialog.warning(string(gettext("Saving Extended Attributes for ")) + info_quoi);
			ino->ea_set_offset(stock->get_position());
			stock->change_algo(compr_used); // always compress EA (no size or filename consideration)
			stock->reset_crc(tools_file_size_to_crc_size(ino->ea_get_size())); // start computing CRC for any read/write on stock
			try
			{
			    ino->get_ea()->dump(*stock);
			}
			catch(...)
			{
			    val = stock->get_crc(); // this keeps "stock" in a coherent status
			    throw;
			}
			val = stock->get_crc();
			ino->ea_set_crc(*val);
			ino->ea_detach();
			stock->flush_write();
			ret = true;
		    }
		    catch(...)
		    {
			if(val != NULL)
			    delete val;
			throw;
		    }
		    if(val != NULL)
			delete val;
		}
		else
		    throw SRC_BUG;
		break;
            case inode::ea_partial:
	    case inode::ea_none:
		break;
	    case inode::ea_fake:
                throw SRC_BUG; //filesystem, must not provide inode in such a status
	    case inode::ea_removed:
		throw SRC_BUG; //filesystem, must not provide inode in such a status
            default:
                throw SRC_BUG;
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
	catch(Ethread_cancel & e)
	{
	    throw;
	}
        catch(Egeneric & e)
        {
            dialog.warning(string(gettext("Error saving Extended Attributes for ")) + info_quoi + ": " + e.get_message());
        }
        return ret;
    }


    static void restore_atime(const string & chemin, const inode * & ptr)
    {
	const file * ptr_f = dynamic_cast<const file *>(ptr);
	if(ptr_f != NULL)
	    tools_noexcept_make_date(chemin, ptr_f->get_last_access(), ptr_f->get_last_modif());
    }


    static void do_EA_transfert(user_interaction &dialog, over_action_ea action, inode *place_ino, const inode *add_ino)
    {
	ea_attributs *tmp_ea = NULL;

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

	if(add_ino == NULL) // to_add is not an inode thus cannot have any EA
	    return;         // we do nothing in any case as there is not different EA set in conflict

	if(place_ino == NULL) // in_place is not an inode thus cannot have any EA
	    return;           // nothing can be done neither here as the resulting object (in_place) cannot handle EA

	    // in the following we know that both in_place and to_add are inode, we use them thanks to their inode * pointers (place_ino and add_ino)

	switch(action)
	{
	case EA_overwrite:
	    switch(add_ino->ea_get_saved_status())
	    {
	    case inode::ea_none:
	    case inode::ea_removed:
		place_ino->ea_set_saved_status(inode::ea_none);
		break;
	    case inode::ea_partial:
	    case inode::ea_fake:
		place_ino->ea_set_saved_status(inode::ea_partial);
		place_ino->set_last_change(add_ino->get_last_change());
		break;
	    case inode::ea_full:
		tmp_ea = new (nothrow) ea_attributs(*add_ino->get_ea()); // we clone the EA of add_ino
		if(tmp_ea == NULL)
		    throw Ememory("filtre::do_EA_transfert");
		try
		{
		    if(place_ino->ea_get_saved_status() == inode::ea_full) // then we must drop the old EA:
			place_ino->ea_detach();
		    else
			place_ino->ea_set_saved_status(inode::ea_full);
		    place_ino->ea_attach(tmp_ea);
		    tmp_ea = NULL;
		    place_ino->set_last_change(add_ino->get_last_change());
		}
		catch(...)
		{
		    if(tmp_ea != NULL)
		    {
			delete tmp_ea;
			tmp_ea = NULL;
		    }
		    throw;
		}
		break;
	    default:
		throw SRC_BUG;
	    }
	    break; // end of case EA_overwrite for action
	case EA_overwrite_mark_already_saved:
	    if(add_ino->ea_get_saved_status() != inode::ea_none)
		place_ino->set_last_change(add_ino->get_last_change());
	    place_ino->ea_set_saved_status(add_ino->ea_get_saved_status()); // at this step, ea_full may be set, it will be changed to ea_partial below.
	    if(place_ino->ea_get_saved_status() == inode::ea_full || place_ino->ea_get_saved_status() == inode::ea_fake)
		place_ino->ea_set_saved_status(inode::ea_partial);
	    break;
	case EA_merge_preserve:
	    if(place_ino->ea_get_saved_status() == inode::ea_full && add_ino->ea_get_saved_status() == inode::ea_full) // we have something to merge
	    {
		tmp_ea = new (nothrow) ea_attributs();
		if(tmp_ea == NULL)
		    throw Ememory("filtre.cpp:do_EA_transfert");
		try
		{
		    merge_ea(*place_ino->get_ea(), *add_ino->get_ea(), *tmp_ea);
		    place_ino->ea_detach();
		    place_ino->ea_attach(tmp_ea);
		    tmp_ea = NULL;
		}
		catch(...)
		{
		    if(tmp_ea != NULL)
		    {
			delete tmp_ea;
			tmp_ea = NULL;
		    }
		    throw;
		}
	    }
	    else
		if(add_ino->ea_get_saved_status() == inode::ea_full)
		{
		    place_ino->ea_set_saved_status(inode::ea_full); // it was not the case else we would have executed the above block
		    tmp_ea = new (nothrow) ea_attributs(*add_ino->get_ea());   // we clone the EA set of to_add
		    if(tmp_ea == NULL)
			throw Ememory("filtre.cpp:do_EA_transfert");
		    try
		    {
			place_ino->ea_attach(tmp_ea);
			tmp_ea = NULL;
		    }
		    catch(...)
		    {
			if(tmp_ea != NULL)
			{
			    delete tmp_ea;
			    tmp_ea = NULL;
			}
			throw;
		    }
		}
		// else nothing is done: either res_ino has full EA but ref_ino has not
		// or res_ino has not full EA nor do has ref_ino and nothing can be done neither
	    break;
	case EA_merge_overwrite:
	    if(place_ino->ea_get_saved_status() == inode::ea_full && add_ino->ea_get_saved_status() == inode::ea_full)
	    {
		tmp_ea = new (nothrow) ea_attributs();
		if(tmp_ea == NULL)
		    throw Ememory("filtre.cpp:do_EA_transfert");
		try
		{
		    merge_ea(*add_ino->get_ea(), *place_ino->get_ea(), *tmp_ea);
		    place_ino->ea_detach();
		    place_ino->ea_attach(tmp_ea);
		    tmp_ea = NULL;
		}
		catch(...)
		{
		    if(tmp_ea != NULL)
		    {
			delete tmp_ea;
			tmp_ea = NULL;
		    }
		    throw;
		}
	    }
	    else
		if(add_ino->ea_get_saved_status() == inode::ea_full)
		{
		    place_ino->ea_set_saved_status(inode::ea_full); // it was not the case else we would have executed the above block
		    tmp_ea = new (nothrow) ea_attributs(*add_ino->get_ea());
		    if(tmp_ea == NULL)
			throw Ememory("filtre.cpp:do_EA_transfert");
		    try
		    {
			place_ino->ea_attach(tmp_ea);
			tmp_ea = NULL;
		    }
		    catch(...)
		    {
			if(tmp_ea != NULL)
			{
			    delete tmp_ea;
			    tmp_ea = NULL;
			}
			throw;
		    }
		}
		// else nothing is done: either res_ino has full EA but ref_ino has not
		// or res_ino has not full EA nor do has ref_ino and nothing can be done neither
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


    static entree *make_clone(const nomme *ref, map<infinint, etoile*> & hard_link_base, const infinint & etiquette_offset)
    {
	entree *dolly = NULL; // will be the address of the cloned object
	string the_name;
	const mirage *ref_mir = dynamic_cast<const mirage *>(ref);

	if(ref == NULL)
	    throw SRC_BUG;

	the_name = ref->get_name();


	if(ref_mir != NULL) // this is hard linked inode
	{
		// check whether this is the first time we see this file (in list of file covered by the file masks)
	    map <infinint, etoile *>::iterator it = hard_link_base.find(ref_mir->get_etiquette() + etiquette_offset);
	    if(it == hard_link_base.end()) // this inode has not been yet recorded in the resulting archive
	    {
		etoile *filante = NULL;
		dolly = ref_mir->get_inode()->clone(); // we must clone the attached inode
		try
		{
		    inode *dollinode = dynamic_cast<inode *>(dolly);

		    if(dollinode == NULL)
			throw Ememory("filtre:make_clone");

		    infinint shift_etiquette = ref_mir->get_etiquette() + etiquette_offset;
		    filante = new (nothrow) etoile(dollinode, shift_etiquette);
		    if(filante == NULL)
			throw Ememory("make_clone");
		    try
		    {
			dolly = NULL; // the inode is now managed by filante
			dolly = new (nothrow) mirage(the_name, filante);
			if(dolly == NULL)
			    throw Ememory("make_clone");
			try
			{
			    hard_link_base[shift_etiquette] = filante; // we now record this file_etiquette in the map of already enrolled hard_link sets
			}
			catch(...)
			{
			    filante = NULL; // now managed by the mirage pointed to by dolly
			    throw;
			}
		    }
		    catch(...)
		    {
			if(filante != NULL)
			{
			    delete filante;
			    filante = NULL;
			}
			throw;
		    }
		}
		catch(...)
		{
		    if(dolly != NULL)
		    {
			delete dolly;
			dolly = NULL;
		    }
		    throw;
		}
	    }
	    else // already added to archive
		dolly = new (nothrow) mirage(the_name, it->second); // we make a new mirage pointing to the etoile already involved in the catalogue under construction
	}
	else // not a hard_link file
	    dolly = ref->clone();  // we just clone the entry

	if(dolly == NULL)
	    throw Ememory("make_clone");

	return dolly;
    }


    static void clean_hard_link_base_from(const mirage *mir, map<infinint, etoile *> & hard_link_base)
    {
	if(mir->get_etoile_ref_count() == 0)
	    throw SRC_BUG; // count should be >= 1

	if(mir->get_etoile_ref_count() == 1)
	{
	    map<infinint, etoile *>::iterator it = hard_link_base.find(mir->get_etiquette());
	    const inode *al_ptr_ino = mir->get_inode();
	    if(al_ptr_ino == NULL)
		throw SRC_BUG;
	    if(it == hard_link_base.end())
		throw SRC_BUG; // the etoile object pointed to by dolly_mir should be known by corres_copy
	    hard_link_base.erase(it);
	}
    }

    static const crit_action *make_overwriting_for_only_deleted()
    {
	const crit_action *ret = new (nothrow) testing(crit_invert(crit_in_place_is_inode()), crit_constant_action(data_preserve, EA_preserve), crit_constant_action(data_overwrite, EA_overwrite));
	if(ret == NULL)
	    throw Ememory("make_overwriting_fir_only_deleted");

	return ret;
    }

} // end of namespace
