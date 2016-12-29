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

#include "../my_config.h"

#include "integers.hpp"
#include "infinint.hpp"
#include "compile_time_features.hpp"

namespace libdar
{
    namespace compile_time
    {

	bool ea()
	{
#ifdef EA_SUPPORT
	    return true;
#else
	    return false;
#endif
	}


	bool largefile()
	{
#if defined( _FILE_OFFSET_BITS ) ||  defined( _LARGE_FILES )
	    return true;
#else
	    return sizeof(off_t) > 4;
#endif
	}


	bool nodump()
	{
	    return FSA_linux_extX();
	}

	bool special_alloc()
	{
#ifdef LIBDAR_SPECIAL_ALLOC
	    return true;
#else
	    return false;
#endif
	}


	U_I bits()
	{
#ifdef LIBDAR_MODE
	    return LIBDAR_MODE;
#else
	    return 0; // infinint
#endif
	}


	bool thread_safe()
	{
#if defined( MUTEX_WORKS ) && !defined ( MISSING_REENTRANT_LIBCALL )
	    return true;
#else
	    return false;
#endif
	}


	bool libz()
	{
#if LIBZ_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}


	bool libbz2()
	{
#if LIBBZ2_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}


	bool liblzo()
	{
#if LIBLZO2_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}

	bool libxz()
	{
#if LIBLZMA_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}

	bool libgcrypt()
	{
#if CRYPTO_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}


	bool furtive_read()
	{
#if FURTIVE_READ_MODE_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}


	endian system_endian()
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

	bool posix_fadvise()
	{
#if HAVE_POSIX_FADVISE
	    return true;
#else
	    return false;
#endif
	}

	bool fast_dir()
	{
#if LIBDAR_FAST_DIR
	    return true;
#else
	    return false;
#endif
	}

	bool FSA_linux_extX()
	{
#ifdef LIBDAR_NODUMP_FEATURE
	    return true;
#else
	    return false;
#endif
	}

	bool FSA_birthtime()
	{
#ifdef LIBDAR_BIRTHTIME
	    return true;
#else
	    return false;
#endif
	}

	bool microsecond_read()
	{
#ifdef LIBDAR_MICROSECOND_READ_ACCURACY
	    return true;
#else
	    return false;
#endif
	}

	bool microsecond_write()
	{
#ifdef LIBDAR_MICROSECOND_WRITE_ACCURACY
	    return true;
#else
	    return false;
#endif
	}

	bool symlink_restore_dates()
	{
#ifdef HAVE_LUTIMES
	    return true;
#else
	    return false;
#endif
	}


	bool public_key_cipher()
	{
#ifdef GPGME_SUPPORT
	    return true;
#else
	    return false;
#endif
	}

	bool libthreadar()
	{
#ifdef LIBTHREADAR_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}

	bool librsync()
	{
#if LIBRSYNC_AVAILABLE
	    return true;
#else
	    return false;
#endif
	}

	bool remote_repository()
	{
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	    return true;
#else
	    return false;
#endif
	}

    } // end of compile_time nested namespace
} // end of libdar namespace
