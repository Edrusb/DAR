/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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

    /// \file libdar5.hpp
    /// \brief backward compatibility to libdar API 5
    /// \ingroup API5


#ifndef LIBDAR5_HPP
#define LIBDAR5_HPP

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
#include "mask_list.hpp"
#include "integers.hpp"
#include "infinint.hpp"
#include "statistics.hpp"
#include "user_interaction.hpp"
#include "user_interaction_callback5.hpp"
#include "deci.hpp"
#include "archive5.hpp"
#include "crypto.hpp"
#include "thread_cancellation.hpp"
#include "compile_time_features.hpp"
#include "capabilities.hpp"
#include "entrepot_libcurl5.hpp"
#include "fichier_local.hpp"
#include "entrepot_local.hpp"
#include "data_tree.hpp"
#include "database5.hpp"
#include "tuyau.hpp"
#include "archive_aux.hpp"
#include "tools.hpp"

    /// \addtogroup API5
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

    /// libdar5 namespace encapsulate all libdar symbols
namespace libdar5
{
	/// \addtogroup API5
	/// @{

	// from integers.hpp
    using libdar::U_8;
    using libdar::U_16;
    using libdar::U_32;
    using libdar::U_64;
    using libdar::U_I;
    using libdar::S_8;
    using libdar::S_16;
    using libdar::S_32;
    using libdar::S_64;
    using libdar::S_I;

	// from infinint.hpp
    using libdar::infinint;

	// from erreurs.hpp
    using libdar::Egeneric;
    using libdar::Ememory;
    using libdar::Esecu_memory;
    using libdar::Ebug;
    using libdar::Einfinint;
    using libdar::Elimitint;
    using libdar::Edeci;
    using libdar::Erange;
    using libdar::Efeature;
    using libdar::Ehardware;
    using libdar::Edata;
    using libdar::Euser_abort;
    using libdar::Escript;
    using libdar::Elibcall;
    using libdar::Ecompilation;
    using libdar::Ethread_cancel;
    using libdar::Esystem;

	// from secu_string.hpp
    using libdar::secu_string;

	// from path.hpp
    using libdar::path;

	// from compressor.hpp
    using libdar::compression;
    constexpr compression none = compression::none;
    constexpr compression gzip = compression::gzip;
    constexpr compression bzip2 = compression::bzip2;
    constexpr compression lzo = compression::lzo;
    constexpr compression xz = compression::xz;
    constexpr compression lzo1x_1_15 = compression::lzo1x_1_15;
    constexpr compression lzo1x_1 = compression::lzo1x_1;

    inline compression char2compression(char a) { return libdar::char2compression(a); }
    inline char compression2char(compression c) { return libdar::compression2char(c); }
    inline std::string compression2string(compression c) { return libdar::compression2string(c); }
    inline compression string2compression(const std::string & a) { return libdar::string2compression(a); }

	// from crypto.hpp
    using libdar::crypto_algo;
    constexpr crypto_algo crypto_none = crypto_algo::none;
    constexpr crypto_algo crypto_scrambling = crypto_algo::scrambling;
    constexpr crypto_algo crypto_blowfish = crypto_algo::blowfish;
    constexpr crypto_algo crypto_aes256 = crypto_algo::aes256;
    constexpr crypto_algo crypto_twofish256 = crypto_algo::twofish256;
    constexpr crypto_algo crypto_serpent256 = crypto_algo::serpent256;
    constexpr crypto_algo crypto_camellia256 = crypto_algo::camellia256;

    using libdar::signator;

	// from statistics.hpp
    using libdar::statistics;

	// from list_entry.hpp
    using libdar::list_entry;

	// from entree_stats.hpp
    using libdar::entree_stats;

	// from archive.hpp
	// using libdar::archive;

	// from capabilities.hpp
    using libdar::capa_status;
    constexpr capa_status capa_set = libdar::capa_status::capa_set;
    constexpr capa_status capa_clear = libdar::capa_status::capa_clear;
    constexpr capa_status capa_unknown = libdar::capa_status::capa_unknown;

	// from mask.hpp
    using libdar::mask;
    using libdar::bool_mask;
    using libdar::simple_mask;
    using libdar::regular_mask;
    using libdar::not_mask;
    using libdar::et_mask;
    using libdar::ou_mask;
    using libdar::simple_path_mask;
    using libdar::same_path_mask;
    using libdar::exclude_dir_mask;

	// from deci.hpp
    using libdar::deci;

	// from thread_cancellation
    using libdar::thread_cancellation;

	// from data_tree.hpp
    using libdar::archive_num;

	// from criterium.hpp
    using libdar::criterium;
    using libdar::crit_in_place_is_inode;
    using libdar::crit_in_place_is_dir;
    using libdar::crit_in_place_is_file;
    using libdar::crit_in_place_is_hardlinked_inode;
    using libdar::crit_in_place_is_new_hardlinked_inode;
    using libdar::crit_in_place_data_more_recent;
    using libdar::crit_in_place_data_more_recent_or_equal_to;
    using libdar::crit_in_place_data_bigger;
    using libdar::crit_in_place_data_saved;
    using libdar::crit_in_place_data_dirty;
    using libdar::crit_in_place_data_sparse;
    using libdar::crit_in_place_has_delta_sig;
    using libdar::crit_in_place_EA_present;
    using libdar::crit_in_place_EA_more_recent;
    using libdar::crit_in_place_EA_more_recent_or_equal_to;
    using libdar::crit_in_place_more_EA;
    using libdar::crit_in_place_EA_bigger;
    using libdar::crit_in_place_EA_saved;
    using libdar::crit_same_type;
    using libdar::crit_not;
    using libdar::crit_and;
    using libdar::crit_or;
    using libdar::crit_invert;

	// from crit_action.hpp
    using libdar::over_action_data;
    constexpr over_action_data data_preserve = libdar::over_action_data::data_preserve;
    constexpr over_action_data data_overwrite = libdar::over_action_data::data_overwrite;
    constexpr over_action_data data_preserve_mark_already_saved = libdar::over_action_data::data_preserve_mark_already_saved;
    constexpr over_action_data data_overwrite_mark_already_saved = libdar::over_action_data::data_overwrite_mark_already_saved;
    constexpr over_action_data data_remove = libdar::over_action_data::data_remove;
    constexpr over_action_data data_undefined = libdar::over_action_data::data_undefined;
    constexpr over_action_data data_ask = libdar::over_action_data::data_ask;

    using libdar::over_action_ea;
    constexpr over_action_ea EA_preserve = libdar::over_action_ea::EA_preserve;
    constexpr over_action_ea EA_overwrite = libdar::over_action_ea::EA_overwrite;
    constexpr over_action_ea EA_clear = libdar::over_action_ea::EA_clear;
    constexpr over_action_ea EA_preserve_mark_already_saved = libdar::over_action_ea::EA_preserve_mark_already_saved;
    constexpr over_action_ea EA_overwrite_mark_already_saved = libdar::over_action_ea::EA_overwrite_mark_already_saved;
    constexpr over_action_ea EA_merge_preserve = libdar::over_action_ea::EA_merge_preserve;
    constexpr over_action_ea EA_merge_overwrite = libdar::over_action_ea::EA_merge_overwrite;
    constexpr over_action_ea EA_undefined = libdar::over_action_ea::EA_undefined;
    constexpr over_action_ea EA_ask = libdar::over_action_ea::EA_ask;

    using libdar::crit_action;
    using libdar::crit_constant_action;
    using libdar::testing;
    using libdar::crit_chain;

	// from mask_list.hpp
    using libdar::mask_list;

	// from hash_fichier.hpp
    using libdar::hash_algo;
    constexpr hash_algo hash_none = hash_algo::none;
    constexpr hash_algo hash_md5 = hash_algo::md5;
    constexpr hash_algo hash_sha1 = hash_algo::sha1;
    constexpr hash_algo hash_sha512= hash_algo::sha512;

	// from fsa_family.hpp
    using libdar::fsa_family;
    constexpr fsa_family fsaf_hfs_plus = fsa_family::fsaf_hfs_plus;
    constexpr fsa_family fsaf_linux_extX = fsa_family::fsaf_linux_extX;

    using libdar::fsa_nature;
    constexpr fsa_nature fsan_unset = fsa_nature::fsan_unset;
    constexpr fsa_nature fsan_creation_date = fsa_nature::fsan_creation_date;
    constexpr fsa_nature fsan_append_only = fsa_nature::fsan_append_only;
    constexpr fsa_nature fsan_compressed = fsa_nature::fsan_compressed;
    constexpr fsa_nature fsan_no_dump = fsa_nature::fsan_no_dump;
    constexpr fsa_nature fsan_immutable = fsa_nature::fsan_immutable;
    constexpr fsa_nature fsan_data_journaling = fsa_nature::fsan_data_journaling;
    constexpr fsa_nature fsan_secure_deletion = fsa_nature::fsan_secure_deletion;
    constexpr fsa_nature fsan_no_tail_merging = fsa_nature::fsan_no_tail_merging;
    constexpr fsa_nature fsan_undeletable = fsa_nature::fsan_undeletable;
    constexpr fsa_nature fsan_noatime_update = fsa_nature::fsan_noatime_update;
    constexpr fsa_nature fsan_synchronous_directory = fsa_nature::fsan_synchronous_directory;
    constexpr fsa_nature fsan_synchronous_udpdate = fsa_nature::fsan_synchronous_update;
    constexpr fsa_nature fsan_top_of_dir_hierarchy = fsa_nature::fsan_top_of_dir_hierarchy;

    using libdar::fsa_scope;
    inline fsa_scope all_fsa_families() { return libdar::all_fsa_families(); }

	// from generic_file.hpp
    using libdar::generic_file;
    using libdar::gf_mode;
    constexpr gf_mode gf_read_only = gf_mode::gf_read_only;
    constexpr gf_mode gf_write_only = gf_mode::gf_write_only;
    constexpr gf_mode gf_read_write = gf_mode::gf_read_write;

	// from tools.hpp
    using libdar::tools_printf;
    using libdar::tools_getcwd;
    using libdar::tools_get_extended_size;
    using libdar::tools_my_atoi;
    using libdar::tools_octal2int;

	// from compile_time_features.hpp
    namespace compile_time
    {
	using namespace libdar::compile_time;
    }

	// from fichier_local.hpp
    using libdar::fichier_local;

	// from mycurl_protocol.hpp
    using libdar::mycurl_protocol;
    constexpr mycurl_protocol proto_ftp = mycurl_protocol::proto_ftp;
    constexpr mycurl_protocol proto_sftp = mycurl_protocol::proto_sftp;

    inline mycurl_protocol string_to_mycurl_protocol(const std::string & arg) { return libdar::string_to_mycurl_protocol(arg); }

	// from memory_file.hpp
    using libdar::memory_file;

	// from entrepot_local
    using libdar::entrepot_local;

	// from datetime
    using libdar::datetime;

	// from tuyau
    using libdar::tuyau;

	// from archive_aux.hpp
    using libdar::modified_data_detection;
    using libdar::comparison_fields;
    constexpr comparison_fields cf_all = comparison_fields::all;
    constexpr comparison_fields cf_ignore_owner = comparison_fields::ignore_owner;
    constexpr comparison_fields cf_mtime = comparison_fields::mtime;
    constexpr comparison_fields cf_inode_type = comparison_fields::inode_type;

	///  libdar Major version defined at compilation time
    const U_I LIBDAR_COMPILE_TIME_MAJOR = 5;
	///  libdar Medium version defined at compilation time
    const U_I LIBDAR_COMPILE_TIME_MEDIUM = 100;
	///  libdar Minor version defined at compilation time
    const U_I LIBDAR_COMPILE_TIME_MINOR = 7;

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
    extern void cancel_thread(pthread_t tid, bool immediate = true, U_64 flag = 0);

	/// consultation of the cancellation status of a given thread

	/// \param[in] tid is the tid of the thread to get status about
	/// \return false if no cancellation has been requested for the given thread
    extern bool cancel_status(pthread_t tid);

	/// thread cancellation deactivation

	/// abort the thread cancellation for the given thread
	/// \return false if no thread cancellation was under process for that thread
	/// or if there is no more pending cancellation (thread has already been canceled).
    extern bool cancel_clear(pthread_t tid);
#endif


	/// @}

} // end of namespace

#endif
