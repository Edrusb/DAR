//*********************************************************************/
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

#include "../my_config.h"

extern "C"
{
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
}

#include "tools.hpp"
#include "nls_swap.hpp"

#include "libssh_connection.hpp"

using namespace std;

namespace libdar
{
#if LIBSSH_AVAILABLE

    libssh_connection::libssh_connection(const shared_ptr<user_interaction> & dialog,
					 const string & login,
					 const secu_string & password,
					 const string & host,
					 const string & port,
					 bool auth_from_file,
					 const string & sftp_pub_keyfile,
					 const string & sftp_prv_keyfile,
					 const string & sftp_known_hosts,
					 U_I waiting_time,
					 bool verbose):
	sess(nullptr),
	sftp_sess(nullptr),
	waiting(waiting_time)
    {
	bool loop = true;

	if(!dialog)
	    throw SRC_BUG;

	do
	{
	    try
	    {
		create_session(host,
			       port,
			       login,
			       sftp_known_hosts,
			       verbose,
			       sftp_pub_keyfile,
			       sftp_prv_keyfile);

		try
		{
		    server_authentication(*dialog);
		    user_authentication(*dialog,
					password,
					auth_from_file,
					login,
					host,
					sftp_pub_keyfile,
					sftp_prv_keyfile);
		    create_sftp_session();
		    set_max_limits();
		    loop = false; // no exception thrown si far, thus "this" object is fully constructed
		}
		catch(...)
		{
		    cleanup_session();
		    throw;
		}
	    }
	    catch(Egeneric & e)
	    {
		Erange* e_r = dynamic_cast<Erange*>(&e);
		Ememory* e_m = dynamic_cast<Ememory*>(&e);

		if(e_r != nullptr
		   || e_m != nullptr)
		{
		    dialog->message(tools_printf(gettext("Waiting %d seconds and retring connection due to: %s"),
						 waiting_time,
						 e.get_message().c_str()));
		    sleep(waiting_time);
		}
		else
		    throw;
	    }
	}
	while(loop); // loop only ends upon success or exception thrown
    }

    const char* libssh_connection::get_sftp_error_msg() const
    {
	switch(sftp_get_error(sftp_sess))
	{
	case SSH_FX_OK:
	    return "no error";
	case SSH_FX_EOF:
	    return "end-of-file encountered";
	case SSH_FX_NO_SUCH_FILE:
	    return "file does not exist";
	case SSH_FX_PERMISSION_DENIED:
	    return "permission denied";
	case SSH_FX_FAILURE:
	    return "generic failure";
	case SSH_FX_BAD_MESSAGE:
	    return "garbage received from server";
	case SSH_FX_NO_CONNECTION:
	    return "no connection has been set up";
	case SSH_FX_CONNECTION_LOST:
	    return "there was a connection, but we lost it";
	case SSH_FX_OP_UNSUPPORTED:
	    return "operation not supported by libssh yet";
	case SSH_FX_INVALID_HANDLE:
	    return "invalid file handle";
	case SSH_FX_NO_SUCH_PATH:
	    return "no such file or directory path exists";
	case SSH_FX_FILE_ALREADY_EXISTS:
	    return "an attempt to create an already existing file or directory has been made";
	case SSH_FX_WRITE_PROTECT:
	    return "write-protected filesystem";
	case SSH_FX_NO_MEDIA:
	    return "no media was in remote drive";
	default:
	    throw SRC_BUG; // unknown error code from libssh
	}
    }

    void libssh_connection::create_session(const string & host,
					   const string & port,
					   const string & login,
					   const string sftp_known_hosts,
					   bool verbose,
					   const string sftp_pub_keyfile,
					   const string & sftp_prv_keyfile)
    {
	int conn_status = SSH_ERROR;

	sess = ssh_new();
	if(sess == nullptr)
	    throw Ememory("libssh_connection::entrepot_libssh");

	try
	{
	    int ssh_verbosity;

	    if(host.empty())
		throw Erange("libssh_connection", gettext("An empty string is not a valid hostname or IP to connect to"));
	    ssh_options_set(sess, SSH_OPTIONS_HOST, host.c_str());

	    if(! port.empty())
		ssh_options_set(sess, SSH_OPTIONS_PORT_STR, port.c_str());

	    if(login.empty())
		throw Erange("libssh_connection", gettext("An empty string is not a valid login name for an sftp connection"));
	    ssh_options_set(sess, SSH_OPTIONS_USER, login.c_str());

	    if(! sftp_known_hosts.empty())
		ssh_options_set(sess, SSH_OPTIONS_KNOWNHOSTS, sftp_known_hosts.c_str());

	    if(verbose)
		ssh_verbosity = SSH_LOG_PROTOCOL;
	    else
		ssh_verbosity = SSH_LOG_NOLOG;
	    ssh_options_set(sess, SSH_OPTIONS_LOG_VERBOSITY, &ssh_verbosity);

	    ssh_verbosity = 1; // recycling ssh_verbosity assuming it is no more access by previou ssh_option_set call
	    ssh_options_set(sess, SSH_OPTIONS_STRICTHOSTKEYCHECK, &ssh_verbosity);
	    if(! sftp_pub_keyfile.empty() && ! sftp_prv_keyfile.empty())
		ssh_options_set(sess, SSH_OPTIONS_PUBKEY_AUTH, &ssh_verbosity);

	    conn_status = ssh_connect(sess);
	    if(conn_status != SSH_OK)
		throw Erange("libssh_connection::create_session",
			     tools_printf(gettext("Error initializing sftp connexion: %s"),
					  ssh_get_error(sess)));
		    }
	catch(...)
	{
	    if(conn_status == SSH_OK)
		ssh_disconnect(sess);
	    ssh_free(sess);
	    throw;
	}
    }


    void libssh_connection::server_authentication(user_interaction & dialog)
    {
	ssh_key srv_pubkey = nullptr;
	unsigned char* hash = nullptr;
	size_t hlen = 0;
	char* finger = nullptr;
	int code;

	try
	{
	    code = ssh_get_server_publickey(sess, &srv_pubkey);
	    if(code != SSH_OK)
		throw Erange("libssh_connection::server_authentication",
			     tools_printf(gettext("Cannot obtain the ssh/sftp server public key: %s"),
					  ssh_get_error(sess)));

	    code = ssh_get_publickey_hash(srv_pubkey,
					  SSH_PUBLICKEY_HASH_SHA256, // ssh_publickey_hash_type
					  &hash,
					  &hlen);
	    if(code != SSH_OK)
		throw Erange("libssh_connection::server_authentication",
			     tools_printf(gettext("Could not obtain a hash from the ssh/sftp server public key: %s"),
					  ssh_get_error(sess)));


	    switch(ssh_session_is_known_server(sess))
	    {
	    case SSH_KNOWN_HOSTS_OK:
		    // ok
		break;
	    case SSH_KNOWN_HOSTS_CHANGED:
		throw Erange("libssh_connection::server_authentication",
			     gettext("Server key has changed, first update the knownhosts file with server authority provided information, then retry"));
	    case SSH_KNOWN_HOSTS_OTHER:
		throw Erange("libssh_connection::server_authentication",
			     gettext("Server provided key type is new from this server, first update the knownhosts file with server authority provided information, then retry"));
	    case SSH_KNOWN_HOSTS_UNKNOWN:   // host not found in knownhosts file
	    case SSH_KNOWN_HOSTS_NOT_FOUND: // no knownhosts file found
		finger = ssh_get_fingerprint_hash(SSH_PUBLICKEY_HASH_SHA256, hash, hlen);
		if(finger == nullptr)
		throw Erange("libssh_connection::server_authentication",
			     tools_printf(gettext("Could not obtain hash representation of the server public key hash: %s"),
					  ssh_get_error(sess)));

		dialog.message(gettext("Unauthenticated server, please ask the ssh/sftp server admin for the server key hashs [ for x in /etc/ssh/*.pub ; do ssh-keygen -l -f \"$x\" ; done ]"));
		dialog.pause(tools_printf(gettext("Does one of these key hashs matches the following one: %s"), finger));
		code = ssh_session_update_known_hosts(sess);
		break;
	    case SSH_KNOWN_HOSTS_ERROR:
		throw Erange("libssh_connection::server_authentication",
			     gettext("An error occurred while checking the host presence in the knownhosts file"));
	    default:
		throw SRC_BUG; // unexpected returned value from libssh
	    }
	}
	catch(...)
	{
	    if(finger != nullptr)
		ssh_string_free_char(finger);
	    if(hash != nullptr)
		ssh_clean_pubkey_hash(&hash);
	    if(srv_pubkey != nullptr)
		ssh_key_free(srv_pubkey);
	    throw;
	}
	if(finger != nullptr)
	    ssh_string_free_char(finger);
	if(hash != nullptr)
	    ssh_clean_pubkey_hash(&hash);
	if(srv_pubkey != nullptr)
	    ssh_key_free(srv_pubkey);
    }

    void libssh_connection::user_authentication(user_interaction & dialog,
						const secu_string & password,
						bool auth_from_file,
						const string & login,
						const string & host,
						const string & sftp_pub_keyfile,
						const string & sftp_prv_keyfile)
    {
	secu_string real_pass = password;
	ssh_key prvkey = nullptr;
	ssh_key pubkey = nullptr;
	int code;

	if(! auth_from_file) // password authentication
	{
	    if(password.empty())
	    {
		real_pass = dialog.get_secu_string(tools_printf(gettext("Please provide the password for login %S at host %S: "),
								  & login,
								  & host),
						     false);
	    }

	    if(real_pass.empty())
		code = ssh_userauth_password(sess, nullptr, "");
	    else
		code = ssh_userauth_password(sess, nullptr, real_pass.c_str());

	    if(code != SSH_OK)
		throw Enet_auth(tools_printf(gettext("Authentication failure: %s"),
					     ssh_get_error(sess)));
	}
	else // public/private key pair authentication (password field if not empty is used as key passphrase)
	{

		// loading the public key from file

	    code = ssh_pki_import_pubkey_file(sftp_pub_keyfile.c_str(), &pubkey);
	    if(code != SSH_OK)
		throw Erange("libssh_connection::user_authentication",
			     tools_printf(gettext("Failed loading the public key: %s"),
					  get_key_error_msg(code)));

	    try
	    {
		string msg;

		    // checking if this public key would be an acceptable authentication method

		code = ssh_userauth_try_publickey(sess, nullptr, pubkey);
		if(code != SSH_AUTH_SUCCESS)
		    throw Erange("entrepot_ssh::user_authentication",
				 tools_printf(gettext("Failed public key authentication: %s"),
					      get_auth_error_msg(code)));

		    // loading the private key (eventually using the password as pass for the key

		code = ssh_pki_import_privkey_file(sftp_prv_keyfile.c_str(),
						   password.empty() ? nullptr : password.c_str(),
						   nullptr,
						   nullptr,
						   &prvkey);

		if(code != SSH_OK && password.empty())
		{
		    real_pass = dialog.get_secu_string(gettext("Failed to load the key pair without passphrase. Please provide the passphrase for this key: "),
						      false);

		    code = ssh_pki_import_privkey_file(sftp_prv_keyfile.c_str(),
						       real_pass.empty() ? nullptr : real_pass.c_str(),
						       nullptr,
						       nullptr,
						       &prvkey);
		}

		if(code != SSH_OK)
		    throw Erange("libssh_connection::user_authentication",
				 tools_printf(gettext("Failed loading the private key: %s"),
					      get_key_error_msg(code)));
		try
		{
		    code = ssh_userauth_publickey(sess,
						  login.c_str(),
						  prvkey);
		    if(code != SSH_AUTH_SUCCESS)
			throw Erange("libssh_connection::user_authentication",
				     tools_printf(gettext("Failed public/private key authentication: %s"),
						  get_auth_error_msg(code)));
		}
		catch(...)
		{
		    ssh_key_free(prvkey);
		    throw;
		}
		ssh_key_free(prvkey);
	    }
	    catch(...)
	    {
		ssh_key_free(pubkey);
		throw;
	    }
	    ssh_key_free(pubkey);
	}
    }


    void libssh_connection::create_sftp_session()
    {
	sftp_sess = sftp_new(sess);
	int code;

	if(sftp_sess == nullptr)
	    throw Erange("libssh_connection::create_sftp_session",
			 tools_printf(gettext("Error allocating SFTP session: %s"),
				      ssh_get_error(sess)));

	code = sftp_init(sftp_sess);
	if(code != SSH_OK)
	    throw Erange("libssh_connection::create_sftp_session",
			 tools_printf(gettext("Error initializing SFTP session: %s"),
				      get_sftp_error_msg()));
    }

    void libssh_connection::set_max_limits()
    {
	sftp_limits_t lim = sftp_limits(sftp_sess);

	max_read = lim->max_read_length;
	max_write = lim->max_write_length;

	if(max_read != lim->max_read_length)
	    throw SRC_BUG;
	if(max_write != lim->max_write_length)
	    throw SRC_BUG;
    }

    void libssh_connection::cleanup_session()
    {
	if(sftp_sess != nullptr)
	{
	    sftp_free(sftp_sess);
	    sftp_sess = nullptr;
	}

	if(sess != nullptr)
	{
	    ssh_disconnect(sess);
	    ssh_free(sess);
	    sess = nullptr;
	}
    }

    const char* libssh_connection::get_key_error_msg(int code)
    {
	switch(code)
	{
	case SSH_OK:
	    return "";
	case SSH_EOF:
	    return "file does not exist or permission denied";
	case SSH_ERROR:
	    return "undefined error from libssh library";
	default:
	    throw SRC_BUG;
	}
    }

    const char* libssh_connection::get_auth_error_msg(int code)
    {
	switch(code)
	{
	case SSH_AUTH_ERROR:
	    return "A serious error happened";
	case SSH_AUTH_DENIED:
	    return "The server doesn't accept that public key as an authentication token. Try another key or another method.";
	case SSH_AUTH_PARTIAL:
	    return "You've been partially authenticated, you still have to use another method.";
	case SSH_AUTH_AGAIN:
	    return "In nonblocking mode, you've got to call this again later.";
	default:
	    throw SRC_BUG; // unexpected error code from libssh
	}
    }

#endif

} // end of namespace
