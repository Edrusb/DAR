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


    /// \file entrepot_libcurl.hpp
    /// \brief defines the implementation for remote filesystem entrepot using libcurl

    /// \ingroup API

#ifndef ENTREPOT_LIBCURL_HPP
#define ENTREPOT_LIBCURL_HPP

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
#include "entrepot.hpp"
#include "secu_string.hpp"
#include "mem_ui.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// implementation for entrepot to access to local filesystem
	///
	/// entrepot_local generates objects of class "fichier_local" inherited class of fichier_global

    class entrepot_libcurl : public entrepot, public mem_ui
    {
    public:
	enum curl_protocol { proto_ftp, proto_http, proto_https, proto_scp, proto_sftp };
	static curl_protocol string_to_curlprotocol(const std::string & arg);

	entrepot_libcurl(user_interaction & dialog,            //< used to report temporary network failure to the user
			 curl_protocol proto,                  //< protocol to use for communication with the remote repository
			 const std::string & login,            //< login to use
			 const secu_string & password,         //< password to authenticate
			 const std::string & host,             //< hostname or IP of the remote repositiry
			 const std::string & port,             //< empty string to use default port in regard to protocol used
			 U_I waiting_time);                    //< seconds to wait before retrying in case of network erorr
	entrepot_libcurl(const entrepot_libcurl & ref) : entrepot(ref), mem_ui(ref) { copy_from(ref); };
	const entrepot_libcurl & operator = (const entrepot_libcurl & ref) { detruit(); copy_from(ref); return *this; };
	~entrepot_libcurl() throw() { detruit(); };

	    // inherited from class entrepot

	    /// \note this is expected to have a double slash after the host:port
	    /// like ftp://www.some.where:8021//tmp/sub/dir
	std::string get_url() const { return base_URL + get_full_path().display(); };
	void read_dir_reset() const;
	bool read_dir_next(std::string & filename) const;
	entrepot *clone() const { return new (get_pool()) entrepot_libcurl(*this); };

    protected:

	    // inherited from class entrepot

	fichier_global *inherited_open(user_interaction & dialog,
				       const std::string & filename,
				       gf_mode mode,
				       bool force_permission,
				       U_I permission,
				       bool fail_if_exists,
				       bool erase) const;

	void inherited_unlink(const std::string & filename) const;
	void read_dir_flush();

    private:
	curl_protocol x_proto;
	std::string base_URL; //< URL of the repository with only minimum path (login/password is given outside the URL)
#if LIBCURL_AVAILABLE
	CURL *easyhandle;
#endif
	std::list<std::string> current_dir;
	std::string reading_dir_tmp;
	U_I wait_delay;

	void set_libcurl_URL();
	void set_libcurl_authentication(const std::string & login, const secu_string & password);
	void copy_from(const entrepot_libcurl & ref);
	void detruit();

	static std::string curl_protocol2string(curl_protocol proto);
	static std::string build_url_from(curl_protocol proto, const std::string & host, const std::string & port);
	static size_t get_ftp_listing_callback(void *buffer, size_t size, size_t nmemb, void *userp);
	static size_t null_callback(void *buffer, size_t size, size_t nmemb, void *userp) { return size*nmemb; };
    };

	/// @}

} // end of namespace

#endif
