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
// $Id: filesystem.hpp,v 1.22.2.1 2006/06/20 20:28:50 edrusb Exp $
//
/*********************************************************************/

    /// \file filesystem.hpp
    /// \brief defines several class that realize the interface with the filesystem
    ///
    /// - filesystem_hard_link_read
    /// - filesystem_backup
    /// - filesystem_diff
    /// - filesystem_hard_link_write
    /// - filesystem_restore
    /// \ingroup Private

#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
} // end extern "C"

#include <map>
#include <vector>
#include "catalogue.hpp"
#include "infinint.hpp"
#include "etage.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// keep trace of hard links when reading the filesystem
    class filesystem_hard_link_read
    {
            // this class is not to be used directly
            // it only provides some routine for the inherited classes

    public:
	filesystem_hard_link_read(user_interaction & dialog) { fs_ui = dialog.clone(); };
	filesystem_hard_link_read(const filesystem_hard_link_read & ref) { copy_from(ref); };
	filesystem_hard_link_read & operator = (const filesystem_hard_link_read & ref) { detruire(); copy_from(ref); return *this; };
	~filesystem_hard_link_read() { detruire(); };


        void forget_etiquette(file_etiquette *obj);
            // tell the filesystem module that the reference of that etiquette does not
            // exist anymore (not covered by filter for example)

    protected:
        void corres_reset() { corres_read.clear(); etiquette_counter = 0; };

        nomme *make_read_entree(path & lieu, const std::string & name, bool see_hard_link, const mask & ea_mask);

	user_interaction & get_fs_ui() const { return *fs_ui; };

    private:
        struct couple
        {
            nlink_t count;
            file_etiquette *obj;
        };
        std::map <ino_t, couple> corres_read;
	infinint etiquette_counter;

	user_interaction *fs_ui;

	void copy_from(const filesystem_hard_link_read & ref);
	void detruire();
    };


	/// make a flow sequence of inode to feed the backup filtering routing
    class filesystem_backup : public filesystem_hard_link_read
    {
    public:
        filesystem_backup(user_interaction & dialog,
			  const path &root,
			  bool x_info_details,
			  const mask & x_ea_mask,
			  bool check_no_dump_flag,
			  bool alter_atime,
			  bool x_cache_directory_tagging,
			  infinint & root_fs_device);
        filesystem_backup(const filesystem_backup & ref) : filesystem_hard_link_read(ref.get_fs_ui()) { copy_from(ref); };
        filesystem_backup & operator = (const filesystem_backup & ref) { detruire(); copy_from(ref); return *this; };
        ~filesystem_backup() { detruire(); };

        void reset_read(infinint & root_fs_device);
        bool read(entree * & ref, infinint & errors, infinint & skipped_dump);
        void skip_read_to_parent_dir();
            //  continue reading in parent directory and
            // ignore all entry not yet read of current directory
    private:

        path *fs_root;
        bool info_details;
	mask *ea_mask;
        bool no_dump_check;
	bool alter_atime;
	bool cache_directory_tagging;
        path *current_dir;      // to translate from an hard linked inode to an  already allocated object
        std::vector<etage> pile;        // to store the contents of a directory

        void detruire();
        void copy_from(const filesystem_backup & ref);
    };

	/// make a flow of inode to feed the difference filter routine
    class filesystem_diff : public filesystem_hard_link_read
    {
    public:
        filesystem_diff(user_interaction & dialog,
			const path &root, bool x_info_details,
			const mask & x_ea_mask, bool alter_atime);
        filesystem_diff(const filesystem_diff & ref) : filesystem_hard_link_read(ref.get_fs_ui()) { copy_from(ref); };
        filesystem_diff & operator = (const filesystem_diff & ref) { detruire(); copy_from(ref); return *this; };
        ~filesystem_diff() { detruire(); };

        void reset_read();
        bool read_filename(const std::string & name, nomme * &ref);
            // looks for a file of name given in argument, in current reading directory
            // if this is a directory subsequent read are done in it
        void skip_read_filename_in_parent_dir();
            // subsequent calls to read_filename will take place in parent directory.

    private:
        struct filename_struct
        {
            infinint last_acc;
            infinint last_mod;
        };

        path *fs_root;
        bool info_details;
	mask *ea_mask;
	bool alter_atime;
        path *current_dir;
        std::vector<filename_struct> filename_pile;

        void detruire();
        void copy_from(const filesystem_diff & ref);
    };

	/// keep trace of already written inodes to restore hard links
    class filesystem_hard_link_write
    {
            // this class is not to be used directly
            // it only provides routines to its inherited classes

    public:
	filesystem_hard_link_write(user_interaction & dialog, bool x_ea_erase) { fs_ui = dialog.clone(); ea_erase = x_ea_erase; };
	filesystem_hard_link_write(const filesystem_hard_link_write & ref) { copy_from(ref); };
	filesystem_hard_link_write & operator = (const filesystem_hard_link_write & ref) { detruire(); copy_from(ref); return *this; };
	~filesystem_hard_link_write() { detruire(); };

        bool ea_has_been_restored(const hard_link *h);
            // true if the inode pointed to by the arg has already got its EA restored
        bool set_ea(const nomme *e, const ea_attributs & list_ea, path spot,
                    bool allow_overwrite, bool warn_overwrite, const mask & ea_mask, bool info_details);
            // check whether the inode for which to restore EA is not a hard link to
            // an already restored inode. if not, it calls the proper ea_filesystem call to restore EA
        void write_hard_linked_target_if_not_set(const etiquette *ref, const std::string & chemin);
            // if a hard linked inode has not been restored (no change, or less recent than the one on filesystem)
            // it is necessary to inform filesystem, where to hard link on, any future hard_link
            // that could be necessary to restore.
        bool known_etiquette(const infinint & eti);
            // return true if an inode in filesystem has been seen for that hard linked inode

	    // return the ea_ease status (whether EA are first erased before being restored, else they are overwritten)
	bool get_ea_erase() const { return ea_erase; };
	    // set the ea_erase status
	void set_ea_erase(bool status) { ea_erase = status; };

    protected:
        void corres_reset() { corres_write.clear(); };
        void make_file(const nomme * ref, const path & ou, bool dir_perm, inode::comparison_fields what_to_check);
            // generate inode or make a hard link on an already restored inode.
        void clear_corres(const infinint & ligne);

	user_interaction & get_fs_ui() const { return *fs_ui; };

    private:
        struct corres_ino_ea
        {
            std::string chemin;
            bool ea_restored;
        };

        std::map <infinint, corres_ino_ea> corres_write;
	user_interaction *fs_ui;
	bool ea_erase;

	void copy_from(const filesystem_hard_link_write & ref);
	void detruire();
    };

	/// receive the flow of inode from the restoration filtering routing and promotes theses to real filesystem objects
    class filesystem_restore : public filesystem_hard_link_write, public filesystem_hard_link_read
    {
    public:
        filesystem_restore(user_interaction & dialog,
			   const path &root, bool x_allow_overwrite, bool x_warn_overwrite, bool x_info_details,
                           const mask & x_ea_mask, inode::comparison_fields what_to_check, bool x_warn_remove_no_match, bool empty, bool ea_erase);
        filesystem_restore(const filesystem_restore  & ref) : filesystem_hard_link_write(ref.filesystem_hard_link_write::get_fs_ui(), ref.get_ea_erase()), filesystem_hard_link_read(ref.filesystem_hard_link_read::get_fs_ui()) { copy_from(ref); };
        filesystem_restore & operator = (const filesystem_restore  & ref) { detruire(); copy_from(ref); return *this; };
        ~filesystem_restore() { restore_stack_dir_ownership(); detruire(); };

        void reset_write();
        bool write(const entree *x, bool & created);
            // the 'x' argument may be an object from class destroy
	    // the 'created' argument is set back to true if no overwriting was necessary to restore the file
            // return true upon success,
            // false if overwriting not allowed or refused
            // throw exception on other errors
        nomme *get_before_write(const nomme *x);
            // in this case the target has to be removed from the filesystem
        void pseudo_write(const directory *dir);
            // do not restore the directory, just stores that we are now
            // inspecting its contents
        bool set_ea(const nomme *e, const ea_attributs & l,
                    bool allow_overwrite,
                    bool warn_overwrite,
                    bool info_details)
            {  return empty ? true : filesystem_hard_link_write::set_ea(e, l, *current_dir,
									allow_overwrite,
									warn_overwrite,
									*ea_mask,
 									info_details);
            };

    protected:
	user_interaction & get_fs_ui() const { return filesystem_hard_link_read::get_fs_ui(); };

    private:
        path *fs_root;
        bool info_details;
	mask *ea_mask;
        bool allow_overwrite;
        bool warn_overwrite;
	inode::comparison_fields what_to_check;
	bool warn_remove_no_match;
        std::vector<directory> stack_dir;
        path *current_dir;
	bool empty;

        void detruire();
        void copy_from(const filesystem_restore & ref);
	void restore_stack_dir_ownership();
    };

	/// @}

} // end of namespace

#endif
