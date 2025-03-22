/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
// to contact the author, see the AUTHOR file
/*********************************************************************/

    /// \file user_interaction_callback.hpp
    /// \brief defines the interaction between libdar and the user based on callback functions
    /// \ingroup API
    ///
    /// Three classes are defined
    /// - user_interaction is the root class that you can use to make your own classes
    /// - user_interaction_callback is a specialized inherited class which is implements
    ///   user interaction thanks to callback functions
    /// - user_interaction_blind provides fully usable objects that do not show anything
    ///   and always assume a negative answer from the user
    /// .


#ifndef USER_INTERACTION_CALLBACK_HPP
#define USER_INTERACTION_CALLBACK_HPP

#include "../my_config.h"

#include <string>
#include "user_interaction.hpp"
#include "secu_string.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{


	/// full implemented class for user_interaction based on callback functions.

	/// this class is an inherited class of user_interaction it is used by
	/// dar command line programs, but you can use it if you wish.
	/// \ingroup API

    class user_interaction_callback : public user_interaction
    {
    public:

	using message_callback = void (*)(const std::string &x, void *context);
	using pause_callback = 	bool (*)(const std::string &x, void *context);
	using get_string_callback = std::string (*)(const std::string &x, bool echo, void *context);
	using get_secu_string_callback = secu_string (*)(const std::string &x, bool echo, void *context);

	    /// constructor which receive the callback functions.

	    /// \param[in] x_message_callback is used by message() method
	    /// \param[in] x_answer_callback is used by the pause() method
	    /// \param[in] x_string_callback is used by get_string() method
	    /// \param[in] x_secu_string_callback is used by get_secu_string() method
	    /// \param[in] context_value will be passed as last argument of callbacks when
	    /// called from this object.
	    /// \note The context argument of each callback is set with the context_value given
	    /// in the user_interaction_callback object constructor. The value can
	    /// can be any arbitrary value (nullptr is valid), and can be used as you wish.
	    /// Note that the listing callback is not defined here, but thanks to a specific method
	user_interaction_callback(message_callback x_message_callback,
				  pause_callback x_answer_callback,
				  get_string_callback x_string_callback,
				  get_secu_string_callback x_secu_string_callback,
				  void *context_value);
	user_interaction_callback(const user_interaction_callback & ref) = default;
	user_interaction_callback(user_interaction_callback && ref) noexcept = default;
	user_interaction_callback & operator = (const user_interaction_callback & ref) = default;
	user_interaction_callback & operator = (user_interaction_callback && ref) noexcept = default;
	~user_interaction_callback() = default;

	    /// listing callback can be now passed directly to archive::get_children_of()

	    /// dar_manager_show_files callback can now be passed directly to database::get_files()

	    /// dar_manager_contents callback is not necessary, use database::get_contents() method

	    /// dar_manager_statistics callback can now be passed directly to database::show_most_recent_stats()

	    /// dar_manager_get_show_version callback can now be passed directly to database::get_version()

    protected:

	    /// overwritting method from parent class.
	virtual void inherited_message(const std::string & message) override;

	    /// overwritting method from parent class.
       	virtual bool inherited_pause(const std::string & message) override;

	    /// overwritting method from parent class.
	virtual std::string inherited_get_string(const std::string & message, bool echo) override;

	    /// overwritting method from parent class.
	virtual secu_string inherited_get_secu_string(const std::string & message, bool echo) override;

	    /// change the context value of the object that will be given to callback functions
	void change_context_value(void *new_value) { context_val = new_value; };

    private:
	message_callback message_cb;
	pause_callback pause_cb;
	get_string_callback get_string_cb;
	get_secu_string_callback get_secu_string_cb;
	void *context_val;
    };

	/// @}

} // end of namespace

#endif
