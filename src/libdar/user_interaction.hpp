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
// $Id: user_interaction.hpp,v 1.5 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/

#ifndef USER_INTERACTION_HPP
#define USER_INTERACTION_HPP

#include "../my_config.h"
#include <string>

namespace libdar
{

        ///////////
        // 2 call back function must be set before using this module
        // the first callback is called by libdar to send a warning to the user
        // the warning message is given in argument
        // the seond callback is called by libdar to ask a question to the user
        // the question is given in argument and the callback must return the
        // user answer (true for yes/OK, false for no/NOK).
        //
        // theses two callback function must be given to libdar using the two
        // following function which argument is the adresse of the callback function
        //
    extern void set_warning_callback(void (*callback)(const std::string &x));
    extern void set_answer_callback(bool (*callback)(const std::string &x));


	// the three following functions are used inside libdar and relies on the callback function
	// set above
    extern void user_interaction_pause(const std::string & message);
    extern void user_interaction_warning(const std::string & message);

	///////////////
	// supported masks for the format
	// %s %c %d %%  (normal behavior)
	// %i (matches infinint *)
	// %S (matches std::string *)
    extern void ui_printf(const char *format, ...);

} // end of namespace

#endif
