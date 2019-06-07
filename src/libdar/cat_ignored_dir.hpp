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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file cat_ignored_dir.hpp
    /// \brief class used to remember in a catalogue that a cat_directory has been ignored
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

	/// the ignored cat_directory class, to be promoted later as empty cat_directory if needed
    class cat_ignored_dir : public cat_inode
    {
    public:
        cat_ignored_dir(const cat_directory &target) : cat_inode(target) {};
        cat_ignored_dir(user_interaction & dialog,
			const smart_pointer<pile_descriptor> & pdesc,
			const archive_version & reading_ver,
			bool small) : cat_inode(dialog, pdesc, reading_ver, s_not_saved, small) { throw SRC_BUG; };

	bool operator == (const cat_entree & ref) const;

        unsigned char signature() const { return 'j'; };
        cat_entree *clone() const { return new (get_pool()) cat_ignored_dir(*this); };

    protected:
        void inherited_dump(const pile_descriptor & pdesc, bool small) const; // behaves like an empty cat_directory

    };

	/// @}

} // end of namespace

#endif
