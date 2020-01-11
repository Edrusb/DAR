/*********************************************************************/
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
#include "range.hpp"
#include "deci.hpp"

using namespace std;

namespace libdar
{

    void range::operator += (const range & ref)
    {
	list<segment>::const_iterator ref_it = ref.parts.begin();

	while(ref_it != ref.parts.end())
	{
	    list<segment>::iterator it = parts.begin();

	    while(it != parts.end() && *it < *ref_it)
		++it;

	    if(it == parts.end())
		parts.push_back(*ref_it);
	    else
		if(*ref_it < *it)
		    parts.insert(it, *ref_it);
		else
		{
		    if(!it->overlaps_with(*ref_it))
			throw SRC_BUG;
		    it->merge_with(*ref_it);
			// we also have to test whether the next segment cannot be merged too
		    list<segment>::iterator next = it;
		    ++next;
		    if(next != parts.end())
		    {
			if(it->overlaps_with(*next))
			{
			    it->merge_with(*next);
			    parts.erase(next);
			}
		    }
		}

	    ++ref_it;
	}
    }

    string range::display() const
    {
	string ret = "";
	list<segment>::const_iterator it = parts.begin();

	while(it != parts.end())
	{
	    ret += it->display();
	    ++it;
	    if(it != parts.end())
		ret += ",";
	}

	if(ret.size() == 0)
	    ret = "";

	return ret;
    }

    bool range::read_next_segment(infinint & low, infinint & high) const
    {
	if(read_cursor != parts.end())
	{
	    low = read_cursor->get_low();
	    high = read_cursor->get_high();
	    ++read_cursor;
	    return true;
	}
	else
	    return false;
    }

    void range::segment::merge_with(const segment & ref)
    {
	if(*this <= ref)
	    low = ref.low;
	else
	    if(*this >= ref)
		high = ref.high;
	    else
		if(contains(ref))
		    return; // nothing to do
		else
		    if(ref.contains(*this))
			*this = ref;
		    else
			throw SRC_BUG;
    }

    string range::segment::display() const
    {
	string ret = "";
	deci dl = low;

	if(low == high)
	    ret = dl.human();
	else
	{
	    deci dh = high;
	    ret = dl.human() + "-" + dh.human();
	}

	return ret;
    }

} // end of namespace
