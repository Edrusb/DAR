/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

    /// \file entree_stats.hpp
    /// \brief datastructure holding the nature of file present in a given archive
    /// \ingroup API

#ifndef ENTREE_STATS_HPP
#define ENTREE_STATS_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "infinint.hpp"
#include "user_interaction.hpp"

#include <memory>

namespace libdar
{
    class cat_entree;

	/// \addtogroup API
	/// @{

	/// holds the statistics contents of a catalogue
    struct entree_stats
    {
        infinint num_x;                  ///< number of file referenced as destroyed since last backup
        infinint num_d;                  ///< number of directories
        infinint num_f;                  ///< number of plain files (hard link or not, thus file directory entries)
	infinint num_c;                  ///< number of char devices
	infinint num_b;                  ///< number of block devices
	infinint num_p;                  ///< number of named pipes
	infinint num_s;                  ///< number of unix sockets
	infinint num_l;                  ///< number of symbolic links
	infinint num_D;                  ///< number of Door
	infinint num_hard_linked_inodes; ///< number of inode that have more than one link (inode with "hard links")
        infinint num_hard_link_entries;  ///< total number of hard links (file directory entry pointing to an
                                         ///< inode already linked in the same or another directory (i.e. hard linked))
        infinint saved;      ///< total number of saved inode (unix inode, not inode class) hard links do not count here
	infinint patched;    ///< total number of saved data as binary delta patch
	infinint inode_only; ///< total number of inode which metadata changed without data being modified
        infinint total;      ///< total number of inode in archive (unix inode, not inode class) hard links do not count here
        void clear() { num_x = num_d = num_f = num_c = num_b = num_p
                = num_s = num_l = num_D = num_hard_linked_inodes
                = num_hard_link_entries = saved = patched = inode_only = total = 0; };
        void add(const cat_entree *ref);
        void listing(user_interaction & dialog) const;
    };

	/// @}

} // end of namespace

#endif
