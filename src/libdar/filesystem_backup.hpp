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

    /// \file filesystem_backup.hpp
    /// \brief filesystem_backup class realizes the interface with the filesystem for backing up
    /// \ingroup Private


#ifndef FILESYSTEM_BACKUP_HPP
#define FILESYSTEM_BACKUP_HPP

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

#include <deque>
#include "infinint.hpp"
#include "etage.hpp"
#include "cat_entree.hpp"
#include "filesystem_hard_link_read.hpp"

#include <set>

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// makes a flow sequence of inode to feed the backup filtering routing

    class filesystem_backup : public filesystem_hard_link_read
    {
    public:
        filesystem_backup(const std::shared_ptr<user_interaction> & dialog,
			  const path &root,
			  bool x_info_details,
			  const mask & x_ea_mask,
			  bool check_no_dump_flag,
			  bool alter_atime,
			  bool furtive_read_mode,
			  bool x_cache_directory_tagging,
			  infinint & root_fs_device,
			  bool x_ignore_unknown,
			  const fsa_scope & scope);
        filesystem_backup(const filesystem_backup & ref) = delete;
	filesystem_backup(filesystem_backup && ref) = delete;
        filesystem_backup & operator = (const filesystem_backup & ref) = delete;
	filesystem_backup & operator = (filesystem_backup && ref) = delete;
        ~filesystem_backup() { detruire(); };

        void reset_read(infinint & root_fs_device);
        bool read(cat_entree * & ref, infinint & errors, infinint & skipped_dump);
        void skip_read_to_parent_dir();
            //  continue reading in parent directory and
            // ignore all entry not yet read of current directory
    private:

        path *fs_root;           ///< filesystem's root to consider
        bool info_details;       ///< detailed information returned to the user
	mask *ea_mask;           ///< mask defining the EA to consider
        bool no_dump_check;      ///< whether to check against the nodump flag presence
	bool alter_atime;        ///< whether to set back atime or not
	bool furtive_read_mode;  ///< whether to use furtive read mode (if true, alter_atime is ignored)
	bool cache_directory_tagging; ///< whether to consider cache directory taggin standard
        path *current_dir;       ///< needed to translate from an hard linked inode to an  already allocated object
        std::deque<etage> pile;  ///< to store the contents of a directory
	bool ignore_unknown;     ///< whether to ignore unknown inode types

        void detruire();
    };

	/// @}

} // end of namespace

#endif
