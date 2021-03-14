/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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
// to allow compilation under Cygwin we need <sys/types.h>
// else Cygwin's <netinet/in.h> lack __int16_t symbol !?!
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
} // end extern "C"

#include <iomanip>

#include "data_dir.hpp"
#include "tools.hpp"
#include "user_interaction.hpp"
#include "deci.hpp"
#include "path.hpp"
#include "datetime.hpp"
#include "cat_all_entrees.hpp"

using namespace std;
using namespace libdar;

namespace libdar
{

    data_dir::data_dir(const string &name) : data_tree(name)
    {
	rejetons.clear();
    }

    data_dir::data_dir(generic_file &f, unsigned char db_version) : data_tree(f, db_version)
    {
	infinint tmp = infinint(f); // number of children
	data_tree *entry = nullptr;
	rejetons.clear();

	try
	{
	    while(!tmp.is_zero())
	    {
		entry = read_next_in_list_from_file(f, db_version);
		if(entry == nullptr)
		    throw Erange("data_dir::data_dir", gettext("Unexpected end of file"));
		rejetons.push_back(entry);
		entry = nullptr;
		--tmp;
	    }
	}
	catch(...)
	{
	    deque<data_tree *>::iterator next = rejetons.begin();
	    while(next != rejetons.end())
	    {
		delete *next;
		*next = nullptr;
		++next;
	    }
	    if(entry != nullptr)
		delete entry;
	    throw;
	}
    }

    data_dir::data_dir(const data_dir & ref) : data_tree(ref)
    {
	rejetons.clear();
    }

    data_dir::data_dir(const data_tree & ref) : data_tree(ref)
    {
	rejetons.clear();
    }

    data_dir::~data_dir()
    {
	deque<data_tree *>::iterator next = rejetons.begin();
	while(next != rejetons.end())
	{
	    delete *next;
	    *next = nullptr;
	    ++next;
	}
    }

    void data_dir::dump(generic_file & f) const
    {
	deque<data_tree *>::const_iterator it = rejetons.begin();
	infinint tmp = rejetons.size();

	data_tree::dump(f);
	tmp.dump(f);
	while(it != rejetons.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    (*it)->dump(f);
	    ++it;
	}
    }


    data_tree *data_dir::find_or_addition(const string & name, bool is_dir, const archive_num & archive)
    {
	const data_tree *fils = read_child(name);
	data_tree *ret = nullptr;

	if(fils == nullptr) // brand-new data_tree to build
	{
	    if(is_dir)
		ret = new (nothrow) data_dir(name);
	    else
		ret = new (nothrow) data_tree(name);
	    if(ret == nullptr)
		throw Ememory("data_dir::find_or_addition");
	    add_child(ret);
	}
	else  // already saved in another archive
	{
		// check if dir/file nature has changed
	    const data_dir *fils_dir = dynamic_cast<const data_dir *>(fils);
	    if(fils_dir == nullptr && is_dir) // need to upgrade data_tree to data_dir
	    {
		ret = new (nothrow) data_dir(*fils); // upgrade data_tree in an empty data_dir
		if(ret == nullptr)
		    throw Ememory("data_dir::find_or_addition");
		try
		{
		    remove_child(name);
		    add_child(ret);
		}
		catch(...)
		{
		    delete ret;
		    throw;
		}
	    }
	    else // no change in dir/file nature
		ret = const_cast<data_tree *>(fils);
	}

	return ret;
    }

    void data_dir::add(const cat_inode *entry, const archive_num & archive)
    {
	const cat_directory *entry_dir = dynamic_cast<const cat_directory *>(entry);
	const cat_file *entry_file = dynamic_cast<const cat_file *>(entry);
	data_tree * tree = find_or_addition(entry->get_name(), entry_dir != nullptr, archive);
	archive_num last_archive;
	db_lookup result;
	datetime last_mod = entry->get_last_modif() > entry->get_last_change() ? entry->get_last_modif() : entry->get_last_change();
	const crc *base = nullptr;
	const crc *res = nullptr;

	switch(entry->get_saved_status())
	{
	case saved_status::saved:
	case saved_status::fake:
	    if(entry_file != nullptr)
	    {
		if(!entry_file->get_crc(res))
		    res = nullptr;
	    }
	    tree->set_data(archive, last_mod, db_etat::et_saved, base, res);
	    break;
	case saved_status::delta:
	    if(entry_file == nullptr)
		throw SRC_BUG;
	    if(!entry_file->get_patch_base_crc(base))
		base = nullptr;
	    if(!entry_file->get_patch_result_crc(res))
		res = nullptr;
	    tree->set_data(archive, last_mod, db_etat::et_patch, base, res);
	    break;
	case saved_status::inode_only:
	    if(entry_file != nullptr)
	    {
		if(!entry_file->get_crc(res))
		    if(!entry_file->get_patch_result_crc(res))
			res = nullptr;
		if(!entry_file->get_patch_base_crc(base))
		    base = nullptr;
	    }
	    tree->set_data(archive, last_mod, db_etat::et_inode, base, res);
	    break;
	case saved_status::not_saved:
	    if(entry_file != nullptr)
	    {
		if(!entry_file->get_crc(res))
		    if(!entry_file->get_patch_result_crc(res))
			res = nullptr;
		if(!entry_file->get_patch_base_crc(base))
		    base = nullptr;
	    }
	    tree->set_data(archive, last_mod, db_etat::et_present, base, res);
	    break;
	default:
	    throw SRC_BUG;
	}

	switch(entry->ea_get_saved_status())
	{
	case ea_saved_status::none:
	    break;
	case ea_saved_status::removed:
	    result = tree->get_EA(last_archive, datetime(0), false);
	    if(result == db_lookup::found_present || result == db_lookup::not_restorable)
		tree->set_EA(archive, entry->get_last_change(), db_etat::et_removed);
		// else no need to add an db_etat::et_remove entry in the map
	    break;
	case ea_saved_status::partial:
	    tree->set_EA(archive, entry->get_last_change(), db_etat::et_present);
	    break;
	case ea_saved_status::fake:
	case ea_saved_status::full:
	    tree->set_EA(archive, entry->get_last_change(), db_etat::et_saved);
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    void data_dir::add(const cat_detruit *entry, const archive_num & archive)
    {
	data_tree * tree = find_or_addition(entry->get_name(), false, archive);
	set<archive_num> last_archive_set;
	archive_num last_archive;
	db_lookup result;

	result = tree->get_data(last_archive_set, datetime(0), false);
	if(result == db_lookup::found_present || result == db_lookup::not_restorable)
	    tree->set_data(archive, entry->get_date(), db_etat::et_removed);
	result = tree->get_EA(last_archive, datetime(0), false);
	if(result == db_lookup::found_present || result == db_lookup::not_restorable)
	    tree->set_EA(archive, entry->get_date(), db_etat::et_removed);
    }

    const data_tree *data_dir::read_child(const string & name) const
    {
	deque<data_tree *>::const_iterator it = rejetons.begin();

	while(it != rejetons.end() && *it != nullptr && (*it)->get_name() != name)
	    ++it;

	if(it == rejetons.end())
	    return nullptr;
	else
	    if(*it == nullptr)
		throw SRC_BUG;
	    else
		return *it;
    }

    void data_dir::read_all_children(vector<string> & fils) const
    {
	deque<data_tree *>::const_iterator it = rejetons.begin();

	fils.clear();
	while(it != rejetons.end())
	    fils.push_back((*it++)->get_name());
    }

    bool data_dir::check_order(user_interaction & dialog, const path & current_path, bool & initial_warn) const
    {
	deque<data_tree *>::const_iterator it = rejetons.begin();
	bool ret = data_tree::check_order(dialog, current_path, initial_warn);
	path subpath = current_path.display() == "." ? get_name() : current_path.append(get_name());

	while(it != rejetons.end() && ret)
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    ret = (*it)->check_order(dialog, subpath, initial_warn);
	    ++it;
	}

	return ret;
    }

    void data_dir::finalize(const archive_num & archive, const datetime & deleted_date, const archive_num & ignore_archives_greater_or_equal)
    {
	datetime new_deleted_date;
	set<archive_num> tmp_archive_set;
	db_etat tmp_presence;

	data_tree::finalize(archive, deleted_date, ignore_archives_greater_or_equal);

	switch(get_data(tmp_archive_set, datetime(0), false))
	{
	case db_lookup::found_present:
	case db_lookup::found_removed:
	    break; // acceptable result
	case db_lookup::not_found:
	    if(fix_corruption())
		throw Edata("This is to signal the caller of this method that this object has to be removed from database"); // exception caugth in data_dir::finalize_except_self
	    throw Erange("data_dir::finalize", gettext("This database has been corrupted probably due to a bug in release 2.4.0 to 2.4.9, and it has not been possible to cleanup this corruption, please rebuild the database from archives or extracted \"catalogues\", if the database has never been used by one of the previously mentioned released, you are welcome to open a bug report and provide as much as possible details about the circumstances"));
	case db_lookup::not_restorable:
	    break;  // also an acceptable result;
	default:
	    throw SRC_BUG;
	}

	if(tmp_archive_set.empty())
	    throw SRC_BUG;
	if(!read_data(*(tmp_archive_set.rbegin()), new_deleted_date, tmp_presence))
	    throw SRC_BUG;

	finalize_except_self(archive, new_deleted_date, ignore_archives_greater_or_equal);
    }

    void data_dir::finalize_except_self(const archive_num & archive, const datetime & deleted_date, const archive_num & ignore_archives_greater_or_equal)
    {
	deque<data_tree *>::iterator it = rejetons.begin();

	while(it != rejetons.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    try
	    {
		(*it)->finalize(archive, deleted_date, ignore_archives_greater_or_equal);
		++it;
	    }
	    catch(Edata & e)
	    {
		delete (*it);
		rejetons.erase(it);
		it = rejetons.begin();
	    }
	}
    }


    bool data_dir::remove_all_from(const archive_num & archive_to_remove, const archive_num & last_archive)
    {
	deque<data_tree *>::iterator it = rejetons.begin();

	while(it != rejetons.end())
	{
	    if((*it) == nullptr)
		throw SRC_BUG;
	    if((*it)->remove_all_from(archive_to_remove, last_archive))
	    {
		delete *it; // release the memory used by the object
		*it = nullptr;
		rejetons.erase(it); // remove the entry from the deque
		it = rejetons.begin(); // does not seems "it" points to the next item after erase, so we restart from the beginning
	    }
	    else
		++it;
	}

	return data_tree::remove_all_from(archive_to_remove, last_archive) && rejetons.size() == 0;
    }

    void data_dir::show(database_listing_show_files_callback callback,
			void *tag,
			archive_num num,
			string marge) const
    {
	deque<data_tree *>::const_iterator it = rejetons.begin();
	set<archive_num> ou_data;
	archive_num ou_ea;
	bool data, ea;
	string name;
	db_lookup lo_data, lo_ea;
	bool even_when_removed = (num != 0);

	while(it != rejetons.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    data_dir *dir = dynamic_cast<data_dir *>(*it);

	    lo_data = (*it)->get_data(ou_data, datetime(0), even_when_removed);
	    lo_ea = (*it)->get_EA(ou_ea, datetime(0), even_when_removed);
	    data = lo_data == db_lookup::found_present && (ou_data.find(num) != ou_data.end() || num == 0);
	    ea = lo_ea == db_lookup::found_present && (ou_ea == num || num == 0);
	    name = marge + (*it)->get_name();
	    if(data || ea || num == 0)
	    {
		if(callback == nullptr)
		    throw Erange("data_dir::show", "nullptr provided as user callback function");

		try
		{
		    callback(tag, name, data, ea);
		}
		catch(...)
		{
		    throw Elibcall("data_dir::show", "user provided callback function should not throw any exception");
		}
	    }
	    if(dir != nullptr)
		dir->show(callback, tag, num, name + "/");
	    ++it;
	}
    }

    void data_dir::apply_permutation(archive_num src, archive_num dst)
    {
	deque<data_tree *>::iterator it = rejetons.begin();

	data_tree::apply_permutation(src, dst);
	while(it != rejetons.end())
	{
	    (*it)->apply_permutation(src, dst);
	    ++it;
	}
    }


    void data_dir::skip_out(archive_num num)
    {
	deque<data_tree *>::iterator it = rejetons.begin();

	data_tree::skip_out(num);
	while(it != rejetons.end())
	{
	    (*it)->skip_out(num);
	    ++it;
	}
    }

    void data_dir::compute_most_recent_stats(deque<infinint> & data,
					     deque<infinint> & ea,
					     deque<infinint> & total_data,
					     deque<infinint> & total_ea) const
    {
	deque<data_tree *>::const_iterator it = rejetons.begin();

	data_tree::compute_most_recent_stats(data, ea, total_data, total_ea);
	while(it != rejetons.end())
	{
	    (*it)->compute_most_recent_stats(data, ea, total_data, total_ea);
	    ++it;
	}
    }

    bool data_dir::fix_corruption()
    {
	while(rejetons.begin() != rejetons.end() && *(rejetons.begin()) != nullptr && (*(rejetons.begin()))->fix_corruption())
	{
	    delete *(rejetons.begin());
	    rejetons.erase(rejetons.begin());
	}

	if(rejetons.begin() != rejetons.end())
	    return false;
	else
	    return data_tree::fix_corruption();

    }

    bool data_dir::data_tree_find(path chemin, const data_tree *& ptr) const
    {
	string filename;
	const data_dir *current = this;
	bool loop = true;

	if(!chemin.is_relative())
	    throw SRC_BUG;

	while(loop)
	{
	    if(!chemin.pop_front(filename))
	    {
		filename = chemin.display();
		loop = false;
	    }
	    ptr = current->read_child(filename);
	    if(ptr == nullptr)
		loop = false;
	    if(loop)
	    {
		current = dynamic_cast<const data_dir *>(ptr);
		if(current == nullptr)
		{
		    loop = false;
		    ptr = nullptr;
		}
	    }
	}

	return ptr != nullptr;
    }

    void data_dir::data_tree_update_with(const cat_directory *dir, archive_num archive)
    {
	const cat_nomme *entry;

	if(dir == nullptr)
	    throw SRC_BUG;

	dir->reset_read_children();
	while(dir->read_children(entry))
	{
	    const cat_directory *entry_dir = dynamic_cast<const cat_directory *>(entry);
	    const cat_inode *entry_ino = dynamic_cast<const cat_inode *>(entry);
	    const cat_mirage *entry_mir = dynamic_cast<const cat_mirage *>(entry);
	    const cat_detruit *entry_det = dynamic_cast<const cat_detruit *>(entry);

	    if(entry_mir != nullptr)
	    {
		entry_ino = entry_mir->get_inode();
		entry_mir->get_inode()->change_name(entry_mir->get_name());
	    }

	    if(entry_ino == nullptr)
		if(entry_det != nullptr)
		{
		    if(!entry_det->get_date().is_null())
			add(entry_det, archive);
			// else this is an old archive that does not store date with cat_detruit objects
		}
		else
		    continue; // continue with next loop, we ignore entree objects that are neither inode nor cat_detruit
	    else
		add(entry_ino, archive);

	    if(entry_dir != nullptr) // going into recursion
	    {
		data_tree *new_root = const_cast<data_tree *>(read_child(entry->get_name()));
		data_dir *new_root_dir = dynamic_cast<data_dir *>(new_root);

		if(new_root == nullptr)
		    throw SRC_BUG; // the add() method did not add an item for "entry"
		if(new_root_dir == nullptr)
		    throw SRC_BUG; // the add() method did not add a data_dir item
		new_root_dir->data_tree_update_with(entry_dir, archive);
	    }
	}
    }

    data_dir *data_dir::data_tree_read(generic_file & f, unsigned char db_version)
    {
	data_tree *lu = read_next_in_list_from_file(f, db_version);
	data_dir *ret = dynamic_cast<data_dir *>(lu);

	if(ret == nullptr && lu != nullptr)
	    delete lu;

	return ret;
    }


    void data_dir::add_child(data_tree *fils)
    {
	if(fils == nullptr)
	    throw SRC_BUG;
	rejetons.push_back(fils);
    }

    void data_dir::remove_child(const string & name)
    {
	deque<data_tree *>::iterator it = rejetons.begin();

	while(it != rejetons.end() && *it != nullptr && (*it)->get_name() != name)
	    ++it;

	if(it != rejetons.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    else
		rejetons.erase(it);
	}
    }

    data_tree *data_dir::read_next_in_list_from_file(generic_file & f, unsigned char db_version)
    {
	char sign;
	data_tree *ret;

	if(f.read(&sign, 1) != 1)
	    return nullptr; // nothing more to read

	if(sign == data_tree::signature())
	    ret = new (nothrow) data_tree(f, db_version);
	else if(sign == data_dir::signature())
	    ret = new (nothrow) data_dir(f, db_version);
	else
	    throw Erange("read_next_in_list_from_file", gettext("Unknown record type"));

	if(ret == nullptr)
	    throw Ememory("read_next_in_list_from_file");

	return ret;
    }

} // end of namesapce


