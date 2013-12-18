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

    /// \file compile_time_features.hpp
    /// \brief nested namespace containing routines that give features activated at compile time
    /// \ingroup API


#ifndef COMPILE_TIME_FEATURES_HPP
#define COMPILE_TIME_FEATURES_HPP

#include "../my_config.h"


    /// \addtogroup API
    /// @{


namespace libdar
{

	/// nested namespace inside libdar

	/// it contains one routine per feature that can be activated or disabled at compile time
	/// this is to replace the "libdar::get_compile_time_feature" function
	/// that cannot be updates without breaking backward compatibility

    namespace compile_time
    {
	    /// returns whether EA support has been activated at compilation time
	bool ea();

	    /// returns whether largefile (>2GiB) support has been activated at compilation time
	bool largefile();

	    /// returns whether nodump flag support has been activated at compilation time
	bool nodump();

	    /// returns whether special allocation support has been activated at compilation time
	    ///
	    /// special allocation support brings from a tiny to an important improvement in
	    /// execution time, depending on the number of small files involved in the operation
	bool special_alloc();

	    /// returns the internal integer type used

	    /// \note zero is returned if infinint type is used
	U_I bits();

	    /// returns whether the current libdar is thread safe
	bool thread_safe();

	    /// returns whether libdar is dependent on libz and if so has gzip compression/decompression available
	bool libz();

	    /// returns whether libdar is dependent on libbz2 and if so has bzip2 compression/decompression available
	bool libbz2();

	    /// returns whether libdar is dependent on liblzo and if so has lzo compression/decompression available
	bool liblzo();

	    /// returns whether libdar is dependent on libgcrypt and if so has strong encryption and hashing features available
	bool libgcrypt();

	    /// returns whether libdar can support furtive read mode when run by privileged user
	bool furtive_read();

	    /// type used to return the endian nature of the current system
	enum endian
	{
	    big = 'B',    //< big endian
	    little = 'L', //< little endian
	    error = 'E'   //< neither big nor little endian! (libdar cannot run on such system)
	};

	    /// returns the detected integer endian of the system
	endian system_endian();

	    /// returns true if libdar has support for posix_fadvise activated available
	bool posix_fadvise();

	    /// returns whether libdar has been built with speed optimization for last directory
	bool fast_dir();

	    /// returns whether libdar has been built with support for linux ext2/3/4 FSA
	bool FSA_linux_extX();

	    /// returns whether libdar has been built with support for HFS+ FSA
	bool FSA_HFS_plus();
    }

} // end of namespace

    /// @}


#endif
