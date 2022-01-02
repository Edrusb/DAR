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

    /// \file user_interaction_blind.hpp
    /// \brief defines the interaction between libdar and a non communcant "blind" user
    /// \note yes blind people do communicate, no offense to them, this class just
    /// "blindly" answer and do never show messages to a particular user
    /// \ingroup API


#ifndef USER_INTERACTION_BLIND_HPP
#define USER_INTERACTION_BLIND_HPP

#include "../my_config.h"

#include <string>
#include "user_interaction.hpp"
#include "secu_string.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

	/// full implementation class for user_interaction, which shows nothing and assumes answer "no" to any question

    class user_interaction_blind : public user_interaction
    {
    public:
	user_interaction_blind() = default;
	user_interaction_blind(const user_interaction_blind & ref) = default;
	user_interaction_blind(user_interaction_blind && ref) noexcept = default;
	user_interaction_blind & operator = (const user_interaction_blind & ref) = default;
	user_interaction_blind & operator = (user_interaction_blind && ref) noexcept = default;
	~user_interaction_blind() = default;

    protected:
	virtual void inherited_message(const std::string & message) override { }; // do nothing
	virtual bool inherited_pause(const std::string & message) override { return false; };
	virtual std::string inherited_get_string(const std::string & message, bool echo) override { return "user_interaction_blind, is blindly answering no"; };
	virtual secu_string inherited_get_secu_string(const std::string & message, bool echo) override { return secu_string(); };
    };

	/// @}

} // end of namespace

#endif
