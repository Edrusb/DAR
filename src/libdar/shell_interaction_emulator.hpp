/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file shell_interaction_emulator.hpp
    /// \brief wrapper class that given a user_interaction send it the shell_interaction formatted output
    /// \ingroup API


#ifndef SHELL_INTERACTION_EMULATOR_HPP
#define SHELL_INTERACTION_EMULATOR_HPP

extern "C"
{
} // end extern "C"

#include "../my_config.h"

#include "shell_interaction.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

    class shell_interaction_emulator : public shell_interaction
    {
    public:
	shell_interaction_emulator(user_interaction *emulator);
	shell_interaction_emulator(const shell_interaction_emulator & ref) = default;
	shell_interaction_emulator(shell_interaction_emulator && ref) noexcept = delete;
	shell_interaction_emulator & operator = (const shell_interaction_emulator & ref) = delete;
	shell_interaction_emulator & operator = (shell_interaction_emulator && ref) noexcept = delete;
	virtual ~shell_interaction_emulator() = default;

    protected:
	virtual void inherited_message(const std::string & message) override { emul->message(message); };
	virtual bool inherited_pause(const std::string & message) override;
	virtual std::string inherited_get_string(const std::string & message, bool echo) override { return emul->get_string(message, echo); };
	virtual secu_string inherited_get_secu_string(const std::string & message, bool echo) override { return emul->get_secu_string(message, echo); };

    private:
	user_interaction *emul;
    };

    /// @}

} // end of namespace

#endif
