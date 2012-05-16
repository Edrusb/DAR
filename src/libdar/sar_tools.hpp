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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: sar_tools.hpp,v 1.5.4.1 2004/08/01 06:11:50 edrusb Exp $
//
/*********************************************************************/
//

#ifndef SAR_TOOLS_HPP
#define SAR_TOOLS_HPP

#include <string>
#include "../my_config.h"
#include "integers.hpp"
#include "generic_file.hpp"

namespace libdar
{

    extern generic_file *sar_tools_open_archive_fichier(const std::string &filename, bool allow_over, bool warn_over);
    extern generic_file *sar_tools_open_archive_tuyau(S_I fd, gf_mode mode);

} // end of namespace

#endif
