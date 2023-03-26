//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

#include "mycurl_slist.hpp"
#include "erreurs.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{
#if LIBCURL_AVAILABLE


    bool mycurl_slist::operator == (const mycurl_slist & ref) const
    {
	if(appended.size() != ref.appended.size())
	    return false;
	else
	{
	    deque<string>::const_iterator itref = ref.appended.begin();
	    deque<string>::const_iterator itme = appended.begin();

	    while(itref != ref.appended.end()
		  && itme != appended.end()
		  && *itref == *itme)
	    {
		++itref;
		++itme;
	    }

	    return itref == ref.appended.end() && itme == appended.end();
	}
    }

    void mycurl_slist::append(const string & s)
    {
	curl_slist *tmp = curl_slist_append(header, s.c_str());
	if(tmp == nullptr)
	    throw Erange("mycurl_slist::append", "Failed to append command to a curl_slist");
	header = tmp;
	appended.push_back(s);
    }

    curl_slist* mycurl_slist::rebuild(const deque<string> & ap)
    {
	curl_slist *ret = nullptr;
	curl_slist *tmp = nullptr;
	deque<string>::const_iterator it = ap.begin();

	try
	{
	    while(it != ap.end())
	    {
		tmp = curl_slist_append(ret, it->c_str());
		if(tmp == nullptr)
		    throw Erange("mycurl_slist::rebuild", "Failed to rebuild an slist from its recorded paramaters");
		ret = tmp;
		++it;
	    }
	}
	catch(...)
	{
	    release(ret);
	    throw;
	}

	return ret;
    }

#endif
} // end of namespace
