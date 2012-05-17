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
// $Id: shell_interaction.hpp,v 1.10 2011/01/09 17:25:58 edrusb Rel $
//
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

user_interaction *shell_interaction_init(ostream *out, ostream *interact, bool silent);
    // arg are output ostream object (on which are sent non interactive messages)
    // and interact ostream object (on which are sent message that require a user interaction)

    // the argument returned must be freed with a call to "delete"

void shell_interaction_change_non_interactive_output(ostream *out);
void shell_interaction_read_char(char & a);
void shell_interaction_close();
void shell_interaction_set_beep(bool mode);

    /// @}

#endif
