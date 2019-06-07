/*********************************************************************/
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{

} // end extern "C"

#include "candidates.hpp"

using namespace std;

namespace libdar
{

    void candidates::add(archive_num val, db_etat st)
    {

	    // considering the new entry to add

	switch(st)
	{
	case db_etat::et_saved:
	    clear();
	    break;
	case db_etat::et_removed:
	case db_etat::et_absent:
	    if(ewr)
		return; // stop here complete ignoring the new entry
	    clear();
	    break;
	case db_etat::et_patch:
	case db_etat::et_present:
	case db_etat::et_inode:
	    break;
	case db_etat::et_patch_unusable:
	    clear();
	    break;
	default:
	    throw SRC_BUG;
	}

	    // considering the current stack status

	if(status.size() > 0)
	{
	    if(st == db_etat::et_present)
		return; // not useful to add et_present when something is already in

	    switch(status.back())
	    {
	    case db_etat::et_saved:
	    case db_etat::et_patch:
		break;
	    case db_etat::et_present:
		throw SRC_BUG;
	    case db_etat::et_inode:
		num.pop_back();    // removing the top of the stack
		status.pop_back(); // removing the top of the stack
		break;
	    case db_etat::et_removed:
	    case db_etat::et_absent:
		clear();
		break;
	    case db_etat::et_patch_unusable:
		return; // stop here
	    default:
		throw SRC_BUG;
	    }
	}

	    // now adding the new entry

	num.push_back(val);
	status.push_back(st);
    }

    db_lookup candidates::get_status() const
    {
	    // considering the bottom of the stack

	if(status.size() > 0)
	{
	    switch(status.front())
	    {
	    case db_etat::et_saved:
		return db_lookup::found_present;
	    case db_etat::et_patch:
	    case db_etat::et_inode:
	    case db_etat::et_present:
	    case db_etat::et_patch_unusable:
		return db_lookup::not_restorable;
	    case db_etat::et_removed:
	    case db_etat::et_absent:
		return db_lookup::found_removed;
	    default:
		throw SRC_BUG;
	    }
	}
	else
	    return db_lookup::not_found;
    }

    void candidates::set_the_set(set<archive_num> & archive) const
    {
	deque<archive_num>::const_iterator it = num.begin();
	deque<db_etat>::const_iterator et = status.begin();
	archive.clear();

	while(it != num.end() && et != status.end())
	{
	    archive.insert(*it);
	    ++it;
	    ++et;
	}

	if(it != num.end() || et != status.end())
	    throw SRC_BUG;
    }



} // end of namespace


