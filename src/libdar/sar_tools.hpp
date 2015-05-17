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
#include "memory_pool.hpp"
#include "label.hpp"


namespace libdar
{

    extern std::string sar_tools_make_filename(const std::string & base_name,
					       const infinint & num,
					       const infinint & min_digits,
					       const std::string & ext);


    extern bool sar_tools_extract_num(const std::string & filename,
				      const std::string & base_name,
				      const infinint & min_digits,
				      const std::string & ext,
				      infinint & ret);

    extern bool sar_tools_get_higher_number_in_dir(entrepot & entr,
						   const std::string & base_name,
						   const infinint & min_digits,
						   const std::string & ext, infinint & ret);

    extern std::string sar_tools_make_padded_number(const std::string & num,
						    const infinint & min_digits);


} // end of namespace

#endif
