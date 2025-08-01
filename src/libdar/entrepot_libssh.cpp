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
#include "fichier_libssh.hpp"
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
	mem_ui(dialog)
#if LIBSSH_AVAILABLE
	,
	sdir(nullptr)
#endif
    {
#if LIBSSH_AVAILABLE
	set_root(path("/"));
	set_location(path("/"));
	set_user_ownership(""); // not used for this type of entrepot //// <<< A REVOIR
	set_group_ownership(""); // not used for this type of entrepot /// <<<< A REVOIR

	connect.reset(new (nothrow) libssh_connection(dialog,
						      login,
						      password,
						      host,
						      port,
						      auth_from_file,
						      sftp_pub_keyfile,
						      sftp_prv_keyfile,
						      sftp_known_hosts,
						      waiting_time,
						      verbose));
	if(!connect)
	    throw Ememory("entrepot_libssh::entrepot_libssh");

	server_url = "sftp://" + login + "@" + host;
	if(!port.empty())
	    server_url += ":" + port;
#else
	throw Efeature("SFTP repository (requires libssh)");
#endif
    }

    entrepot_libssh::entrepot_libssh(const entrepot_libssh & ref):
	mem_ui(ref)
#if LIBSSH_AVAILABLE
	,
	server_url(ref.server_url),
	sdir(nullptr),
	connect(ref.connect)
#endif
    {
	set_root(ref.get_root());
	set_location(ref.get_location());
	set_user_ownership(ref.get_user_ownership());
	set_group_ownership(ref.get_group_ownership());
    }

    string entrepot_libssh::get_url() const
    {
#if LIBSSH_AVAILABLE
	return server_url + get_full_path().display();
#else
	throw Efeature("SFTP repository (requires libssh)");
#endif
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
#if LIBSSH_AVAILABLE
	string where = get_full_path().display();

	if(sdir != nullptr)
	{
	    read_dir_flush();
	    if(sdir != nullptr)
		throw SRC_BUG;
	}

	if(!connect)
	    throw SRC_BUG;

	sdir = sftp_opendir(connect->get_sftp_session(), where.c_str());
	if(sdir == nullptr)
	    throw Erange("entrepot_libssh::read_dir_reset_dirinfo",
			 tools_printf(gettext("Could not open directory %s: %s"),
				      where.c_str(),
				      ssh_get_error(connect->get_ssh_session())));
#else
	throw Efeature("SFTP repository (requires libssh)");
#endif
    }

    bool entrepot_libssh::read_dir_next_dirinfo(std::string & filename, inode_type & tp) const
    {
#if LIBSSH_AVAILABLE
	sftp_attributes attrib;

	if(sdir == nullptr)
	    throw Erange("entrepot_libssh::read_dir_next_dirinfo",
			 gettext("No directory has been openned, cannot read a directory content"));

	if(!connect)
	    throw SRC_BUG;

	attrib = sftp_readdir(connect->get_sftp_session(), sdir);
	if(attrib != nullptr)
	{
	    filename = attrib->name;
	    switch(attrib->type)
	    {
	    case SSH_FILEXFER_TYPE_DIRECTORY:
		tp = inode_type::isdir;
		break;
	    case SSH_FILEXFER_TYPE_SYMLINK:
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
					  connect->get_sftp_error_msg()));
	    read_dir_flush();
	    return false;
	}
#else
	throw Efeature("SFTP repository (requires libssh)");
#endif
    }

    void entrepot_libssh::create_dir(const std::string & dirname, U_I permission)
    {
#if LIBSSH_AVAILABLE
	path where = get_full_path().append(dirname);
	int code;

	if(!connect)
	    throw SRC_BUG;

	code = sftp_mkdir(connect->get_sftp_session(), where.display().c_str(), permission);

	if(code != SSH_OK)
	    throw Erange("entrepot_libss::create_dir",
			 tools_printf(gettext("Failed creating directory %s: %s"),
				      where.display().c_str(),
				      connect->get_sftp_error_msg()));
#else
	throw Efeature("SFTP repository (requires libssh)");
#endif
    }

    fichier_global* entrepot_libssh::inherited_open(const std::shared_ptr<user_interaction> & dialog,
						    const std::string & filename,
						    gf_mode mode,
						    bool force_permission,
						    U_I permission,
						    bool fail_if_exists,
						    bool erase) const
    {
#if LIBSSH_AVAILABLE
	U_I perm = force_permission ? permission : 0666;
	string fullname = (get_full_path().append(filename)).display();
	fichier_libssh* ptr = new (nothrow) fichier_libssh(dialog,
							   connect,
							   fullname,
							   mode,
							   perm,
							   fail_if_exists,
							   erase);

	if(ptr == nullptr)
	    throw Ememory("entrepot_libssh::inherited_open");

	try
	{
	    if(force_permission)
		ptr->change_permission(permission);

	    if(get_user_ownership() != "" || get_group_ownership() != "")
	    {
		try
		{
		    ptr->change_ownership(get_user_ownership(),
					  get_group_ownership());
		}
		catch(Ebug & e)
		{
		    throw;
		}
		catch(Egeneric & e)
		{
		    e.prepend_message("Failed setting user and/or group ownership: ");
		    throw Edata(e.get_message());
		}
	    }
	}
	catch(...)
	{
	    delete ptr;
	    ptr = nullptr;
	    throw;
	}

	return ptr;
#else
	throw Efeature("SFTP repository (requires libssh)");
#endif
    }

    void entrepot_libssh::inherited_unlink(const std::string & filename) const
    {
#if LIBSSH_AVAILABLE
	path where = get_full_path().append(filename);
	int code;

	if(!connect)
	    throw SRC_BUG;

	code = sftp_unlink(connect->get_sftp_session(), where.display().c_str());

	if(code != SSH_OK)
	    throw Erange("entrepot_libss::create_dir",
			 tools_printf(gettext("Failed delete entry %s: %s"),
				      where.display().c_str(),
				      connect->get_sftp_error_msg()));
#else
	throw Efeature("SFTP repository (requires libssh)");
#endif
    }

    void entrepot_libssh::read_dir_flush() const
    {
#if LIBSSH_AVAILABLE
	if(sdir != nullptr)
	{
	    sftp_closedir(sdir);
	    sdir = nullptr;
	}
#endif
    }


} // end of namespace
