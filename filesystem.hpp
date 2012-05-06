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
// $Id: filesystem.hpp,v 1.12 2002/03/18 11:00:54 denis Rel $
//
/*********************************************************************/

#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include "catalogue.hpp"

extern void filesystem_set_root(const path &root, bool allow_overwrite, bool warn_overwrite, bool info_details);
extern void filesystem_freemem();

extern void filesystem_reset_read();	
extern bool filesystem_read(entree * &ref);
extern void filesystem_skip_read_to_parent_dir();

extern void filesystem_reset_write();
extern void filesystem_write(const entree *x);	// the argument may be an object from class destroy 
    // in this case the target is removed from the filesystem
extern void filesystem_pseudo_write(const directory *dir); // do not restore the directory, just stores that we are now 
    // inspecting its contents

#endif
