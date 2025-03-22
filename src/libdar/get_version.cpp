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

#if LIBCURL_AVAILABLE
#ifdef HAVE_CURL_CURL_H
#include <curl/curl.h>
#endif
#endif

#if HAVE_TIME_H
#include <time.h>
#endif

#if HAVE_GPGME_H
#include <gpgme.h>
#endif
} // end extern "C"

#include <string>
#include <iostream>
#include "get_version.hpp"
#include "erreurs.hpp"
#include "nls_swap.hpp"
#include "tools.hpp"
#include "thread_cancellation.hpp"
#include "mycurl_easyhandle_node.hpp"

#ifdef LIBTHREADAR_AVAILABLE
#include "parallel_tronconneuse.hpp"
#endif

using namespace std;

namespace libdar
{
    static void libdar_init(bool init_libgcrypt_if_not_done, // drives the "libdar_initialized" variable
			    bool init_gpgme); // whether to initialize gpgme

    static bool libdar_initialized = false; //< static variable modified once during the first get_version call
#ifdef CRYPTO_AVAILABLE
    static bool libdar_initialized_gcrypt = false; //< to record whether libdar did initialized libgcrypt
#endif


    void get_version(U_I & major, U_I & minor, bool init_libgcrypt)
    {
	NLS_SWAP_IN;
        major = LIBDAR_COMPILE_TIME_MAJOR;
        minor = LIBDAR_COMPILE_TIME_MINOR;
	libdar_init(init_libgcrypt, true);
	NLS_SWAP_OUT;
    }

    void get_version(bool init_libgcrypt)
    {
	U_I maj, min, med;
	get_version(maj, min, med, init_libgcrypt);
    }

    void get_version(U_I & major, U_I & medium, U_I & minor, bool init_libgcrypt)
    {
	NLS_SWAP_IN;

        major = LIBDAR_COMPILE_TIME_MAJOR;
	medium = LIBDAR_COMPILE_TIME_MEDIUM;
        minor = LIBDAR_COMPILE_TIME_MINOR;
	libdar_init(init_libgcrypt, true);
	NLS_SWAP_OUT;
    }

    void get_version(U_I & major, U_I & medium, U_I & minor, bool init_libgcrypt, bool init_gpgme)
    {
	NLS_SWAP_IN;

        major = LIBDAR_COMPILE_TIME_MAJOR;
	medium = LIBDAR_COMPILE_TIME_MEDIUM;
        minor = LIBDAR_COMPILE_TIME_MINOR;
	libdar_init(init_libgcrypt, init_gpgme);
	NLS_SWAP_OUT;
    }

    static void libdar_init(bool init_libgcrypt_if_not_done, bool init_gpgme)
    {
	if(!libdar_initialized)
	{
		// locale for gettext

	    if(string(DAR_LOCALEDIR) != string(""))
		if(bindtextdomain(PACKAGE, DAR_LOCALEDIR) == nullptr)
		    throw Erange("", "Cannot open the translated messages directory, native language support will not work");

		// pseudo random generator

	    srand(::time(nullptr)+getpid()+getppid());

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
			throw Erange("libdar_init_libgcrypt", tools_printf(gettext("Too old version for libgcrypt, minimum required version is %s"), MIN_VERSION_GCRYPT));

			// initializing default sized secured memory for libgcrypt
		    (void)gcry_control(GCRYCTL_INIT_SECMEM, 262144);
			// if secured memory could not be allocated, further request of secured memory will fail
			// and a warning will show at that time (we cannot send a warning (just failure notice) at that time).

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
		    throw Erange("libdar_init_libgcrypt", tools_printf(gettext("Too old version for libgcrypt, minimum required version is %s"), MIN_VERSION_GCRYPT));
#endif

	    // initializing gpgme

#if GPGME_SUPPORT
	    if(init_gpgme)
	    {
		if(gpgme_check_version(GPGME_MIN_VERSION) == nullptr)
		{
		    string tmp = "GPGME_SUPPORT";
		    throw Erange("libdar_init_gpgme", tools_printf(gettext("GPGME version requirement is not satisfied, requires version > %s"), tmp.c_str()));
		}

		if(gpgme_err_code(gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP)) != GPG_ERR_NO_ERROR)
		    throw Erange("libdar_init_gpgme", tools_printf(gettext("GPGME engine not available: %s"), gpgme_get_protocol_name(GPGME_PROTOCOL_OpenPGP)));
	    }
#endif
	    // initializing libcurl

#if LIBCURL_AVAILABLE
	    CURLcode curlret = curl_global_init(CURL_GLOBAL_ALL);
	    if(curlret != 0)
	    {
		const char *msg = curl_easy_strerror(curlret);
		throw Erange("libdar_init_libcurl", tools_printf(gettext("libcurl initialization failed: %s"), msg));
	    }
	    const curl_version_info_data *cvers = curl_version_info(CURLVERSION_FOURTH);
	    if(cvers->age < CURLVERSION_FOURTH)
		throw Erange("libdar_init_libcurl", tools_printf(gettext("libcurl initialization failed: %s"), "libcurl version not available"));
	    if(cvers->version_num < 0x072600)
		throw Erange("libdar_init_libcurl", tools_printf(gettext("libcurl initialization failed: %s"), "libcurl version is too old"));

		// now that libgcrypt is eventually initialized (as well as libcurl, but it does not matter here)
		// we can initialize the default values for mycurl_easyhandle_node::defaults static field

	    mycurl_easyhandle_node::init_defaults();
#endif

		// checking libthreadar version

#if LIBTHREADAR_AVAILABLE
	    unsigned int maj, med, min;
	    libthreadar::get_version(maj, med, min);

	    if((signed int)maj != atoi(EXPECTED_MAJ_VERSION_THREADAR)
	       || (signed int)med < atoi(MIN_MED_VERSION_THREADAR)
	       || ((signed int)med == atoi(MIN_MED_VERSION_THREADAR)
		   && (signed int)min < atoi(MIN_MIN_VERSION_THREADAR)))
		throw Erange("libdar_init_libthreadar",
			     tools_printf(gettext("libthreader version %d.%d.%d is too old, need version %s.%s.%s or more recent"),
					  maj,med,min,
					  EXPECTED_MAJ_VERSION_THREADAR,
					  MIN_MED_VERSION_THREADAR,
				 	  MIN_MIN_VERSION_THREADAR));
#endif

	    tools_init();

		// so now libdar is ready for use!

	    libdar_initialized = true;
	}
    }

    extern void close_and_clean()
    {
#ifdef LIBCURL_AVAILABLE
	    // mycurl_easyhandle_node::efaults static field, may contain secu_string that rely on libgcrypt secured memory
	mycurl_easyhandle_node::release_defaults();
	    // it must thus be released before released libgcrypt below
#endif
#ifdef CRYPTO_AVAILABLE
	if(libdar_initialized_gcrypt)
	    gcry_control(GCRYCTL_TERM_SECMEM, 0); // by precaution if not already done by libgcrypt itself
#endif
#if LIBCURL_AVAILABLE
	curl_global_cleanup();
#endif
	tools_end();
    }


#if MUTEX_WORKS
    void cancel_thread(pthread_t tid, bool immediate, U_64 flag)
    {
	thread_cancellation::cancel(tid, immediate, flag);
    }

    bool cancel_status(pthread_t tid)
    {
	return thread_cancellation::cancel_status(tid);
    }

    bool cancel_clear(pthread_t tid)
    {
	return thread_cancellation::clear_pending_request(tid);
    }

    U_I get_thread_count()
    {
	return thread_cancellation::count();
    }

#endif


} // end of namespace

extern "C"
{
    unsigned int for_autoconf(unsigned int x)
    {
        return x+1;
    }
}

