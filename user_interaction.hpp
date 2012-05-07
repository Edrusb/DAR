/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: user_interaction.hpp,v 1.8 2002/10/28 20:39:44 edrusb Rel $
//
/*********************************************************************/

#ifndef USER_INTERACTION_HPP
#define USER_INTERACTION_HPP

#include <string>

using namespace std;

extern void user_interaction_init(ostream *out, ostream *interact);
    // arg are the input file descriptor, output ostream object (on which are sent non interactive messages)
    // and intereact ostream object on which are sent message that require a user interaction
extern void user_interaction_change_non_interactive_output(ostream *out);
extern void user_interaction_pause(string message);
extern void user_interaction_warning(string message);
extern ostream &user_interaction_stream();
extern void user_interaction_set_beep(bool mode);
extern void ui_printf(char *format, ...);
    // sometimes easier to use printf-like call than iostream (cout << etc.)
    //////
extern void user_interaction_close();

#endif
