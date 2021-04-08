//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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
#include "mycurl_protocol.hpp"

using namespace std;

namespace libdar
{
    mycurl_protocol string_to_mycurl_protocol(const std::string & arg)
    {
	mycurl_protocol ret;

	if(strcasecmp(arg.c_str(), "ftp") == 0)
	    ret = proto_ftp;
	else if(strcasecmp(arg.c_str(), "sftp") == 0)
	    ret = proto_sftp;
	else
	    throw Erange("entrepot_libcurl::string_to_curlprotocol", tools_printf(gettext("Unknown protocol: %S"), &arg));

	return ret;
    }
} // end of namespace
