/*********************************************************************/
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

#include "../my_config.h"
#include "range.hpp"

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

    void range::reset_read() const
    {
	range *me = const_cast<range *>(this);
	me->read_cursor = parts.begin();
    }

    bool range::read_next_segment(infinint & low, infinint & high) const
    {
	range *me = const_cast<range *>(this);
	if(me->read_cursor != parts.end())
	{
	    low = me->read_cursor->get_low();
	    high = me->read_cursor->get_high();
	    ++me->read_cursor;
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
