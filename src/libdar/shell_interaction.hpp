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

namespace libdar
{

	/// \addtogroup CMDLINE
	/// @{

    class shell_interaction : public user_interaction
    {
    public:
	    /// constructor
	    ///
	    /// \param[in] out defines where are sent non interactive messages (informative messages)
	    /// \param[in] interact defines where are sent interactive messages (those requiring an answer or confirmation) from the user
	    /// \param[in] silent whether to send a warning message if the process is not attached to a terminal
	shell_interaction(std::shared_ptr<std::ostream> out,
			  std::shared_ptr<std::ostream> interact,
			  bool silent);

	    /// copy constructor
	shell_interaction(const shell_interaction & ref);

	    /// no move constructor
	shell_interaction(shell_interaction && ref) noexcept = delete;

	    /// assignment operator (not implmented)
	shell_interaction & operator = (const shell_interaction & ref) = delete;

	    /// no move operator
	shell_interaction & operator = (shell_interaction && ref) noexcept = delete;

	    /// destructor
	~shell_interaction();

	void change_non_interactive_output(std::shared_ptr<std::ostream> out);
	void read_char(char & a);
	void set_beep(bool mode) { beep = mode; };

	    /// make a pause each N line of output when calling the warning method

	    //! \param[in] num is the number of line to display at once, zero for unlimited display
	    //! \note. Since API 3.1, the warning method is no more a pure virtual function
	    //! you need to call the parent warning method in your method for this warning_with_more
	    //! method works as expected.
	void warning_with_more(U_I num) { at_once = num; count = 0; };

    protected:
	    // inherited methods from user_interaction class

	virtual void inherited_message(const std::string & message) override;
	virtual bool inherited_pause(const std::string &message) override;
	virtual std::string inherited_get_string(const std::string & message, bool echo) override;
	virtual secu_string inherited_get_secu_string(const std::string & message, bool echo) override;

    private:
	    // data type

	enum mode { m_initial, m_inter, m_noecho };

	    // object fields and methods

	S_I input;               //< filedescriptor to read from the user's answers
	std::shared_ptr<std::ostream> output;    //< holds the destination for non interactive messages
	std::shared_ptr<std::ostream> inter;     //< holds the destination for interactive messages
	bool beep;               //< whether to issue bell char before displaying a new interactive message
        termios initial;         //< controlling terminal configuration when the object has been created
	termios interaction;     //< controlling terminal configuration to use when requiring user interaction
	termios initial_noecho;  //< controlling terminal configuration to use when noecho has been requested
	bool has_terminal;       //< true if a terminal could be found
	U_I at_once, count;      //< used by warning_with_more

	void set_term_mod(mode m);
	void my_message(const std::string & mesg);

	    // class fields and methods

	static const U_I bufsize;

    };

    /// @}

} // end of namespace

#endif
