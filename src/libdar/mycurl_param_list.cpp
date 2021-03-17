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

#include "mycurl_param_list.hpp"

#include <typeinfo>
#include "erreurs.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

#if LIBCURL_AVAILABLE

    void mycurl_param_list::clear(CURLoption opt)
    {
	map<CURLoption, unique_ptr<mycurl_param_element_generic> >::iterator it = element_list.find(opt);

	if(it != element_list.end())
	    element_list.erase(it);
	reset_read();
    }

    bool mycurl_param_list::read_next(CURLoption & opt)
    {
	if(cursor == element_list.end())
	    return false;

	opt = cursor->first;
	return true;
    }

    list<CURLoption> mycurl_param_list::update_with(const mycurl_param_list & wanted)
    {
	list<CURLoption> ret;

	std::map<CURLoption, unique_ptr<mycurl_param_element_generic> >::const_iterator wit; // will be used on wanted
	std::map<CURLoption, unique_ptr<mycurl_param_element_generic> >::iterator mit; // will be used on "this"

	    // adding changed parameters

	wit = wanted.element_list.begin();

	while(wit != wanted.element_list.end())
	{
	    if(!wit->second)
		throw Erange("mycurl_param_list", "empty value in mycurl_param_list");

	    mit = element_list.find(wit->first);
	    if(mit == element_list.end()            // entry not found
	       || ! mit->second                     // found but associated to no value (!)
	       || *(mit->second) != *(wit->second)) // found, associated to a value but different from the one in wanted
	    {
		add_clone(wit->first, *(wit->second));
		ret.push_back(wit->first);
	    }

	    ++wit;
	}
	reset_read();

	return ret;
    }

    void mycurl_param_list::copy_from(const mycurl_param_list & ref)
    {
	map<CURLoption, unique_ptr<mycurl_param_element_generic> >::const_iterator it = ref.element_list.begin();

	while(it != ref.element_list.end())
	{
	    if(it->second)
		element_list[it->first] = it->second->clone();
	    else
		throw SRC_BUG;

	    ++it;
	}
	reset_read();
    }

#endif

} // end of namespace
