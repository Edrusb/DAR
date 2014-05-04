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

    /// \file shell_interaction.hpp
    /// \brief user_interaction class for command_line tools
    /// \ingroup CMDLINE


#ifndef SHELL_INTERACTION_HPP
#define SHELL_INTERACTION_HPP

#include "../my_config.h"
#include <iostream>

#include "user_interaction.hpp"

using namespace std;
using namespace libdar;

    /// \addtogroup CMDLINE
    /// @{

    ///////////////
    // THIS AN OLD IMPLEMENTATION (a la C) THAT SHOULD BE REVIEWED USING A INHERITED CLASS FROM LIBDAR::USER_INTERACTION_CALLBACK
    ///////////////

    /// create and return the address of the (unique) shell_interaction object
    ///
    /// \param[in] out defines where are sent non interactive messages (informative messages)
    /// \param[in] interact defines where are sent interactive messages (those requiring an answer or confirmation) from the user
    /// \param[in] silent whether to send a warning message if the process is not attached to a terminal
    /// \note the returned object must be freed by a call to "delete"
user_interaction *shell_interaction_init(ostream *out, ostream *interact, bool silent);

void shell_interaction_change_non_interactive_output(ostream *out);
void shell_interaction_read_char(char & a);
void shell_interaction_close();
void shell_interaction_set_beep(bool mode);

    /// @}

#endif
