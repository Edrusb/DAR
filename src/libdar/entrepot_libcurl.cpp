//*********************************************************************/
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
// $Id: entrepot.cpp,v 1.1 2012/04/27 11:24:30 edrusb Exp $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
}

#include "tools.hpp"
#include "entrepot_libcurl.hpp"
#include "fichier_libcurl.hpp"
#include "cache_global.hpp"

using namespace std;

namespace libdar
{

    entrepot_libcurl::mycurl_protocol entrepot_libcurl::string_to_mycurlprotocol(const std::string & arg)
    {
	mycurl_protocol ret;

	if(strcasecmp(arg.c_str(), "ftp") == 0)
	    ret = proto_ftp;
	else if(strcasecmp(arg.c_str(), "sftp") == 0)
	    ret = proto_sftp;
	else if(strcasecmp(arg.c_str(), "scp") == 0)
	    ret = proto_scp;
	else if(strcasecmp(arg.c_str(), "http") == 0)
	    ret = proto_http;
	else if(strcasecmp(arg.c_str(), "https") == 0)
	    ret = proto_https;
	else
	    throw Erange("entrepot_libcurl::string_to_curlprotocol", tools_printf(gettext("Unknown protocol: %S"), &arg));

	return ret;
    }

    entrepot_libcurl::entrepot_libcurl(user_interaction & dialog,
				       mycurl_protocol proto,
				       const string & login,
				       const secu_string & password,
				       const string & host,
				       const string & port,
				       bool auth_from_file,
				       U_I waiting_time): mem_ui(dialog),
							  x_proto(proto),
							  base_URL(build_url_from(proto, host, port)),
							  wait_delay(waiting_time)
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	current_dir.clear();
	reading_dir_tmp.clear();

	    // initializing the fields from parent class entrepot

	set_root("/");
	set_location("/");
	set_user_ownership(""); // not used for this type of entrepot
	set_group_ownership(""); // not used for this type of entrepot

	if(!mycurl_is_protocol_available(proto))
	{
	    string named_proto = mycurl_protocol2string(proto);
	    throw Erange("entrepot_libcurl::entrepot_libcurl",
			 tools_printf(gettext("protocol %S is not supported by libcurl, aborting"), & named_proto));
	}

	easyhandle = curl_easy_init();
	if(easyhandle == nullptr)
	    throw Erange("entrepot_libcurl::entrepot_libcurl", string(gettext("Error met while creating a libcurl handle")));
	set_libcurl_authentication(host, login, password, auth_from_file);

#ifdef LIBDAR_NO_OPTIMIZATION
	CURLcode err = curl_easy_setopt(easyhandle, CURLOPT_VERBOSE, 1);
	if(err != CURLE_OK)
	    throw Erange("entrepot_libcurl::entrepot_libcurl",
			 tools_printf(gettext("Error met while setting verbosity on handle: %s"),
				      curl_easy_strerror(err)));
#endif

#else

#if ! defined ( LIBCURL_AVAILABLE )
	throw Efeature("libcurl");
#elif ! defined ( LIBTHREADAR_AVAILABLE )
	throw Efeature("libthreadar");
#else
	throw SRC_BUG;
#endif

#endif
    }

    void entrepot_libcurl::read_dir_reset() const
    {
#if LIBCURL_AVAILABLE
	CURLcode err = CURLE_OK;
	long listonly;
	entrepot_libcurl *me = const_cast<entrepot_libcurl *>(this);

	if(me == nullptr)
	    throw SRC_BUG;

	me->current_dir.clear();
	me->reading_dir_tmp = "";
	me->set_libcurl_URL();

	try
	{
	    switch(x_proto)
	    {
	    case proto_ftp:
	    case proto_sftp:
		try
		{
		    listonly = 1;
		    err = curl_easy_setopt(easyhandle, CURLOPT_DIRLISTONLY, listonly);
		    if(err != CURLE_OK)
			throw Erange("","");
		    err = curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, get_ftp_listing_callback);
		    if(err != CURLE_OK)
			throw Erange("","");
		    err = curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, this);
		    if(err != CURLE_OK)
			throw Erange("","");
		}
		catch(Erange & e)
		{
		    throw Erange("entrepot_libcurl::inherited_unlink",
				 tools_printf(gettext("Error met while preparing directory listing: %s"),
					      curl_easy_strerror(err)));
		}

		do
		{
		    err = curl_easy_perform(easyhandle);
		    fichier_libcurl_check_wait_or_throw(get_ui(),
							err,
							wait_delay,
							tools_printf(gettext("Error met while listing FTP directory %s"),
								     get_url().c_str()));
		}
		while(err != CURLE_OK);

		if(!reading_dir_tmp.empty())
		{
		    me->current_dir.push_back(reading_dir_tmp);
		    me->reading_dir_tmp.clear();
		}
		break;
	    case proto_http:
	    case proto_https:
	    case proto_scp:
		throw Efeature("unlink http, https, scp");
	    default:
		throw SRC_BUG;
	    }
	}
	catch(...)
	{
		// putting back handle in its default state

	    switch(x_proto)
	    {
	    case proto_ftp:
	    case proto_sftp:
		listonly = 0;
		(void)curl_easy_setopt(easyhandle, CURLOPT_DIRLISTONLY, listonly);
		(void)curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, null_callback);
		break;
	    case proto_http:
	    case proto_https:
	    case proto_scp:
		throw Efeature("unlink http, https, scp");
	    default:
		break; // exception thrown in the try block
	    }

	    throw;
	}

	    // putting back handle in its default state

	switch(x_proto)
	{
	case proto_ftp:
	case proto_sftp:
	    listonly = 0;
	    (void)curl_easy_setopt(easyhandle, CURLOPT_DIRLISTONLY, listonly);
	    (void)curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, null_callback);
	    break;
	case proto_http:
	case proto_https:
	case proto_scp:
	    throw Efeature("unlink http, https, scp");
		default:
	    throw SRC_BUG;
	}
#else
    throw Efeature("libcurl");
#endif
    }

    bool entrepot_libcurl::read_dir_next(string & filename) const
    {
#if LIBCURL_AVAILABLE
	entrepot_libcurl *me = const_cast<entrepot_libcurl *>(this);

	if(me == nullptr)
	    throw SRC_BUG;

	if(current_dir.empty())
	    return false;
	else
	{
	    filename = current_dir.front();
	    me->current_dir.pop_front();
	    return true;
	}
#else
    throw Efeature("libcurl");
#endif
    }

    fichier_global *entrepot_libcurl::inherited_open(user_interaction & dialog,
						     const string & filename,
						     gf_mode mode,
						     bool force_permission,
						     U_I permission,
						     bool fail_if_exists,
						     bool erase) const
    {
#if LIBCURL_AVAILABLE
	fichier_global *ret = nullptr;
	cache_global *rw = nullptr;
	entrepot_libcurl *me = const_cast<entrepot_libcurl *>(this);
	gf_mode hidden_mode = mode;

	if(me == nullptr)
	    throw SRC_BUG;

	if(fail_if_exists)
	{
	    string tmp;

	    me->read_dir_reset();
	    while(me->read_dir_next(tmp))
		if(tmp == filename)
		    throw Esystem("entrepot_libcurl::inherited_open", "File exists on remote repository" , Esystem::io_exist);
	}

	string chemin = (path(get_url(), true) + filename).display();

	if(hidden_mode == gf_read_write)
	    hidden_mode = gf_write_only;

	try
	{
#ifdef LIBTHREADAR_AVAILABLE
	    fichier_libcurl *ret_libcurl = new (get_pool()) fichier_libcurl(dialog,
									    chemin,
									    easyhandle,
									    hidden_mode,
									    wait_delay,
									    force_permission,
									    permission,
									    erase);
	    ret = ret_libcurl;

	    if(ret == nullptr)
		throw Ememory("entrepot_libcurl::inherited_open");

	    if(x_proto != proto_ftp)
		ret_libcurl->read_limited_size_at_once();
#else
	    throw Efeature("libthreadar");
#endif

	    switch(mode)
	    {
	    case gf_read_write:
		rw = new (nothrow) cache_global(dialog, ret, true);
		if(rw != nullptr)
		{
		    ret = nullptr; // the former object pointed to by ret is now managed by rw
		    rw->change_to_read_write();
		    ret = rw;      // only now because if change_to_read_write() generate an exception
			// we must not delete twice the same object that would be pointed to once by ret and once again by rw
		    rw = nullptr;
		}
		else
		    throw Ememory("entrpot_libcurl::inherited_open");
		break;
	    case gf_read_only:
		rw = new (nothrow) cache_global(dialog, ret, false);
		if(rw != nullptr)
		{
		    ret = nullptr;  // the former object pointed to by ret is now managed by rw
		    ret = rw;
		    rw = nullptr;
		}
		break;
	    case gf_write_only:
		break; // no cache in write only mode
	    default:
		throw SRC_BUG;
	    }
	}
	catch(...)
	{
	    if(ret != nullptr)
		delete ret;
	    if(rw != nullptr)
		delete rw;
	    throw;
	}

	return ret;
#else
    throw Efeature("libcurl");
#endif
    }

    void entrepot_libcurl::inherited_unlink(const string & filename) const
    {
#if LIBCURL_AVAILABLE
	struct curl_slist *headers = nullptr;
	string order;
	CURLcode err;
	entrepot_libcurl *me = const_cast<entrepot_libcurl *>(this);

	if(me == nullptr)
	    throw SRC_BUG;
	me->set_libcurl_URL();

	try
	{
	    switch(x_proto)
	    {
	    case proto_ftp:
	    case proto_sftp:
		order = "DELE " + filename;
		headers = curl_slist_append(headers, order.c_str());
		err = curl_easy_setopt(easyhandle, CURLOPT_QUOTE, headers);
		if(err != CURLE_OK)
		    throw Erange("entrepot_libcurl::inherited_unlink",
				 tools_printf(gettext("Error met while setting up connection for file %S removal: %s"),
					      &filename, curl_easy_strerror(err)));
		err = curl_easy_setopt(easyhandle, CURLOPT_NOBODY, 1);
		if(err != CURLE_OK)
		    throw Erange("entrepot_libcurl::inherited_unlink",
				 tools_printf(gettext("Error met while setting up connection for file %S removal: %s"),
					      &filename, curl_easy_strerror(err)));
		do
		{
		    err = curl_easy_perform(easyhandle);
		    fichier_libcurl_check_wait_or_throw(get_ui(),
							err,
							wait_delay,
							tools_printf(gettext("Error met while removing file %S"),
								     &filename));
		}
		while(err != CURLE_OK);

		if(err != CURLE_OK)
		    throw Erange("entrepot_libcurl::inherited_unlink",
				 tools_printf(gettext("Error met while removing file %S: %s"),
					      &filename, curl_easy_strerror(err)));
		break;
	    case proto_http:
	    case proto_https:
	    case proto_scp:
		throw Efeature("unlink http, https, scp");
	    default:
		throw SRC_BUG;
	    }
	}
	catch(...)
	{
	    switch(x_proto)
	    {
	    case proto_ftp:
	    case proto_sftp:
		(void)curl_easy_setopt(easyhandle, CURLOPT_QUOTE, nullptr);
		(void)curl_easy_setopt(easyhandle, CURLOPT_NOBODY, 0);
		break;
	    case proto_http:
	    case proto_https:
	    case proto_scp:
		throw Efeature("unlink http, https, scp");
	    default:
    		break;
	    }

	    if(headers != nullptr)
		curl_slist_free_all(headers);

	    throw;
	}

	switch(x_proto)
	{
	case proto_ftp:
	case proto_sftp:
	    (void)curl_easy_setopt(easyhandle, CURLOPT_QUOTE, nullptr);
	    (void)curl_easy_setopt(easyhandle, CURLOPT_NOBODY, 0);
	    break;
	case proto_http:
	case proto_https:
	case proto_scp:
	    throw Efeature("unlink http, https, scp");
	default:
	    break;
	}

	if(headers != nullptr)
	    curl_slist_free_all(headers);
#else
    throw Efeature("libcurl");
#endif
    }

    void entrepot_libcurl::read_dir_flush()
    {
	current_dir.clear();
    }

    void entrepot_libcurl::set_libcurl_URL()
    {
#if LIBCURL_AVAILABLE
	CURLcode err;
	string target = get_url();

	if(target.size() == 0)
	    throw SRC_BUG;
	else
	    if(target[target.size() - 1] != '/')
		target += "/";

	err = curl_easy_setopt(easyhandle, CURLOPT_URL, target.c_str());
	if(err != CURLE_OK)
	    throw Erange("entrepot_libcurl::set_libcurl_URL",tools_printf(gettext("Failed assigning URL to libcurl: %s"),
									  curl_easy_strerror(err)));
#else
    throw Efeature("libcurl");
#endif
    }

    void entrepot_libcurl::set_libcurl_authentication(const string & location,
						      const string & login,
						      const secu_string & password,
						      bool auth_from_file)
    {
#if LIBCURL_AVAILABLE
	CURLcode err;
	secu_string real_pass = password;
	string real_login = login;

	if(login.empty())
	    real_login = "anonymous";

	if(auth_from_file)
	{
	    err = curl_easy_setopt(easyhandle, CURLOPT_USERNAME, real_login.c_str());
	    if(err != CURLE_OK)
		throw Erange("entrepot_libcurl::set_libcurl_authentication",
			     tools_printf(gettext("Error met while passing username to libcurl: %s"),
					  curl_easy_strerror(err)));

	    switch(x_proto)
	    {
	    case proto_ftp:  // using ~/.netrc for authentication
		err = curl_easy_setopt(easyhandle, CURLOPT_NETRC, CURL_NETRC_OPTIONAL);
		if(err != CURLE_OK)
		    throw Erange("entrepot_libcurl::set_libcurl_authentication",
				 tools_printf(gettext("Error met while asking libcurl to consider ~/.netrc for authentication: %s"),
					      curl_easy_strerror(err)));
		break;
	    case proto_http:
		throw Efeature(gettext("authentication from file with HTTP protocol"));
	    case proto_https:
		throw Efeature(gettext("authentication from file with HTTPS protocol"));
	    case proto_scp:
	    case proto_sftp:
		throw Efeature(gettext("Public key authentication for ssh/sftp"));
		    /*
		err = curl_easy_setopt(easyhandle, CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PUBLICKEY);
		if(err != CURLE_OK)
		    throw Erange("entrepot_libcurl::set_libcurl_authentication",
				 tools_printf(gettext("Error met while instructing libcurl to use public key authentication: %s"),
					      curl_easy_strerror(err)));


		err = curl_easy_setopt(easyhandle, CURLOPT_SSH_PUBLIC_KEYFILE, "/home/.../.ssh/id_rsa.pub");

		err = curl_easy_setopt(easyhandle, CURLOPT_SSH_PRIVATE_KEYFILE, "/home/.../.ssh/id_rsa");

		curl_easy_setopt(easyhandle, CURLOPT_SSH_KNOWNHOSTS, "/home/.../.ssh/known_hosts");
		    */

		break;
	    default:
		throw SRC_BUG;
	    }
	}
	else // login + password authentication
	{
	    if(password.empty())
	    {
		real_pass = get_ui().get_secu_string(tools_printf(gettext("Please provide the password for login %S at host %S: "),
								  &real_login,
								  &location),
						     false);
	    }

	    secu_string auth(real_login.size() + 1 + real_pass.get_size() + 1);

	    auth.append(real_login.c_str(), real_login.size());
	    auth.append(":", 1);
	    auth.append(real_pass.c_str(), real_pass.get_size());

	    err = curl_easy_setopt(easyhandle, CURLOPT_USERPWD, auth.c_str());
	    if(err != CURLE_OK)
		throw Erange("entrepot_libcurl::set_libcurl_authentication",
			     tools_printf(gettext("Error met while setting libcurl authentication: %s"),
					  curl_easy_strerror(err)));
	}
#else
    throw Efeature("libcurl");
#endif
    }

    void entrepot_libcurl::copy_from(const entrepot_libcurl & ref)
    {
#if LIBCURL_AVAILABLE
	entrepot *ent_me = this;
	const entrepot *ent_ref = &ref;
	*ent_me = *ent_ref;

	mem_ui *mem_me = this;
	const mem_ui *mem_ref = &ref;
	*mem_me = *mem_ref;

	x_proto = ref.x_proto;
	base_URL = ref.base_URL;
	current_dir = ref.current_dir;
	reading_dir_tmp = ref.reading_dir_tmp;
	wait_delay = ref.wait_delay;
	if(ref.easyhandle != nullptr)
	{
	    easyhandle = curl_easy_duphandle(ref.easyhandle);
	    if(easyhandle == nullptr)
		throw Erange("entrepot_libcurl::copy_from", string(gettext("Error met while duplicating libcurl handle")));
	}
	else
	    easyhandle = nullptr;
#else
	throw Efeature("libcurl");
#endif
    }

    void entrepot_libcurl::detruit()
    {
#if LIBCURL_AVAILABLE
	curl_easy_cleanup(easyhandle);
#endif
    }

    string entrepot_libcurl::mycurl_protocol2string(mycurl_protocol proto)
    {
	string ret;

	switch(proto)
	{
	case proto_ftp:
	    ret = "ftp";
	    break;
	case proto_http:
	    ret = "http";
	    break;
	case proto_https:
	    ret = "https";
	    break;
	case proto_scp:
	    ret = "scp";
	    break;
	case proto_sftp:
	    ret = "sftp";
	    break;
	default:
	    throw SRC_BUG;
	}

	return ret;
    }

    string entrepot_libcurl::build_url_from(mycurl_protocol proto, const string & host, const string & port)
    {
	string ret = mycurl_protocol2string(proto) + "://" + host;

	if(!port.empty())
	    ret += ":" + port;

	ret += "/";
	    // to have any path added in the future to refer to the root
	    // of the remote repository and not relative to the landing directory

	return ret;
    }

    size_t entrepot_libcurl::get_ftp_listing_callback(void *buffer, size_t size, size_t nmemb, void *userp)
    {
	entrepot_libcurl *me = (entrepot_libcurl *)(userp);
	char *ptr = (char *)buffer;

	if(me == nullptr)
	    return size > 0 ? 0 : 1; // report error to libcurl

	for(unsigned int mi = 0; mi < nmemb; ++mi)
	    for(unsigned int i = 0; i < size; ++i)
	    {
		switch(*ptr)
		{
		case '\n':
		    me->current_dir.push_back(me->reading_dir_tmp);
		    me->reading_dir_tmp.clear();
		    break;
		case '\r':
		    break; // just ignore it
		default: // normal character
		    me->reading_dir_tmp += *ptr;
		    break;
		}
		++ptr;
	    }
	return size*nmemb;
    }

    bool entrepot_libcurl::mycurl_is_protocol_available(mycurl_protocol proto)
    {
	curl_version_info_data *data = curl_version_info(CURLVERSION_NOW);
	const char **ptr = nullptr;
	string named_proto = mycurl_protocol2string(proto);

	if(data == nullptr)
	    throw SRC_BUG;

	ptr = const_cast<const char **>(data->protocols);
	if(ptr == nullptr)
	    throw SRC_BUG;

	while(*ptr != nullptr && strcmp(*ptr, named_proto.c_str()) != 0)
	    ++ptr;

	return *ptr != nullptr;
    }

} // end of namespace
