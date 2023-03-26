/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

    /// \file mycurl_protocol.hpp
    /// \brief datastructure defining the network protocols available for entrepot_libcurl class
    /// \ingroup API

#ifndef MYCURL_PROTOCOL_HPP
#define MYCURL_PROTOCOL_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"


namespace libdar
{
        /// \addtogroup API
	/// @{

	/// libcurl protocols supported by libdar

	/// \note dar needs to list a directory content to find
	/// the last slice available in a directory, which
	/// feature is not always available with http and https protocols
	/// and never available with scp protocol
	/// Thus dar only supports ftp and sftp
    enum mycurl_protocol
    {
	proto_ftp,  ///< FTP file transfer protocol
	proto_sftp  ///< SFTP Secure FTP (over ssh)
    };

	/// extract mycurl_protocol from a given URL
    extern mycurl_protocol string_to_mycurl_protocol(const std::string & arg);

   	/// @}

} // end of namespace

#endif
