//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

#include "../my_config.h"

#include "mycurl_shared_handle.hpp"

using namespace std;

namespace libdar
{
#if LIBCURL_AVAILABLE

    mycurl_shared_handle::mycurl_shared_handle(const smart_pointer<mycurl_easyhandle_node> & node) : ref(node)
    {
	if(node.is_null())
	    throw SRC_BUG;
	if(node->get_used_mode())
	    throw SRC_BUG;
	ref->set_used_mode(true);
    }

    mycurl_shared_handle::mycurl_shared_handle(mycurl_shared_handle && arg) noexcept
    {
	ref = std::move(arg.ref);
    }

    mycurl_shared_handle & mycurl_shared_handle::operator = (mycurl_shared_handle && arg) noexcept
    {
	ref = std::move(arg.ref);
	return *this;
    }

#endif
} // end of namespace
