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

    /// \file gf_mode.hpp
    /// \brief generic modes to open file
    /// \ingroup Private
    /// \note API included module due to dependencies

#ifndef GF_MODE_HPP
#define GF_MODE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// generic_file openning modes
    enum gf_mode
    {
	gf_read_only,  ///< read only access
	gf_write_only, ///< write only access
	gf_read_write  ///< read and write access
    };

	/// provides a human readable string defining the gf_mode given in argument
    extern const char * generic_file_get_name(gf_mode mode);

	/// @}

} // end of namespace

#endif
