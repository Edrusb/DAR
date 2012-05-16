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
// $Id: filesystem.hpp,v 1.8.2.1 2003/12/20 23:05:34 edrusb Rel $
//
/*********************************************************************/

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

    class filesystem_hard_link_read
    {

            // this class is not to be used directly
            // it only provides some routine for the inherited classes

    public:
        void forget_etiquette(file_etiquette *obj);
            // tell the filesystem module that the reference of that etiquette does not
            // exist anymore (not covered by filter for example)

    protected:
        void corres_reset() { corres_read.clear(); };

        nomme *make_read_entree(path & lieu, const std::string & name, bool see_hard_link, bool ea_root_mode, bool ea_user_mode);

    private:
        struct couple
        {
            nlink_t count;
            file_etiquette *obj;
        };
        std::map <ino_t, couple> corres_read;
    };


    class filesystem_backup : public filesystem_hard_link_read
    {
    public:
        filesystem_backup(const path &root, bool x_info_details, bool root_ea, bool user_ea, bool check_no_dump_flag);
        filesystem_backup(const filesystem_backup & ref) { copy_from(ref); };
        filesystem_backup & operator = (const filesystem_backup & ref) { detruire(); copy_from(ref); return *this; };
        ~filesystem_backup() { detruire(); };

        void reset_read();
        bool read(entree * & ref);
        void skip_read_to_parent_dir();
            //  continue reading in parent directory and
            // ignore all entry not yet read of current directory
    private:

        struct filename_struct
        {
            infinint last_acc;
            infinint last_mod;
        };

        path *fs_root;
        bool info_details;
        bool ea_root;
        bool ea_user;
        bool no_dump_check;
        path *current_dir;      // to translate from an hard linked inode to an  already allocated object
        std::vector<filename_struct> filename_pile; // to be able to restore last access of directory we open for reading
        std::vector<etage> pile;        // to store the contents of a directory

        void detruire();
        void copy_from(const filesystem_backup & ref);
    };

    class filesystem_diff : public filesystem_hard_link_read
    {
    public:
        filesystem_diff(const path &root, bool x_info_details, bool root_ea, bool user_ea);
        filesystem_diff(const filesystem_diff & ref) { copy_from(ref); };
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
        bool ea_root;
        bool ea_user;
        path *current_dir;
        std::vector<filename_struct> filename_pile;

        void detruire();
        void copy_from(const filesystem_diff & ref);
    };

    class filesystem_hard_link_write
    {
            // this class is not to be used directly
            // it only provides routines to its inherited classes
            // this not public part is present.

    public:
        bool ea_has_been_restored(const hard_link *h);
            // true if the inode pointed to by the arg has already got its EA restored
        bool set_ea(const nomme *e, const ea_attributs & l, path spot,
                    bool allow_overwrite, bool warn_overwrite, bool info_details);
            // check the inode for which to restore EA, is not a hard link to
            // an already restored inode, else call the proper ea_filesystem call
        void write_hard_linked_target_if_not_set(const etiquette *ref, const std::string & chemin);
            // if a hard linked inode has not been restored (no change, or less recent than the one on filesystem)
            // it is necessary to inform filesystem, where to hard link on, any future hard_link
            // that could be necessary to restore.
        bool known_etiquette(const infinint & eti);
            // return true if an inode in filesystem has been seen for that hard linked inode

    protected:
        void corres_reset() { corres_write.clear(); };
        void make_file(const nomme * ref, const path & ou, bool dir_perm, bool ignore_owner);
            // generate inode or make a hard link on an already restored inode.
        void clear_corres(const infinint & ligne);

    private:
        struct corres_ino_ea
        {
            std::string chemin;
            bool ea_restored;
        };

        std::map <infinint, corres_ino_ea> corres_write;
    };


    class filesystem_restore : public filesystem_hard_link_write, public filesystem_hard_link_read
    {
    public:
        filesystem_restore(const path &root, bool x_allow_overwrite, bool x_warn_overwrite, bool x_info_details,
                           bool root_ea, bool user_ea, bool ignore_owner, bool x_warn_remove_no_match, bool empty);
        filesystem_restore(const filesystem_restore  & ref) { copy_from(ref); };
        filesystem_restore & operator =(const filesystem_restore  & ref) { detruire(); copy_from(ref); return *this; };
        ~filesystem_restore() { detruire(); };

        void reset_write();
        bool write(const entree *x);
            // the argument may be an object from class destroy
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
									info_details);
            };

    private:
        path *fs_root;
        bool info_details;
        bool ea_root;
        bool ea_user;
        bool allow_overwrite;
        bool warn_overwrite;
        bool ignore_ownership;
	bool warn_remove_no_match;
        std::vector<directory> stack_dir;
        path *current_dir;
	bool empty;

        void detruire();
        void copy_from(const filesystem_restore & ref);

    };

} // end of namespace

#endif
