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

    // NOTE : The following comments are used by doxygen to generate the documentation of reference

    /// \mainpage
    /// You will find here the reference documentation for the dar and libdar source code, split in several "modules".
    /// - API module: contains all information for using libdar within your program
    /// - Tools module: contains some routines you may like to have a look at (but using them with caution)
    /// - Private module: contains all libdar internal documentation, it is not necessary to read it to be able to use libdar
    /// - CMDLINE module: contains the documentation for command-line tools, you might want to have a look for illustration of library usage.
    /// .
    /// CMDLINE module is out of any namespace as it is not inteded to be used by external application. All other symbols are part of the libdar namespace.
    ///
    /// Please not that an API tutorial is also available for a higher view of this library.


    /// \defgroup API API
    /// \brief APlication Interface
    ///
    /// This gathers all symbols that may be accessed from an external
    /// program. Other symbols are not as much documented, and
    /// may change or be removed without any warning or backward
    /// compatibility support. So only use the function, macro, types,
    /// classes... defined as member of the API module in you external programs.


    /// \defgroup Private Private
    /// \brief Symbols that are not to be used by external software.
    ///
    /// never use threses symboles (function, macro, variables, types, etc.)
    /// they are not intended to be used by external programs
    /// and may change or disapear without any warning or backward
    /// compatibility. Some are however documented for libdar development ease.


    /// \file libdar.hpp
    /// \brief the main file of the libdar API definitions
    /// \ingroup API



#ifndef LIBDAR_HPP
#define LIBDAR_HPP

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
#include "compressor.hpp"
#include "path.hpp"
#include "mask.hpp"
#include "integers.hpp"
#include "infinint.hpp"
#include "statistics.hpp"
#include "user_interaction.hpp"
#include "deci.hpp"
#include "archive.hpp"
#include "crypto.hpp"
#include "thread_cancellation.hpp"
#include "compile_time_features.hpp"
#include "capabilities.hpp"

    /// \addtogroup API
    /// @{

    ///  The following macro are used in the "exception" argument of the *_noexcept() functions
#define LIBDAR_XXXXXXXX

    /// normal return no exception has been thrown
#define LIBDAR_NOEXCEPT 0
    /// memory has been exhausted
#define LIBDAR_EMEMORY 1
    /// internal bug error.
#define LIBDAR_EBUG 2
    /// division by zero or other arithmetic error
#define LIBDAR_EINFININT 3
    /// limitint overflow
#define LIBDAR_ELIMITINT 4
    /// range error
#define LIBDAR_ERANGE 5
    /// decimal representation error
#define LIBDAR_EDECI 6
    /// feature not (yet) implemented
#define LIBDAR_EFEATURE 7
    /// hardware failure
#define LIBDAR_EHARDWARE 8
    /// user has aborted the operation
#define LIBDAR_EUSER_ABORT 9
    /// data inconsistency, error concerning the treated data
#define LIBDAR_EDATA 10
    /// inter slice script failure
#define LIBDAR_ESCRIPT 11
    /// libdar invalid call (wrong argument given to call, etc.)
#define LIBDAR_ELIBCALL 12
    /// unknown error
#define LIBDAR_UNKNOWN 13
    /// feature not activated at compilation time
#define LIBDAR_ECOMPILATION 14
    /// thread cancellation has been requested
#define LIBDAR_THREAD_CANCEL 15
    /// @}

    /// libdar namespace encapsulate all libdar symbols
namespace libdar
{
	/// \addtogroup API
	/// @{


	///  libdar Major version defined at compilation time
    const U_I LIBDAR_COMPILE_TIME_MAJOR = 5;
	///  libdar Medium version defined at compilation time
    const U_I LIBDAR_COMPILE_TIME_MEDIUM = 11;
	///  libdar Minor version defined at compilation time
    const U_I LIBDAR_COMPILE_TIME_MINOR = 0;

	////////////////////////////////////////////////////////////////////////
	// LIBDAR INITIALIZATION METHODS                                      //
	//                                                                    //
	//      A FUNCTION OF THE get_version*() FAMILY *MUST* BE CALLED      //
	//            BEFORE ANY OTHER FUNCTION OF THIS LIBRARY               //
	//                                                                    //
	// CLIENT PROGRAM MUST CHECK THAT THE MAJOR NUMBER RETURNED           //
	// BY THIS CALL IS NOT GREATER THAN THE VERSION USED AT COMPILATION   //
        // TIME. IF SO, THE PROGRAM MUST ABORT AND RETURN A WARNING TO THE    //
	// USER TELLING THE DYNAMICALLY LINKED VERSION IS TOO RECENT AND NOT  //
	// COMPATIBLE WITH THIS SOFTWARE. THE MESSAGE MUST INVITE THE USER    //
	// TO UPGRADE HIS SOFTWARE WITH A MORE RECENT VERSION COMPATIBLE WITH //
	// THIS LIBDAR RELEASE.                                               //
	////////////////////////////////////////////////////////////////////////

	/// return the libdar version, and make libdar initialization (may throw Exceptions)

	/// It is mandatory to call this function (or another one of the get_version* family)
	/// \param[out] major the major number of the version
	/// \param[out] medium the medium number of the version
	/// \param[out] minor the minor number of the version
	/// \param[in] init_libgcrypt whether to initialize libgcrypt if not already done (not used if libcrypt is not linked with libdar)
	/// \note the calling application must match that the major function
	/// is the same as the libdar used at compilation time. See API tutorial for a
	/// sample code.
    extern void get_version(U_I & major, U_I & medium, U_I & minor, bool init_libgcrypt = true);

	/// return the libdar version, and make libdar initialization (does not throw exceptions)

	/// It is mandatory to call this function (or another one of the get_version* family)
	/// \param[out] major the major number of the version
	/// \param[out] medium the medium number of the version
	/// \param[out] minor the minor number of the version
	/// \param[out] exception is to be compared with the LIBDAR_* macro to know whether the call succeeded
	/// \param[out] except_msg in case exception is not equal to LIBDAR_NOEXCEPT this argument contains
	/// \param[in] init_libgcrypt whether to initialize libgcrypt if not already done (not used if libcrypt is not linked with libdar)
	/// a human readable explaination of the error met.
	/// \note the calling application must match that the major function
	/// is the same as the libdar used at compilation time. See API tutorial for a
	/// sample code.
    extern void get_version_noexcept(U_I & major, U_I & medium, U_I & minor, U_16 & exception, std::string & except_msg, bool init_libgcrypt = true);


	///////////////////////////////////////////////
	// CLOSING/CLEANING LIBDAR                   //
	///////////////////////////////////////////////

	// while libdar has only a single boolean as global variable
	// that defines whether the library is initialized or not
	// it must proceed to mutex, and dependent libraries initializations
	// (liblzo, libgcrypt, etc.), which is done during the get_version() call
	// Some library also need to clear some data so the following call
	// is provided in that aim and must be called when libdar will no more
	// be used by the application.

    extern void close_and_clean();


	//////////
	// WRAPPER FUNCTIONS AROUND archive class methods to trap exceptions and convert them in error code and message
	// these are intended for C program/programmers not enough confident with C++.
	//
	// FOR LIBDAR C++ APPLICATIONS, YOU WOULD RATHER USE THE archive C++ CLASS THAN THESE FOLLOWING WRAPPERS
	//
	//////////


	/// this is a wrapper around the archive constructor known as the "read" constructor

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern archive* open_archive_noexcept(user_interaction & dialog,
					  const path & chem, const std::string & basename,
					  const std::string & extension,
					  const archive_options_read & options,
					  U_16 & exception,
					  std::string & except_msg);


	/// this is a wrapper around the archive constructor known as the "create" constructor

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern archive *create_archive_noexcept(user_interaction & dialog,
					    const path & fs_root,
					    const path & sauv_path,
					    const std::string & filename,
					    const std::string & extension,
					    const archive_options_create & options,
					    statistics * progressive_report,
					    U_16 & exception,
					    std::string & except_msg);



	/// this is a wrapper around the archive constructor known as the "isolate" constructor

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern archive *isolate_archive_noexcept(user_interaction & dialog,
					     archive *ptr,
					     const path &sauv_path,
					     const std::string & filename,
					     const std::string & extension,
					     const archive_options_isolate & options,
					     U_16 & exception,
					     std::string & except_msg);

	/// this is a wrapper around the archive constructor known as the "merging" constructor

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern archive *merge_archive_noexcept(user_interaction & dialog,
					   const path & sauv_path,
					   archive *ref_arch1,
					   const std::string & filename,
					   const std::string & extension,
					   const archive_options_merge & options,
					   statistics * progressive_report,
					   U_16 & exception,
					   std::string & except_msg);


	/// this is wrapper around the archive destructor

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern void close_archive_noexcept(archive *ptr,
				       U_16 & exception,
				       std::string & except_msg);


	/// this is wrapper around the op_extract method

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern statistics op_extract_noexcept(user_interaction & dialog,
					  archive *ptr,
					  const path &fs_root,
					  const archive_options_extract & options,
					  statistics * progressive_report,
					  U_16 & exception,
					  std::string & except_msg);


	/// this is wrapper around the op_listing method

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern void op_listing_noexcept(user_interaction & dialog,
				    archive *ptr,
				    const archive_options_listing & options,
				    U_16 & exception,
				    std::string & except_msg);


	/// this is wrapper around the op_diff method

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern statistics op_diff_noexcept(user_interaction & dialog,
				       archive *ptr,
				       const path & fs_root,
				       const archive_options_diff & options,
				       statistics * progressive_report,
				       U_16 & exception,
				       std::string & except_msg);


	/// this is wrapper around the op_test method

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern statistics op_test_noexcept(user_interaction & dialog,
				       archive *ptr,
				       const archive_options_test & options,
				       statistics * progressive_report,
				       U_16 & exception,
				       std::string & except_msg);


	/// this is wrapper around the get_children_of method

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern bool get_children_of_noexcept(user_interaction & dialog,
					 archive *ptr,
					 const std::string & dir,
					 U_16 & exception,
					 std::string & except_msg);



	///////////////////////////////////////////////
	// TOOLS ROUTINES                            //
	///////////////////////////////////////////////


        /// routine provided to convert std::string to char *

	/// \param[in] x the string to convert
	/// \param[out] exception the return status of the call
	/// \param[out] except_msg the message taken from the caught exception in case of error
	/// for an explaination of the two last arguments exception and except_msg check
	/// the get_version_noexcept function
        /// \return the address of a newly allocated memory
        /// which must be released calling the "delete []"
        /// operator when no more needed.
        /// \return nullptr in case of error
    extern char *libdar_str2charptr_noexcept(const std::string & x, U_16 & exception, std::string & except_msg);

	///////////////////////////////////////////////
	// THREAD CANCELLATION ROUTINES              //
	///////////////////////////////////////////////

#if MUTEX_WORKS
	/// thread cancellation activation

	/// ask that any libdar code running in the thread given as argument be cleanly aborted
	/// when the execution will reach the next libdar checkpoint
	/// \param[in] tid is the Thread ID to cancel libdar in
	/// \param[in] immediate whether to cancel thread immediately or just signal the request to the thread
	/// \param[in] flag an arbitrary value passed as-is through libdar
    inline extern void cancel_thread(pthread_t tid, bool immediate = true, U_64 flag = 0) { thread_cancellation::cancel(tid, immediate, flag); }

	/// consultation of the cancellation status of a given thread

	/// \param[in] tid is the tid of the thread to get status about
	/// \return false if no cancellation has been requested for the given thread
    inline extern bool cancel_status(pthread_t tid) { return thread_cancellation::cancel_status(tid); }

	/// thread cancellation deactivation

	/// abort the thread cancellation for the given thread
	/// \return false if no thread cancellation was under process for that thread
	/// or if there is no more pending cancellation (thread has already been canceled).
    inline extern bool cancel_clear(pthread_t tid) { return thread_cancellation::clear_pending_request(tid); }
#endif


	/// @}

} // end of namespace

#endif
