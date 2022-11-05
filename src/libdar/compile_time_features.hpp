/*********************************************************************/
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

    /// \file compile_time_features.hpp
    /// \brief nested namespace containing routines that give features activated at compile time
    /// \ingroup API


#ifndef COMPILE_TIME_FEATURES_HPP
#define COMPILE_TIME_FEATURES_HPP

#include "../my_config.h"

namespace libdar
{

        /// \addtogroup API
	/// @{

	/// nested namespace inside libdar

	/// it contains one routine per feature that can be activated or disabled at compile time
	/// this is to replace the "libdar::get_compile_time_feature" function
	/// that cannot be updates without breaking backward compatibility

    namespace compile_time
    {
	    /// returns whether EA support has been activated at compilation time
	bool ea() noexcept;

	    /// returns whether largefile (>2GiB) support has been activated at compilation time
	bool largefile() noexcept;

	    /// returns whether nodump flag support has been activated at compilation time
	bool nodump() noexcept;

	    /// returns whether special allocation support has been activated at compilation time

	    /// special allocation support brings from a tiny to an important improvement in
	    /// execution time, depending on the number of small files involved in the operation
	bool special_alloc() noexcept;

	    /// returns the internal integer type used

	    /// \note zero is returned if infinint type is used
	U_I bits() noexcept;

	    /// returns whether the current libdar is thread safe
	bool thread_safe() noexcept;

	    /// returns whether libdar is dependent on libz and if so has gzip compression/decompression available
	bool libz() noexcept;

	    /// returns whether libdar is dependent on libbz2 and if so has bzip2 compression/decompression available
	bool libbz2() noexcept;

	    /// returns whether libdar is dependent on liblzo and if so has lzo compression/decompression available
	bool liblzo() noexcept;

	    /// returns whether libdar is dependent on liblxz/liblzma and if so has xz compression/decompression available
	bool libxz() noexcept;

	    /// returns whether libdar is dependent on libzstd and if so has zstd compression/decompression available
	bool libzstd() noexcept;

	    /// returns whether libdar is dependen in liblz4 and if so has lz4 compression/decompression available
	bool liblz4() noexcept;

	    /// returns whether libdar is dependent on libgcrypt and if so has strong encryption and hashing features available
	bool libgcrypt() noexcept;

	    /// returns whether libdar is dependent on libargon2 and if it has thus argon2 hash algorithm feature available
	bool libargon2() noexcept;

	    /// returns whether libdar can support furtive read mode when run by privileged user
	bool furtive_read() noexcept;

	    /// type used to return the endian nature of the current system
	enum endian
	{
	    big = 'B',    //< big endian
	    little = 'L', //< little endian
	    error = 'E'   //< neither big nor little endian! (libdar cannot run on such system)
	};

	    /// returns the detected integer endian of the system
	endian system_endian() noexcept;

	    /// returns true if libdar has support for posix_fadvise activated available
	bool posix_fadvise() noexcept;

	    /// returns whether libdar has been built with speed optimization for last directory
	bool fast_dir() noexcept;

	    /// returns whether libdar has been built with support for linux ext2/3/4 FSA
	bool FSA_linux_extX() noexcept;

	    /// returns whether libdar has been built with support for HFS+ FSA
	bool FSA_birthtime() noexcept;

	    /// returns whether libdar has been built with support for Linux statx()
	bool Linux_statx() noexcept;

	    /// returns whether libdar is able to read timestamps at least at microsecond accuracy
	bool microsecond_read() noexcept;

	    /// returns whether libdar is able to read timestamps at least at nanosecond accuracy
	bool nanosecond_read() noexcept;

	    /// returns whether libdar is able to write timestamps at least at microsecond accuracy
	bool microsecond_write() noexcept;

	    /// returns whether libdar is able to write timestamps at least at nanosecond accuracy
	bool nanosecond_write() noexcept;

	    /// returns whether libdar is able to restore dates of symlinks
	bool symlink_restore_dates() noexcept;

	    /// returns whether public key cipher (relying on gpgme) are available
	bool public_key_cipher() noexcept;

	    /// returns whether libthreadar linking will be done, allowing libdar to span several threads
	bool libthreadar() noexcept;

	    /// return libthreadar version or empty string libthreadar is not available
	std::string libthreadar_version() noexcept;

	    /// returns whether delta compression is available and delta diff stuff with it
	bool librsync() noexcept;

	    /// returns whether remote repository feature is available (implemented using libcurl)
	bool remote_repository() noexcept;

	    /// returns the libcurl version used or empty string if not available
	std::string libcurl_version() noexcept;

    } // end of compile_time namespace

        /// @}

} // end of libdar namespace

#endif
