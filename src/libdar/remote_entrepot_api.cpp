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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "tools.hpp"
#include "remote_entrepot_api.hpp"
#include "entrepot_libcurl.hpp"
#include "entrepot_libssh.hpp"

using namespace std;

namespace libdar
{
    remote_entrepot_type string_to_remote_entrepot_type(const std::string & arg)
    {
	remote_entrepot_type ret;

	if(strcasecmp(arg.c_str(), "ftp") == 0)
	    ret = remote_entrepot_type::ftp;
	else if(strcasecmp(arg.c_str(), "sftp") == 0)
	    ret = remote_entrepot_type::sftp;
	else
	    throw Erange("string_to_entrepot_type", tools_printf(gettext("Unknown protocol: %S"), &arg));

	return ret;
    }

    shared_ptr<entrepot> create_remote_entrepot(shared_ptr<user_interaction> & dialog,
						     remote_entrepot_type proto,
						     const string & login,
						     const secu_string & pass,
						     const string & host,
						     const string & port,
						     bool auth_from_file,
						     const string & sftp_pub_filekey,
						     const string & sftp_prv_filekey,
						     const string & sftp_known_hosts,
						     U_I network_retry,
 						     bool remote_verbose)
    {
	shared_ptr<entrepot> ret;

	switch(proto)
	{
	case remote_entrepot_type::ftp:
	    ret.reset(new (nothrow) entrepot_libcurl(dialog,
						     proto,
						     login,
						     pass,
						     host,
						     port,
						     auth_from_file,
						     sftp_pub_filekey,
						     sftp_prv_filekey,
						     sftp_known_hosts,
						     network_retry,
						     remote_verbose));
	    break;
	case remote_entrepot_type::sftp:
	    ret.reset(new (nothrow) entrepot_libssh(dialog,
						    login,
						    pass,
						    host,
						    port,
						    auth_from_file,
						    sftp_pub_filekey,
						    sftp_prv_filekey,
						    sftp_known_hosts,
						    network_retry,
						    remote_verbose));
	    break;
	default:
	    throw SRC_BUG;
	}

	if(! ret)
	    throw Ememory("create_remote_entrepot");

	return ret;
    }


} // end of namespace
