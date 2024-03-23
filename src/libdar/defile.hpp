/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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

    /// \file defile.hpp
    /// \brief here is defined the defile class
    /// \ingroup Private

#ifndef DEFILE_HPP
#define DEFILE_HPP

#include "../my_config.h"
#include "path.hpp"
#include "cat_entree.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the defile class keep trace of the real path of files while the flow in the filter routines

	/// the filter routines manipulates flow of inode, where their relative order
	/// represent the directory structure. To be able to know what is the real path
	/// of the current inode, all previously passed inode must be known.
	/// this class is used to display the progression of the filtering routing,
	/// and the file on which the filtering routine operates
    class defile
    {
    public :
        defile(const path &racine) : chemin(racine) { init = true; cache_set = none; };
	defile(const defile & ref) = default;
	defile(defile && ref) noexcept = default;
	defile & operator = (const defile & ref) = default;
	defile & operator = (defile && ref) noexcept = default;
	~defile() = default;

        void enfile(const cat_entree *e);
        const path & get_path() const { return chemin; };
        const std::string & get_string() const;
	const std::string & get_string_without_root() const;

    private :
        path chemin; ///< current path
        bool init;   ///< true if reached the "root" (all pushed arguments have been poped)
	mutable enum { none, full, without_root } cache_set; ///< whether cache is accurate
	mutable std::string cache; ///< cache of "chemin" converted into string
    };

	/// @}

} // end of namespace

#endif
