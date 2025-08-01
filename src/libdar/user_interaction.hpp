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

    /// \file user_interaction.hpp
    /// \brief defines the interaction interface between libdar and users.
    /// \ingroup API

#ifndef USER_INTERACTION_HPP
#define USER_INTERACTION_HPP

#include "../my_config.h"

extern "C"
{
#if MUTEX_WORKS
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#endif
}

#include <string>
#include <list>
#include "secu_string.hpp"
#include "infinint.hpp"
#include "thread_cancellation.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

	/// This is a pure virtual class that is used by libdar when interaction with the user is required.

	/// You can base your own class on it using C++ inheritance
	/// or use the predifined inherited classes user_interaction_callback which implements
	/// the interaction based on callback functions.
    class user_interaction
    {
    public:
	user_interaction() {};
	user_interaction(const user_interaction & ref) = default;
	user_interaction(user_interaction && ref) noexcept = default;
	user_interaction & operator = (const user_interaction & ref) = default;
	user_interaction & operator = (user_interaction && ref) noexcept = default;
	virtual ~user_interaction() = default;

	    // the following methode are used by libdar and rely in their inherited_* versions
	    // than must be defined in the inherited classes

	void message(const std::string & message);
	void pause(const std::string & message);
	std::string get_string(const std::string & message, bool echo);
	secu_string get_secu_string(const std::string & message, bool echo);

	    /// libdar uses this call to format output before sending to the message() method.

	    /// This is not a virtual method, it has not to be overwritten, it is
	    /// just a sublayer over warning()
	    /// Supported masks for the format string are:
	    /// - \%s \%c \%d \%\%  (normal behavior)
	    /// - \%i (matches infinint *)
	    /// - \%S (matches std::string *)
	    /// .
	virtual void printf(const char *format, ...);

	    /// known whether cancellation was requested for the current thread or an added thread

	bool cancellation_requested() const;

	    /// add a thread to monitor

	    /// when a thread cancellation is requested for a thread
	    /// libdar do no more pause() but assume negative answer and
	    /// invoke message() with the context and question. However
	    /// the user_interaction object may run in a different thread
	    /// than a libdar thread, it is thus necessary to inform the
	    /// user interaction object of the additional thread_id to monitor
	    /// for thread cancellation request and adapt the behavior of
	    /// this object. This is the purpose of this call, in addition
	    /// to the thread the user_interaction is running in, at the time of any
	    /// pause() request the user_interaction will call message() if
	    /// the current thread or any of the added thread_id are under
	    /// a cancellation request

	void add_thread_to_monitor(pthread_t tid);

	    /// remove a thread from monitoring

	void remove_thread_from_monitor(pthread_t tid);


    protected:
	    /// method used to display a warning or a message to the user.
	    ///
	    /// \param[in] message is the message to display.
	    /// \note messages passed by libdar are not ending with a newline by default
	    /// its up to the implementation to separate messages by the adequate mean
	virtual void inherited_message(const std::string & message) = 0;

	    /// method used to ask a boolean question to the user.

	    /// \param[in] message The boolean question to ask to the user
	    /// \return the answer of the user (true/yes or no/false)
	    /// \note messages passed by libdar are not ending with a newline by default
	    /// its up to the implementation to separate messages by the adequate mean
	virtual bool inherited_pause(const std::string & message) = 0;


	    /// method used to ask a question that needs an arbitrary answer.

	    /// \param[in] message is the question to display to the user.
	    /// \param[in] echo is set to false is the answer must not be shown while the user answers.
	    /// \return the user's answer.
	    /// \note messages passed by libdar are not ending with a newline by default
	    /// its up to the implementation to separate messages by the adequate mean
	virtual std::string inherited_get_string(const std::string & message, bool echo) = 0;

	    /// same a get_string() but uses libdar::secu_string instead of std::string

	    /// \param[in] message is the question to display to the user.
	    /// \param[in] echo is set to false is the answer must not be shown while the user answers.
	    /// \return the user's answer.
	    /// \note messages passed by libdar are not ending with a newline by default
	    /// its up to the implementation to separate messages by the adequate mean
	virtual secu_string inherited_get_secu_string(const std::string & message, bool echo) = 0;

    private:

#if MUTEX_WORKS
#if HAVE_PTHREAD_H
	thread_cancellation thcancel;

	std::list<pthread_t> monitoring;
#endif
#endif

    };

	/// @}

} // end of namespace

#endif
