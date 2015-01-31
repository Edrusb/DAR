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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file libdar_4_4.hpp
    /// \brief this file provide an alternative namespace to libdar for OLD API
    /// \ingroup OLD_API_4_4
    ///
    /// This is the OLD, deprecated but backward compatible APlication Interfaces (API 4.4.x found in release 2.3.x (with x >= 5) )
    /// If you want to compile a program using an old libdar API against a recent libdar library
    /// you have to include this file in place of libdar.hpp and change the
    /// "namespace libdar" by the "namespace libdar_4_4", then link normally with
    /// libdar library.


#ifndef LIBDAR_4_4_HPP
#define LIBDAR_4_4_HPP

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


#include "erreurs.hpp"

    /// libdar_4_4 namespace encapsulate all symbols of the backward compatible API
namespace libdar_4_4
{

    /// \addtogroup OLD_API_4_4
    /// @{

    typedef libdar::Egeneric Egeneric;
    typedef libdar::Ememory Ememory;
    typedef libdar::Ebug Ebug;
    typedef libdar::Einfinint Einfinint;
    typedef libdar::Elimitint Elimitint;
    typedef libdar::Erange Erange;
    typedef libdar::Edeci Edeci;
    typedef libdar::Efeature Efeature;
    typedef libdar::Ehardware Ehardware;
    typedef libdar::Euser_abort Euser_abort;
    typedef libdar::Edata Edata;
    typedef libdar::Escript Escript;
    typedef libdar::Elibcall Elibcall;
    typedef libdar::Ecompilation Ecompilation;
    typedef libdar::Ethread_cancel Ethread_cancel;
}

#include "compressor.hpp"
namespace libdar_4_4
{
    typedef libdar::compression compression;

    const compression none = libdar::none;
    const compression zip = libdar::gzip;
    const compression gzip = libdar::gzip;
    const compression bzip2 = libdar::bzip2;

    inline compression char2compression(char a) { return libdar::char2compression(a); }
    inline char compression2char(compression c) { return libdar::compression2char(c); }
    inline std::string compression2string(compression c) { return libdar::compression2string(c); }
    inline compression string2compression(const std::string & a) { return libdar::string2compression(a); }

    typedef libdar::compressor compressor;
}

#include "path.hpp"
namespace libdar_4_4
{
    typedef libdar::path path;
}

#include "mask.hpp"
namespace libdar_4_4
{
    typedef libdar::mask mask;
    typedef libdar::bool_mask bool_mask;
    typedef libdar::simple_mask simple_mask;
    typedef libdar::bool_mask bool_mask;
    typedef libdar::regular_mask regular_mask;
    typedef libdar::not_mask not_mask;
    typedef libdar::et_mask et_mask;
    typedef libdar::ou_mask ou_mask;
    typedef libdar::simple_path_mask simple_path_mask;
    typedef libdar::same_path_mask same_path_mask;
    typedef libdar::exclude_dir_mask exclude_dir_mask;
}

#include "integers.hpp" // OK
namespace libdar_4_4
{
    typedef libdar::U_8 U_8;
    typedef libdar::U_16 U_16;
    typedef libdar::U_32 U_32;
    typedef libdar::U_64 U_64;
    typedef libdar::U_I U_I;
    typedef libdar::S_8 S_8;
    typedef libdar::S_16 S_16;
    typedef libdar::S_32 S_32;
    typedef libdar::S_64 S_64;
    typedef libdar::S_I S_I;
}


#include "infinint.hpp"
namespace libdar_4_4
{
    typedef libdar::infinint infinint;
}

#include "statistics.hpp"
namespace libdar_4_4
{
    typedef libdar::statistics statistics;
}

#include "user_interaction.hpp" // OK
namespace libdar_4_4
{
	/// wrapper class for user_interaction

    class user_interaction : public libdar::user_interaction
    {
    public:

	virtual void dar_manager_show_version(U_I number,
                                              const std::string & data_date,
                                              const std::string & ea_date);
    protected:
	libdar::secu_string get_secu_string(const std::string & message, bool echo)
	{
		// this is a backward compatibile API, yes, we loose the secured storage feature for keys
	    std::string tmp = get_string(message, echo);
	    libdar::secu_string ret = libdar::secu_string(tmp.c_str(), tmp.size());

	    return ret;
	};
    private:
	void dar_manager_show_version(U_I number,
				      const std::string & data_date,
				      const std::string & data_presence,
				      const std::string & ea_date,
				      const std::string & ea_presence)
	{
	    dar_manager_show_version(number, data_date, ea_date);
	}
    };

    typedef libdar::user_interaction_callback user_interaction_callback;
}

#include "deci.hpp"
namespace libdar_4_4
{
    typedef libdar::deci deci;
}

#include "archive_version.hpp"
namespace libdar_4_4
{
    typedef libdar::archive_version dar_version;
}

#include "crypto.hpp"
namespace libdar_4_4
{
    typedef libdar::crypto_algo crypto_algo;

    const crypto_algo crypto_none = libdar::crypto_none;
    const crypto_algo crypto_scrambling = libdar::crypto_scrambling;
    const crypto_algo crypto_blowfish = libdar::crypto_blowfish;
    const crypto_algo crypto_blowfish_weak = libdar::crypto_blowfish;

    libdar::secu_string string2secu_string(const std::string & st);

    inline void crypto_split_algo_pass(const std::string & all, crypto_algo & algo, std::string & pass)
    {
	libdar::secu_string sall = string2secu_string(all);
	libdar::secu_string spass;
	libdar::crypto_split_algo_pass(sall, algo, spass);
	pass = spass.c_str();
    }

	/// wrapper class for blowfish

    class blowfish : public libdar::crypto_sym
    {
	blowfish(user_interaction & dialog,
		 U_32 block_size,
		 const std::string & password,
		 generic_file & encrypted_side,
		 const dar_version & reading_ver,
 		 bool weak_mode)
	    : libdar::crypto_sym(block_size, string2secu_string(password), encrypted_side, false, reading_ver, libdar::crypto_blowfish) {};
    };
}

#include "catalogue.hpp"
namespace libdar_4_4
{
    typedef libdar::inode inode;
}

#include "archive.hpp"
namespace libdar_4_4
{


	/// wrapper class for archive

    class archive : public libdar::archive
    {
    public:
	    /// convertion from libdar::archive * to libdar_4_4::archive *

	    /// \note this is possible because the libdar_4_4::archive class
	    /// does not own any additional field compared to libdar::archive
	    /// \note this piggy convertion is used in several places but it
	    /// implemented only in this method.
	    /// \note The correct implementation would have to create a libdar_4_4::archive
	    /// constructor having libdar::archive as argument, and relying on the
	    /// copy constructor of libdar::archive to create the new object. However
	    /// this copy constructor in libdar::archive does not exist for some
	    /// good reasons, and this solution would have been perfectly inefficient
	static archive *piggy_convert(libdar::archive * ref);

	    /// defines the way archive listing is done:
	typedef libdar::archive_options_listing::listformat listformat;
	static const listformat normal = libdar::archive_options_listing::normal;
	static const listformat tree = libdar::archive_options_listing::tree;
	static const listformat xml = libdar::archive_options_listing::xml;

	archive(user_interaction & dialog,
                const path & chem,
		const std::string & basename,
		const std::string & extension,
                crypto_algo crypto,
                const std::string &pass,
                U_32 crypto_size,
                const std::string & input_pipe,
                const std::string & output_pipe,
                const std::string & execute,
                bool info_details);  // read constructor

	archive(user_interaction & dialog,
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
                const infinint & pause,
                bool empty_dir,
                compression algo,
                U_I compression_level,
                const infinint &file_size,
                const infinint &first_file_size,
                const mask & ea_mask,
                const std::string & execute,
                crypto_algo crypto,
                const std::string & pass,
                U_32 crypto_size,
                const mask & compr_mask,
                const infinint & min_compr_size,
		bool nodump,
		inode::comparison_fields what_to_check,
		const infinint & hourshift,
		bool empty,
		bool alter_atime,
		bool same_fs,
		bool snapshot,
		bool cache_directory_tagging,
		bool display_skipped,
		const infinint & fixed_date,
		statistics * progressive_report); // create constructor

	archive(user_interaction & dialog,
                const path &sauv_path,
                archive *ref_arch,
                const std::string & filename,
                const std::string & extension,
                bool allow_over,
                bool warn_over,
                bool info_details,
                const infinint & pause,
                compression algo,
                U_I compression_level,
                const infinint &file_size,
                const infinint &first_file_size,
                const std::string & execute,
                crypto_algo crypto,
                const std::string & pass,
                U_32 crypto_size,
                bool empty);  // isolate constructor


	archive(user_interaction & dialog,
                const path & sauv_path,
                archive *ref_arch1,
                archive *ref_arch2,
                const mask & selection,
                const mask & subtree,
                const std::string & filename,
                const std::string & extension,
                bool allow_over,
                bool warn_over,
                bool info_details,
                const infinint & pause,
                bool empty_dir,
                compression algo,
                U_I compression_level,
                const infinint & file_size,
                const infinint & first_file_size,
                const mask & ea_mask,
                const std::string & execute,
                crypto_algo crypto,
                const std::string & pass,
                U_32 crypto_size,
                const mask & compr_mask,
                const infinint & min_compr_size,
                bool empty,
                bool display_skipped,
                bool keep_compressed,
                statistics * progressive_report); // merging constructor

	statistics op_extract(user_interaction & dialog,
                              const path &fs_root,
                              const mask &selection,
                              const mask &subtree,
                              bool allow_over,
                              bool warn_over,
                              bool info_details,
                              bool detruire,
                              bool only_more_recent,
                              const mask & ea_mask,
                              bool flat,
                              inode::comparison_fields what_to_check,
                              bool warn_remove_no_match,
                              const infinint & hourshift,
                              bool empty,
                              bool ea_erase,
                              bool display_skipped,
                              statistics *progressive_report);

	void op_listing(user_interaction & dialog,
                        bool info_details,
                        archive::listformat list_mode,
                        const mask &selection,
                        bool filter_unsaved);

	statistics op_diff(user_interaction & dialog,
                           const path & fs_root,
                           const mask &selection,
                           const mask &subtree,
                           bool info_details,
                           const mask & ea_mask,
                           inode::comparison_fields what_to_check,
                           bool alter_atime,
                           bool display_skipped,
                           statistics * progressive_report,
			   const infinint & hourshift = 0);

	statistics op_test(user_interaction & dialog,
                           const mask &selection,
                           const mask &subtree,
                           bool info_details,
                           bool display_skipped,
                           statistics * progressive_report);
    };
}


#include "thread_cancellation.hpp"
namespace libdar_4_4
{
    typedef libdar::thread_cancellation thread_cancellation;
}

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

namespace libdar_4_4
{

	///  libdar Major version defined at compilation time
    const U_I LIBDAR_COMPILE_TIME_MAJOR = 4;
	///  libdar Medium version defined at compilation time
	// the last libdar version of releases 2.3.x is 4.5.0, so we skip by one the medium to make the difference,
	// but we keep the major to 4 (to avoid alarming external programs expecting the libdar API version 4)
    const U_I LIBDAR_COMPILE_TIME_MEDIUM = 6;
	///  libdar Minor version defined at compilation time
    const U_I LIBDAR_COMPILE_TIME_MINOR = 14;

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

	/// depreacted old get_version function
    void get_version(U_I & major, U_I & minor, bool init_libgcrypt = true);

	/// depreacted old get_version function
    void get_version_noexcept(U_I & major, U_I & minor, U_16 & exception, std::string & except_msg, bool init_libgcrypt = true);

	/// return the libdar version, and make libdar initialization (may throw Exceptions)

	/// It is mandatory to call this function (or another one of the get_version* family)
	/// \param[out] major the major number of the version
	/// \param[out] medium the medium number of the version
	/// \param[out] minor the minor number of the version
	/// \param[in] init_libgcrypt whether to initialize libgcrypt if not already done (not used if libcrypt is not linked with libdar)
	/// \note the calling application must match that the major function
	/// is the same as the libdar used at compilation time. See API tutorial for a
	/// sample code.
    void get_version(U_I & major, U_I & medium, U_I & minor, bool init_libgcrypt = true);

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
    void get_version_noexcept(U_I & major, U_I & medium, U_I & minor, U_16 & exception, std::string & except_msg, bool init_libgcrypt = true);


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
	/// \param[out] new_blowfish whether new blowfish implementation is available
	/// \note This function does never throw exceptions, so there is no
	/// get_compile_time_features_noexcept() function available.
    void get_compile_time_features(bool & ea, bool & largefile, bool & nodump, bool & special_alloc, U_I & bits,
				   bool & thread_safe,
				   bool & libz, bool & libbz2, bool & libcrypto,
				   bool & new_blowfish);

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
    archive* open_archive_noexcept(user_interaction & dialog,
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
					    const infinint & pause,
					    bool empty_dir,
					    compression algo,
					    U_I compression_level,
					    const infinint &file_size,
					    const infinint &first_file_size,
					    const mask & ea_mask,
					    const std::string & execute,
					    crypto_algo crypto,
					    const std::string & pass,
					    U_32 crypto_size,
					    const mask & compr_mask,
					    const infinint & min_compr_size,
					    bool nodump,
					    inode::comparison_fields what_to_check,
					    const infinint & hourshift,
					    bool empty,
					    bool alter_atime,
					    bool same_fs,
					    bool snapshot,
					    bool cache_directory_tagging,
					    bool display_skipped,
					    const infinint & fixed_date,
					    statistics * progressive_report,
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
					     const infinint & pause,
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

	/// this is a wrapper around the archive constructor known as the "merging" constructor

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern archive *merge_archive_noexcept(user_interaction & dialog,
					   const path & sauv_path,
					   archive *ref_arch1,
					   archive *ref_arch2,
					   const mask & selection,
					   const mask & subtree,
					   const std::string & filename,
					   const std::string & extension,
					   bool allow_over,
					   bool warn_over,
					   bool info_details,
					   const infinint & pause,
					   bool empty_dir,
					   compression algo,
					   U_I compression_level,
					   const infinint & file_size,
					   const infinint & first_file_size,
					   const mask & ea_mask,
					   const std::string & execute,
					   crypto_algo crypto,
					   const std::string & pass,
					   U_32 crypto_size,
					   const mask & compr_mask,
					   const infinint & min_compr_size,
					   bool empty,
					   bool display_skipped,
					   bool keep_compressed,
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
					  const mask &selection,
					  const mask &subtree,
					  bool allow_over,
					  bool warn_over,
					  bool info_details,
					  bool detruire,
					  bool only_more_recent,
					  const mask & ea_mask,
					  bool flat,
					  inode::comparison_fields what_to_check,
					  bool warn_remove_no_match,
					  const infinint & hourshift,
					  bool empty,
					  bool ea_erase,
					  bool display_skipped,
					  statistics * progressive_report,
					  U_16 & exception,
					  std::string & except_msg);


	/// this is wrapper around the op_listing method

	/// check the archive class for details
	/// for an explaination of the two extra arguments exception and except_msg check
	/// the get_version_noexcept function
    extern void op_listing_noexcept(user_interaction & dialog,
				    archive *ptr,
				    bool info_details,
				    archive::listformat list_mode,
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
				       const mask & ea_mask,
				       inode::comparison_fields what_to_check,
				       bool alter_atime,
				       bool display_skipped,
				       statistics * progressive_report,
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
				       bool display_skipped,
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
        /// \return NULL in case of error
    extern char *libdar_str2charptr_noexcept(const std::string & x, U_16 & exception, std::string & except_msg);
}

#include "criterium.hpp"

namespace libdar_4_4
{

	/// Defines an overwriting policy based on pre 2.4.0 options

	/// \param[in] allow_over whether overwriting is allowed or forbidden at all
	/// \param[in] detruire whether "restoration" of file recorded as deleted is allowed (in other words, whether file removal at restoration time is allowed)
	/// \param[in] more_recent whether to only restore more recent files
	/// \param[in] hourshift the hourshift to use to compare "more recent" dates
	/// \param[in] ea_erase whether to erase existing EA before restoring them from archive
	/// \param[out] overwrite that will be set to a newly allocated object to be deleted by the caller at a later time
    extern void tools_4_4_build_compatible_overwriting_policy(bool allow_over,
							      bool detruire,
							      bool more_recent,
							      const libdar::infinint & hourshift,
							      bool ea_erase,
							      const libdar::crit_action * & overwrite);


	///////////////////////////////////////////////
	// THREAD CANCELLATION ROUTINES              //
	///////////////////////////////////////////////

#if MUTEX_WORKS
	/// thread cancellation activation

	/// ask that any libdar code running in the thread given as argument be cleanly aborted
	/// when the execution will reach the next libdar checkpoint
	/// \param[in] tid is the Thread ID to cancel libdar in
	/// \param[in] immediate set to false, libdar will abort nicely terminating the current work and producing a usable archive for example
	/// \param[in] flag is an arbitrary value that is passed through libdar
    inline void cancel_thread(pthread_t tid, bool immediate = true, U_64 flag = 0) { thread_cancellation::cancel(tid, immediate, flag); }

	/// consultation of the cancellation status of a given thread

	/// \param[in] tid is the tid of the thread to get status about
	/// \return false if no cancellation has been requested for the given thread
    inline bool cancel_status(pthread_t tid) { return thread_cancellation::cancel_status(tid); }

	/// thread cancellation deactivation

	/// abort the thread cancellation for the given thread
	/// \return false if no thread cancellation was under process for that thread
	/// or if there is no more pending cancellation (thread has already been canceled).
    inline bool cancel_clear(pthread_t tid) { return thread_cancellation::clear_pending_request(tid); }
#endif


	/// @}

} // end of namespace


#endif
