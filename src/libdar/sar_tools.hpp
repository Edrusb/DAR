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
// $Id: sar_tools.hpp,v 1.16 2011/01/09 17:25:58 edrusb Rel $
//
/*********************************************************************/
//

    /// \file sar_tools.hpp
    /// \brief a set of tools aims to help Segmentation And Reassemblement (sar) class
    /// \ingroup Private

#ifndef SAR_TOOLS_HPP
#define SAR_TOOLS_HPP

#include "../my_config.h"

#include <string>
#include "integers.hpp"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "header.hpp"
#include "sar.hpp"

namespace libdar
{

	/// \brief create an container to write a archive to a pipe
	/// \ingroup Private
    extern trivial_sar *sar_tools_open_archive_tuyau(user_interaction & dialog,
						     S_I fd,
						     gf_mode mode,
						     const label & data_name,
						     const std::string & execute);

} // end of namespace

#endif
