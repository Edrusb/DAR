//*********************************************************************/
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
// $Id: libdar.hpp,v 1.38.2.4 2005/09/11 18:35:47 edrusb Rel $
//
/*********************************************************************/
//

    /// \file libdar.hpp
    /// \brief the main file of the libdar API definitions

    /// \addtogroup Private
    /// \brief Symbols that are not to be used by external software.
    ///
    /// never use threses symboles (function, macro, variables, types, etc.)
    /// they are not intended to be used by external programs
    /// and may change or disapear without any warning or backward
    /// compatibility

    /// \addtogroup API
    /// \brief APlication Interface
    ///
    /// This gather all symbols that may be accessed from an external
    /// program. Other symbols are not as much documented, and
    /// may change or be removed without any warning or backward
    /// compatibility. So only use the function, macro, types
    /// defined as member of the API in you external programs.


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
    /// @}

    /// libdar namespace encapsulate all libdar symbols
namespace libdar
{
    /// \addtogroup API
    /// @{


	///  libdar Major version defined at compilation time
    const U_I LIBDAR_COMPILE_TIME_MAJOR = 3;
	///  libdar Medium version defined at compilation time
    const U_I LIBDAR_COMPILE_TIME_MEDIUM = 1;
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

    extern void get_version(U_I & major, U_I & minor);
    extern void get_version_noexcept(U_I & major, U_I & minor, U_16 & exception, std::string & except_msg);

	/// return the libdar version, and make libdar initialization (may throw Exceptions)

	/// It is mandatory to call this function (or another one of the get_version* family)
	/// \param[out] major the major number of the version
	/// \param[out] medium the medium number of the version
	/// \param[out] minor the minor number of the version
	/// \note the calling application must match that the major function
	/// is the same as the libdar used at compilation time. See API tutorial for a
	/// sample code.
    extern void get_version(U_I & major, U_I & medium, U_I & minor);

	/// return the libdar version, and make libdar initialization (does not throw exceptions)

	/// It is mandatory to call this function (or another one of the get_version* family)
	/// \param[out] major the major number of the version
	/// \param[out] medium the medium number of the version
	/// \param[out] minor the minor number of the version
	/// \param[out] exception is to be compared with the LIBDAR_* macro to know whether the call succeeded
	/// \param[out] except_msg in case exception is not equal to LIBDAR_NOEXCEPT this argument contains
	/// a human readable explaination of the error met.
	/// \note the calling application must match that the major function
	/// is the same as the libdar used at compilation time. See API tutorial for a
	/// sample code.
    extern void get_version_noexcept(U_I & major, U_I & medium, U_I & minor, U_16 & exception, std::string & except_msg);


	/// return the options activated that have been activated at compilation time

	/// \param[out] ea whether Extended Attribute support is available
	/// \param[out] largefile whether large file support is available
	/// \param[out] nodump whether the nodump feature is available
	/// \param[out] special_alloc whether special allocation is activated
	/// \param[out] bits the internal integer type used
	/// \param[out] thread_safe whether thread safe support is available
	/// \param[out] libz whether gzip compression is available
	/// \param[out] libbz2 whether bz2 compression is available
	/// \param[out] libcrypto whether strong encryption is available
	/// \note This function does never throw exceptions, so there is no
	/// get_compile_time_features_noexcept() function available.
    extern void get_compile_time_features(bool & ea, bool & largefile, bool & nodump, bool & special_alloc, U_I & bits, bool & thread_safe,
					  bool & libz, bool & libbz2, bool & libcrypto);

	//////////
	// WRAPPER FUNCTIONS AROUND archive class methods to trap exceptions and convert them in error code and message
	// theses are intended for C program/programmers not enough confident with C++.
	//
	// FOR LIBDAR C++ APPLICATIONS, YOU WOULD RATHER USE THE archive C++ CLASS THAN THESES FOLLOWING WRAPPERS
	//
	//////////


	/// this is a wrapper around the archive constructor known as the "read" constructor

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern archive* open_archive_noexcept(user_interaction & dialog,
					  const path & chem, const std::string & basename,
					  const std::string & extension,
					  crypto_algo crypto, const std::string &pass, U_32 crypto_size,
					  const std::string & input_pipe, const std::string & output_pipe,
					  const std::string & execute, bool info_details,
					  U_16 & exception,
					  std::string & except_msg);


	/// this is a wrapper around the archive constructor known as the "create" constructor

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern archive *create_archive_noexcept(user_interaction & dialog,
					    const path & fs_root,
					    const path & sauv_path,
					    archive *ref_arch,
					    const mask & selection,
					    const mask & subtree,
					    const std::string & filename,
					    const std::string & extension,
					    bool allow_over,
					    bool warn_over,
					    bool info_details,
					    bool pause,
					    bool empty_dir,
					    compression algo,
					    U_I compression_level,
					    const infinint &file_size,
					    const infinint &first_file_size,
					    bool root_ea,
					    bool user_ea,
					    const std::string & execute,
					    crypto_algo crypto,
					    const std::string & pass,
					    U_32 crypto_size,
					    const mask & compr_mask,
					    const infinint & min_compr_size,
					    bool nodump,
					    bool ignore_owner,
					    const infinint & hourshift,
					    bool empty,
					    bool alter_atime,
					    bool same_fs,
					    statistics & ret,
					    U_16 & exception,
					    std::string & except_msg);


	/// this is a wrapper around the archive constructor known as the "isolate" constructor

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern archive *isolate_archive_noexcept(user_interaction & dialog,
					     const path &sauv_path,
					     archive *ref_arch,
					     const std::string & filename,
					     const std::string & extension,
					     bool allow_over,
					     bool warn_over,
					     bool info_details,
					     bool pause,
					     compression algo,
					     U_I compression_level,
					     const infinint &file_size,
					     const infinint &first_file_size,
					     const std::string & execute,
					     crypto_algo crypto,
					     const std::string & pass,
					     U_32 crypto_size,
					     bool empty,
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
					  const mask &selection,
					  const mask &subtree,
					  bool allow_over,
					  bool warn_over,
					  bool info_details,
					  bool detruire,
					  bool only_more_recent,
					  bool restore_ea_root,
					  bool restore_ea_user,
					  bool flat,
					  bool ignore_owner,
					  bool warn_remove_no_match,
					  const infinint & hourshift,
					  bool empty,
					  U_16 & exception,
					  std::string & except_msg);


	/// this is wrapper around the op_listing method

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern void op_listing_noexcept(user_interaction & dialog,
				    archive *ptr,
				    bool info_details,
				    bool tar_format,
				    const mask &selection,
				    bool filter_unsaved,
				    U_16 & exception,
				    std::string & except_msg);


	/// this is wrapper around the op_diff method

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern statistics op_diff_noexcept(user_interaction & dialog,
				       archive *ptr,
				       const path & fs_root,
				       const mask &selection,
				       const mask &subtree,
				       bool info_details,
				       bool check_ea_root,
				       bool check_ea_user,
				       bool ignore_owner,
				       bool alter_atime,
				       U_16 & exception,
				       std::string & except_msg);


	/// this is wrapper around the op_test method

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern statistics op_test_noexcept(user_interaction & dialog,
				       archive *ptr,
				       const mask &selection,
				       const mask &subtree,
				       bool info_details,
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
        /// \return NULL in case of error
    extern char *libdar_str2charptr_noexcept(const std::string & x, U_16 & exception, std::string & except_msg);

	///////////////////////////////////////////////
	// THREAD CANCELLATION ROUTINES              //
	///////////////////////////////////////////////

#if MUTEX_WORKS
	/// thread cancellation activation

	/// ask that any libdar code running in the thread given as argument be cleanly aborted
	/// when the execution will reach the next libdar checkpoint
	/// \param[in] tid is the Thread ID to cancel libdar in
    inline extern void cancel_thread(pthread_t tid) { thread_cancellation::cancel(tid); }

	/// thread consultation of the current cancellation process

	/// \param[out] tid is the tid of the thread in which libdar must abort its operation
	/// \return false if currently no cancellation is under process
    inline extern bool cancel_current(pthread_t & tid) { return thread_cancellation::current_cancel(tid); }

	/// thread cancellation deactivation

	/// abort the thread cancellation initiated by cancel_thread() function
	/// if the libdar has not already aborted its operation.
	/// \return false if the libdar operation in the given thread had already aborted
	/// or if there is no more pending cancellation.
    inline extern bool cancel_clear() { return thread_cancellation::clear_pending_request(); }
#endif

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// ALL OTHER CALLS OF THE LIBDAR LIBRARY ARE NOT PART OF THE API AND WILL NOT BE DOCUMENTED FOR NOW //
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	/// @}

} // end of namespace

#endif



