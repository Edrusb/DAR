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
// $Id: thread_cancellation.hpp,v 1.1.2.1 2005/05/06 10:56:41 edrusb Rel $
//
/*********************************************************************/

    /// \file thread_cancellation.hpp
    /// \brief to be able to cancel libdar operation while running in a given thread.
    ///
    /// the class thread_cancellation implemented in this module
    /// permits to define checkpoints where is looked whether the current
    /// thread has been marked as to be canceled by the user
    /// The advantage of this class is that it then throws a Euser_abort
    /// exception which properly terminates the libdar operation in the thread
    /// freeing allocated memory and release mutex properly. Note that the thread
    /// is not canceled but libdar call in this thread returns as soon as a checkpoint
    /// is met during the execution.

#ifndef THREAD_CANCELLATION_HPP
#define THREAD_CANCELLATION_HPP

#include "../my_config.h"

extern "C"
{
#if MUTEX_WORKS
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#else
#define pthread_t U_I
#endif
}

namespace libdar
{

	/// class to be used as parent to provide checkpoints to inherited classes

	/// the class provides a checkpoints to inherited classes and a mechanism
	/// that let any libdar external code to ask the termination of a libdar
	/// call executing in a given thread. This does not imply the termination
	/// of the thread itself but it implies the return of the thread execution
	/// to external program.
	/// \ingroup Private

    class thread_cancellation
    {
    public:

	    /// the constructor
	thread_cancellation();

	    /// the checkpoint where is seen whether the current libdar call must abort

	    /// \exception Euser_abort is thrown if the thread the checkpoint is running
	    /// from is marked as to be canceled.
	void check_self_cancellation() const;

	    /// mandatory initialization static method

	    /// must be called once before any call to thread_cancellation class's methods
	static void init();

	    /// marks the thread given in argument as to be canceled

	    //! \param[in] tid the thread ID of the thread where any libdar call must abort
	    //! \return true if no already pending thread cancellation is under process
	static bool cancel(pthread_t tid);

	    /// gives the thread ID of the current thread in cancellation process

	    //! \param[out] tid the tid of the current thread under cancellation
	    //! \return true if a current thread is waited
	    //! to be canceled, and then gives its tid in argument.
	static bool current_cancel(pthread_t & tid);

	    /// abort the pending thread cancellation

	    /// \return true if the pending thread was still running and
	    /// false if it has already aborted.
	static bool clear_pending_request();

#if MUTEX_WORKS
    private:
	pthread_t tid;                         ///< thread id of the current thread

	static bool cancellation;               ///< true if a thread has to be canceled
	static pthread_t to_cancel;            ///< thread to cancel
	static pthread_mutex_t access;         ///< mutex for the access to "to_cancel"
	static bool initialized;               ///< true if the mutex has been initialized
#endif
    };

} // end of namespace

#endif
