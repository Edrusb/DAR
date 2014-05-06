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

extern "C"
{
#if HAVE_TERMIOS_H
#include <termios.h>
#endif
} // end extern "C"

#include "../my_config.h"
#include <iostream>

#include "user_interaction.hpp"

using namespace std;
using namespace libdar;

    /// \addtogroup CMDLINE
    /// @{

class shell_interaction : public user_interaction_callback
{
public:
	/// constructor
	///
	/// \param[in] out defines where are sent non interactive messages (informative messages)
	/// \param[in] interact defines where are sent interactive messages (those requiring an answer or confirmation) from the user
	/// \param[in] silent whether to send a warning message if the process is not attached to a terminal
	/// \note out and interact must exist during the whole life of the shell_interaction object, its remains the duty
	/// of the caller to releases theses ostream objects if necessary.
    shell_interaction(ostream *out, ostream *interact, bool silent);

	/// copy constructor
    shell_interaction(const shell_interaction & ref);

	/// destructor
    ~shell_interaction();

    void change_non_interactive_output(ostream *out);
    void read_char(char & a);
    void set_beep(bool mode) { beep = mode; };


	    /// overwritting method from parent class.
    virtual user_interaction *clone() const { user_interaction *ret = new (get_pool()) shell_interaction(*this); if(ret == NULL) throw Ememory("shell_interaction::clone"); return ret; };

private:
	// data type

    enum mode { m_initial, m_inter, m_noecho };

	// object fields and methods

    S_I input;               //< filedescriptor to read from the user's answers
    ostream *output;         //< holds the destination for non interactive messages
    ostream *inter;          //< holds the destination for interactive messages
    bool beep;               //< whether to issue bell char before displaying a new interactive message
    termios initial;         //< controlling terminal configuration when the object has been created
    termios interaction;     //< controlling terminal configuration to use when requiring user interaction
    termios initial_noecho;  //< controlling terminal configuration to use when noecho has been requested
    bool has_terminal;       //< true if a terminal could be found

    void set_term_mod(mode m);

	// class fields and methods

    static const U_I bufsize;

    static bool interaction_pause(const string &message, void *context);
    static void interaction_warning(const string & message, void *context);
    static string interaction_string(const string & message, bool echo, void *context);
    static secu_string interaction_secu_string(const string & message, bool echo, void *context);
};

    /// @}

#endif
