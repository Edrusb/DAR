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
// $Id: ea_filesystem.hpp,v 1.7 2005/05/08 12:12:00 edrusb Rel $
//
/*********************************************************************/
//

    /// \file ea_filesystem.hpp
    /// \brief filesystem dependent Extended Attributes operations
    ///
    /// This file contains a set of routines used to manipulate
    /// (read, write or test the presence of) Extended Attributes
    /// \ingroup Private

#ifndef EA_FILESYSTEM_HPP
#define EA_FILESYSTEM_HPP

#include "../my_config.h"
#include <string>
#include "ea.hpp"
#include "mask.hpp"

namespace libdar
{
	/// read EA associated to a given file

	/// \param[in] chemin is the path to the file to read attributes of
	/// \param[out] val is the resulting list of EA associated to this file
	/// \param[in] filter is a mask that defines which attributes names have to be considered only
	/// \return true whether some attribute could be found
    extern void ea_filesystem_read_ea(const std::string & chemin, ea_attributs & val, const mask & filter);

	/// overwrite some attribute to a given file's attribute list

	/// \param[in] chemin is the path of the file to write attribute to
	/// \param[in] val is a list of attribute amoung which a subset will be added to file's attribute list
	/// \param[in] filter a mask that define which attribute's names will be written and which will be ignored from the given list
	/// \return true if some attribute could be set
	/// \note if an EA is already present, it is not modified unless the mask covers its name and a value
	/// is available for that attribute name in the given list.
    extern bool ea_filesystem_write_ea(const std::string & chemin, const ea_attributs & val, const mask & filter);

	/// remove all EA of a given file that match a given mask

	/// \param[in] name is the filename which EA must be altered
	/// \param[in] filter is a mask that defines which EA names have to be removed
	/// \note unless the given mask is logically equivalent to bool_mask(true) some
	/// EA may remain associated to the file after the operation.
    extern void ea_filesystem_clear_ea(const std::string & name, const mask & filter);

	/// test the presence of EA for a given file

	/// \param[in] name is the filename which EA presence must be check against
	/// \return true if at least one EA has been found
    extern bool ea_filesystem_has_ea(const std::string & name);

	/// test the presence of EA for a given file

	/// \param[in] name is the filename which EA presence must be check against
	/// \param[in] list list of attributes
	/// \param[in] filter
	/// \return true if at least one EA covered by filter and present in the list exists as EA of the given file
    extern bool ea_filesystem_has_ea(const std::string & name, const ea_attributs & list, const mask & filter);

} // end of namespace

#endif
