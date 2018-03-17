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

    /// \file crit_action_cmd_line.hpp
    /// \brief contains routines to convert command-line overwriting criterium to their crit_action counterparts
    /// \ingroup CMDLINE


#ifndef CRIT_ACTION_CMD_LINE_HPP
#define CRIT_ACTION_CMD_LINE_HPP

#include "../my_config.h"
#include "libdar.hpp"

    /// \addtogroup CMDLINE
    /// @{

    /// canonizes a criterium description string

    /// \param[in] argument is a not canonized criterium description string
    /// \return a canonized description string
    /// \note criterium description strings when not canonized may include spaces
    /// tabs and carriage return for readability. Canonizing a criterium description string
    /// is to remove all these non significant characters.
extern std::string crit_action_canonize_string(const std::string & argument);


    /// creates a criterium object as defined by the command line's given string

    /// \param[in] dialog for user interaction
    /// \param[in] argument is a *canonized* criterium argument
    /// \param[in] hourshift the hourshift used to compare dates "more recent than"
    /// \return a criterium object
    /// \note this function is recursive, it may throw Erange exception in case of syntaxical error
    /// \note second point, the returned object is dynamically allocated, this is the duty of the caller
    /// to release its memory calling the delete operator.
extern const libdar::crit_action * crit_action_create_from_string(libdar::user_interaction & dialog,
								  const std::string & argument,
								   const libdar::infinint & hourshift);

    /// @}

#endif
