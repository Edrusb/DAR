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
#include "tools.hpp"
#include "list_entry.hpp"

using namespace std;

namespace libdar
{

    unsigned char list_entry::get_removed_type() const
    {
	if(is_removed_entry())
	{
	    if(target.size() != 1)
		throw SRC_BUG;
	    return target[0];
	}
	else
	    return '!';
    }

    string list_entry::get_data_flag() const
    {
	switch(data_status)
	{
	case saved_status::saved:
	    if(dirty)
		return gettext("[DIRTY]");
	    else
		return gettext("[Saved]");
	case saved_status::inode_only:
	    return gettext("[Inode]");
	case saved_status::fake:
	    return gettext("[InRef]");
	case saved_status::not_saved:
	    return "[     ]";
	case saved_status::delta:
	    return "[Delta]";
	default:
	    throw SRC_BUG;
	}
    }

    string list_entry::get_ea_flag() const
    {
	switch(ea_status)
	{
	case ea_saved_status::full:
	    return gettext("[Saved]");
	case ea_saved_status::fake:
	    return gettext("[InRef]");
	case ea_saved_status::partial:
	    return "[     ]";
	case ea_saved_status::none:
	    return "       ";
	case ea_saved_status::removed:
	    return "[Suppr]";
	default:
	    throw SRC_BUG;
	}
    }

    string list_entry::get_fsa_flag() const
    {
	string ret = "";

	if(fsa_status != fsa_saved_status::none)
	{
	    fsa_scope sc = get_fsa_scope();
	    bool upper = fsa_status == fsa_saved_status::full;
	    ret = fsa_scope_to_string(upper, sc);
	    while(ret.size() < 3)
		ret += "-";
	    ret = "[" + ret + "]";
	}
	else
	    ret = "[---]";

	return ret;
    }

    string list_entry::get_uid(bool try_resolving_name) const
    {
	if(try_resolving_name)
	    return tools_name_of_uid(uid);
	else
	    return deci(uid).human();
    }

    string list_entry::get_gid(bool try_resolving_name) const
    {
	if(try_resolving_name)
	    return tools_name_of_gid(gid);
	else
	    return deci(gid).human();
    }

    string list_entry::get_last_modif() const
    {
	if(!is_removed_entry())
	    return last_modif.is_null() ? "" : tools_display_date(last_modif);
	else
	    return "";
    }

    string list_entry::get_removal_date() const
    {
	if(is_removed_entry())
	    return last_modif.is_null() ? "Unknown date" : tools_display_date(last_modif);
	else
	    return "";
    }

    time_t list_entry::get_last_modif_s() const
    {
	if(!is_removed_entry())
	    return datetime2time_t(last_modif);
	else
	    return 0;
    }

    time_t list_entry::get_removal_date_s() const
    {
	if(is_removed_entry())
	    return datetime2time_t(last_modif);
	else
	    return 0;
    }

    string list_entry::get_file_size(bool size_in_bytes) const
    {
	if(size_in_bytes)
	    return deci(file_size).human();
	else
	    return tools_display_integer_in_metric_system(file_size , "o", true);
    }

    string list_entry::get_compression_ratio() const
    {
	if((is_file() && has_data_present_in_the_archive())
	   || is_dir())
	    return tools_get_compression_ratio(storage_size,
					       file_size,
					       compression_algo != compression::none
					       || is_sparse()
					       || is_dir());
	else
	    return "";
    }

    string list_entry::get_compression_ratio_flag() const
    {
	string ret = get_compression_ratio();

	if(ret.size() == 0)
	    return "[-----]";
	else
	    return "[" + ret + "]";
    }

    string list_entry::get_delta_flag() const
    {
	if(!is_file())
	    return "[-]";
	else
	{
	    if(has_delta_signature())
		return "[D]";
	    else
		return "[ ]";
	}
    }

    string list_entry::get_storage_size_for_data(bool size_in_bytes) const
    {
	if(size_in_bytes)
	    return deci(storage_size_for_data).human();
	else
	    return tools_display_integer_in_metric_system(storage_size_for_data, "o", true);
    }

    bool list_entry::get_ea_read_next(std::string & key) const
    {
	if(it_ea != ea.end())
	{
	    key = *it_ea;
	    ++it_ea;
	    return true;
	}
	else
	    return false;
    }

    void list_entry::set_removal_date(const datetime & val)
    {
	if(!is_removed_entry())
	    throw SRC_BUG;
	last_modif = val;
	    // we recycle last_modif for this purpose
	    // when the entry is a "removed" one
    }

    void list_entry::set_removed_type(unsigned char val)
    {
	if(!is_removed_entry())
	    throw SRC_BUG;

	target.clear();
	target += val;
	if(target.size() != 1)
	    throw SRC_BUG;
    }

    void list_entry::set_ea(const ea_attributs & arg)
    {
	string key, val;

	ea.clear();

	arg.reset_read();
	while(arg.read(key, val))
	    ea.push_back(key); // we ignore val
	get_ea_reset_read();
    }

    void list_entry::clear()
    {
	my_name = "";
	hard_link = false;
	type = ' ';
	uid = 0;
	gid = 0;
	perm = 0;
	last_access.nullify();
	last_modif.nullify();
	data_status = saved_status::saved;
	ea_status = ea_saved_status::none;
	last_change.nullify();
	fsa_status = fsa_saved_status::none;
	file_size = 0;
	storage_size = 0;
	sparse_file = false;
	compression_algo = compression::none;
	dirty = false;
	target = "";
	major = 0;
	minor = 0;
	slices.clear();
	delta_sig = false;
	offset_for_data = 0;
	storage_size_for_data = 0;
	offset_for_EA = 0;
	storage_size_for_EA = 0;
	offset_for_FSA = 0;
	storage_size_for_FSA = 0;
	ea.clear();
	it_ea = ea.begin();
	etiquette = 0;
	empty_dir = false;
    }

    time_t list_entry::datetime2time_t(const datetime & val)
    {
	time_t ret = 0;
	time_t unused;

	(void) val.get_value(ret, unused, datetime::tu_second);

	return ret;
    }

} // end of namespace
