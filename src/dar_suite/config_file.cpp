/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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
#if STDC_HEADERS
#include <ctype.h>
#endif
} // end extern "C"

#include "config_file.hpp"
#include "tools.hpp"

using namespace libdar;

config_file::config_file(const deque<string> & target, generic_file &f) : hide_file(f)
{
    deque<string>::const_iterator it = target.begin();

    cibles.clear();
    while(it != target.end())
    {
	cibles.push_back(t_cible(*it));
	++it;
    }
}

deque<string> config_file::get_read_targets() const
{
    deque<string> ret;
    deque<t_cible>::const_iterator it = cibles.begin();

    while(it != cibles.end())
    {
	if(it->seen)
	    ret.push_back(it->target);
	++it;
    }

    return ret;
}

void config_file::fill_morceau()
{
    string read_target;
    partie tmp;
    infinint last_offset = 0;
    infinint first, last;
    enum { debut, fin } status = fin;

	// we read any text put before the first condition
    tmp.debut = 0;
    tmp.offset = 0;
    morceau.clear();

    if(ref == nullptr)
        throw SRC_BUG;
    ref->skip(0);

    while(find_next_target(*ref, first, read_target, last))
    {
	switch(status)
	{
	case fin:
	    if(first < tmp.debut)
		throw SRC_BUG; // first byte of the line where next target
		// resides is before where we started looking for new target !
	    tmp.longueur = first - tmp.debut;
	    last_offset += tmp.longueur;
	    if(tmp.longueur > 0)
		morceau.push_back(tmp);
	    status = debut;
		// no break !
	case debut:
	    if(is_a_target(read_target))
	    {
		tmp.debut = last;
		tmp.offset = last_offset;
		status = fin;
	    }
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    if(status == fin)
    {
	if(ref->get_position() < tmp.debut)
	    throw SRC_BUG;
	tmp.longueur = ref->get_position() - tmp.debut;
	if(tmp.longueur > 0)
	    morceau.push_back(tmp);
    }
}


bool config_file::is_a_target(const string & val)
{
    deque<t_cible>::iterator it = cibles.begin();

    while(it != cibles.end() && it->target != val)
	++it;

    if(it != cibles.end())
    {
	it->seen = true;
	return true;
    }
    else
	return false;
}


bool config_file::find_next_target(generic_file &f, infinint & debut, string & nature, infinint & fin)
{
    char a;
    bool found = false;
    enum { init, search, end, purge } status = init;
    debut = f.get_position();


    while(!found && f.read(&a, 1) == 1)
    {
	switch(status)
	{
	case init:  // looking for next word beginning
	    switch(a)
	    {
	    case ' ':
	    case '\t':
		break;
	    case '\n':
		debut = f.get_position();
		break;
	    default:
		if(isalpha(a))
		{
		    nature = a;
		    status = search;
		}
		else
		    status = purge;
	    }
	    break;
	case search: // reading the current word
	    if(isalnum(a) || a == '_' || a == '-')
		nature += a;
	    else
		if(a == ':')
		    status = end;
		else
		    if(a == '\n')
		    {
			status = init;
			debut = f.get_position();
		    }
		    else
			status = purge;
	    break;
	case end: // got a target word (= word + `:' )
	    switch(a)
	    {
	    case ' ':
	    case '\t':
	    case '\r':
		break;
	    case '\n':
		fin = f.get_position();
		found = true;
		break;
	    default:
		status = purge;
	    }
	    break;
	case purge: // just read word is not a target, resetting to the next line
	    if(a == '\n')
	    {
		status = init;
		debut = f.get_position();
	    }
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    return found;
}
