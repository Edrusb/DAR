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

    /// \file entrepot_libssh.hpp
    /// \brief defines the implementation for libssh filesystem entrepot
    /// The entrepot_libssh correspond to the libssh filesystems.
    /// \ingroup API

#ifndef ENTREPOT_LIBSSH_HPP
#define ENTREPOT_LIBSSH_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_LIBSSH_LIBSSH_H
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#endif
}

#include <string>
#include <memory>

#include "user_interaction.hpp"
#include "entrepot.hpp"
#include "fichier_global.hpp"
#include "libssh_connection.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// implementation for SFTP entrepot unsing libssh backend
	///
	/// entrepot_libssh generates objects of class "fichier_libssh" inherited class of fichier_global

    class entrepot_libssh : public entrepot
    {
    public:

	    /// constructor

	entrepot_libssh(const std::shared_ptr<user_interaction> & dialog, ///< for user interaction
			const std::string & login,              ///< user login on remote host
			const secu_string & password,           ///< user password on remote host (empty for file auth or user interaction)
			const std::string & host,               ///< the remote server to connect to
			const std::string & port,               ///< TCP/UDP port to connec to (empty string for default)
			bool auth_from_file,                    ///< if true, use private/public key authentication and password is then the key pass, else use password authentication
			const std::string & sftp_pub_keyfile,   ///< where to fetch the public key (sftp only)
			const std::string & sftp_prv_keyfile,   ///< where to fetch the private key (sftp only)
			const std::string & sftp_known_hosts,   ///< location of the known_hosts file (empty string to disable this security check)
			U_I waiting_time,                       ///< time in second to wait before retrying in case of network error
			bool verbose = false                    ///< whether to have verbose messages from libcurl
	    );

	entrepot_libssh(const entrepot_libssh & ref);
	entrepot_libssh(entrepot_libssh && ref) noexcept = delete;
	entrepot_libssh & operator = (const entrepot_libssh & ref) = delete;
	entrepot_libssh & operator = (entrepot_libssh && ref) noexcept = delete;
	~entrepot_libssh() noexcept { read_dir_flush(); };

	virtual std::string get_url() const override;

	virtual void read_dir_reset() const override;
	virtual bool read_dir_next(std::string & filename) const override;
	virtual void read_dir_reset_dirinfo() const override;
	virtual bool read_dir_next_dirinfo(std::string & filename, inode_type & tp) const override;

	virtual void create_dir(const std::string & dirname, U_I permission) override;

	virtual entrepot *clone() const override { return new (std::nothrow) entrepot_libssh(*this); };



    protected:
	virtual fichier_global *inherited_open(const std::shared_ptr<user_interaction> & dialog,
					       const std::string & filename,
					       gf_mode mode,
					       bool force_permission,
					       U_I permission,
					       bool fail_if_exists,
					       bool erase) const override;

	virtual void inherited_unlink(const std::string & filename) const override;
	virtual void read_dir_flush() const override;

    private:
	    // constructor received parameters
	std::string server_url;

	    // libssh structures
	mutable sftp_dir sdir;    // using for read_dir/read_next[_dirinfo]() paradygm

	    // shared libssh structures
	std::shared_ptr<libssh_connection> connect;
    };

	/// @}

} // end of namespace

#endif
