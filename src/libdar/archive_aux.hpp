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

    /// \file archive_aux.hpp
    /// \brief set of datastructures used to interact with a catalogue object
    /// \ingroup API


#ifndef CATALOGUE_AUX_HPP
#define CATALOGUE_AUX_HPP

#include "../my_config.h"

namespace libdar
{

    	/// \addtogroup API
	/// @{

	/// how to detect data has changed when some fields
    enum class modified_data_detection
    {
	any_inode_change, ///< historical behavior, resave an inode on any metadata change
        mtime_size,       ///< default behavior since release 2.6.0 resave only if file size of mtime changed
    };


	/// how to consider file change during comparison and incremental backup
    enum class comparison_fields
    {
	all,          ///< consider any available field for comparing inodes
	ignore_owner, ///< consider any available field except ownership fields
	mtime,        ///< consider any available field except ownership and permission fields
	inode_type    ///< only consider the file type
    };

    enum class hash_algo
    {
	hash_none,  ///< no hashing algorithm
	hash_md5,   ///< MD5 algorithm
	hash_sha1,  ///< SHA1 algorithm
	hash_sha512 ///< SHA-512 algorithm
    };


	/// @}

} // end of namespace

#endif
