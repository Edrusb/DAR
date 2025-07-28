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

    /// \file remote_entrepot_api.hpp
    /// \brief Libdar API to create entrepot objects
    /// \ingroup API

#ifndef REMOTE_ENTREPOT_API_HPP
#define REMOTE_ENTREPOT_API_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "entrepot.hpp"

namespace libdar
{
        /// \addtogroup API
	/// @{

	/// type of entrepot to create
    enum class remote_entrepot_type
    {
	ftp,   ///< filesystem accessed through FTP protocol
	sftp   ///< filesystem accessed through SFTP protocol
    };


	// for backward compatibility with API found in release 2.7.x and before
    typedef remote_entrepot_type mycurl_protocol;
    const remote_entrepot_type proto_ftp = remote_entrepot_type::ftp;
    const remote_entrepot_type proto_sftp = remote_entrepot_type::sftp;

	/// extract entrepot type from a given URL
    extern remote_entrepot_type string_to_remote_entrepot_type(const std::string & arg);


	/// create a remote entrepot of given type
    std::shared_ptr<entrepot> create_remote_entrepot(std::shared_ptr<user_interaction> & dialog,
						     remote_entrepot_type proto,
						     const std::string & login,
						     const secu_string & pass,
						     const std::string & host,
						     const std::string & port,
						     bool auth_from_file,
						     const std::string & sftp_pub_filekey,
						     const std::string & sftp_prv_filekey,
						     const std::string & sftp_known_hosts,
						     U_I network_retry,
						     bool remote_verbose);


   	/// @}

} // end of namespace

#endif
