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

    /// \file user_interaction_blind.hpp
    /// \brief defines the interaction between libdar and a non communcant "blind" user
    /// \note yes blind people do communicate, no offense to them, this class just
    /// "blindly" answer and do never show messages to a particular user
    /// \ingroup API
    ///

#ifndef USER_INTERACTION_BLIND_HPP
#define USER_INTERACTION_BLIND_HPP

#include "../my_config.h"

#include <string>
#include "user_interaction.hpp"
#include "erreurs.hpp"
#include "integers.hpp"
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

	virtual bool pause2(const std::string & message) override { return false; };

	virtual std::string get_string(const std::string & message, bool echo) override { return "user_interaction_blind, is blindly answering no"; };
	virtual secu_string get_secu_string(const std::string & message, bool echo) override { return secu_string(); };

	virtual user_interaction *clone() const override { user_interaction *ret = new (std::nothrow) user_interaction_blind(); if(ret == nullptr) throw Ememory("user_interaction_blind::clone"); return ret; };

    protected:
	virtual void inherited_warning(const std::string & message) override {}; // do not display any warning, this is "bind user_interaction" !

    };

	/// @}

} // end of namespace

#endif
