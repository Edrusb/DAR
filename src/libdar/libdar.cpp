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

#include "../my_config.h"

extern "C"
{
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if LIBLZO2_AVAILABLE
#if HAVE_LZO_LZO1X_H
#include <lzo/lzo1x.h>
#endif
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_GCRYPT_H
#ifndef GCRYPT_NO_DEPRECATED
#define GCRYPT_NO_DEPRECATED
#endif
#include <gcrypt.h>
#endif

#if HAVE_TIME_H
#include <time.h>
#endif

} // end extern "C"

#include <string>
#include <iostream>
#include <new>
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
    static void libdar_init(bool init_libgcrypt_if_not_done); // drives the "libdar_initialized" variable

    static bool libdar_initialized = false; //< static variable modified once during the first get_version call
#ifdef CRYPTO_AVAILABLE
    static bool libdar_initialized_gcrypt = false; //< to record whether libdar did initialized libgcrypt
#endif


    void get_version(U_I & major, U_I & minor, bool init_libgcrypt)
    {
	NLS_SWAP_IN;
        if(&major == NULL)
            throw Elibcall("get_version", gettext("Argument given to \"major\" is a NULL pointer"));
        if(&minor == NULL)
            throw Elibcall("get_version", gettext("Argument given to \"minor\" is a NULL pointer"));
        major = LIBDAR_COMPILE_TIME_MAJOR;
        minor = LIBDAR_COMPILE_TIME_MINOR;
	libdar_init(init_libgcrypt);
	NLS_SWAP_OUT;
    }

    void get_version_noexcept(U_I & major, U_I & minor, U_16 & exception, string & except_msg, bool init_libgcrypt)
    {
	NLS_SWAP_IN;
	WRAPPER_IN
	    get_version(major, minor, init_libgcrypt);
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
    }

    void get_version(U_I & major, U_I & medium, U_I & minor, bool init_libgcrypt)
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
	libdar_init(init_libgcrypt);
	NLS_SWAP_OUT;
    }

    void get_version_noexcept(U_I & major, U_I & medium, U_I & minor, U_16 & exception, std::string & except_msg, bool init_libgcrypt)
    {
	NLS_SWAP_IN;
	WRAPPER_IN
	    get_version(major, medium, minor, init_libgcrypt);
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
    }

    archive* open_archive_noexcept(user_interaction & dialog,
				   const path & chem,
				   const std::string & basename,
				   const std::string & extension,
				   const archive_options_read & options,
				   U_16 & exception,
				   std::string & except_msg)
    {
	archive *ret = NULL;
        NLS_SWAP_IN;
	WRAPPER_IN
	    ret = new (nothrow) archive(dialog, chem,  basename,  extension,
					options);
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
	return ret;
    }

    archive *create_archive_noexcept(user_interaction & dialog,
				     const path & fs_root,
				     const path & sauv_path,
				     const std::string & filename,
				     const std::string & extension,
				     const archive_options_create & options,
				     statistics * progressive_report,
				     U_16 & exception,
				     std::string & except_msg)
    {
	archive *arch_ret = NULL;
	NLS_SWAP_IN;
	WRAPPER_IN
	    arch_ret = new (nothrow) archive(dialog,
					     fs_root,
					     sauv_path,
					     filename,
					     extension,
					     options,
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
				      const archive_options_isolate & options,
				      U_16 & exception,
				      std::string & except_msg)
    {
	archive *ret = NULL;
	NLS_SWAP_IN;
	WRAPPER_IN
	    ret = new (nothrow) archive(dialog,
					sauv_path,
					ref_arch,
					filename,
					extension,
					options);
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
	return ret;
    }

    archive *merge_archive_noexcept(user_interaction & dialog,
				    const path & sauv_path,
				    archive *ref_arch1,
				    const std::string & filename,
				    const std::string & extension,
				    const archive_options_merge & options,
				    statistics * progressive_report,
				    U_16 & exception,
				    std::string & except_msg)
    {
	archive *ret = NULL;
	NLS_SWAP_IN;
	WRAPPER_IN
	    ret = new (nothrow) archive(dialog,
					sauv_path,
					ref_arch1,
					filename,
					extension,
					options,
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

    static void libdar_init(bool init_libgcrypt_if_not_done)
    {
	if(!libdar_initialized)
	{
		// locale for gettext

	    if(string(DAR_LOCALEDIR) != string(""))
		if(bindtextdomain(PACKAGE, DAR_LOCALEDIR) == NULL)
		    throw Erange("", "Cannot open the translated messages directory, native language support will not work");

		// pseudo random generator

	    srand(time(NULL)+getpid()+getppid());


		// initializing mutex

#ifdef MUTEX_WORKS
#ifdef LIBDAR_SPECIAL_ALLOC
	    special_alloc_init_for_thread_safe();
#endif
	    thread_cancellation::init();
#endif

		// building the (later read only) database of user/UID and group/GID of the system

#ifdef __DYNAMIC__
	    user_group_bases::class_init();
#endif

		// initializing LIBLZO2

#if HAVE_LIBLZO2
	    if(lzo_init() != LZO_E_OK)
		throw Erange("libdar_init_thread_safe", gettext("Initialization problem for liblzo2 library"));
#endif

		// initializing libgcrypt

#ifdef CRYPTO_AVAILABLE
	    if(!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P))
	    {
		if(init_libgcrypt_if_not_done)
		{
		    gcry_error_t err;

			// no multi-thread support activated for gcrypt
			// this must be done from the application as stated
			// by the libgcrypt documentation

		    if(!gcry_check_version(MIN_VERSION_GCRYPT))
			throw Erange("libdar_init_libgcrypt", tools_printf(gettext("Too old version for libgcrypt, minimum required version is %s\n"), MIN_VERSION_GCRYPT));

			// initializing default sized secured memory for libgcrypt
		    (void)gcry_control(GCRYCTL_INIT_SECMEM, 65536);
			// if secured memory could not be allocated, further request of secured memory will fail
			// and a warning will show at that time (we cannot send a warning (just failure notice) at that time).

		    err = gcry_control(GCRYCTL_ENABLE_M_GUARD);
		    if(err != GPG_ERR_NO_ERROR)
			throw Erange("libdar_init",tools_printf(gettext("Error while activating libgcrypt's memory guard: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

		    err = gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
		    if(err != GPG_ERR_NO_ERROR)
			throw Erange("libdar_init",tools_printf(gettext("Error while telling libgcrypt that initialization is finished: %s/%s"), gcry_strsource(err),gcry_strerror(err)));

		    libdar_initialized_gcrypt = true;
		}
		else
		    throw Erange("libdar_init_libgcrypt", gettext("libgcrypt not initialized and libdar not allowed to do so"));
	    }
	    else
		if(!gcry_check_version(MIN_VERSION_GCRYPT))
		    throw Erange("libdar_init_libgcrypt", tools_printf(gettext("Too old version for libgcrypt, minimum required version is %s\n"), MIN_VERSION_GCRYPT));
#endif

	    tools_init();

		 // so now libdar is ready for use!

	     libdar_initialized = true;
	 }
     }

     extern void close_and_clean()
     {
#ifdef CRYPTO_AVAILABLE
	 if(libdar_initialized_gcrypt)
	     gcry_control(GCRYCTL_TERM_SECMEM, 0); // by precaution if not already done by libgcrypt itself
#endif
	 tools_end();
    }



    statistics op_extract_noexcept(user_interaction & dialog,
				   archive *ptr,
				   const path &fs_root,
				   const archive_options_extract & options,
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
			      options,
			      progressive_report);
	WRAPPER_OUT(exception, except_msg)
	NLS_SWAP_OUT;
	return ret;
    }


    void op_listing_noexcept(user_interaction & dialog,
			     archive *ptr,
			     const archive_options_listing & options,
			     U_16 & exception,
			     std::string & except_msg)
    {
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == NULL)
		throw Elibcall("op_extract_noexcept", gettext("Invalid NULL argument given to 'ptr'"));
	ptr->op_listing(dialog,
			options);
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
    }

    statistics op_diff_noexcept(user_interaction & dialog,
				archive *ptr,
				const path & fs_root,
				const archive_options_diff & options,
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
			   options,
			   progressive_report);
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
	return ret;
    }


    statistics op_test_noexcept(user_interaction & dialog,
				archive *ptr,
				const archive_options_test & options,
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
			   options,
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
	bool ret = false;
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
