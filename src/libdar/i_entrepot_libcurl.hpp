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

    /// \file i_entrepot_libcurl.hpp
    /// \brief defines the implementation for remote filesystem entrepot using libcurl
    /// \ingroup Private

#ifndef I_ENTREPOT_LIBCURL_HPP
#define I_ENTREPOT_LIBCURL_HPP

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
#include "secu_string.hpp"
#include "mem_ui.hpp"
#include "mycurl_easyhandle_sharing.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// implementation of libcurl_entrepot (pimpl paradigm)

#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )

    class entrepot_libcurl::i_entrepot_libcurl : public entrepot, public mem_ui
    {
    public:
	i_entrepot_libcurl(const std::shared_ptr<user_interaction> & dialog,         ///< for user interaction
			   mycurl_protocol proto,             ///< network protocol to use
			   const std::string & login,              ///< user login on remote host
			   const secu_string & password,      ///< user password on remote host (empty for file auth or user interaction)
			   const std::string & host,               ///< the remote server to connect to
			   const std::string & port,               ///< TCP/UDP port to connec to (empty string for default)
			   bool auth_from_file,               ///< whether to check $HOME/.netrc for password
			   const std::string & sftp_pub_keyfile,   ///< where to fetch the public key (sftp only)
			   const std::string & sftp_prv_keyfile,   ///< where to fetch the private key (sftp only)
			   const std::string & sftp_known_hosts,   ///< location of the known_hosts file (empty string to disable this security check)
			   U_I waiting_time                        ///< time in second to wait before retrying in case of network error
	    );
	i_entrepot_libcurl(const i_entrepot_libcurl & ref) = default;
	i_entrepot_libcurl(i_entrepot_libcurl && ref) = default;
	i_entrepot_libcurl & operator = (const i_entrepot_libcurl & ref) = delete;
	i_entrepot_libcurl & operator = (i_entrepot_libcurl && ref) noexcept = delete;
	~i_entrepot_libcurl() throw () {};


	    // inherited from class entrepot

	    /// \note this is expected to have a double slash after the host:port
	    /// like ftp://www.some.where:8021//tmp/sub/dir
	virtual std::string get_url() const override { return base_URL + get_full_path().display(); };
	virtual void read_dir_reset() const override;
	virtual bool read_dir_next(std::string & filename) const override;
	virtual entrepot *clone() const override { return new (std::nothrow) i_entrepot_libcurl(*this); };

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
	mycurl_protocol x_proto;
	std::string base_URL; ///< URL of the repository with only minimum path (login/password is given outside the URL)
	mycurl_easyhandle_sharing easyh;
	std::deque<std::string> current_dir;
	std::string reading_dir_tmp;
	U_I wait_delay;

	std::string get_libcurl_URL() const;
	void set_libcurl_authentication(user_interaction & dialog,         ///< for user interaction
					const std::string & location,      ///< server to authenticate with
					const std::string & login,         ///< login to use
					const secu_string & password,      ///< password (emtpy for interaction or file auth)
					bool auth_from_file,               ///< if set, check for $HOME/.netrc for password
					const std::string & sftp_pub_keyfile,  ///< where to fetch the public key (sftp only)
					const std::string & sftp_prv_keyfile,  ///< where to fetch the private key (sftp only)
					const std::string & sftp_known_hosts   ///< where to fetch the .known_hosts file (sftp only)
	    );
	void detruit();

	static std::string mycurl_protocol2string(mycurl_protocol proto);
	static std::string build_url_from(mycurl_protocol proto, const std::string & host, const std::string & port);
	static size_t get_ftp_listing_callback(void *buffer, size_t size, size_t nmemb, void *userp);
	static size_t null_callback(void *buffer, size_t size, size_t nmemb, void *userp) { return size*nmemb; };
	static bool mycurl_is_protocol_available(mycurl_protocol proto);

	    // needed to access the entrepot inherited protected methods
	friend class entrepot_libcurl;
    };

#endif
	/// @}

} // end of namespace

#endif
