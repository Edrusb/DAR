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

    /// \file filesystem_ids.hpp
    /// \brief gather the ids of different filesystem to provide a filter based on filesystem
    /// \ingroup Private

#ifndef FILESYSTEM_IDS_HPP
#define FILESYSTEM_IDS_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include <set>
#include "infinint.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{

    class filesystem_ids
    {
    public:
	filesystem_ids() {};
	filesystem_ids(const filesystem_ids & ref) = default;
	filesystem_ids(filesystem_ids && ref) noexcept = default;
	filesystem_ids & operator = (const filesystem_ids & ref) = default;
	filesystem_ids & operator = (filesystem_ids && ref) noexcept = default;
	~filesystem_ids() = default;

	    /// set or replace the filesystem to be taken as root filesystem

	    /// \param[in] path the filesystem where is located path will be
	    /// flagged as root, which just mean it will never be excluded
	    /// and is_covered() method will always return true.
	void set_as_root_fs(const std::string & path) { root_fs = path2fs_id(path); };

	    /// include the filesystem where the given path is stored

	    /// \note if no fs is included, the filesystem_ids() behaves
	    /// like a blacklist, is_covered() will return true except for
	    /// files located on the explictely excluded filesystems.
	void include_fs_at(const std::string & path) { included.insert(path2fs_id(path)); };

	    /// exclude the filessytem where the given path is stored

	    /// \note if a filesystem is both included and excluded
	    /// it is excluded. The only filesystem that is never
	    /// excluded is the one set as root filesystem
	    /// \note if no fs is excluded, the filesystem_ids() behaves
	    /// like a whitelist: is_covered() will return true only for the
	    /// files located on the explictely included filesystems
	    /// \note if both include and excluded list are empty,
	    /// is_covered() always return true.
	void exclude_fs_at(const std::string & path) { excluded.insert(path2fs_id(path)); };

	    /// returns true if the fs_id is included and not excluded,
	    /// true is also always returned for the fs set as root_fs
	bool is_covered(const infinint & fs_id) const;

	    /// reset the filesystem_ids object
	void clear() { included.clear(); excluded.clear(); };

    private:
	infinint root_fs;
	std::set<infinint> included;
	std::set<infinint> excluded;

	static infinint path2fs_id(const std::string & path);
    };

	/// @}

} // end of namespace

#endif
