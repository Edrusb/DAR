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
// $Id: libdar.cpp,v 1.70.2.5 2008/02/09 11:14:38 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"

extern "C"
{
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_LIBINTL_H
#include <libintl.h>
#endif

} // end extern "C"

#include <string>
#include <iostream>
#include "libdar.hpp"
#include "erreurs.hpp"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "user_interaction.hpp"
#include "archive.hpp"
#include "nls_swap.hpp"
#ifdef __DYNAMIC__
#include "user_group_bases.hpp"
#endif

#include <list>

using namespace std;

#define WRAPPER_IN try {  //
#define WRAPPER_OUT(code,msg)                      \
                       code = LIBDAR_NOEXCEPT;     \
                    }                              \
                    catch(Ememory & e)             \
                    {                              \
			code = LIBDAR_EMEMORY;     \
                        msg = e.get_message();     \
                    }                              \
                    catch(Ebug & e)                \
                    {                              \
			code = LIBDAR_EBUG;        \
                        msg = e.get_message();     \
                    }                              \
                    catch(Einfinint & e)           \
                    {                              \
			code = LIBDAR_EINFININT;   \
                        msg = e.get_message();     \
                    }                              \
                    catch(Elimitint & e)           \
                    {                              \
			code = LIBDAR_ELIMITINT;   \
                        msg = e.get_message();     \
                    }                              \
                    catch(Erange & e)              \
                    {                              \
			code = LIBDAR_ERANGE;      \
                        msg = e.get_message();     \
                    }                              \
                    catch(Edeci & e)               \
                    {                              \
			code = LIBDAR_EDECI;       \
                        msg = e.get_message();     \
                    }                              \
                    catch(Efeature & e)            \
                    {                              \
			code = LIBDAR_EFEATURE;    \
                        msg = e.get_message();     \
                    }                              \
                    catch(Ehardware & e)           \
                    {                              \
			code = LIBDAR_EHARDWARE;   \
                        msg = e.get_message();     \
                    }                              \
                    catch(Euser_abort & e)         \
                    {                              \
			code = LIBDAR_EUSER_ABORT; \
                        msg = e.get_message();     \
                    }                              \
                    catch(Edata & e)               \
                    {                              \
			code = LIBDAR_EDATA;       \
                        msg = e.get_message();     \
                    }                              \
                    catch(Escript & e)             \
                    {                              \
			code = LIBDAR_ESCRIPT;     \
                        msg = e.get_message();     \
                    }                              \
                    catch(Elibcall & e)            \
                    {                              \
			code = LIBDAR_ELIBCALL;    \
                        msg = e.get_message();     \
                    }                              \
                    catch(Ecompilation & e)        \
                    {                              \
                        code = LIBDAR_ECOMPILATION;\
                        msg = e.get_message();     \
                    }                              \
                    catch(Ethread_cancel & e)      \
                    {                              \
                        code = LIBDAR_THREAD_CANCEL;\
                        msg = e.get_message();     \
                    }                              \
                    catch(Egeneric & e)            \
                    {/*unknown Egeneric exception*/\
			code = LIBDAR_EBUG;        \
                        msg = string(gettext("Caught an unknown Egeneric exception: ")) + e.get_message();  \
                    }                              \
                    catch(...)                     \
                    {/* unknown Egeneric exception*/\
			code = LIBDAR_UNKNOWN;     \
                        msg = gettext("Caught a none libdar exception");  \
                    }                              //


namespace libdar
{
    static void libdar_init_thread_safe();

    void get_version(U_I & major, U_I & minor)
    {
	NLS_SWAP_IN;
        if(&major == NULL)
            throw Elibcall("get_version", gettext("Argument given to \"major\" is a NULL pointer"));
        if(&minor == NULL)
            throw Elibcall("get_version", gettext("Argument given to \"minor\" is a NULL pointer"));
        major = LIBDAR_COMPILE_TIME_MAJOR;
        minor = LIBDAR_COMPILE_TIME_MINOR;
	libdar_init_thread_safe();
	NLS_SWAP_OUT;
    }

    void get_version_noexcept(U_I & major, U_I & minor, U_16 & exception, string & except_msg)
    {
	NLS_SWAP_IN;
	WRAPPER_IN
	    get_version(major, minor);
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
    }


    void get_version(U_I & major, U_I & medium, U_I & minor)
    {
	NLS_SWAP_IN;
        if(&major == NULL)
            throw Elibcall("get_version", gettext("Argument given to \"major\" is a NULL pointer"));
	if(&medium == NULL)
	    throw Elibcall("get_version", gettext("Argument given to \"medium\" is a NULL pointer"));
        if(&minor == NULL)
            throw Elibcall("get_version", gettext("argument given to \"minor\" is a NULL pointer"));

        major = LIBDAR_COMPILE_TIME_MAJOR;
	medium = LIBDAR_COMPILE_TIME_MEDIUM;
        minor = LIBDAR_COMPILE_TIME_MINOR;
	libdar_init_thread_safe();
	NLS_SWAP_OUT;
    }

    void get_version_noexcept(U_I & major, U_I & medium, U_I & minor, U_16 & exception, std::string & except_msg)
    {
	NLS_SWAP_IN;
	WRAPPER_IN
	    get_version(major, medium, minor);
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
    }

    archive* open_archive_noexcept(user_interaction & dialog,
				   const path & chem, const std::string & basename,
				   const std::string & extension,
				   crypto_algo crypto, const std::string &pass, U_32 crypto_size,
				   const std::string & input_pipe, const std::string & output_pipe,
				   const std::string & execute, bool info_details,
				   U_16 & exception,
				   std::string & except_msg)
    {
	archive *ret = NULL;
        NLS_SWAP_IN;
	WRAPPER_IN
	    ret = new archive(dialog, chem,  basename,  extension,
			      crypto, pass, crypto_size,
			      input_pipe, output_pipe,
			      execute, info_details);
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
	return ret;
    }

    archive *create_archive_noexcept(user_interaction & dialog,
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
				     std::string & except_msg)
    {
	archive *arch_ret = NULL;
	NLS_SWAP_IN;
	WRAPPER_IN
	    arch_ret = new archive(dialog,
				   fs_root,
				   sauv_path,
				   ref_arch,
				   selection,
				   subtree,
				   filename,
				   extension,
				   allow_over,
				   warn_over,
				   info_details,
				   pause,
				   empty_dir,
				   algo,
				   compression_level,
				   file_size,
				   first_file_size,
				   ea_mask,
				   execute,
				   crypto,
				   pass,
				   crypto_size,
				   compr_mask,
				   min_compr_size,
				   nodump,
				   what_to_check,
				   hourshift,
				   empty,
				   alter_atime,
				   same_fs,
				   snapshot,
				   cache_directory_tagging,
				   display_skipped,
				   fixed_date,
				   progressive_report);
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
	return arch_ret;
    }

    archive *isolate_archive_noexcept(user_interaction & dialog,
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
				      std::string & except_msg)
    {
	archive *ret = NULL;
	NLS_SWAP_IN;
	WRAPPER_IN
	    ret = new archive(dialog,
			      sauv_path,
			      ref_arch,
			      filename,
			      extension,
			      allow_over,
			      warn_over,
			      info_details,
			      pause,
			      algo,
			      compression_level,
			      file_size,
			      first_file_size,
			      execute,
			      crypto,
			      pass,
			      crypto_size,
			      empty);
	WRAPPER_OUT(exception, except_msg)
	NLS_SWAP_OUT;
	return ret;
    }

    archive *merge_archive_noexcept(user_interaction & dialog,
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
				    std::string & except_msg)
    {
	archive *ret = NULL;
	NLS_SWAP_IN;
	WRAPPER_IN
	    ret = new archive(dialog,
			      sauv_path,
			      ref_arch1,
			      ref_arch2,
			      selection,
			      subtree,
			      filename,
			      extension,
			      allow_over,
			      warn_over,
			      info_details,
			      pause,
			      empty_dir,
			      algo,
			      compression_level,
			      file_size,
			      first_file_size,
			      ea_mask,
			      execute,
			      crypto,
			      pass,
			      crypto_size,
			      compr_mask,
			      min_compr_size,
			      empty,
			      display_skipped,
			      keep_compressed,
			      progressive_report);
	WRAPPER_OUT(exception, except_msg)
	NLS_SWAP_OUT;
	return ret;
    }




    void close_archive_noexcept(archive *ptr,
				U_16 & exception,
				std::string & except_msg)
    {
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == NULL)
		throw Elibcall("close_archive_noexcept", gettext("Invalid NULL pointer given to close_archive"));
	    else
	    {
		delete ptr;
		ptr = NULL;
	    }
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: libdar.cpp,v 1.70.2.5 2008/02/09 11:14:38 edrusb Rel $";
        dummy_call(id);
    }

    char *libdar_str2charptr_noexcept(const std::string & x, U_16 & exception, std::string & except_msg)
    {
        char *ret = NULL;
        NLS_SWAP_IN;
        WRAPPER_IN
	    ret = tools_str2charptr(x);
	WRAPPER_OUT(exception, except_msg)
	NLS_SWAP_OUT;
	return ret;
    }

    void get_compile_time_features(bool & ea, bool & largefile, bool & nodump, bool & special_alloc, U_I & bits,
				   bool & thread_safe,
				   bool & libz, bool & libbz2, bool & libcrypto,
				   bool & new_blowfish)
    {
#ifdef EA_SUPPORT
        ea = true;
#else
        ea = false;
#endif
#if defined( _FILE_OFFSET_BITS ) ||  defined( _LARGE_FILES )
        largefile = true;
#else
        largefile = sizeof(off_t) > 4;
#endif
#ifdef LIBDAR_NODUMP_FEATURE
        nodump = true;
#else
        nodump = false;
#endif
#ifdef LIBDAR_SPECIAL_ALLOC
        special_alloc = true;
#else
        special_alloc = false;
#endif
#ifdef LIBDAR_MODE
        bits = LIBDAR_MODE;
#else
        bits = 0; // infinint
#endif
#if ( defined( MUTEX_WORKS ) || ! defined( LIBDAR_SPECIAL_ALLOC ) ) && ! defined( TEST_MEMORY )
	thread_safe = true;
#else
	thread_safe = false;
#endif
#if LIBZ_AVAILABLE
	libz = true;
#else
	libz = false;
#endif
#if LIBBZ2_AVAILABLE
	libbz2 = true;
#else
	libbz2 = false;
#endif

#if CRYPTO_AVAILABLE
	libcrypto = true;
#if CRYPTO_FULL_BF_AVAILABLE
        new_blowfish = true;
#else
        new_blowfish = false;
#endif
#else
	libcrypto = false;
	new_blowfish = false;
#endif
    }


#ifdef MUTEX_WORKS
    static bool thread_safe_initialized = false;
#endif

    static void libdar_init_thread_safe()
    {
#ifdef MUTEX_WORKS
	if(thread_safe_initialized)
	    return;
#ifdef LIBDAR_SPECIAL_ALLOC
	special_alloc_init_for_thread_safe();
#endif
	thread_safe_initialized = true;
	thread_cancellation::init();
#endif
  	if(string(DAR_LOCALEDIR) != string(""))
	    if(bindtextdomain(PACKAGE, DAR_LOCALEDIR) == NULL)
		throw Erange("", "Cannot open the translated messages directory, native language support will not work");
#ifdef __DYNAMIC__
	user_group_bases::class_init();
#endif
    }

    statistics op_extract_noexcept(user_interaction & dialog,
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
				   std::string & except_msg)
    {
	statistics ret;
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == NULL)
		throw Elibcall("op_extract_noexcept", gettext("Invalid NULL argument given to 'ptr'"));
	ret = ptr->op_extract(dialog,
			      fs_root,
			      selection,
			      subtree,
			      allow_over,
			      warn_over,
			      info_details,
			      detruire,
			      only_more_recent,
			      ea_mask,
			      flat,
			      what_to_check,
			      warn_remove_no_match,
			      hourshift,
			      empty,
			      ea_erase,
			      display_skipped,
			      progressive_report);
	WRAPPER_OUT(exception, except_msg)
	NLS_SWAP_OUT;
	return ret;
    }


    void op_listing_noexcept(user_interaction & dialog,
			     archive *ptr,
			     bool info_details,
			     archive::listformat list_mode,
			     const mask &selection,
			     bool filter_unsaved,
			     U_16 & exception,
			     std::string & except_msg)
    {
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == NULL)
		throw Elibcall("op_extract_noexcept", gettext("Invalid NULL argument given to 'ptr'"));
	ptr->op_listing(dialog,
			info_details,
			list_mode,
			selection,
			filter_unsaved);
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
    }

    statistics op_diff_noexcept(user_interaction & dialog,
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
				std::string & except_msg)
    {
	statistics ret;
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == NULL)
		throw Elibcall("op_extract_noexcept", gettext("Invalid NULL argument given to 'ptr'"));
	ret = ptr->op_diff(dialog,
			   fs_root,
			   selection,
			   subtree,
			   info_details,
			   ea_mask,
			   what_to_check,
			   alter_atime,
			   display_skipped,
			   progressive_report);
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
	return ret;
    }


    statistics op_test_noexcept(user_interaction & dialog,
				archive *ptr,
				const mask &selection,
				const mask &subtree,
				bool info_details,
				bool display_skipped,
				statistics * progressive_report,
				U_16 & exception,
				std::string & except_msg)
    {
	statistics ret;
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == NULL)
		throw Elibcall("op_extract_noexcept", gettext("Invalid NULL argument given to 'ptr'"));
	ret = ptr->op_test(dialog,
			   selection,
			   subtree,
			   info_details,
			   display_skipped,
			   progressive_report);
	WRAPPER_OUT(exception, except_msg)
	NLS_SWAP_OUT;
	return ret;
    }


    bool get_children_of_noexcept(user_interaction & dialog,
				  archive *ptr,
				  const std::string & dir,
				  U_16 & exception,
				  std::string & except_msg)
    {
	bool ret;
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == NULL)
		throw Elibcall("op_extract_noexcept", gettext("Invalid NULL argument given to 'ptr'"));
	ret = ptr->get_children_of(dialog,
				   dir);
	WRAPPER_OUT(exception, except_msg)
	NLS_SWAP_OUT;
	return ret;
    }

} // end of namespace
