/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
} // end extern "C"

#include "restore_tree.hpp"
#include "data_dir.hpp"

using namespace std;

namespace libdar
{

    restore_tree::restore_tree(const data_tree* source,
			       const datetime & max_date)
    {
	set<archive_num> data_set;
	archive_num ea_num;
	db_lookup data_ret;
	db_lookup ea_ret;
	const data_dir* source_dir = dynamic_cast<const data_dir*>(source);

	if(source == nullptr)
	    throw SRC_BUG;

	    //*** setting up locations

	data_ret = source->get_data(data_set, max_date, false);
	ea_ret = source->get_EA(ea_num, max_date, false);

	if(data_ret == db_lookup::found_present)
	{
	    if(ea_ret == db_lookup::found_present
	       || ea_ret == db_lookup::found_removed)
		data_set.insert(ea_num);

	    locations = std::move(data_set);
	}
	    // else locations stays empty (entry is not available in any archive referred from the database)
	    // which is the case for the root node.

	    //*** setting up children if any

	if(source_dir != nullptr)
	{
	    vector<string> fils;
	    const data_tree* tmp = nullptr;
	    unique_ptr<restore_tree> tmp_ptr;

	    source_dir->read_all_children(fils);
	    for(vector<string>::iterator it = fils.begin(); it != fils.end(); ++it)
	    {
		tmp = source_dir->read_child(*it);

		if(tmp == nullptr)
		    throw SRC_BUG;
		tmp_ptr.reset(new (nothrow) restore_tree(tmp, max_date));
		if(!tmp_ptr)
		    throw Ememory("restore_tree:restore_tree");
		else
		{
			// we merge the child archive num, because if this
			// parent directly is excluded by the path filter,
			// all child will be excluded by the same filter.
		    for(set<archive_num>::iterator pr = tmp_ptr->locations.begin(); pr != tmp_ptr->locations.end(); ++pr)
			locations.insert(*pr);

		    children[*it] = std::move(tmp_ptr);
		}
	    }
	}
    }

    bool restore_tree::restore_from(const string & chem, archive_num num) const
    {
	const restore_tree* found = nullptr;

	if(chem.empty())
	    throw SRC_BUG;
	else
	{
	    path tmp = path(chem);

	    if(! tmp.is_relative())
		throw SRC_BUG;

	    tmp.reset_read();
	    found = lookup(tmp);
	}

	if(found == nullptr)
	    throw SRC_BUG;

	return found->result_for(num);
    }

    const restore_tree* restore_tree::lookup(path & chem) const
    {
	string tmp;

	if(chem.read_subdir(tmp))
	{
	    map<string, unique_ptr<restore_tree> >::const_iterator it = children.find(tmp);
	    if(it == children.end())
		throw SRC_BUG; // requested entry should exist!

	    if(! it->second)
		throw SRC_BUG; // empty unique_ptr in map!

	    return it->second->lookup(chem);
	}
	else
	    return this;
    }

} // end of namespace
