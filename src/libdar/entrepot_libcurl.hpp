/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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

    /// \file entrepot_libcurl.hpp
    /// \brief defines the implementation for remote filesystem entrepot using libcurl
    /// \ingroup API

#ifndef ENTREPOT_LIBCURL_HPP
#define ENTREPOT_LIBCURL_HPP

#include "../my_config.h"

extern "C"
{
}

#include <string>
#include <deque>
#include "entrepot.hpp"
#include "secu_string.hpp"
#include "mycurl_protocol.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

	/// for managing archive into a remote repository

    class entrepot_libcurl : public entrepot
    {
    public:
	entrepot_libcurl(const std::shared_ptr<user_interaction> & dialog, ///< for user interaction
			 mycurl_protocol proto,                  ///< network protocol to use
			 const std::string & login,              ///< user login on remote host
			 const secu_string & password,           ///< user password on remote host (empty for file auth or user interaction)
			 const std::string & host,               ///< the remote server to connect to
			 const std::string & port,               ///< TCP/UDP port to connec to (empty string for default)
			 bool auth_from_file,                    ///< whether to check $HOME/.netrc for password
			 const std::string & sftp_pub_keyfile,   ///< where to fetch the public key (sftp only)
			 const std::string & sftp_prv_keyfile,   ///< where to fetch the private key (sftp only)
			 const std::string & sftp_known_hosts,   ///< location of the known_hosts file (empty string to disable this security check)
			 U_I waiting_time,                       ///< time in second to wait before retrying in case of network error
			 bool verbose = false                    ///< whether to have verbose messages from libcurl
	    );

	entrepot_libcurl(const entrepot_libcurl & ref) = default;
	entrepot_libcurl(entrepot_libcurl && ref) noexcept = default;
	entrepot_libcurl & operator = (const entrepot_libcurl & ref) = default;
	entrepot_libcurl & operator = (entrepot_libcurl && ref) noexcept = default;
	~entrepot_libcurl() throw () {};


	    // inherited from class entrepot

	virtual void set_location(const path & chemin) override;
	virtual void set_root(const path & p_root) override;
	virtual path get_full_path() const override;
	    /// \note this is expected to have a double slash after the host:port
	    /// like ftp://www.some.where:8021//tmp/sub/dir5A
	virtual std::string get_url() const override;
	virtual const path & get_location() const override;
	virtual const path & get_root() const override;
	virtual void change_user_interaction(const std::shared_ptr<user_interaction> & new_dialog) override;
	virtual std::shared_ptr<user_interaction> get_current_user_interaction() const override;
	virtual void read_dir_reset() const override;
	virtual bool read_dir_next(std::string & filename) const override;
	virtual void read_dir_reset_dirinfo() const override;
	virtual bool read_dir_next_dirinfo(std::string & filename, bool & isdir) const override;

	virtual void create_dir(const std::string & dirname, U_I permission) override;

	virtual entrepot *clone() const override { return new (std::nothrow) entrepot_libcurl(*this); };

    protected:

	    // inherited from class entrepot

	virtual fichier_global *inherited_open(const std::shared_ptr<user_interaction> & dialog,
					       const std::string & filename,
					       gf_mode mode,
					       bool force_permission,
					       U_I permission,
					       bool fail_if_exists,
					       bool erase) const override;

	virtual void inherited_unlink(const std::string & filename) const override;
	virtual void read_dir_flush() override;

    private:
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	class i_entrepot_libcurl;
	std::shared_ptr<i_entrepot_libcurl> pimpl;
#endif
    };

	/// @}

} // end of namespace

#endif
