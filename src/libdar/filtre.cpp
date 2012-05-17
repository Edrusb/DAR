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
// to contact the author : dar.linux@free.fr
/*********************************************************************/

#include "../my_config.h"

#include <map>
#include "filtre.hpp"
#include "user_interaction.hpp"
#include "erreurs.hpp"
#include "filesystem.hpp"
#include "ea.hpp"
#include "defile.hpp"
#include "test_memory.hpp"
#include "null_file.hpp"
#include "thread_cancellation.hpp"

using namespace std;

#define SKIPPED "Skipping file: "

namespace libdar
{

	// returns false if file has changed during backup (inode is saved however, but the saved data may be invalid)
	// return true if file has not change
	// throw exceptions in case of error
    static bool save_inode(user_interaction & dialog,
			   const string &info_quoi, inode * & ino,
			   compressor *stock, bool info_details,
			   const mask &compr_mask, compression compr_used,
			   const infinint & min_size_compression, bool alter_atime,
			   bool check_change,
			   bool compute_crc,
			   bool keep_compressed);
    static bool save_ea(user_interaction & dialog,
			const string & info_quoi, inode * & ino, compressor *stock,
			const inode * ref, bool info_details, compression compr_used);
    static void restore_atime(const string & chemin, const inode * & ptr);

    void filtre_restore(user_interaction & dialog,
			const mask & filtre,
                        const mask & subtree,
                        catalogue & cat,
                        bool detruire,
                        const path & fs_racine,
                        bool fs_allow_overwrite,
                        bool fs_warn_overwrite,
                        bool info_details,
                        statistics & st,
                        bool only_if_more_recent,
			const mask & ea_mask,
                        bool flat,
			inode::comparison_fields what_to_check,
			bool warn_remove_no_match,
                        const infinint & hourshift,
			bool empty,
			bool ea_erase,
			bool display_skipped)
    {
        defile juillet = fs_racine;
        const eod tmp_eod;
        const entree *e;
	thread_cancellation thr_cancel;

        filesystem_restore fs = filesystem_restore(dialog, fs_racine, fs_allow_overwrite, fs_warn_overwrite, info_details, ea_mask, what_to_check, warn_remove_no_match, empty, ea_erase);
        filesystem_diff fs_flat = filesystem_diff(dialog, fs_racine, false, ea_mask, false);
	    // info detail is set to false, to avoid duplication of informational messages already given by 'fs'
	    // alter_atime is set to true, to preserve atime to its original value, ctime could not be
	    // restored so its value has no need to be preserved here.

        st.clear();
        cat.reset_read();

        while(cat.read(e))
        {
            const nomme *e_nom = dynamic_cast<const nomme *>(e);
            const directory *e_dir = dynamic_cast<const directory *>(e);

            juillet.enfile(e);
	    thr_cancel.check_self_cancellation();
            if(e_nom != NULL)
            {
                try
                {
                    if(subtree.is_covered(juillet.get_string()) && (e_dir != NULL || filtre.is_covered(e_nom->get_name())))
                    {
                        const detruit *e_det = dynamic_cast<const detruit *>(e);
                        const inode *e_ino = dynamic_cast<const inode *>(e);
                        const hard_link *e_hard = dynamic_cast<const hard_link *>(e);
                        const etiquette *e_eti = dynamic_cast<const etiquette *>(e);
                        entree *dolly = NULL; // inode of replacement for hard links

                        try
                        {
                            if(e_hard != NULL)
                            {
                                inode *tmp = NULL;
                                dolly = e_hard->get_inode()->clone();
                                if(dolly == NULL)
                                    throw Ememory("filtre_restore");
                                tmp = dynamic_cast<inode *>(dolly);
                                if(tmp == NULL)
                                    throw SRC_BUG; // should be an inode
                                tmp->change_name(e_hard->get_name());
                                e_ino = const_cast<const inode *>(tmp);
                                if(e_ino == NULL)
                                    throw SRC_BUG; // !?! how is this possible ?
                                st.incr_hard_links();
                            }

                            if(e_det != NULL)
                            {
                                if(detruire && !flat)
                                {
				    bool notusedhere;

                                    if(info_details)
                                        dialog.warning(string(gettext("Removing file: ")) + juillet.get_string());
                                    if(fs.write(e, notusedhere))
                                        st.incr_deleted();
                                }
                            }
                            else
                                if(e_ino != NULL)
                                {
                                    nomme *exists_nom = NULL;
                                    inode *exists = NULL;
				    bool created = false; // will carry the information whether the restoration had created a new file or overwrote an old one
				    bool restored = false; // will be true if file restoration succeeded

                                        // checking if file to restore already exists, retreiving info if available
                                    if(!flat)
                                        exists_nom = fs.get_before_write(e_ino);
                                    else
                                    {
                                        string tmp = e_ino->get_name();

                                        if(e_dir != NULL || !fs_flat.read_filename(tmp, exists_nom))
                                            exists_nom = NULL;
                                    }

                                    exists = dynamic_cast<inode *>(exists_nom);
                                    if(exists_nom != NULL && exists == NULL)
                                        throw SRC_BUG; // filesystem should always provide inode or nothing

                                    try
                                    {
                                            // checking the file contents & inode
                                        if(e_ino->get_saved_status() == s_saved || (e_hard != NULL && fs.known_etiquette(e_hard->get_etiquette())))
                                        {
                                            if(!only_if_more_recent || exists == NULL || e_ino->is_more_recent_than(*exists, hourshift))
                                            {
                                                if(!flat || e_dir == NULL)
                                                {
                                                    if(info_details)
                                                        dialog.warning(string(gettext("Restoring file: ")) + juillet.get_string());
						    restored = fs.write(e, created); // e and not e_ino, it may be a hard link now
                                                    if(restored)
                                                        st.incr_treated();
                                                }
                                                else
                                                    st.incr_ignored();
                                            }
                                            else // file is less recent than the one in the filesystem
                                            {
                                                    // if it is a directory, just recording we go in it now
                                                if(e_dir != NULL && !flat)
                                                    fs.pseudo_write(e_dir);
                                                if(e_eti != NULL) // future hard link will get linked against this file
                                                    fs.write_hard_linked_target_if_not_set(e_eti, flat ? (fs_racine+e_ino->get_name()).display() : juillet.get_string());
                                                if(e_dir == NULL || !flat)
                                                    st.incr_tooold();
                                                else
                                                    st.incr_ignored();
                                            }
                                        }
                                        else // no data saved for this entry (no change since reference backup)
                                        {
                                                // if it is a directory, just recording we go in it now
                                            if(e_dir != NULL && !flat)
                                                fs.pseudo_write(e_dir);
                                            if(e_eti != NULL) // future hard link will get linked against this file
                                                fs.write_hard_linked_target_if_not_set(e_eti, flat ? (fs_racine+e_ino->get_name()).display() : juillet.get_string());
                                            if(e_dir == NULL || !flat)
                                                st.incr_skipped();
                                            else
                                                st.incr_ignored();
                                        }

					    // checking the EA list
					    //
					    // need to have EA data to restore and an
					    // existing inode of the same type in
					    // filesystem, to be able to set EA to
					    // an existing inode
					if(e_ino->ea_get_saved_status() == inode::ea_full // we have EA available in archive
					   &&
					   ((exists != NULL && exists->same_as(*e_ino))  // the file now exists in filesystem
					    || e_ino->get_saved_status() == s_saved)   // either initially or just restored
					   &&
					   (!flat || e_dir == NULL))                   // we are not in flat mode restoring a directory
					{
					    try
					    {
						bool ea_erase = fs.get_ea_erase();

						try
						{
						    bool local_fs_allow_over = fs_allow_overwrite;
						    bool local_fs_warn_over = fs_warn_overwrite;

						    if(restored && created)
						    {
							// this hack is necessary when the directory we are
							// restoring in has default EA set. The file we have
							// restored just above has already some EA set, we must
							// clear them, then we can restore the EA.

							fs.set_ea_erase(true);
							local_fs_allow_over = true;
							local_fs_warn_over = false;
						    }
						    if(fs.set_ea(e_nom, *(e_ino->get_ea(dialog)),
								 local_fs_allow_over,
								 local_fs_warn_over,
								 info_details))
							st.incr_ea_treated();
						}
						catch(...)
						{
						    fs.set_ea_erase(ea_erase);
						    throw;
						}
						fs.set_ea_erase(ea_erase);
					    }
					    catch(Erange & e)
					    {
						dialog.warning(string(gettext("Error while restoring EA for ")) + juillet.get_string() + " : " + e.get_message());
					    }
					    e_ino->ea_detach(); // in any case we clear memory
					}
                                    }
                                    catch(...)
                                    {
                                        if(exists_nom != NULL)
                                            delete exists_nom;
                                        throw;
                                    }
                                    if(exists_nom != NULL)
                                        delete exists_nom;
                                }
                                else
                                    throw SRC_BUG; // a nomme that is neither a detruit nor an inode !
                        }
                        catch(...)
                        {
                            if(dolly != NULL)
                                delete dolly;
                            throw;
                        }
                        if(dolly != NULL)
                            delete dolly;
                    }
                    else // inode not covered
                    {
			if(display_skipped)
			    dialog.warning(string(gettext(SKIPPED)) + juillet.get_string());

                        st.incr_ignored();
                        if(e_dir != NULL)
                        {
                            cat.skip_read_to_parent_dir();
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
		    dialog.warning(juillet.get_string() + gettext(" not restored (user choice)"));

                    if(e_dir != NULL && !flat)
                    {
                        dialog.warning(gettext("No file in this directory will be restored."));
                        cat.skip_read_to_parent_dir();
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
                catch(Egeneric & e)
                {
                    dialog.warning(string(gettext("Error while restoring ")) + juillet.get_string() + " : " + e.get_message());

                    if(e_dir != NULL && !flat)
                    {
                        dialog.warning(string(gettext("Warning! No file in that directory will be restored: ")) + juillet.get_string());
                        cat.skip_read_to_parent_dir();
                        juillet.enfile(&tmp_eod);
                    }
                    st.incr_errored();
                }
            }
            else // eod
                if(!flat)
		{
		    bool notusedhere;
                    (void)fs.write(e, notusedhere); // eod; don't care returned value
		}
        }
    }

    void filtre_sauvegarde(user_interaction & dialog,
			   const mask &filtre,
                           const mask &subtree,
                           compressor *stockage,
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
			   bool same_fs,
			   inode::comparison_fields what_to_check,
			   bool snapshot,
			   bool cache_directory_tagging,
			   bool display_skipped,
			   const infinint & fixed_date)
    {
        entree *e;
        const entree *f;
        defile juillet = fs_racine;
        const eod tmp_eod;
        compression stock_algo = stockage->get_algo();
	infinint root_fs_device;
        filesystem_backup fs = filesystem_backup(dialog,
						 fs_racine,
						 info_details,
						 ea_mask,
						 nodump,
						 alter_atime,
						 cache_directory_tagging,
						 root_fs_device);
	thread_cancellation thr_cancel;
	infinint skipped_dump, fs_errors;

        st.clear();
        cat.reset_add();
        ref.reset_compare();

	try
	{
	    while(fs.read(e, fs_errors, skipped_dump))
	    {
		nomme *nom = dynamic_cast<nomme *>(e);
		directory *dir = dynamic_cast<directory *>(e);
		inode *e_ino = dynamic_cast<inode *>(e);

		st.add_to_ignored(skipped_dump);
		st.add_to_errored(fs_errors);

		juillet.enfile(e);
		thr_cancel.check_self_cancellation();
		if(nom != NULL)
		{
		    try
		    {
			if(subtree.is_covered(juillet.get_string())
			   && (dir != NULL || filtre.is_covered(nom->get_name()))
			   && (! same_fs || e_ino == NULL || e_ino->get_device() == root_fs_device))
			{
			    hard_link *e_hard = dynamic_cast<hard_link *>(e);

			    if(e_hard != NULL)
			    {
				if(info_details)
				    dialog.warning(string(gettext("Recording hard link to archive: ")) + juillet.get_string());
				cat.add(e);
				st.incr_hard_links();
			    }
			    else // "e" is an inode
			    {
				bool known;

				if(fixed_date == 0)
				    known = ref.compare(e, f);
				else
				{
				    known = true; // emulate the presence of an inode
				    f = NULL;
				}

				try
				{
				    if(known || snapshot)
				    {
					const inode *f_ino = snapshot ? NULL : dynamic_cast<const inode *>(f);

					if(e_ino == NULL || (f_ino == NULL && !snapshot && fixed_date == 0))
					    throw SRC_BUG; // filesystem has provided a "nomme" which is not an "inode" thus which is a "detruit"

					if(!snapshot && ((fixed_date > 0 && e_ino->get_last_modif() >= fixed_date) || (fixed_date == 0 && e_ino->has_changed_since(*f_ino, hourshift, what_to_check))))
					{  // inode has changed
					    if(e_ino->get_saved_status() != s_saved)
						throw SRC_BUG; // filsystem should always provide "saved" "entree"

					    if(!save_inode(dialog, juillet.get_string(), e_ino, stockage, info_details, compr_mask, stock_algo, min_compr_size, alter_atime, true, true, false))
						st.incr_tooold();
					    st.incr_treated();
					}
					else // inode has not changed since last backup
					{
					    e_ino->set_saved_status(s_not_saved);
					    st.incr_skipped();
					}

					if(fixed_date == 0 || (e_ino->ea_get_saved_status() != inode::ea_none && e_ino->get_last_change() >= fixed_date))
					    if(!snapshot)
					    {
						if(save_ea(dialog, juillet.get_string(), e_ino, stockage, f_ino, info_details, stock_algo))
						    st.incr_ea_treated();
					    }
					    else  // doing a snapshot
					    {
						if(e_ino->ea_get_saved_status() == inode::ea_full)
						    e_ino->ea_set_saved_status(inode::ea_partial);
					    }
					else // EA has not to be saved (older than fixed_date)
					    if(e_ino->ea_get_saved_status() == inode::ea_full)
						e_ino->ea_set_saved_status(inode::ea_partial);

				    }
				    else // inode not present in the catalogue of reference
					if(e_ino != NULL)
					{
					    if(!save_inode(dialog, juillet.get_string(), e_ino, stockage, info_details, compr_mask, stock_algo, min_compr_size, alter_atime, true, true, false))
						st.incr_tooold();
					    st.incr_treated();

					    if(save_ea(dialog, juillet.get_string(), e_ino, stockage, NULL, info_details, stock_algo))
						st.incr_ea_treated();
					}
					else
					    throw SRC_BUG;  // filesystem has provided a "nomme" which is not a "inode" thus which is a "detruit"

				    file *tmp = dynamic_cast<file *>(e);
				    if(tmp != NULL)
					tmp->clean_data();

				    cat.add(e);
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
			    file_etiquette *eti = dynamic_cast<file_etiquette *>(e);

			    if(display_skipped)
				dialog.warning(string(gettext(SKIPPED)) + juillet.get_string());

			    if(dir != NULL && make_empty_dir)
				ig = ignode = new ignored_dir(*dir);
			    else
				ig = new ignored(nom->get_name());
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
			    if(eti != NULL) // must update filesystem to forget this etiquette
				fs.forget_etiquette(eti);
			    delete e; // we don't keep this object in the catalogue so we must destroy it
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
			    nomme *tmp = new ignored(nom->get_name());
			    dialog.warning(string(gettext("Error while saving ")) + juillet.get_string() + ": " + ex.get_message());
			    st.incr_errored();
			    file_etiquette *eti = dynamic_cast<file_etiquette *>(e);

				// we need to remove "e" from filesystem hard link reference if necessary
			    if(eti != NULL)
				fs.forget_etiquette(eti);

				// now we can destroy the object
			    delete e;

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
		    if(fixed_date == 0)
			ref.compare(e, f);
		    cat.add(e);
		}
	    }
	}
	catch(Ethread_cancel & e)
	{
	    if(!e.immediate_cancel())
		stockage->change_algo(stock_algo);
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
			   inode::comparison_fields what_to_check,
			   bool display_skipped,
			   const infinint & hourshift)
    {
        const entree *e;
        defile juillet = fs_racine;
        const eod tmp_eod;
        filesystem_diff fs = filesystem_diff(dialog, fs_racine ,info_details, ea_mask, alter_atime);
	thread_cancellation thr_cancel;

        st.clear();
        cat.reset_read();
        while(cat.read(e))
        {
            const directory *e_dir = dynamic_cast<const directory *>(e);
            const nomme *e_nom = dynamic_cast<const nomme *>(e);

            juillet.enfile(e);
	    thr_cancel.check_self_cancellation();
            try
            {
                if(e_nom != NULL)
                {
                    if(subtree.is_covered(juillet.get_string()) && (e_dir != NULL || filtre.is_covered(e_nom->get_name())))
                    {
                        nomme *exists_nom = NULL;
                        const inode *e_ino = dynamic_cast<const inode *>(e);

                        if(e_ino != NULL)
                            if(fs.read_filename(e_ino->get_name(), exists_nom))
                            {
                                try
                                {
                                    inode *exists = dynamic_cast<inode *>(exists_nom);
                                    directory *exists_dir = dynamic_cast<directory *>(exists_nom);

                                    if(exists != NULL)
                                    {
                                        try
                                        {
                                            e_ino->compare(dialog, *exists, ea_mask, what_to_check, hourshift);
                                            if(info_details)
                                                dialog.warning(string(gettext("OK   "))+juillet.get_string());
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
                                    throw;
                                }
                                delete exists_nom;
                            }
                            else // can't compare, nothing of that name in filesystem
                            {
                                dialog.warning(string(gettext("DIFF "))+ juillet.get_string() + gettext(": file not present in filesystem"));
                                if(e_dir != NULL)
                                {
                                    cat.skip_read_to_parent_dir();
                                    juillet.enfile(&tmp_eod);
                                }

                                st.incr_errored();
                            }
                        else // not an inode (for example a detruit, hard_link etc...), nothing to do
                            st.incr_treated();
                    }
                    else // not covered by filters
                    {
			if(display_skipped)
			    dialog.warning(string(gettext(SKIPPED)) + juillet.get_string());

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
                     statistics & st,
		     bool display_skipped)
    {
        const entree *e;
        defile juillet = FAKE_ROOT;
        null_file black_hole = null_file(dialog, gf_write_only);
        ea_attributs ea;
        infinint offset;
        crc check, original;
        const eod tmp_eod;
	thread_cancellation thr_cancel;

        st.clear();
        cat.reset_read();
        while(cat.read(e))
        {
            juillet.enfile(e);
	    thr_cancel.check_self_cancellation();
            try
            {
                const file *e_file = dynamic_cast<const file *>(e);
                const inode *e_ino = dynamic_cast<const inode *>(e);
                const directory *e_dir = dynamic_cast<const directory *>(e);
                const nomme *e_nom = dynamic_cast<const nomme *>(e);

                if(e_nom != NULL)
                {
                    if(subtree.is_covered(juillet.get_string()) && (e_dir != NULL || filtre.is_covered(e_nom->get_name())))
                    {
                            // checking data file if any
                        if(e_file != NULL && e_file->get_saved_status() == s_saved)
                        {
                            generic_file *dat = e_file->get_data(dialog);
                            if(dat == NULL)
                                throw Erange("filtre_test", gettext("Can't read saved data."));
                            try
                            {
                                dat->skip(0);
                                dat->copy_to(black_hole, check);
                                if(e_file->get_crc(original)) // CRC is not present in format "01"
                                    if(!same_crc(check, original))
                                        throw Erange("fitre_test", gettext("CRC error: data corruption."));
                            }
                            catch(...)
                            {
                                delete dat;
                                throw;
                            }
                            delete dat;
                        }
                            // checking inode EA if any
                        if(e_ino != NULL && e_ino->ea_get_saved_status() == inode::ea_full)
                        {
                            ea_attributs tmp = *(e_ino->get_ea(dialog));
                            e_ino->ea_detach();
                        }
                        st.incr_treated();

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
        map<infinint, file_etiquette *> corres;
	thread_cancellation thr_cancel;

        ref.reset_read();
        cat.reset_add();

        if(info_details)
            dialog.warning(gettext("Removing references to saved data from catalogue..."));

	thr_cancel.block_delayed_cancellation(true);
        while(ref.read(e))
        {
            const inode *e_ino = dynamic_cast<const inode *>(e);

            if(e_ino != NULL) // specific treatment for inode
            {
                entree *f = e_ino->clone();
                inode *f_ino = dynamic_cast<inode *>(f);
                file_etiquette *f_eti = dynamic_cast<file_etiquette *>(f);
                    // note about file_etiquette: the cloned object has the same etiquette
                    // and thus each etiquette correspond to two instances

                try
                {
                    if(f_ino == NULL)
                        throw SRC_BUG; // inode should clone an inode

                        // all data must be dropped
                    if(f_ino->get_saved_status() == s_saved)
                        f_ino->set_saved_status(s_fake);
                        // s_fake keep trace that this inode was saved
                        // in reference catalogue, else it is s_not_saved

                        // all EA must be dropped also
                    if(f_ino->ea_get_saved_status() == inode::ea_full)
                        f_ino->ea_set_saved_status(inode::ea_fake);

                        // mapping each etiquette to its file_etiquette clone address
                    if(f_eti != NULL)
                    {
                        if(corres.find(f_eti->get_etiquette()) == corres.end()) // not found
                            corres[f_eti->get_etiquette()] = f_eti;
                        else
                            throw SRC_BUG;
                            // two file_etiquette clones have the same etiquette
                            // this could be caused by a write error
                            // a bit error in an infinint is still possible and
                            // may make the value of the infinint (= etiquette here)
                            // be changed without incoherence.
                            // But, this error should have been detected at
                            // catalogue reading as some hard_link cannot be associate
                            // with a file_etiquette, thus this is a bug here.
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
                    entree *f = e->clone();
                    hard_link *f_hard = dynamic_cast<hard_link *>(f);

                    try
                    {
                        if(f_hard != NULL)
                        {
                            map<infinint,file_etiquette *>::iterator it = corres.find(f_hard->get_etiquette());

                            if(it != corres.end())
                                f_hard->set_reference(it->second);
                            else
                                throw SRC_BUG;
                                // no file_etiquette of that etiquette has ever been cloned,
                                // the order being respected, an file_etiquette is come always first
                                // before any hard_link on it, as there is no filter to skip the
                                // file_etiquette, thus it's a bug.
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
                else
                    throw SRC_BUG; // read provided NULL while returning true
        }
	thr_cancel.block_delayed_cancellation(false);
    }


    void filtre_merge(user_interaction & dialog,
		      const mask & filtre,
		      const mask & subtree,
		      compressor *stockage,
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
		      bool keep_compressed)
    {
	compression stock_algo = stockage->get_algo();
	thread_cancellation thr_cancel;
	const eod tmp_eod;
	catalogue *ref_tab[] = { ref1, ref2, NULL };
	infinint etiquette_offset = 0;
	map <infinint, file_etiquette *> corres_copy;
	const entree *e = NULL;
	st.clear();
	U_I index = 0;
	defile juillet = FAKE_ROOT;

	while(ref_tab[index] != NULL)
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
		    ptr = gettext("next");
		    break;
		}
		dialog.printf(gettext("Merging/filtering files from the %s archive"), ptr);
	    }

	    while(ref_tab[index]->read(e))
	    {
		const nomme *e_nom = dynamic_cast<const nomme *>(e);
		const directory *e_dir = dynamic_cast<const directory *>(e);

		juillet.enfile(e);
		thr_cancel.check_self_cancellation();
		if(e_nom != NULL)
		{
		    try
		    {
			const hard_link *e_hard = dynamic_cast<const hard_link *>(e);
			const etiquette *e_eti = dynamic_cast<const etiquette *>(e);
			const detruit *e_detruit = dynamic_cast<const detruit *>(e);
			const directory * e_dir = dynamic_cast<const directory *>(e);

			if(subtree.is_covered(juillet.get_string()) && (e_dir != NULL || filtre.is_covered(e_nom->get_name())))
			{
			    entree *dolly = NULL;
			    const nomme *already_here = NULL;
			    string the_name = e_nom->get_name();

			    if(!cat.read_if_present(& the_name, already_here)) // No entry of that name already exists in the resulting catalogue
			    {
				    // first we must make a copy (a clone) of the covered object

				    // and we must consider hard links
				if(e_eti != NULL) // this is either a file_etiquette or a hard_link
				{
					// check whether this is the first time we see this file (in list of file covered by the file masks)
				    map <infinint, file_etiquette *>::iterator it = corres_copy.find(e_eti->get_etiquette() + etiquette_offset);
				    if(it == corres_copy.end()) // this inode has not been yet recorded in the resulting archive
				    {
					const file_etiquette *tmp = e_eti->get_inode();
					file_etiquette *dolly_eti = NULL;

					if(tmp == NULL)
					    throw SRC_BUG;
					dolly = tmp->clone(); // we must build a file_etiquette, copy of the one referenced by this etiquette entry (entry which may be a hard_link)
					if(dolly == NULL)
					    throw Ememory("filtre:filtre_merge");
					dolly_eti = dynamic_cast<file_etiquette *>(dolly);
					if(dolly_eti == NULL)
					    throw SRC_BUG; // a clone of a file_etiquette is not a file_etiquette ?!?
					try
					{
					    infinint shift_etiquette = e_eti->get_etiquette() + etiquette_offset;
					    dolly_eti->change_etiquette(shift_etiquette);
					    dolly_eti->change_name(the_name);
					    corres_copy[shift_etiquette] = dolly_eti; // we must now record this file_etiquette in the map of already enrolled hard_link sets
					}
					catch(...)
					{
					    if(dolly != NULL)
						delete dolly;
					    dolly = NULL;
					    throw;
					}
				    }
				    else // already added to archive
					dolly = new hard_link(the_name, it->second); // we make a hard link pointing to the file_etiquette already involved in the archive
				}
				else // not a hard_link file
				    dolly = e->clone();  // we just clone the entry

				if(dolly == NULL)
				    throw Ememory("filtre_merge");

				    // now that we have a clone object we must add the copied object to the catalogue

				try
				{
				    cat.add(dolly);
				    st.incr_treated();

				    if(e_hard != NULL)
					st.incr_hard_links();

				    if(e_detruit != NULL)
					st.incr_deleted();

				    if(e_dir != NULL) // we must chdir also for the *reading* method of this catalogue object
				    {
					if(!cat.read_if_present(&the_name, already_here))
					    throw SRC_BUG;
				    }
				}
				catch(...)
				{
				    if(dolly != NULL)
					delete dolly;
				    throw;
				}
			    }
			    else // entry already present in the resulting catalogue
			    {
				if(display_skipped)
				    dialog.warning(string(gettext(SKIPPED)) + juillet.get_string() + gettext(" File already present in archive"));

				st.incr_ignored();
				if(e_dir != NULL)
				{
				    ref_tab[index]->skip_read_to_parent_dir();
				    cat.skip_read_to_parent_dir();
				    juillet.enfile(&tmp_eod);
				}
			    }
			}
			else // excluded by filters
			{
			    if(e_dir != NULL && make_empty_dir)
			    {
				ignored_dir *igndir = new ignored_dir(*e_dir);
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
		else  // entree is not a nomme object (eod)
		{
		    entree *tmp = e->clone();
		    try
		    {
			const nomme *already = NULL;

			if(tmp == NULL)
			    throw Ememory("filtre_merge");
			if(dynamic_cast<eod *>(tmp) == NULL)
			    throw SRC_BUG;
			cat.add(tmp); // add eod to catalogue
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

	if(info_details)
	    dialog.warning("Copying filtered files to the resulting archive...");

	cat.reset_read();
	juillet = FAKE_ROOT;
	while(cat.read(e))
	{
	    entree *e_var = const_cast<entree *>(e);
	    inode *e_ino = dynamic_cast<inode *>(e_var);
	    file *e_file = dynamic_cast<file *>(e_var);

	    juillet.enfile(e);
	    thr_cancel.check_self_cancellation();
	    if(e_ino != NULL) // inode
	    {
		bool compute_file_crc = false;

		if(e_file != NULL)
		{
		    crc val;

		    if(!e_file->get_crc(val)) // this can occur when reading an old archive format
			compute_file_crc = true;
		}

		if(!save_inode(dialog, juillet.get_string(), e_ino, stockage, info_details, compr_mask, stock_algo, min_compr_size, true, false, compute_file_crc, keep_compressed))
		    throw SRC_BUG;
		else
		{
		    if(e_file != NULL)
		    {
			e_file->change_location(stockage);
			if(!keep_compressed)
			    e_file->change_compression_algo_used(stock_algo);
		    }
		}

		if(e_ino->ea_get_saved_status() == inode::ea_full)
		{
		    if(save_ea(dialog, juillet.get_string(), e_ino, stockage, NULL, info_details, stock_algo))
		    {
			st.incr_ea_treated();
			e_ino->change_ea_location(stockage);
		    }
		}
	    }
	}

        stockage->change_algo(stock_algo);
    }


    static bool save_inode(user_interaction & dialog,
			   const string & info_quoi, inode * & ino,
			   compressor *stock, bool info_details,
			   const mask &compr_mask, compression compr_used,
			   const infinint & min_size_compression, bool alter_atime,
			   bool check_change,
			   bool compute_crc,
			   bool keep_compressed)
    {
	bool ret = true;

        if(ino == NULL || stock == NULL)
            throw SRC_BUG;
        if(ino->get_saved_status() != s_saved)
            return ret;
        if(info_details)
            dialog.warning(string(gettext("Adding file to archive: ")) + info_quoi);

        file *fic = dynamic_cast<file *>(ino);

        if(fic != NULL)
        {
            infinint start = stock->get_position();
            generic_file *source = fic->get_data(dialog, keep_compressed);
            crc val;

            fic->set_offset(start);
            if(source != NULL)
            {
                try
                {
                    bool compr_debraye = !compr_mask.is_covered(fic->get_name())
                        || fic->get_size() < min_size_compression;

                    if((compr_debraye && stock->get_algo() != none) || keep_compressed)
                        stock->change_algo(none);
                    else
                        if(!compr_debraye && stock->get_algo() == none)
                            stock->change_algo(compr_used);

		    if(compute_crc)
		    {
			source->copy_to(*stock, val);
			stock->flush_write();
			fic->set_crc(val);
		    }
		    else
		    {
			source->skip(0);
			source->copy_to(*stock);
			stock->flush_write();
		    }

		    if(!keep_compressed)
			if(!compr_debraye && stock->get_algo() != none)
			    fic->set_storage_size(stock->get_position() - start);
			else
			    fic->set_storage_size(0);
			// means no compression, thus the real storage size is the filesize
		    else // keep file compressed
			if(fic->get_compression_algo_used() == none)
			    fic->set_storage_size(0);
			// in older archive not using compression, the storage size was let equal
			// to the filesize, but now we must set it to zero to explicitely says
			// this file is not compressed as the other archive may use compression

                }
                catch(...)
                {
                    delete source;
			// restore atime of source
		    if(!alter_atime)
			tools_noexcept_make_date(info_quoi, ino->get_last_access(), ino->get_last_modif());
                    throw;
                }
                delete source;

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
			dialog.warning(tools_printf(gettext("File has been removed while reading it for backup: %S . It has however been properly saved in the archive"), &info_quoi));
			changed = false;
		    }

		    if(changed)
		    {
			dialog.warning(string(gettext("WARNING! File modified while reading it for backup: ")) + info_quoi);
			ret = false;
		    }
		}

		    // restore atime of source
		if(!alter_atime)
		    tools_noexcept_make_date(info_quoi, ino->get_last_access(), ino->get_last_modif());
            }
            else
                throw SRC_BUG; // saved_status == s_saved, but no data available, and no exception raised;
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
                if(ref == NULL || ref->ea_get_saved_status() == inode::ea_none || ref->get_last_change() < ino->get_last_change())
                {
                    if(ino->get_ea(dialog) != NULL)
                    {
                        crc val;

                        if(info_details)
                            dialog.warning(string(gettext("Saving Extended Attributes for ")) + info_quoi);
                        ino->ea_set_offset(stock->get_position());
                        stock->change_algo(compr_used); // always compress EA (no size or filename consideration)
                        stock->reset_crc(); // start computing CRC for any read/write on stock
                        try
                        {
                            ino->get_ea(dialog)->dump(*stock);
                        }
                        catch(...)
                        {
                            stock->get_crc(val); // keeps stocks in a coherent status
                            throw;
                        }
                        stock->get_crc(val);
                        ino->ea_set_crc(val);
                        ino->ea_detach();
                        stock->flush_write();
                        ret = true;
                    }
                    else
                        throw SRC_BUG;
                }
                else // EA have not changed, dropping the EA infos
                    ino->ea_set_saved_status(inode::ea_partial);
                break;
            case inode::ea_partial:
                throw SRC_BUG; //filesystem, must not provide inode in such a status
            case inode::ea_none: // no EA has been seen
                if(ref != NULL && ref->ea_get_saved_status() != inode::ea_none) // if there was some before
                {
                        // we must record the EA have been dropped since ref backup
                    ea_attributs ea;
		    crc val;

                    ino->ea_set_saved_status(inode::ea_full);
		    ea.clear(); // be sure it is empty
                    if(info_details)
                        dialog.warning(string(gettext("Saving Extended Attributes for ")) + info_quoi);
		    ino->ea_set_offset(stock->get_position());
		    stock->change_algo(compr_used);
		    stock->reset_crc();
		    try
		    {
			ea.dump(*stock);
		    }
		    catch(...)
		    {
			stock->get_crc(val);
			throw;
		    }
		    stock->get_crc(val);
		    ino->ea_set_crc(val);
                        // no need to detach, as the brand new ea has not been attached
                    stock->flush_write();
                    ret = true;
                }
                break;
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

} // end of namespace
