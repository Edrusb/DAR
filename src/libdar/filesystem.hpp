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

    /// \file filesystem.hpp
    /// \brief defines several classes that realize the interface with the filesystem
    /// \ingroup Private
    ///
    /// - filesystem_hard_link_read
    /// - filesystem_backup
    /// - filesystem_diff
    /// - filesystem_hard_link_write
    /// - filesystem_restore

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
#include "criterium.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// keep trace of hard links when reading the filesystem

    class filesystem_hard_link_read : virtual protected mem_ui
    {
            // this class is not to be used directly
            // it only provides some routine for the inherited classes

    public:
	filesystem_hard_link_read(user_interaction & dialog,
				  bool x_furtive_read_mode) : mem_ui(dialog) { furtive_read_mode = x_furtive_read_mode; };

	    // the copy of the current object would make copy of addresses in
	    // corres_read that could be released twice ... thus, copy constructor and
	    // assignement are forbidden for this class:

	filesystem_hard_link_read(const filesystem_hard_link_read & ref) : mem_ui(ref.get_ui()) { throw SRC_BUG; };
	const filesystem_hard_link_read & operator = (const filesystem_hard_link_read & ref) { throw SRC_BUG; };

	    // get the last assigned number for a hard linked inode
	const infinint & get_last_etoile_ref() const { return etiquette_counter; };

	virtual ~filesystem_hard_link_read() {};

    protected:
	    // reset the whole list of hard linked inodes (hard linked inode stay alive but are no more referenced by the current object)
        void corres_reset() { corres_read.clear(); etiquette_counter = 0; };

	    // create and return a libdar object corresponding to one pointed to by its path
	    // during this operation, hard linked inode are recorded in a list to be easily pointed
	    // to by a new reference to it.
        nomme *make_read_entree(path & lieu,               //< path of the file to read
				const std::string & name,  //< name of the file to read
				bool see_hard_link,        //< whether we want to detect hard_link and eventually return a mirage object (not necessary when diffing an archive with filesystem)
				const mask & ea_mask);    //< which EA to consider when creating the object

    private:

	    // private datastructure

        struct couple
        {
            nlink_t count;       //< counts the number of hard link on that inode that have not yet been found in filesystem, once this count reaches zero, the "couple" structure can be dropped and the "holder" too (no more expected hard links to be found)
            etoile *obj;         //< the address of the corresponding etoile object for that inode
	    mirage holder;       //< it increments by one the obj internal counters, thus, while this object is alive, the obj will not be destroyed

	    couple(etoile *ptr, nlink_t ino_count) : holder("FAKE", ptr) { count = ino_count; obj = ptr; };
        };

	struct node
	{
	    node(ino_t num, dev_t dev) { numnode = num; device = dev; };

		// this operator is required to use the type node in a std::map
	    bool operator < (const node & ref) const { return numnode < ref.numnode || (numnode == ref.numnode && device < ref.device); };
	    ino_t numnode;
	    dev_t device;
	};

	    // private variable

        std::map <node, couple> corres_read;
	infinint etiquette_counter;
	bool furtive_read_mode;

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
			  bool furtive_read_mode,
			  bool x_cache_directory_tagging,
			  infinint & root_fs_device,
			  bool x_ignore_unknown);
        filesystem_backup(const filesystem_backup & ref) : mem_ui(ref.get_ui()), filesystem_hard_link_read(ref.get_ui(), ref.furtive_read_mode) { copy_from(ref); };
        const filesystem_backup & operator = (const filesystem_backup & ref) { detruire(); copy_from(ref); return *this; };
        ~filesystem_backup() { detruire(); };

        void reset_read(infinint & root_fs_device);
        bool read(entree * & ref, infinint & errors, infinint & skipped_dump);
        void skip_read_to_parent_dir();
            //  continue reading in parent directory and
            // ignore all entry not yet read of current directory
    private:

        path *fs_root;           //< filesystem's root to consider
        bool info_details;       //< detailed information returned to the user
	mask *ea_mask;           //< mask defining the EA to consider
        bool no_dump_check;      //< whether to check against the nodump flag presence
	bool alter_atime;        //< whether to set back atime or not
	bool furtive_read_mode;  //< whether to use furtive read mode (if true, alter_atime is ignored)
	bool cache_directory_tagging; //< whether to consider cache directory taggin standard
        path *current_dir;       //< needed to translate from an hard linked inode to an  already allocated object
        std::vector<etage> pile; //< to store the contents of a directory
	bool ignore_unknown;     //< whether to ignore unknown inode types

        void detruire();
        void copy_from(const filesystem_backup & ref);
    };


	/// make a flow of inode to feed the difference filter routine

    class filesystem_diff : public filesystem_hard_link_read
    {
    public:
        filesystem_diff(user_interaction & dialog,
			const path &root,
			bool x_info_details,
			const mask & x_ea_mask,
			bool alter_atime,
			bool furtive_read_mode);
        filesystem_diff(const filesystem_diff & ref) : mem_ui(ref.get_ui()), filesystem_hard_link_read(ref.get_ui(), ref.furtive_read_mode) { copy_from(ref); };
        const filesystem_diff & operator = (const filesystem_diff & ref) { detruire(); copy_from(ref); return *this; };
        ~filesystem_diff() { detruire(); };

        void reset_read();
        bool read_filename(const std::string & name, nomme * &ref);
            // looks for a file of name given in argument, in current reading directory
            // if this is a directory, subsequent read take place in it

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
	bool furtive_read_mode;
        path *current_dir;
        std::vector<filename_struct> filename_pile;

        void detruire();
        void copy_from(const filesystem_diff & ref);
    };

	/// keep trace of already written inodes to restore hard links

    class filesystem_hard_link_write : virtual protected mem_ui
    {
            // this class is not to be used directly
            // it only provides routines to its inherited classes

    public:
	filesystem_hard_link_write(user_interaction & dialog) : mem_ui(dialog) { corres_write.clear(); };
	filesystem_hard_link_write(const filesystem_hard_link_write & ref) : mem_ui(ref) { throw SRC_BUG; };
	const filesystem_hard_link_write & operator = (const filesystem_hard_link_write & ref) { throw SRC_BUG; };

        void write_hard_linked_target_if_not_set(const mirage *ref, const std::string & chemin);
            // if a hard linked inode has not been restored (no change, or less recent than the one on filesystem)
            // it is necessary to inform filesystem, where to hard link on, any future hard_link
            // that could be necessary to restore.

        bool known_etiquette(const infinint & eti);
            // return true if an inode in filesystem has been seen for that hard linked inode

	    /// forget everything about a hard link if the path used to build subsequent hard links is the one given in argument
	    /// \param[in] ligne is the etiquette number for that hard link
	    /// \param[in] path if the internaly recorded path to build subsequent hard link to that inode is equal to path, forget everything about this hard linked inode
        void clear_corres_if_pointing_to(const infinint & ligne, const std::string & path);

    protected:
        void corres_reset() { corres_write.clear(); };
        void make_file(const nomme * ref,                       //< object to restore in filesystem
		       const path & ou,                         //< where to restore it
		       bool dir_perm,                           //< false for already existing directories, this makes dar set the minimum available permission to be able to restore files in that directory at a later time
		       inode::comparison_fields what_to_check); //< defines whether to restore permission, ownership, dates, etc.
            // generate inode or make a hard link on an already restored or existing inode.


	    /// add the given EA matching the given mask to the file pointed to by "e" and spot

	    /// \param[in] e may be an inode or a hard link to an inode,
	    /// \param[in] list_ea the list of EA to restore
	    /// \param[in] spot the path where to restore these EA (full path required, including the filename of 'e')
	    /// \param[in] ea_mask the EA entry to restore from the list_ea (other entries are ignored)
	    /// \return true if EA could be restored, false if "e" is a hard link to an inode that has its EA already restored previously
        bool raw_set_ea(const nomme *e,
			const ea_attributs & list_ea,
			const std::string & spot,
			const mask & ea_mask);
            // check whether the inode for which to restore EA is not a hard link to
            // an already restored inode. if not, it calls the proper ea_filesystem call to restore EA

	    /// remove EA set from filesystem's file, allows subsequent raw_set_ea

	    /// \param[in] e this object may be a hard link to or an inode
	    /// \param[in] path the path in the filesystem where reside the object whose EA to clear
	    /// \return true if EA could be cleared, false if "e" is a hard link to an inode that has its  EA already restored previously
	bool raw_clear_ea_set(const nomme *e, const std::string & path);


    private:
        struct corres_ino_ea
        {
            std::string chemin;
            bool ea_restored;
        };

        std::map <infinint, corres_ino_ea> corres_write;
    };


	/// receive the flow of inode from the restoration filtering routing and promotes these to real filesystem objects

    class filesystem_restore : public filesystem_hard_link_write, public filesystem_hard_link_read
    {
    public:
	    /// constructor
        filesystem_restore(user_interaction & dialog,
			   const path & root,
			   bool x_warn_overwrite,
			   bool x_info_details,
                           const mask & x_ea_mask,
			   inode::comparison_fields what_to_check,
			   bool x_warn_remove_no_match,
			   bool empty,
			   const crit_action *x_overwrite,
			   bool x_only_overwrite);
	    /// copy constructor is forbidden (throws an exception)
        filesystem_restore(const filesystem_restore & ref) : mem_ui(ref), filesystem_hard_link_write(ref), filesystem_hard_link_read(get_ui(), true) { throw SRC_BUG; };
	    /// assignment operator is forbidden (throws an exception)
        const filesystem_restore & operator = (const filesystem_restore  & ref) { throw SRC_BUG; };
	    /// destructor
        ~filesystem_restore() { restore_stack_dir_ownership(); detruire(); };

	    /// reset the writing process for the current object
        void reset_write();

	typedef enum
	{
	    done_data_restored,     //< data has been restored to filesystem
	    done_no_change_no_data, //< no change in filesystem because no data present in archive
	    done_no_change_policy,  //< no change in filesystem because of overwiting policy decision
	    done_data_removed       //< data (= whole inode) removed from filesystem
	} action_done_for_data;

	    /// restore a libdar object to a filesystem entry both data and EA

	    /// \param[in] x is the libdar object to restore
	    /// \param[out] data_restored true if data has been restored (inode or hard link created), false if either there is no data to restore or if this action is forbidden by the overwriting policy
	    /// \param[out] ea_restored  true if EA has been restored, false if either no EA to restore or if forbidden by overwriting policy
	    /// \param[out] data_created true if data has been restored leading to file creation, false in any other case
	    /// \param[out] hard_link true when data_restored is true and only a hard link to an already existing inode has been created
	    /// \note any failure to restore data or EA that is not due to its absence in "x" nor to an interdiction from the overwriting policy is signaled
	    /// through an exception.
	void write(const entree *x, action_done_for_data & data_restored, bool & ea_restored, bool & data_created, bool & hard_link);


	    /// ask for no warning or user interaction for the next write operation
	    /// \note this is used when a file has been saved several times due to its changes at the time of the backup
	    /// and is restored in sequential read. Restoring each failed backup would lead to ask each time the
	    /// actions to take about overwriting... anoying for the user
	void ignore_overwrite_restrictions_for_next_write() { ignore_over_restricts = true; };


    private:
	class stack_dir_t : public directory
	{
	public:
	    stack_dir_t(const directory & ref, bool restore) : directory(ref) { restore_date = restore; };

	    bool get_restore_date() const { return restore_date; };
	    void set_restore_date(bool val) { restore_date = val; };

	private:
	    bool restore_date;
	};

        path *fs_root;
        bool info_details;
	mask *ea_mask;
        bool warn_overwrite;
	inode::comparison_fields what_to_check;
	bool warn_remove_no_match;
        std::vector<stack_dir_t> stack_dir;
        path *current_dir;
	bool empty;
	bool ignore_over_restricts;
	const crit_action *overwrite;
	bool only_overwrite;

        void detruire();
	void restore_stack_dir_ownership();

	    // subroutines of write()
	void action_over_remove(const inode *in_place, const detruit *to_be_added, const std::string & spot, over_action_data action);
	void action_over_data(const inode *in_place,
			      const nomme *to_be_added,
			      const std::string & spot,
			      over_action_data action,
			      action_done_for_data & data_done);
	bool action_over_ea(const inode *in_place, const nomme *to_be_added, const std::string & spot, over_action_ea action);
    };

	/// @}

} // end of namespace

#endif
