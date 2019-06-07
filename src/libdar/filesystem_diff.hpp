/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file filesystem_diff.hpp
    /// \brief class filesystem_diff makes a flow of inode to feed the difference filter routine
    /// \ingroup Private

#ifndef FILESYSTEM_DIFF_HPP
#define FILESYSTEM_DIFF_HPP

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
#include "cat_nomme.hpp"
#include "filesystem_hard_link_read.hpp"

#include <set>

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// make a flow of inode to feed the difference filter routine

    class filesystem_diff : public filesystem_hard_link_read
    {
    public:
        filesystem_diff(const std::shared_ptr<user_interaction> & dialog,
			const path &root,
			bool x_info_details,
			const mask & x_ea_mask,
			bool alter_atime,
			bool furtive_read_mode,
			const fsa_scope & scope);
        filesystem_diff(const filesystem_diff & ref) = delete;
	filesystem_diff(filesystem_diff && ref) = delete;
        filesystem_diff & operator = (const filesystem_diff & ref) = delete;
	filesystem_diff & operator = (filesystem_diff && ref) = delete;
        ~filesystem_diff() { detruire(); };

        void reset_read();
        bool read_filename(const std::string & name, cat_nomme * &ref);
            // looks for a file of name given in argument, in current reading directory
            // if this is a directory, subsequent read take place in it

        void skip_read_filename_in_parent_dir();
            // subsequent calls to read_filename will take place in parent directory.

    private:
        struct filename_struct
        {
            datetime last_acc;
            datetime last_mod;
        };

        path *fs_root;
        bool info_details;
	mask *ea_mask;
	bool alter_atime;
	bool furtive_read_mode;
        path *current_dir;
        std::deque<filename_struct> filename_pile;

        void detruire();
    };

	/// @}

} // end of namespace

#endif
