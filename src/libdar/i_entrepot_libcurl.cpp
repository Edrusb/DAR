//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
}

#include "tools.hpp"
#include "i_entrepot_libcurl.hpp"
#include "fichier_libcurl.hpp"
#include "cache_global.hpp"

using namespace std;

namespace libdar
{

#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )

    entrepot_libcurl::i_entrepot_libcurl::i_entrepot_libcurl(const shared_ptr<user_interaction> & dialog,         //< for user interaction
					   mycurl_protocol proto,             //< network protocol to use
					   const string & login,              //< user login on remote host
					   const secu_string & password,      //< user password on remote host (empty for file auth or user interaction)
					   const string & host,               //< the remote server to connect to
					   const string & port,               //< TCP/UDP port to connec to (empty string for default)
					   bool auth_from_file,               //< whether to check $HOME/.netrc for password
					   const string & sftp_pub_keyfile,   //< where to fetch the public key (sftp only)
					   const string & sftp_prv_keyfile,   //< where to fetch the private key (sftp only)
					   const string & sftp_known_hosts,   //< location of the known_hosts file (empty string to disable this security check)
					   U_I waiting_time): mem_ui(dialog),
							      x_proto(proto),
							      base_URL(build_url_from(proto, host, port)),
							      wait_delay(waiting_time)
    {
	current_dir.clear();
	reading_dir_tmp.clear();

	    // initializing the fields from parent class entrepot

	set_root(path("/"));
	set_location(path("/"));
	set_user_ownership(""); // not used for this type of entrepot
	set_group_ownership(""); // not used for this type of entrepot

	if(!mycurl_is_protocol_available(proto))
	{
	    string named_proto = mycurl_protocol2string(proto);
	    throw Erange("entrepot_libcurl::i_entrepot_libcurl::i_entrepot_libcurl",
			 tools_printf(gettext("protocol %S is not supported by libcurl, aborting"), & named_proto));
	}

	set_libcurl_authentication(*dialog,
				   host,
				   login,
				   password,
				   auth_from_file,
				   sftp_pub_keyfile,
				   sftp_prv_keyfile,
				   sftp_known_hosts);

#ifdef LIBDAR_NO_OPTIMIZATION
	easyh.setopt_global(CURLOPT_VERBOSE, 1);
#endif
    }

    void entrepot_libcurl::i_entrepot_libcurl::read_dir_reset() const
    {
	long listonly;
	i_entrepot_libcurl *me = const_cast<i_entrepot_libcurl *>(this);
	shared_ptr<mycurl_easyhandle_node> node;

	if(me == nullptr)
	    throw SRC_BUG;

	node = me->easyh.alloc_instance();
	if(!node)
	    throw SRC_BUG;

	me->current_dir.clear();
	me->reading_dir_tmp = "";

	switch(x_proto)
	{
	case proto_ftp:
	case proto_sftp:
	    listonly = 1;
	    node->setopt(CURLOPT_URL, get_libcurl_URL());
	    node->setopt(CURLOPT_DIRLISTONLY, listonly);
	    node->setopt(CURLOPT_WRITEFUNCTION, (void*)get_ftp_listing_callback);
	    node->setopt(CURLOPT_WRITEDATA, this);
	    node->apply(get_pointer(), wait_delay);

	    if(!reading_dir_tmp.empty())
	    {
		me->current_dir.push_back(reading_dir_tmp);
		me->reading_dir_tmp.clear();
	    }
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    bool entrepot_libcurl::i_entrepot_libcurl::read_dir_next(string & filename) const
    {
	i_entrepot_libcurl *me = const_cast<i_entrepot_libcurl *>(this);

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
    }

    fichier_global *entrepot_libcurl::i_entrepot_libcurl::inherited_open(const shared_ptr<user_interaction> & dialog,
						       const string & filename,
						       gf_mode mode,
						       bool force_permission,
						       U_I permission,
						       bool fail_if_exists,
						       bool erase) const
    {
	fichier_global *ret = nullptr;
	cache_global *rw = nullptr;
	i_entrepot_libcurl *me = const_cast<i_entrepot_libcurl *>(this);
	gf_mode hidden_mode = mode;

	if(me == nullptr)
	    throw SRC_BUG;

	if(fail_if_exists)
	{
	    string tmp;

	    me->read_dir_reset();
	    while(me->read_dir_next(tmp))
		if(tmp == filename)
		    throw Esystem("i_entrepot_libcurl::inherited_open", "File exists on remote repository" , Esystem::io_exist);
	}

	string chemin = (path(get_url(), true).append(filename)).display();

	if(hidden_mode == gf_read_write)
	    hidden_mode = gf_write_only;

	try
	{
	    fichier_libcurl *ret_libcurl = new (nothrow) fichier_libcurl(dialog,
									 chemin,
									 x_proto,
									 me->easyh.alloc_instance(),
									 hidden_mode,
									 wait_delay,
									 force_permission,
									 permission,
									 erase);
	    ret = ret_libcurl;

	    if(ret == nullptr)
		throw Ememory("entrepot_libcurl::i_entrepot_libcurl::inherited_open");

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
    }

    void entrepot_libcurl::i_entrepot_libcurl::inherited_unlink(const string & filename) const
    {
	mycurl_slist headers;
	shared_ptr<mycurl_easyhandle_node> node;
	string order;
	i_entrepot_libcurl *me = const_cast<i_entrepot_libcurl *>(this);

	if(me == nullptr)
	    throw SRC_BUG;

	node = me->easyh.alloc_instance();
	if(!node)
	    throw SRC_BUG;

	switch(x_proto)
	{
	case proto_ftp:
	case proto_sftp:
	    if(x_proto == proto_ftp)
		order = "DELE " + filename;
	    else
		order = "rm " + filename;
	    headers.append(order);
	    node->setopt(CURLOPT_QUOTE, headers);
	    node->setopt(CURLOPT_URL, get_libcurl_URL());
	    node->setopt(CURLOPT_NOBODY, 1);
	    node->apply(get_pointer(), wait_delay);
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    void entrepot_libcurl::i_entrepot_libcurl::read_dir_flush()
    {
	current_dir.clear();
    }

    string entrepot_libcurl::i_entrepot_libcurl::get_libcurl_URL() const
    {
	string target = get_url();

	if(target.size() == 0)
	    throw SRC_BUG;
	else
	    if(target[target.size() - 1] != '/')
		target += "/";

	return target;
    }

    void entrepot_libcurl::i_entrepot_libcurl::set_libcurl_authentication(user_interaction & dialog,
									  const string & location,
									  const string & login,
									  const secu_string & password,
									  bool auth_from_file,
									  const string & sftp_pub_keyfile,
									  const string & sftp_prv_keyfile,
									  const string & sftp_known_hosts)
    {
	secu_string real_pass = password;
	string real_login = login;


	    // checking server authentication

	switch(x_proto)
	{
	case proto_ftp:
	    break;
	case proto_sftp:
	    if(!sftp_known_hosts.empty())
		easyh.setopt_global(CURLOPT_SSH_KNOWNHOSTS, sftp_known_hosts);
	    else
		dialog.message("Warning: known_hosts file check has been disabled, connecting to remote host is subjet to man-in-the-middle attack and login/password credential for remote sftp server to be stolen");

	    easyh.setopt_global(CURLOPT_SSH_PUBLIC_KEYFILE, sftp_pub_keyfile);
	    easyh.setopt_global(CURLOPT_SSH_PRIVATE_KEYFILE, sftp_prv_keyfile);
	    easyh.setopt_global(CURLOPT_SSH_AUTH_TYPES, CURLSSH_AUTH_PUBLICKEY|CURLSSH_AUTH_PASSWORD);
	    break;
	default:
	    throw SRC_BUG;
	}

	    // checking for user credentials

	if(real_login.empty())
	    real_login = "anonymous";

	if(auth_from_file)
	{
	    easyh.setopt_global(CURLOPT_USERNAME, real_login);
	    easyh.setopt_global(CURLOPT_NETRC, CURL_NETRC_OPTIONAL);
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

	    easyh.setopt_global(CURLOPT_USERPWD, auth);
	}

    }

    string entrepot_libcurl::i_entrepot_libcurl::mycurl_protocol2string(mycurl_protocol proto)
    {
	string ret;

	switch(proto)
	{
	case proto_ftp:
	    ret = "ftp";
	    break;
	case proto_sftp:
	    ret = "sftp";
	    break;
	default:
	    throw SRC_BUG;
	}

	return ret;
    }

    string entrepot_libcurl::i_entrepot_libcurl::build_url_from(mycurl_protocol proto, const string & host, const string & port)
    {
	string ret = mycurl_protocol2string(proto) + "://" + host;

	if(!port.empty())
	    ret += ":" + port;

	ret += "/";
	    // to have any path added in the future to refer to the root
	    // of the remote repository and not relative to the landing directory

	return ret;
    }

    size_t entrepot_libcurl::i_entrepot_libcurl::get_ftp_listing_callback(void *buffer, size_t size, size_t nmemb, void *userp)
    {
	i_entrepot_libcurl *me = (i_entrepot_libcurl *)(userp);
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

    bool entrepot_libcurl::i_entrepot_libcurl::mycurl_is_protocol_available(mycurl_protocol proto)
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

#endif

} // end of namespace
