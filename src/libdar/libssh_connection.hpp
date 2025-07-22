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

    /// \file libssh_connection.hpp
    /// \brief class holding libssh related data structure for an ssh and sftp session
    /// \note this class is shared between entrepot_libssh and fichier_libssh by mean of std::shared_ptr

    /// \ingroup Private

#ifndef LIBSSH_CONNECTION_HPP
#define LIBSSH_CONNECTION_HPP


#include "../my_config.h"

extern "C"
{
#if HAVE_LIBSSH_LIBSSH_H
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#endif
} // end extern "C"

#include <string>

#include "integers.hpp"
#include "user_interaction.hpp"
#include "secu_string.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


	/// class used to share libssh constructs between entrepot_libssh and fichier_libssh by mean of std::shared_ptr

    class libssh_connection
    {
    public:

	libssh_connection(const std::shared_ptr<user_interaction> & dialog,
			  const std::string & login,
			  const secu_string & password,
			  const std::string & host,
			  const std::string & port,
			  bool auth_from_file,
			  const std::string & sftp_pub_keyfile,
			  const std::string & sftp_prv_keyfile,
			  const std::string & sftp_known_hosts,
			  U_I waiting_time,
			  bool verbose = false);

	libssh_connection(const libssh_connection & ref) = delete;
	libssh_connection(libssh_connection && ref) = delete;
	libssh_connection & operator = (const libssh_connection & ref) = delete;
	libssh_connection & operator = (libssh_connection && ref) = delete;
	~libssh_connection() { cleanup_session(); };

	ssh_session & get_ssh_session() { return sess; };
	sftp_session & get_sftp_session() { return sftp_sess; };
	U_I get_retry_delay() const { return waiting; };
	U_I get_max_read() const { return max_read; };
	U_I get_max_write() const { return max_write; };

	const char* get_sftp_error_msg() const;

    private:
	ssh_session sess;
	sftp_session sftp_sess; // this is sftp subsystem handle inside the ssh "sess" session
	U_I waiting;
	uint64_t max_read;
	uint64_t max_write;


	void create_session(const std::string & host,
			    const std::string & port,
			    const std::string & login,
			    const std::string sftp_known_hosts,
			    bool verbose,
			    const std::string sftp_pub_keyfile,
			    const std::string & sftp_prv_keyfile);

	void server_authentication();
	void user_authentication(user_interaction & dialog,
				 const secu_string & password,
				 bool auth_from_file,
				 const std::string & login,
				 const std::string & host,
				 const std::string & sftp_pub_keyfile,
				 const std::string & sftp_prv_keyfile);
	void create_sftp_session();
	void set_max_limits();
	void cleanup_session();

	static const char* get_key_error_msg(int code);
	static const char* get_auth_error_msg(int code);
    };

	/// @}

} // end of namespace

#endif
