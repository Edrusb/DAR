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

#include "tools.hpp"
#include "entrepot_libcurl.hpp"
#include "fichier_libcurl.hpp"

using namespace std;

namespace libdar
{

    entrepot_libcurl::entrepot_libcurl(curl_protocol proto,
				       const string & login,
				       const secu_string & password,
				       const string & host,
				       const string & port): x_proto(proto),
							     base_URL(build_url_from(proto, host, port))
    {
	current_dir.clear();
	reading_dir_tmp.clear();

	if(!login.empty())
	{
	    auth.resize(login.size() + 1 + password.get_size() + 1);
	    auth.append(login.c_str(), login.size());
	    auth.append(":", 1);
	    auth.append(password.c_str(), password.get_size());
	}
	else
	    auth.clear();

	    // initializing the fields from parent class entrepot

	set_root("/");
	set_location("/");
	set_user_ownership(""); // not used for this type of entrepot
	set_group_ownership(""); // not used for this type of entrepot

	easyhandle = curl_easy_init();
	if(easyhandle == nullptr)
	    throw Erange("entrepot_libcurl::entrepot_libcurl", string(gettext("Error met while creating a libcurl handle")));
	set_libcurl_URL();
	set_libcurl_authentication();
    }

    void entrepot_libcurl::read_dir_reset()
    {
	CURLcode err;
	long listonly;

	current_dir.clear();
	reading_dir_tmp = "";
	set_libcurl_URL();

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

		err = curl_easy_perform(easyhandle);
		if(err != CURLE_OK)
		    throw Erange("entrepot_libcurl::inherited_unlink",
				 tools_printf(gettext("Error met while listing FTP directory %s: %s"),
					      get_url().c_str(),
					      curl_easy_strerror(err)));
		if(!reading_dir_tmp.empty())
		{
		    current_dir.push_back(reading_dir_tmp);
		    reading_dir_tmp.clear();
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
    }

    bool entrepot_libcurl::read_dir_next(string & filename)
    {
	if(current_dir.empty())
	    return false;
	else
	{
	    filename = current_dir.front();
	    current_dir.pop_front();
	    return true;
	}
    }

    fichier_global *entrepot_libcurl::inherited_open(user_interaction & dialog,
						     const string & filename,
						     gf_mode mode,
						     bool force_permission,
						     U_I permission,
						     bool fail_if_exists,
						     bool erase) const
    {
	fichier_libcurl *ret = nullptr;
	entrepot_libcurl *me = const_cast<entrepot_libcurl *>(this);

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

	ret = new (get_pool()) fichier_libcurl(dialog,
					       chemin,
					       easyhandle,
					       mode,
					       force_permission,
					       permission,
					       erase);
	if(ret == nullptr)
	    throw Ememory("entrepot_libcurl::inherited_open");

	return ret;
    }

    void entrepot_libcurl::inherited_unlink(const string & filename) const
    {
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
		err = curl_easy_perform(easyhandle);
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
    }

    void entrepot_libcurl::read_dir_flush()
    {
	current_dir.clear();
    }

    void entrepot_libcurl::set_libcurl_URL()
    {
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
    }

    void entrepot_libcurl::set_libcurl_authentication()
    {
	CURLcode err;

	err = curl_easy_setopt(easyhandle, CURLOPT_USERPWD, auth.c_str());
	if(err != CURLE_OK)
	    throw Erange("entrepot_libcurl::handle_reset", tools_printf(gettext("Error met while setting libcurl authentication: %s"),
									curl_easy_strerror(err)));
    }

    void entrepot_libcurl::copy_from(const entrepot_libcurl & ref)
    {
	base_URL = ref.base_URL;
	auth = ref.auth;
	current_dir = ref.current_dir;
	if(ref.easyhandle != nullptr)
	{
	    easyhandle = curl_easy_duphandle(ref.easyhandle);
	    if(easyhandle == nullptr)
		throw Erange("entrepot_libcurl::copy_from", string(gettext("Error met while duplicating libcurl handle")));
	}
	else
	    easyhandle = nullptr;
    }

    void entrepot_libcurl::detruit()
    {
	curl_easy_cleanup(easyhandle);
    }

    string entrepot_libcurl::curl_protocol2string(curl_protocol proto)
    {
	string ret;

	switch(proto)
	{
	case proto_ftp:
	    ret = "ftp://";
	    break;
	case proto_http:
	    ret = "http://";
	    break;
	case proto_https:
	    ret = "https://";
	    break;
	case proto_scp:
	    ret = "scp://";
	    break;
	case proto_sftp:
	    ret = "sftp://";
	    break;
	default:
	    throw SRC_BUG;
	}

	return ret;
    }

    string entrepot_libcurl::build_url_from(curl_protocol proto, const string & host, const string & port)
    {
	string ret = curl_protocol2string(proto) + host;

	if(!port.empty())
	    ret += ":" + port;

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

} // end of namespace
