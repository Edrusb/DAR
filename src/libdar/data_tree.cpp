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
// to contact the author : dar.linux@free.fr
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
#include "data_tree.hpp"
#include "tools.hpp"
#include "user_interaction.hpp"
#include "deci.hpp"

using namespace std;
using namespace libdar;

static data_tree *read_from_file(generic_file & f);
static void read_from_file(generic_file &f, archive_num &a);
static void write_to_file(generic_file &f, archive_num a);
static void display_line(user_interaction & dialog, archive_num num, const infinint *data, const infinint *ea);

namespace libdar
{

    data_tree::data_tree(const string & name)
    {
	filename = name;
    }

    data_tree::data_tree(generic_file & f)
    {
	archive_num k;

	    // signature has already been read
	tools_read_string(f, filename);
	infinint tmp = infinint(f.get_gf_ui(), NULL, &f); // number of entry in last_mod map
	while(tmp > 0)
	{
	    read_from_file(f, k);
	    last_mod[k] = infinint(f.get_gf_ui(), NULL, &f);
	    --tmp;
	}

	tmp = infinint(f.get_gf_ui(), NULL, &f); // number of entry in last_change map
	while(tmp > 0)
	{
	    read_from_file(f, k);
	    last_change[k] = infinint(f.get_gf_ui(), NULL, &f);
	    --tmp;
	}
    }

    void data_tree::dump(generic_file & f) const
    {
	char tmp = obj_signature();
	infinint sz;
	map<archive_num, infinint>::const_iterator it = last_mod.begin();

	f.write(&tmp, 1);
	tools_write_string(f, filename);

	sz = last_mod.size();
	sz.dump(f);
	while(it != last_mod.end())
	{
	    write_to_file(f, it->first); // key
	    it->second.dump(f); // value
	    ++it;
	}

	sz = last_change.size();
	sz.dump(f);
	it = last_change.begin();
	while(it != last_change.end())
	{
	    write_to_file(f, it->first); // key
	    it->second.dump(f); // value
	    ++it;
	}
    }


    bool data_tree::get_data(archive_num & archive, const infinint & date) const
    {
	infinint max = 0;
	map<archive_num, infinint>::const_iterator it  = last_mod.begin();

	archive = 0; // 0 is never assigned to an archive number
	while(it != last_mod.end())
	{
	    if(it->second >= max  // > and = because there should never be twice the same value
		   // and we must be able to see a value of 0 (initially max = 0) which is valid.
	       && (date == 0 || it->second <= date))
	    {
		max = it->second;
		archive = it->first;
	    }
	    ++it;
	}

	return archive != 0;
    }

    bool data_tree::get_EA(archive_num & archive, const infinint & date) const
    {
	infinint max = 0;
	map<archive_num, infinint>::const_iterator it = last_change.begin();

	archive = 0; // 0 is never assigned to an archive number
	while(it != last_change.end())
	{
	    if(it->second >= max  // > and = because there should never be twice the same value
		   // and we must be able to see a value of 0 (initially max = 0) which is valid.
	       && (date == 0 || it->second <= date))
	    {
		max = it->second;
		archive = it->first;
	    }
	    ++it;
	}

	return archive != 0;
    }

    bool data_tree::read_data(archive_num num, infinint & val) const
    {
	map<archive_num, infinint>::const_iterator it = last_mod.find(num);

	if(it != last_mod.end())
	{
	    val = it->second;
	    return true;
	}
	else
	    return false;
    }

    bool data_tree::read_EA(archive_num num, infinint & val) const
    {
	map<archive_num, infinint>::const_iterator it = last_change.find(num);

	if(it != last_change.end())
	{
	    val = it->second;
	    return true;
	}
	else
	    return false;
    }

    bool data_tree::remove_all_from(const archive_num & archive)
    {
	map<archive_num, infinint>::iterator it = last_mod.begin();

	while(it != last_mod.end())
	{
	    if(it->first == archive)
	    {
		last_mod.erase(it);
		break; // stops the while loop, as there is at most one element with that key
	    }
	    else
		++it;
	}

	it = last_change.begin();
	while(it != last_change.end())
	{
	    if(it->first == archive)
	    {
		last_change.erase(it);
		break; // stops the while loop, as there is at most one element with that key
	    }
	    else
		++it;
	}

	return last_mod.empty() && last_change.empty();
    }

    void data_tree::listing(user_interaction & dialog) const
    {
	map<archive_num, infinint>::const_iterator it = last_mod.begin();
	map<archive_num, infinint>::const_iterator ut = last_change.begin();

	dialog.printf(gettext("Archive number |  Data      |  EA\n"));
	dialog.printf("---------------+------------+------------\n");

	while(it != last_mod.end() || ut != last_change.end())
	{
	    if(it != last_mod.end())
		if(ut != last_change.end())
		    if(it->first == ut->first)
		    {
			display_line(dialog, it->first, &(it->second), &(ut->second));
			++it;
			++ut;
		    }
		    else // not in the same archive
			if(it->first < ut->first) // it only
			{
			    display_line(dialog, it->first, &(it->second), NULL);
			    ++it;
			}
			else // ut only
			{
			    display_line(dialog, ut->first, NULL, &(ut->second));
			    ++ut;
			}
		else // ut at end of list thus it != last_mod.end() (see while condition)
		{
		    display_line(dialog, it->first, &(it->second), NULL);
		    ++it;
		}
	    else // it at end of list, this ut != last_change.end() (see while condition)
	    {
		display_line(dialog, ut->first, &(ut->second), NULL);
		++ut;
	    }
	}
    }

    void data_tree::apply_permutation(archive_num src, archive_num dst)
    {
	map<archive_num, infinint> transfert;
	map<archive_num, infinint>::iterator it = last_mod.begin();

	transfert.clear();
	while(it != last_mod.end())
	{
	    transfert[data_tree_permutation(src, dst, it->first)] = it->second;
	    ++it;
	}
	last_mod = transfert;

	transfert.clear();
	it = last_change.begin();
	while(it != last_change.end())
	{
	    transfert[data_tree_permutation(src, dst, it->first)] = it->second;
	    ++it;
	}
	last_change = transfert;
    }

    void data_tree::skip_out(archive_num num)
    {
	map<archive_num, infinint> resultant;
	map<archive_num, infinint>::iterator it = last_mod.begin();
	infinint tmp;

	while(it != last_mod.end())
	{
	    if(it->first > num)
		resultant[it->first-1] = it->second;
	    else
		resultant[it->first] = it->second;
	    ++it;
	}
	last_mod = resultant;
	resultant.clear();
	it = last_change.begin();
	while(it != last_change.end())
	{
	    if(it->first > num)
		resultant[it->first-1] = it->second;
	    else
		resultant[it->first] = it->second;
	    ++it;
	}
	last_change = resultant;
    }

    void data_tree::compute_most_recent_stats(vector<infinint> & data, vector<infinint> & ea,
					      vector<infinint> & total_data, vector<infinint> & total_ea) const
    {
	archive_num most_recent = 0;
	infinint max = 0;
	map<archive_num, infinint>::const_iterator it = last_mod.begin();

	while(it != last_mod.end())
	{
	    if(it->second >= max)
	    {
		most_recent = it->first;
		max = it->second;
	    }
	    ++total_data[it->first];
	    ++it;
	}
	if(most_recent > 0)
	    ++data[most_recent];

	it = last_change.begin();

	max = 0;
	most_recent = 0;
	while(it != last_change.end())
	{
	    if(it->second >= max)
	    {
		most_recent = it->first;
		max = it->second;
	    }
	    ++total_ea[it->first];
	    ++it;
	}
	if(most_recent > 0)
	    ++ea[most_recent];
    }

////////////////////////////////////////////////////////////////

    data_dir::data_dir(const string &name) : data_tree(name)
    {
	rejetons.clear();
    }

    data_dir::data_dir(generic_file &f) : data_tree(f)
    {
	infinint tmp = infinint(f.get_gf_ui(), NULL, &f); // number of children
	data_tree *entry = NULL;
	rejetons.clear();

	try
	{
	    while(tmp > 0)
	    {
		entry = read_from_file(f);
		if(entry == NULL)
		    throw Erange("data_dir::data_dir", gettext("Unexpected end of file"));
		rejetons.push_back(entry);
		entry = NULL;
		--tmp;
	    }
	}
	catch(...)
	{
	    list<data_tree *>::iterator next = rejetons.begin();
	    while(next != rejetons.end())
		delete *next++;
	    if(entry != NULL)
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
	list<data_tree *>::iterator next = rejetons.begin();
	while(next != rejetons.end())
	{
	    delete *next;
	    ++next;
	}
    }

    void data_dir::dump(generic_file & f) const
    {
	list<data_tree *>::const_iterator it = rejetons.begin();
	infinint tmp = rejetons.size();

	data_tree::dump(f);
	tmp.dump(f);
	while(it != rejetons.end())
	{
	    if(*it == NULL)
		throw SRC_BUG;
	    (*it)->dump(f);
	    ++it;
	}
    }


    void data_dir::add(const inode *entry, const archive_num & archive)
    {
	const data_tree *fils = read_child(entry->get_name());
	const directory *entry_dir = dynamic_cast<const directory *>(entry);
	data_tree *fils_mod = NULL;

	if(fils == NULL) // brand-new data_tree to build
	{
	    if(entry_dir != NULL)
		fils_mod = new data_dir(entry->get_name());
	    else
		fils_mod = new data_tree(entry->get_name());
	    if(fils_mod == NULL)
		throw Ememory("data_dir::add");
	    add_child(fils_mod);
	}
	else  // already saved in another archive
	{
		// check if dir/file nature has changed
	    const data_dir *fils_dir = dynamic_cast<const data_dir *>(fils);
	    if(fils_dir == NULL && entry_dir != NULL) // need to upgrade data_tree to data_dir
	    {
		fils_mod = new data_dir(*fils); // upgrade data_tree in an empty data_dir
		if(fils_mod == NULL)
		    throw Ememory("data_dir::add");
		try
		{
		    remove_child(entry->get_name());
		    add_child(fils_mod);
		}
		catch(...)
		{
		    delete fils_mod;
		    throw;
		}
	    }
	    else // no change in dir/file nature
		fils_mod = const_cast<data_tree *>(fils);
	}

	if(entry->get_saved_status() != s_not_saved)
	    fils_mod->set_data(archive, entry->get_last_modif());
	if(entry->ea_get_saved_status() == inode::ea_full || entry->ea_get_saved_status() == inode::ea_fake)
	    fils_mod->set_EA(archive, entry->get_last_change());
    }

    const data_tree *data_dir::read_child(const string & name) const
    {
	list<data_tree *>::const_iterator it = rejetons.begin();

	while(it != rejetons.end() && *it != NULL && (*it)->get_name() != name)
	    ++it;

	if(it == rejetons.end())
	    return NULL;
	else
	    if(*it == NULL)
		throw SRC_BUG;
	    else
		return *it;
    }

    void data_dir::read_all_children(vector<string> & fils) const
    {
	list<data_tree *>::const_iterator it = rejetons.begin();

	fils.clear();
	while(it != rejetons.end())
	    fils.push_back((*it++)->get_name());
    }

    bool data_dir::remove_all_from(const archive_num & archive)
    {
	list<data_tree *>::iterator it = rejetons.begin();

	while(it != rejetons.end())
	{
	    if((*it) == NULL)
		throw SRC_BUG;
	    if((*it)->remove_all_from(archive))
	    {
		delete *it; // release the memory used by the object
		rejetons.erase(it); // remove the entry from the list
		it = rejetons.begin(); // does not seems "it" points to the next item after erase
	    }
	    else
		++it;
	}

	return data_tree::remove_all_from(archive) && rejetons.size() == 0;
    }

    void data_dir::show(user_interaction & dialog, archive_num num, string marge) const
    {
	list<data_tree *>::const_iterator it = rejetons.begin();
	archive_num ou;
	bool data, ea;
	string etat, name;

	while(it != rejetons.end())
	{
	    if(*it == NULL)
		throw SRC_BUG;
	    data_dir *dir = dynamic_cast<data_dir *>(*it);
	    data = (*it)->get_data(ou, 0) && (ou == num || num == 0);
	    ea = (*it)->get_EA(ou, 0) && (ou == num || num == 0);
	    name = (*it)->get_name();
	    if(data || ea)
	    {
		etat = string(data ? gettext("[Data]") : "[    ]") + (ea ? "[EA]" : "[  ]");
		if(dialog.get_use_dar_manager_show_files())
		    dialog.dar_manager_show_files(name, data, ea);
		else
		    dialog.printf("%S  %S%S\n", &etat, &marge, &name);
	    }
	    if(dir != NULL)
		dir->show(dialog, num, marge+name+"/");
	    ++it;
	}
    }

    void data_dir::apply_permutation(archive_num src, archive_num dst)
    {
	list<data_tree *>::iterator it = rejetons.begin();

	data_tree::apply_permutation(src, dst);
	while(it != rejetons.end())
	{
	    (*it)->apply_permutation(src, dst);
	    ++it;
	}
    }


    void data_dir::skip_out(archive_num num)
    {
	list<data_tree *>::iterator it = rejetons.begin();

	data_tree::skip_out(num);
	while(it != rejetons.end())
	{
	    (*it)->skip_out(num);
	    ++it;
	}
    }

    void data_dir::compute_most_recent_stats(vector<infinint> & data, vector<infinint> & ea,
					     vector<infinint> & total_data, vector<infinint> & total_ea) const
    {
	list<data_tree *>::const_iterator it = rejetons.begin();

	data_tree::compute_most_recent_stats(data, ea, total_data, total_ea);
	while(it != rejetons.end())
	{
	    (*it)->compute_most_recent_stats(data, ea, total_data, total_ea);
	    ++it;
	}
    }

    void data_dir::add_child(data_tree *fils)
    {
	if(fils == NULL)
	    throw SRC_BUG;
	rejetons.push_back(fils);
    }

    void data_dir::remove_child(const string & name)
    {
	list<data_tree *>::iterator it = rejetons.begin();

	while(it != rejetons.end() && *it != NULL && (*it)->get_name() != name)
	    ++it;

	if(it != rejetons.end())
	{
	    if(*it == NULL)
		throw SRC_BUG;
	    else
		rejetons.erase(it);
	}
    }


////////////////////////////////////////////////////////////////

    data_dir *data_tree_read(generic_file & f)
    {
	data_tree *lu = read_from_file(f);
	data_dir *ret = dynamic_cast<data_dir *>(lu);

	if(ret == NULL && lu != NULL)
	    delete lu;

	return ret;
    }

    bool data_tree_find(path chemin, const data_dir & racine, const data_tree *& ptr)
    {
	string filename;
	const data_dir *current = &racine;
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
	    if(ptr == NULL)
		loop = false;
	    if(loop)
	    {
		current = dynamic_cast<const data_dir *>(ptr);
		if(current == NULL)
		{
		    loop = false;
		    ptr = NULL;
		}
	    }
	}

	return ptr != NULL;
    }

    void data_tree_update_with(const directory *dir, archive_num archive, data_dir *racine)
    {
	const nomme *entry;

	dir->reset_read_children();
	while(dir->read_children(entry))
	{
	    const directory *entry_dir = dynamic_cast<const directory *>(entry);
	    const inode *entry_ino = dynamic_cast<const inode *>(entry);

	    if(entry_ino == NULL)
		continue; // continue with next loop
	    else
		racine->add(entry_ino, archive);

	    if(entry_dir != NULL) // going into recursion
	    {
		data_tree *new_root = const_cast<data_tree *>(racine->read_child(entry->get_name()));
		data_dir *new_root_dir = dynamic_cast<data_dir *>(new_root);

		if(new_root == NULL)
		    throw SRC_BUG; // the racine->add method did not add an item for "entry"
		if(new_root_dir == NULL)
		    throw SRC_BUG; // the racine->add method did not add an data_dir item
		data_tree_update_with(entry_dir, archive, new_root_dir);
	    }
	}
    }

    archive_num data_tree_permutation(archive_num src, archive_num dst, archive_num x)
    {
	if(src < dst)
	    if(x < src || x > dst)
		return x;
	    else
		if(x == src)
		    return dst;
		else
		    return x-1;
	else
	    if(src == dst)
		return x;
	    else // src > dst
		if(x > src || x < dst)
		    return x;
		else
		    if(x == src)
			return dst;
		    else
			return x+1;
    }

} // end of namespace


////////////////////////////////////////////////////////////////

static data_tree *read_from_file(generic_file & f)
{
    char sign;
    data_tree *ret;

    if(f.read(&sign, 1) != 1)
        return NULL; // nothing more to read

    if(sign == data_tree::signature())
        ret = new data_tree(f);
    else if (sign == data_dir::signature())
        ret = new data_dir(f);
    else
        throw Erange("read_from_file", gettext("Unknown record type"));

    if(ret == NULL)
        throw Ememory("read_from_file");

    return ret;
}

static void read_from_file(generic_file &f, archive_num &a)
{
    char buffer[sizeof(archive_num)];
    archive_num *ptr = (archive_num *)&(buffer[0]);

    f.read(buffer, sizeof(archive_num));
    a = ntohs(*ptr);
}

static void write_to_file(generic_file &f, archive_num a)
{
    char buffer[sizeof(archive_num)];
    archive_num *ptr = (archive_num *)&(buffer[0]);

    *ptr = htons(a);
    f.write(buffer, sizeof(archive_num));
}

static void display_line(user_interaction & dialog, archive_num num, const infinint *data, const infinint *ea)
{
    string data_date = data == NULL ? "   " : tools_display_date(*data);
    string ea_date = ea == NULL ? "   " : tools_display_date(*ea);

    if(dialog.get_use_dar_manager_show_version())
	dialog.dar_manager_show_version(num, data_date, ea_date);
    else
	dialog.printf(" \t%u\t%S\t%S\n", num, &data_date, &ea_date);
}

