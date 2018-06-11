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

    /// \file op_tools.hpp
    /// \brief contains tools helper for overwriting policy management
    /// \ingroup Private

#ifndef OP_TOOLS_HPP
#define OP_TOOLS_HPP

#include "../my_config.h"

#include <deque>

#include "crit_action.hpp"
#include "cat_entree.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// ask user for EA action

	/// \param[in] dialog for user interaction
	/// \param[in] full_name full path to the entry do ask decision for
	/// \param[in] already_here pointer to the object 'in place'
	/// \param[in] dolly pointer to the object 'to be added'
	/// \return the action decided by the user. The user may also choose to abort, which will throw an Euser_abort exception
    extern over_action_ea op_tools_crit_ask_user_for_EA_action(user_interaction & dialog, const std::string & full_name, const cat_entree *already_here, const cat_entree *dolly);

	/// ask user for FSA action

	/// \param[in] dialog for user interaction
	/// \param[in] full_name full path to the entry do ask decision for
	/// \param[in] already_here pointer to the object 'in place'
	/// \param[in] dolly pointer to the object 'to be added'
	/// \return the action decided by the user. The user may also choose to abort, which will throw an Euser_abort exception
    extern over_action_ea op_tools_crit_ask_user_for_FSA_action(user_interaction & dialog, const std::string & full_name, const cat_entree *already_here, const cat_entree *dolly);

	/// ask user for Data action

	/// \param[in] dialog for user interaction
	/// \param[in] full_name full path to the entry do ask decision for
	/// \param[in] already_here pointer to the object 'in place'
	/// \param[in] dolly pointer to the object 'to be added'
	/// \return the action decided by the user. The user may also choose to abort, which will throw an Euser_abort exception
    extern over_action_data op_tools_crit_ask_user_for_data_action(user_interaction & dialog, const std::string & full_name, const cat_entree *already_here, const cat_entree *dolly);


	/// show information suited for user comparison and decision for entry in conflict

	/// \param[in] dialog for user interaction
	/// \param[in] full_name path to the entry of the entry to display information
	/// \param[in] already_here pointer to the object 'in place'
	/// \param[in] dolly pointer to the object 'to be added'
    extern void op_tools_crit_show_entry_info(user_interaction & dialog, const std::string & full_name, const cat_entree *already_here, const cat_entree *dolly);

	/// @}

} // end of namespace

#endif
