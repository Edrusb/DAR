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

    /// \file entrepot_aux.hpp
    /// \brief set of datastructures used to interact with entrepot objects
    /// \ingroup API


#ifndef ENTREPOT_AUX_HPP
#define ENTREPOT_AUX_HPP

#include "../my_config.h"

#include "integers.hpp"
#include <string>

namespace libdar
{

    	/// \addtogroup API
	/// @{

	/// type of inode
    enum class inode_type { nondir, isdir, unknown };


	/// @}

} // end of namespace

#endif
