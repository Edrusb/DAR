//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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

#include "integers.hpp"
#include "infinint.hpp"
#include "tools.hpp"
#include "compile_time_features.hpp"
#if HAVE_LIBTHREADAR_LIBTHREADAR_HPP
#include <libthreadar/libthreadar.hpp>
#endif

extern "C"
{
#if LIBCURL_AVAILABLE
#if HAVE_CURL_CURL_H
#include <curl/curl.h>
#endif
#endif
}

namespace libdar
{
    namespace compile_time
    {

	bool ea() noexcept
	{
#ifdef EA_SUPPORT
	    return true;
#else
	    return false;
#endif
	}


	bool largefile() noexcept
	{
#if defined( _FILE_OFFSET_BITS ) ||  defined( _LARGE_FILES )
	    return true;
#else
	    return sizeof(off_t) > 4;
#endif
	}


	bool nodump() noexcept
	{
	    return FSA_linux_extX();
	}

	bool special_alloc() noexcept
	{
	    return false;
	}


	U_I bits() noexcept
	{
#ifdef LIBDAR_MODE
	    return LIBDAR_MODE;
#else
	    return 0; // infinint
#endif
	}


	bool thread_safe() noexcept
	{
#if defined( MUTEX_WORKS ) && !defined ( MISSING_REENTRANT_LIBCALL )
	    return true;
#else
	    return false;
#endif
	}


	bool libz() noexcept
	{
#if LIBZ_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}


	bool libbz2() noexcept
	{
#if LIBBZ2_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}


	bool liblzo() noexcept
	{
#if LIBLZO2_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}

	bool libxz() noexcept
	{
#if LIBLZMA_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}

	bool libzstd() noexcept
	{
#if LIBZSTD_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}


	bool liblz4() noexcept
	{
#if LIBLZ4_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}


	bool libgcrypt() noexcept
	{
#if CRYPTO_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}


	bool libargon2() noexcept
	{
#if LIBARGON2_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}

	bool furtive_read() noexcept
	{
#if FURTIVE_READ_MODE_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}


	endian system_endian() noexcept
	{
	    endian ret;

	    try
	    {
		ret = infinint::is_system_big_endian() ? big : little;
	    }
	    catch(...)
	    {
		ret = error;
	    }

	    return ret;
	}

	bool posix_fadvise() noexcept
	{
#if HAVE_POSIX_FADVISE
	    return true;
#else
	    return false;
#endif
	}

	bool fast_dir() noexcept
	{
#if LIBDAR_FAST_DIR
	    return true;
#else
	    return false;
#endif
	}

	bool FSA_linux_extX() noexcept
	{
#ifdef LIBDAR_NODUMP_FEATURE
	    return true;
#else
	    return false;
#endif
	}

	bool FSA_birthtime() noexcept
	{
#ifdef LIBDAR_BIRTHTIME
	    return true;
#else
	    return false;
#endif
	}

	bool Linux_statx() noexcept
	{
#ifdef HAVE_STATX_SYSCALL
	    return true;
#else
	    return false;
#endif
	}

	bool microsecond_read() noexcept
	{
#if LIBDAR_TIME_READ_ACCURACY == LIBDAR_TIME_ACCURACY_MICROSECOND || LIBDAR_TIME_READ_ACCURACY == LIBDAR_TIME_ACCURACY_NANOSECOND
	    return true;
#else
	    return false;
#endif
	}

	bool nanosecond_read() noexcept
	{
#if LIBDAR_TIME_READ_ACCURACY == LIBDAR_TIME_ACCURACY_NANOSECOND
	    return true;
#else
	    return false;
#endif
	}


	bool microsecond_write() noexcept
	{
#if LIBDAR_TIME_WRITE_ACCURACY == LIBDAR_TIME_ACCURACY_MICROSECOND || LIBDAR_TIME_WRITE_ACCURACY == LIBDAR_TIME_ACCURACY_NANOSECOND
	    return true;
#else
	    return false;
#endif
	}

	bool nanosecond_write() noexcept
	{
#if LIBDAR_TIME_WRITE_ACCURACY == LIBDAR_TIME_ACCURACY_NANOSECOND
	    return true;
#else
	    return false;
#endif
	}

	bool symlink_restore_dates() noexcept
	{
#ifdef HAVE_LUTIMES
	    return true;
#else
	    return false;
#endif
	}


	bool public_key_cipher() noexcept
	{
#ifdef GPGME_SUPPORT
	    return true;
#else
	    return false;
#endif
	}

	bool libthreadar() noexcept
	{
#ifdef LIBTHREADAR_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}

	std::string libthreadar_version() noexcept
	{
	    std::string ret = "";
#ifdef LIBTHREADAR_AVAILABLE
	    unsigned int min, med, maj;

	    libthreadar::get_version(maj, med, min);
#ifdef LIBTHREADAR_BARRIER_MAC
	    std::string barrier_flavor = libthreadar::barrier::used_implementation();
	    std::string barrier_impl = tools_printf(gettext("barrier using %S"), &barrier_flavor);
#else
	    std::string barrier_impl = "";
#endif
	    ret = tools_printf("%d.%d.%d - %S", maj, med, min, &barrier_impl);
#endif
	    return ret;
	}


	bool librsync() noexcept
	{
#if LIBRSYNC_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}

	bool remote_repository() noexcept
	{
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	    return true;
#else
	    return false;
#endif
	}

	bool whirlpool_hash() noexcept
	{
#if RHASH_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}

	std::string libcurl_version() noexcept
	{
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	    return curl_version();
#else
	    return "";
#endif
	}

    } // end of compile_time nested namespace
} // end of libdar namespace
