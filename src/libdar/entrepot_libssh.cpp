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
}

#include "tools.hpp"
// #include "fichier_libssh.hpp"
#include "nls_swap.hpp"
#include "entrepot_libssh.hpp"

using namespace std;

namespace libdar
{

    entrepot_libssh::entrepot_libssh(const shared_ptr<user_interaction> & dialog,
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
	mem_ui(dialog),
	x_login(login),
	x_password(password),
	x_host(host),
	x_port(port),
	x_auth_from_file(auth_from_file),
	x_sftp_pub_keyfile(sftp_pub_keyfile),
	x_sftp_prv_keyfile(sftp_prv_keyfile),
	x_sftp_known_hosts(sftp_known_hosts),
	x_waiting_time(x_waiting_time),
	x_verbose(verbose),
	sess(nullptr),
	sftp_sess(nullptr),
	sdir(nullptr)
    {
	set_root(path("/"));
	set_location(path("/"));
	set_user_ownership(""); // not used for this type of entrepot //// <<< A REVOIR
	set_group_ownership(""); // not used for this type of entrepot /// <<<< A REVOIR

	init();
    }

    entrepot_libssh::entrepot_libssh(const entrepot_libssh & ref):
	mem_ui(ref),
	x_login(ref.x_login),
	x_password(ref.x_password),
	x_host(ref.x_host),
	x_port(ref.x_port),
	x_auth_from_file(ref.x_auth_from_file),
	x_sftp_pub_keyfile(ref.x_sftp_pub_keyfile),
	x_sftp_prv_keyfile(ref.x_sftp_prv_keyfile),
	x_sftp_known_hosts(ref.x_sftp_known_hosts),
	x_waiting_time(ref.x_waiting_time),
	x_verbose(ref.x_verbose),
	sess(nullptr),
	sftp_sess(nullptr),
	sdir(nullptr)
    {
	set_root(ref.get_root());
	set_location(ref.get_location());
	set_user_ownership(ref.get_user_ownership());
	set_group_ownership(ref.get_group_ownership());

    	init();
    }

    string entrepot_libssh::get_url() const
    {
	string ret = string("sftp://") + x_login + "@" + x_host;

	if(!x_port.empty())
	    ret += ":" + x_port;

	ret += get_full_path().display();

	return ret;
    }

    void entrepot_libssh::read_dir_reset() const
    {
	read_dir_reset_dirinfo();
    }

    bool entrepot_libssh::read_dir_next(std::string & filename) const
    {
	inode_type unused;
	return read_dir_next_dirinfo(filename, unused);
    }

    void entrepot_libssh::read_dir_reset_dirinfo() const
    {
	string where = get_full_path().display();

	if(sdir != nullptr)
	{
	    read_dir_flush();
	    if(sdir != nullptr)
		throw SRC_BUG;
	}

	sdir = sftp_opendir(sftp_sess, where.c_str());
	if(sdir == nullptr)
	    throw Erange("entrepot_libssh::read_dir_reset_dirinfo",
			 tools_printf(gettext("Could not open directory %s: %s"),
				      where.c_str(),
				      ssh_get_error(sess)));
    }

    bool entrepot_libssh::read_dir_next_dirinfo(std::string & filename, inode_type & tp) const
    {
	sftp_attributes attrib;

	if(sdir == nullptr)
	    throw Erange("entrepot_libssh::read_dir_next_dirinfo",
			 gettext("No directory has been openned, cannot read a directory content"));

	attrib = sftp_readdir(sftp_sess, sdir);
	if(attrib != nullptr)
	{
	    filename = attrib->name;
	    switch(attrib->type & S_IFMT)
	    {
	    case S_IFDIR:
		tp = inode_type::isdir;
		break;
	    case S_IFLNK:
		tp = inode_type::symlink;
		break;
	    default:
		tp = inode_type::nondir;
	    }

	    return true;
	}
	else
	{
	    if(!sftp_dir_eof(sdir))
		throw Erange("Entrepot_libssh::read_dir_next_dirinfo",
			     tools_printf(gettext("Failed getting next entry of directory %s: %s"),
					  get_full_path().display().c_str(),
					  get_sftp_error_msg(sftp_get_error(sftp_sess))));
	    read_dir_flush();
	    return false;
	}
    }

    void entrepot_libssh::create_dir(const std::string & dirname, U_I permission)
    {
	path where = get_full_path().append(dirname);
	int code = sftp_mkdir(sftp_sess, where.display().c_str(), permission);

	if(code != SSH_OK)
	    throw Erange("entrepot_libss::create_dir",
			 tools_printf(gettext("Failed creating directory %s: %s"),
				      where.display().c_str(),
				      get_sftp_error_msg(sftp_get_error(sftp_sess))));

    }

    fichier_global* entrepot_libssh::inherited_open(const std::shared_ptr<user_interaction> & dialog,
						    const std::string & filename,
						    gf_mode mode,
						    bool force_permission,
						    U_I permission,
						    bool fail_if_exists,
						    bool erase) const
    {

	return nullptr;
    }

    void entrepot_libssh::inherited_unlink(const std::string & filename) const
    {
	path where = get_full_path().append(filename);
	int code = sftp_unlink(sftp_sess, where.display().c_str());

	if(code != SSH_OK)
	    throw Erange("entrepot_libss::create_dir",
			 tools_printf(gettext("Failed delete entry %s: %s"),
				      where.display().c_str(),
				      get_sftp_error_msg(sftp_get_error(sftp_sess))));
    }

    void entrepot_libssh::read_dir_flush() const
    {
	if(sdir != nullptr)
	{
	    sftp_closedir(sdir);
	    sdir = nullptr;
	}
    }


    void entrepot_libssh::init()
    {
	bool loop = true;

	do
	{
	    try
	    {
		create_session();

		try
		{
		    server_authentication();
		    user_authentication();
		    create_sftp_session();
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
		    get_ui().message(tools_printf(gettext("Waiting %d seconds and retring connection due to: %S"),
						  x_waiting_time,
						  e.get_message()));
		    sleep(x_waiting_time);
		}
		else
		    throw;
	    }
	}
	while(loop); // loop only ends upon success or exception thrown
    }

    void entrepot_libssh::create_session()
    {
	int conn_status = SSH_ERROR;

	sess = ssh_new();
	if(sess == nullptr)
	    throw Ememory("entrepot_libssh::entrepot_libssh");

	try
	{
	    int ssh_verbosity;

	    ssh_options_set(sess, SSH_OPTIONS_HOST, x_host.c_str());
	    ssh_options_set(sess, SSH_OPTIONS_PORT_STR, x_port.c_str());
	    ssh_options_set(sess, SSH_OPTIONS_USER, x_login.c_str());
	    if(! x_sftp_known_hosts.empty())
		ssh_options_set(sess, SSH_OPTIONS_KNOWNHOSTS, x_sftp_known_hosts.c_str());
	    if(x_verbose)
		ssh_verbosity = SSH_LOG_PROTOCOL;
	    else
		ssh_verbosity = SSH_LOG_WARNING;
	    ssh_options_set(sess, SSH_OPTIONS_LOG_VERBOSITY, &ssh_verbosity);

	    ssh_verbosity = 1; // recycling ssh_verbosity assuming it is no more access by previou ssh_option_set call
	    ssh_options_set(sess, SSH_OPTIONS_STRICTHOSTKEYCHECK, &ssh_verbosity);
	    if(! x_sftp_pub_keyfile.empty() && ! x_sftp_prv_keyfile.empty())
		ssh_options_set(sess, SSH_OPTIONS_PUBKEY_AUTH, &ssh_verbosity);

	    conn_status = ssh_connect(sess);
	    if(conn_status != SSH_OK)
		throw Erange("entrepot_libssh::entrepot_libssh",
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


    void entrepot_libssh::server_authentication()
    {
	switch(ssh_session_is_known_server(sess))
	{
	case SSH_KNOWN_HOSTS_OK:
		// ok
	    break;
	case SSH_KNOWN_HOSTS_CHANGED:
	    throw Erange("entrepot_libssh::server_authentication",
			 gettext("Server key has changed, first update the knownhosts file with server authority provided information, then retry"));
	case SSH_KNOWN_HOSTS_OTHER:
	    throw Erange("entrepot_libssh::server_authentication",
			 gettext("Server provided key type is new from this server, first update the knownhosts file with server authority provided information, then retry"));
	case SSH_KNOWN_HOSTS_UNKNOWN:
	    throw Erange("entrepot_libssh::server_authentication",
			 gettext("This is the first time we connect to this server, first update the knownhosts file with server authority provided information, then retry"));
	case SSH_KNOWN_HOSTS_NOT_FOUND:
	    throw Erange("entrepot_libssh::server_authentication",
			 gettext("the knownhosts file is not accessible, cannot check the authenticity of the received server key; double check you provided correct parameters for the knownhosts file"));
	case SSH_KNOWN_HOSTS_ERROR:
	    throw Erange("entrepot_libssh::server_authentication",
			 gettext("There had been an error checking the host"));
	default:
	    throw SRC_BUG; // unexpected returned value from libssh
	}
    }

    void entrepot_libssh::user_authentication()
    {
	secu_string real_pass = x_password;
	ssh_key prvkey = nullptr;
	ssh_key pubkey = nullptr;
	int code;

	if(! x_auth_from_file) // password authentication
	{
	    if(x_password.empty())
	    {
		real_pass = get_ui().get_secu_string(tools_printf(gettext("Please provide the password for login %S at host %S: "),
								  &x_login,
								  &x_host),
						     false);
	    }

	    if(ssh_userauth_password(sess, NULL, x_password.c_str()) != SSH_OK)
		throw Enet_auth(tools_printf(gettext("Authentication failure: %s"),
					     ssh_get_error(sess)));
	}
	else // public/private key pair authentication (password field if not empty is used as key passphrase)
	{

		// loading the public key from file

	    code = ssh_pki_import_pubkey_file(x_sftp_pub_keyfile.c_str(), &pubkey);
	    if(code != SSH_OK)
		throw Erange("entrepot_libssh::user_authentication",
			     tools_printf(gettext("Failed loading the public key: %s"),
					  get_key_error_msg(code)));

	    try
	    {
		string msg;

		    // checking if this public key would be an acceptable authentication method

		code = ssh_userauth_try_publickey(sess, NULL, pubkey);
		if(code != SSH_AUTH_SUCCESS)
		    throw Erange("entrepot_ssh::user_authentication",
				 tools_printf(gettext("Failed public key authentication: %s"),
					      get_auth_error_msg(code)));

		    // loading the private key (eventually using the password as pass for the key

		code = ssh_pki_import_privkey_file(x_sftp_prv_keyfile.c_str(),
						   x_password.empty() ? NULL : x_password.c_str(),
						   NULL,
						   NULL,
						   &prvkey);

		if(code != SSH_OK)
		    throw Erange("entrepot_libssh::user_authentication",
				 tools_printf(gettext("Failed loading the private key: %s"),
					      get_key_error_msg(code)));
		try
		{
		    code = ssh_userauth_publickey(sess,
						  x_login.c_str(),
						  prvkey);
		    if(code != SSH_AUTH_SUCCESS)
			throw Erange("entrepot_ssh::user_authentication",
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


    void entrepot_libssh::create_sftp_session()
    {
	sftp_sess = sftp_new(sess);
	int code;

	if(sftp_sess == nullptr)
	    throw Erange("entrepot_libcurl::create_sftp_session",
			 tools_printf(gettext("Error allocating SFTP session: %s"),
				      ssh_get_error(sess)));

	code = sftp_init(sftp_sess);
	if(code != SSH_OK)
	    throw Erange("entrepot_libcurl::create_sftp_session",
			 tools_printf(gettext("Error initializing SFTP session: %s"),
				      get_sftp_error_msg(sftp_get_error(sftp_sess))));
    }

    void entrepot_libssh::cleanup_session()
    {
	read_dir_flush();

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


    const char* entrepot_libssh::get_key_error_msg(int code)
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

    const char* entrepot_libssh::get_auth_error_msg(int code)
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

    const char* entrepot_libssh::get_sftp_error_msg(int code)
    {
	switch(code)
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

} // end of namespace
