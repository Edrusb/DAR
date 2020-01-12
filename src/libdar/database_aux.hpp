/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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

    /// \file database_aux.hpp
    /// \brief set of datastructures used to interact with a database object
    /// \ingroup API


#ifndef DATABASE_AUX_HPP
#define DATABASE_AUX_HPP

#include "../my_config.h"

namespace libdar
{

	/// \addtogroup API
	/// @{

	/// the available status of a lookup

    enum class db_lookup
    {
	found_present,  ///< file data/EA has been found completely usable
	found_removed,  ///< file data/EA has been found as removed at that date
	not_found,      ///< no such file has been found in any archive of the base
	not_restorable  ///< file data/EA has been found existing at that date but not possible to restore (lack of data, missing archive in base, etc.)
    };

	/// the status for a founded entry

    	/// \note when restoring an et_patch entry, it is required to also restore all  to all intermediate
	/// et_patch since the last et_saved status. While restoring et_inode
	/// only need to restore the previous et_saved and the latest et_inode available. Intermediate et_inode
	/// have not to be restored.
    enum class db_etat
    {
	et_saved,       ///< data/EA present in the archive
	et_patch,       ///< data present as patch since the previous version
	et_patch_unusable, ///< data present as patch but base version not found in archive set
	et_inode,       ///< only inode metadata has been modified since previous version
	et_present,     ///< file/EA present in the archive but data not saved (differential backup)
	et_removed,     ///< file/EA stored as deleted since archive of reference or file/EA not present in the archive
	et_absent       ///< file not even mentionned in the archive, This entry is equivalent to et_removed, but is required to be able to properly re-order the archive when user asks to do so. The dates associated to this state are computed from neighbor archives in the database
    };

	/// @}

} // end of namespace

#endif
