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

    /// \file sar_tools.hpp
    /// \brief a set of tools aims to help Segmentation And Reassemblement (sar) class
    /// \ingroup Private

#ifndef SAR_TOOLS_HPP
#define SAR_TOOLS_HPP

#include "../my_config.h"

#include <string>
#include "infinint.hpp"
#include "entrepot.hpp"

namespace libdar
{

	/// \addtogroup Private
        /// @{

    extern std::string sar_tools_make_filename(const std::string & base_name,
					       const infinint & num,
					       const infinint & min_digits,
					       const std::string & ext);


    extern bool sar_tools_extract_num(const std::string & filename,
				      const std::string & base_name,
				      const infinint & min_digits,
				      const std::string & ext,
				      infinint & ret);

    extern bool sar_tools_get_higher_number_in_dir(user_interaction & ui,
						   entrepot & entr,
						   const std::string & base_name,
						   const infinint & min_digits,
						   const std::string & ext, infinint & ret);

    extern void sar_tools_remove_higher_slices_than(entrepot & entr,
						    const std::string & base_name,
						    const infinint & min_digits,
						    const std::string & ext,
						    const infinint & higher_slice_num_to_keep,
						    user_interaction & ui);


    extern std::string sar_tools_make_padded_number(const std::string & num,
						    const infinint & min_digits);


	/// @}

} // end of namespace

#endif
