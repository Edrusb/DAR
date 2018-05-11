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
// $Id: entrepot.hpp,v 1.1 2012/04/27 11:24:30 edrusb Exp $
//
/*********************************************************************/


    /// \file entrepot_libcurl5.hpp
    /// \brief defines the implementation for remote filesystem entrepot using libcurl
    /// \ingroup API

#ifndef ENTREPOT_LIBCURL5_HPP
#define ENTREPOT_LIBCURL5_HPP

#include "../my_config.h"

extern "C"
{
#if LIBCURL_AVAILABLE
#if HAVE_CURL_CURL_H
#include <curl/curl.h>
#endif
#endif
}

#include <string>
#include <deque>
#include "entrepot_libcurl.hpp"
#include "user_interaction5.hpp"

namespace libdar5
{

	/// \addtogroup Private
	/// @{

	/// implementation for entrepot to access to local filesystem

	/// entrepot_local generates objects of class "fichier_local" inherited class of fichier_global

    using libdar::mycurl_protocol;

    class entrepot_libcurl : public libdar::entrepot_libcurl
    {
    public:
	entrepot_libcurl(user_interaction & dialog,         ///< for user interaction
			 mycurl_protocol proto,             ///< network protocol to use
			 const std::string & login,              ///< user login on remote host
			 const secu_string & password,      ///< user password on remote host (empty for file auth or user interaction)
			 const std::string & host,               ///< the remote server to connect to
			 const std::string & port,               ///< TCP/UDP port to connec to (empty string for default)
			 bool auth_from_file,               ///< whether to check $HOME/.netrc for password
			 const std::string & sftp_pub_keyfile,   ///< where to fetch the public key (sftp only)
			 const std::string & sftp_prv_keyfile,   ///< where to fetch the private key (sftp only)
			 const std::string & sftp_known_hosts,   ///< location of the known_hosts file (empty string to disable this security check)
			 U_I waiting_time):
	    libdar::entrepot_libcurl(user_interaction5_clone_to_shared_ptr(dialog),
				     proto,
				     login,
				     password,
				     host,
				     port,
				     auth_from_file,
				     sftp_pub_keyfile,
				     sftp_prv_keyfile,
				     sftp_known_hosts,
				     waiting_time)
	{}

	entrepot_libcurl(const entrepot_libcurl & ref) = default;
	entrepot_libcurl(entrepot_libcurl && ref) noexcept = default;
	entrepot_libcurl & operator = (const entrepot_libcurl & ref) = default;
	entrepot_libcurl & operator = (entrepot_libcurl && ref) noexcept = default;
	~entrepot_libcurl() throw () {};

    };

	/// @}

} // end of namespace

#endif
