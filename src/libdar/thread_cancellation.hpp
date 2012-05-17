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
// $Id: thread_cancellation.hpp,v 1.5.2.4 2006/12/12 18:25:35 edrusb Rel $
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
#endif
}
#include <list>
#include "integers.hpp"

namespace libdar
{

	/// class to be used as parent to provide checkpoints to inherited classes

	/// the class provides a checkpoints to inherited classes and a mechanism
	/// that let any libdar external code to ask the termination of a libdar
	/// call executing in a given thread. This does not imply the termination
	/// of the thread itself but it implies the return of the thread execution
	/// to the code that called libdar
	/// \ingroup Private

    class thread_cancellation
    {
    public:

	    /// the constructor
	thread_cancellation();

	    /// the destructor
	virtual ~thread_cancellation();

	    /// the checkpoint where is seen whether the current libdar call must abort

	    /// \exception Euser_abort is thrown if the thread the checkpoint is running
	    /// from is marked as to be canceled.
	void check_self_cancellation() const;

	    /// by default delayed (non immediate) cancellation generate an specific exception
	    /// it is possible for delayed cancellation only, do block such exceptions for a certain time

	    ///\param[in] mode can be set to true to block delayed cancellations
	    ///\note when unblocking delayed cancellations, if a delayed cancellation has been
	    ///requested during the ignore time, it will be thrown by this call
	void block_delayed_cancellation(bool mode);


	    /// mandatory initialization static method

	    /// must be called once before any call to thread_cancellation class's methods
	static void init();


#if MUTEX_WORKS
	    /// marks the thread given in argument as to be canceled

	    //! \param[in] tid the thread ID of the thread where any libdar call must abort
	    //! \param[in] x_immediate whether the cancellation must be as fast as possible or can take a
	    //! \param[in] x_flag is a value to transmit to the Ethread_cancel exception used to cancel libdar's call stack
	    //! little time to make a usable archive
	static void cancel(pthread_t tid, bool x_immediate, U_64 x_flag);

	    /// gives the cancellation status of the given thread ID

	    //! \param[in] tid the thread to check
	    //! \return true if the given thread is under cancellation
	static bool cancel_status(pthread_t tid);

	    /// abort the pending thread cancellation

	    /// \return true if the pending thread was still running and
	    /// false if it has already aborted.
	static bool clear_pending_request(pthread_t tid);
#endif

	    /// method for debugging/control purposes
	static U_I count()
	{
#if MUTEX_WORKS
	    return info.size();
#else
	    return 0;
#endif
	};

#if MUTEX_WORKS
    private:

	    // class types

	struct fields
	{
	    pthread_t tid;             ///< thread id of the current thread
	    bool block_delayed;        ///< whether we buffer any delayed cancellation requests for "this" thread
	    bool immediate;            ///< whether we take a few more second to make a real usable archive
	    bool cancellation;         ///< true if a thread has to be canceled
	    U_64 flag;                 ///< user defined informational field, given to the Ethread_cancel constructor
	};

	    // object information

	fields status;

	    // class's static variables and types

	static pthread_mutex_t access;         ///< mutex for the access to "info"
	static bool initialized;               ///< true if the mutex has been initialized
	static std::list<thread_cancellation *> info;  ///< list of all object
	static std::list<fields> preborn;      ///< canceled thread that still not have a thread_cancellation object to deal with cancellation

#endif
    };

} // end of namespace

#endif
