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

#include "mycurl_easyhandle_sharing.hpp"

using namespace std;

namespace libdar
{
#if LIBCURL_AVAILABLE

    shared_ptr<mycurl_easyhandle_node> mycurl_easyhandle_sharing::alloc_instance()
    {
	shared_ptr<mycurl_easyhandle_node> ret;
	deque<shared_ptr<mycurl_easyhandle_node> >::const_iterator it = table.begin();

	while(it != table.end() && it->use_count() > 1)
	    ++it;

	if(it != table.end())
	    ret = *it;
	else
	{
	    try
	    {
		table.push_back(make_shared<mycurl_easyhandle_node>());
		if(table.back().use_count() != 1)
		    throw SRC_BUG;
		else
		    ret = table.back();
	    }
	    catch(bad_alloc & e)
	    {
		throw Ememory("mycurl_easyhandle_sharing::alloc_instance");
	    }
	}

	ret->setopt_list(global_params);
	return ret;
    }




#endif
} // end of namespace
