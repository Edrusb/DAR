/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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

    /// \file compression.hpp
    /// \brief compression parameters for API
    /// \ingroup API

#ifndef COMPRESSION_HPP
#define COMPRESSION_HPP

#include "../my_config.h"
#include <string>

namespace libdar
{

    	/// \addtogroup API
	/// @{

	/// the different compression algorithm available

	/// values to be used as argument of libdar API calls
	/// \note lzo1x_1_15 and lzo1x_1 should never be found in archive but instead lzo should
	/// be put in place. In consequence, the two letters 'j' and 'k' reserved here, shall
	/// well be modified to other value if necessary in the future, thus would not break
	/// any backward compatibility.
    enum class compression
    {
	none = 'n',  ///< no compression
	gzip = 'z',  ///< gzip compression
	bzip2 = 'y', ///< bzip2 compression
	lzo = 'l',   ///< lzo compression
	xz = 'x',     ///< lzma compression
	lzo1x_1_15 = 'j', ///< lzo degraded algo corresponding to lzop -1
	lzo1x_1 = 'k' ///< lzo degraded algo corresponding to lzo -2 to lzo -6
    };


	/// convert a char as stored in archive to its compression value
    extern compression char2compression(char a);

	/// convert a compression value to a char for storing in archive
    extern char compression2char(compression c);

	/// convert a compression to its string representation
    extern std::string compression2string(compression c);

	/// convert a string representing a compression algorithm to its enum compression value
    extern compression string2compression(const std::string & a); // throw Erange if an unknown string is given

	/// @}

} // end of namespace

#endif
