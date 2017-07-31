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

#include "mycurl_easyhandle_node.hpp"

using namespace std;

namespace libdar
{
#if LIBCURL_AVAILABLE


    mycurl_easyhandle_node::mycurl_easyhandle_node()
    {
	handle = curl_easy_init();
	if(handle == nullptr)
	    throw Erange("mycurl_easyhandle_node::mycurl_easyhandle_node",
			 gettext("Error met while creating a libcurl handle"));
	used = false;
    }

    mycurl_easyhandle_node::mycurl_easyhandle_node(const mycurl_easyhandle_node & ref)
    {
	if(ref.handle == nullptr)
	    throw SRC_BUG;

	handle = curl_easy_duphandle(ref.handle);
	if(handle == nullptr)
	    throw Erange("mycurl_easyhandle_node::mycurl_easyhandle_node",
			 gettext("Error met while duplicating libcurl handle"));
	used = false;
    }

    mycurl_easyhandle_node::mycurl_easyhandle_node(mycurl_easyhandle_node && ref)
    {
	handle = ref.handle;
	used = ref.used;
	ref.handle = nullptr;
	ref.used = false;
    }

#endif
} // end of namespace
