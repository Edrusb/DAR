/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: filesystem.hpp,v 1.8 2002/10/28 20:39:32 edrusb Rel $
//
/*********************************************************************/

#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include "catalogue.hpp"

//// set of function for initialization
extern void filesystem_set_root(const path &root, bool allow_overwrite, bool warn_overwrite, bool info_details, bool root_ea, bool user_ea);
extern void filesystem_ignore_owner(bool mode);
extern void filesystem_freemem();


//// set of functions for filesystem reading (backup)
extern void filesystem_reset_read();
    // reset reading. for both filesystem_read() and filesystem_read_filename() methods
extern bool filesystem_read(entree * &ref);
extern void filesystem_skip_read_to_parent_dir();
    // to use with filesystem_read() to continue reading in parent directory and
    // ignore all entry not yet read of current directory

//// set of functions for filesystem reading (comparision with existing archive)
extern bool filesystem_read_filename(const string & name, nomme * &ref);
    // looks a file of name given in argument, in current reading directory 
    // WARNING : cannot mix fileystem_read() and filesystem_read_filename()
    // filesystem_reset_read() must be called before changing of 
    // reading method
extern void filesystem_skip_read_filename_in_parent_dir();
    // must be called when using filesystem_read_filename() to explore parent dir


//// set of functions for writing (restoration)
extern void filesystem_reset_write();
extern bool filesystem_write(const entree *x);	
    // the argument may be an object from class destroy
    // return true upon success, 
    // false if overwriting not allowed or refused 
    // throw exception on other errors
extern nomme *filesystem_get_before_write(const nomme *x);
    // in this case the target has to be removed from the filesystem
extern void filesystem_pseudo_write(const directory *dir);
    // do not restore the directory, just stores that we are now 
    // inspecting its contents

//// set of functions to manage EA (trapped and redirected to ea.hpp module) 
extern bool filesystem_ea_has_been_restored(const hard_link *h);
    // true if the inode pointed to by the arg has already got its EA restored
extern bool filesystem_set_ea(const nomme *e, const ea_attributs & l);
    // check the inode for which to restore EA, is not a hard link to
    // an already restored inode, else call the proper ea_filesystem call

//// set of functions to manage hard links
extern void filesystem_write_hard_linked_target_if_not_set(const etiquette *ref, const string & chemin);
    // if a hard linked inode has not been restored (no change, or less recent than the one on filesystem)
    // it is necessary to inform filesystem, where to hard link on, any future hard_link 
    // that could be necessary to restore.
extern bool filesystem_known_etiquette(const infinint & eti);
    // return true if an inode in filesystem has been seen for that hard linked inode
extern void filesystem_forget_etiquette(file_etiquette *obj);
    // tell the filesystem module that the reference of that etiquette does not
    // exist anymore (not covered by filter for example)

#endif
