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

    /// \file restore_tree.hpp
    /// \brief restore_tree class stores archive location needed to restore files
    ///
    /// This class forms a tree of restore_tree (add_child() and find() methods)
    /// following a path structure: At each level of the tree, a node has a name
    /// correspondig to the subdir name of a path (where from find() receives a path
    /// as argument and returns a pointer to the corresponding restore_tree object
    ///
    /// each node of the tree, correspond to an inode of a dar_manager database
    /// (see constructor argument) and stores the archive number to use to restore
    /// that file. In particular, if a more recent version is found in a archive
    /// with a higher index, the list of archive num will not mention lower archive
    /// indexes.
    ///
    /// \ingroup Private

#ifndef RESTORE_TREE_HPP
#define RESTORE_TREE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include <string>
#include <memory>
#include <map>

#include "path.hpp"
#include "data_tree.hpp"
#include "datetime.hpp"

namespace libdar
{
    class restore_tree
    {
    public:
	restore_tree(const data_tree* source, const datetime & ignore_more_recent_than_that);
	restore_tree(const restore_tree & ref) = delete;
	restore_tree(restore_tree && ref) noexcept = default;
	restore_tree & operator = (const restore_tree & ref) = delete;
	restore_tree & operator = (restore_tree && ref) noexcept = default;
	~restore_tree() = default;

	    /// return wether the provided path should trigger restoration from archive number num

	    /// \param[in] chem path to the inode to be restored (should always be a relative path)
	    /// \param[in] num the archive num to be restored from
	    /// \return true if the restoration should be applied to that path in the archive 'num', false else.
	    /// \note false may be returned because the archive num does not have restorable data for this entry,
	    /// but also because more recent archive have better version to be restored instread.
	bool restore_from(const path & chem, archive_num num) const;

    private:
	std::string node_name;
	std::set<archive_num> locations;
	std::map<std::string, std::unique_ptr<restore_tree> > children;

	bool restore_from_recursive(const path & chem, archive_num num) const;
    };

} // end of namespace

#endif
