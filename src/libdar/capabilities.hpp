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

    /// \file capabilities.hpp
    /// \brief provide information about current thread (underlying using the widthdrawn POSIX.1e API)
    /// \ingroup Private

#ifndef CAPABILITIES_HPP
#define CAPABILITIES_HPP

#include "../my_config.h"

#include "user_interaction.hpp"

namespace libdar
{
	/// \addtogroup API
	/// @{

	// return from the EFFECTIVE set the value of the associated capability for the calling thread

    enum capa_status { capa_set,       //< current thread has the requested capability
		       capa_clear,     //< current thread has not the requested capability
		       capa_unknown }; //< impossible to determine whether the current thread has the requested capability

    extern capa_status capability_LINUX_IMMUTABLE(user_interaction & ui, bool verbose);

    extern capa_status capability_SYS_RESOURCE(user_interaction & ui, bool verbose);

    extern capa_status capability_FOWNER(user_interaction & ui, bool verbose);

    extern capa_status capability_CHOWN(user_interaction & ui, bool verbose);


	/// @}

} // end of namespace

#endif
