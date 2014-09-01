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

    /// \file cat_ignored_dir.hpp
    /// \brief class used to remember in a catalogue that a directory has been ignored
    /// \ingroup Private

#ifndef CAT_IGNORED_DIR_HPP
#define CAT_IGNORED_DIR_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "cat_inode.hpp"
#include "cat_directory.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the ignored directory class, to be promoted later as empty directory if needed
    class ignored_dir : public inode
    {
    public:
        ignored_dir(const directory &target) : inode(target) {};
        ignored_dir(user_interaction & dialog,
		    generic_file & f,
		    const archive_version & reading_ver,
		    compressor *efsa_loc,
		    escape *ptr) : inode(dialog, f, reading_ver, s_not_saved, efsa_loc, ptr) { throw SRC_BUG; };

        unsigned char signature() const { return 'j'; };
        entree *clone() const { return new (get_pool()) ignored_dir(*this); };

    protected:
        void inherited_dump(generic_file & f, bool small) const; // behaves like an empty directory

    };

	/// @}

} // end of namespace

#endif
