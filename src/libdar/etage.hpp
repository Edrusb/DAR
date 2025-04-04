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

    /// \file etage.hpp
    /// \brief definition of the etage structure is done here
    /// \ingroup Private


#ifndef ETAGE_HPP
#define ETAGE_HPP

#include "../my_config.h"
#include <deque>
#include <string>
#include "datetime.hpp"
#include "user_interaction.hpp"
#include "entrepot_aux.hpp"

namespace libdar
{

        /// \addtogroup Private
        /// @{

	/// the etage structure keep trace of directory contents

	/// it relies on the [fd]opendir() system call family that
	/// cannot be used recursively. Thus each etage structure is used
	/// to cache the contents of a directory, and can then be stored
	/// beside other etage structures corresponding to subdirectories

    class etage
    {
    public:
	etage() { fichier.clear(); last_mod = datetime(0); last_acc = datetime(0); }; // required to fake an empty dir when one is impossible to open
        etage(user_interaction & ui,
	      const char *dirname,
	      const datetime & x_last_acc,
	      const datetime & x_last_mod,
	      bool cache_directory_tagging,
	      bool furtive_read_mode);
	etage(const etage & ref) = default;
	etage(etage && ref) = default;
	etage & operator = (const etage & ref) = default;
	etage & operator = (etage && ref) noexcept = default;
	~etage() = default;

        bool read(std::string & ref, inode_type & tp);
	bool is_empty() const { return fichier.empty(); };
	datetime get_last_mod() const { return last_mod; };
	datetime get_last_acc() const { return last_acc; };

    private:

	struct cell
	{
	    std::string name;
	    inode_type type;

	    cell(const std::string & filename,
		 inode_type tp): name(filename), type(tp) {};
	};

        std::deque<cell> fichier;     ///< holds the list of entry in the directory
        datetime last_mod;            ///< the last_lod of the directory itself
	datetime last_acc;            ///< the last_acc of the directory itself
    };

	/// @}

} // end of namespace

#endif
