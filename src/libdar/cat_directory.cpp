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

extern "C"
{
} // end extern "C"

#include "cat_all_entrees.hpp"
#include "cat_tools.hpp"

using namespace std;

namespace libdar
{

	// static field of class directory

    const eod directory::fin;

	// methods of class directory

    directory::directory(const infinint & xuid, const infinint & xgid, U_16 xperm,
			 const datetime & last_access,
			 const datetime & last_modif,
			 const datetime & last_change,
			 const string & xname,
			 const infinint & fs_device) : inode(xuid, xgid, xperm, last_access, last_modif, last_change, xname, fs_device)
    {
	parent = NULL;
#ifdef LIBDAR_FAST_DIR
	fils.clear();
#endif
	ordered_fils.clear();
	it = ordered_fils.begin();
	set_saved_status(s_saved);
	recursive_has_changed = true;
	updated_sizes = false;
    }

    directory::directory(const directory &ref) : inode(ref)
    {
	parent = NULL;
#ifdef LIBDAR_FAST_DIR
	fils.clear();
#endif
	ordered_fils.clear();
	it = ordered_fils.begin();
	recursive_has_changed = ref.recursive_has_changed;
	updated_sizes = false;
    }

    const directory & directory::operator = (const directory & ref)
    {
	const inode *ref_ino = &ref;
	inode * this_ino = this;

	*this_ino = *ref_ino; // this assigns the inode part of the object
	    // we don't modify the existing subfiles or subdirectories nor we copy them from the reference directory
	recursive_flag_size_to_update();
	return *this;
    }


    directory::directory(user_interaction & dialog,
			 generic_file & f,
			 const archive_version & reading_ver,
			 saved_status saved,
			 entree_stats & stats,
			 std::map <infinint, etoile *> & corres,
			 compression default_algo,
			 generic_file *data_loc,
			 compressor *efsa_loc,
			 bool lax,
			 bool only_detruit,
			 escape *ptr) : inode(dialog, f, reading_ver, saved, efsa_loc, ptr)
    {
	cat_entree *p;
	nomme *t;
	directory *d;
	detruit *x;
	mirage *m;
	eod *fin = NULL;
	bool lax_end = false;

	parent = NULL;
#ifdef LIBDAR_FAST_DIR
	fils.clear();
#endif
	ordered_fils.clear();
	recursive_has_changed = true; // need to call recursive_has_changed_update() first if this fields has to be used
	updated_sizes = false;

	try
	{
	    while(fin == NULL && !lax_end)
	    {
		try
		{
		    p = cat_entree::read(dialog, get_pool(), f, reading_ver, stats, corres, default_algo, data_loc, efsa_loc, lax, only_detruit, ptr);
		}
		catch(Euser_abort & e)
		{
		    throw;
		}
		catch(Ethread_cancel & e)
		{
		    throw;
		}
		catch(Egeneric & e)
		{
		    if(!lax)
			throw;
		    else
		    {
			dialog.warning(string(gettext("LAX MODE: Error met building a catalogue entry, skipping this entry and continuing. Skipped error is: ")) + e.get_message());
			p = NULL;
		    }
		}

		if(p != NULL)
		{
		    d = dynamic_cast<directory *>(p);
		    fin = dynamic_cast<eod *>(p);
		    t = dynamic_cast<nomme *>(p);
		    x = dynamic_cast<detruit *>(p);
		    m = dynamic_cast<mirage *>(p);

		    if(!only_detruit || d != NULL || x != NULL || fin != NULL || m != NULL)
		    {
			    // we must add the mirage object, else
			    // we will trigger an incoherent catalogue structure
			    // as the mirages without inode cannot link to the mirage with inode
			    // carring the same etiquette if we destroy them right now.
			if(t != NULL) // p is a "nomme"
			{
#ifdef LIBDAR_FAST_DIR
			    fils[t->get_name()] = t;
#endif
			    ordered_fils.push_back(t);
			}
			if(d != NULL) // p is a directory
			    d->parent = this;
			if(t == NULL && fin == NULL)
			    throw SRC_BUG; // neither an eod nor a nomme ! what's that ???
		    }
		    else
		    {
			delete p;
			p = NULL;
			d = NULL;
			fin = NULL;
			t = NULL;
			x = NULL;
		    }
		}
		else
		    if(!lax)
			throw Erange("directory::directory", gettext("missing data to build a directory"));
		    else
			lax_end = true;
	    }
	    if(fin != NULL)
	    {
		delete fin; // no need to keep it
		fin = NULL;
	    }

	    it = ordered_fils.begin();
	}
	catch(Egeneric & e)
	{
	    clear();
	    throw;
	}
    }

    directory::~directory()
    {
	clear();
    }

    void directory::inherited_dump(generic_file & f, bool small) const
    {
	list<nomme *>::const_iterator x = ordered_fils.begin();
	inode::inherited_dump(f, small);

	if(!small)
	{
	    while(x != ordered_fils.end())
	    {
		if(*x == NULL)
		    throw SRC_BUG;
		if(dynamic_cast<ignored *>(*x) != NULL)
		    ++x; // "ignored" need not to be saved, they are only useful when updating_destroyed
		else
		{
		    (*x)->specific_dump(f, small);
		    ++x;
		}
	    }
	}
	    // else in small mode, we do not dump any children
	    // an inode may have children while small dump is asked
	    // when performing a merging operation

	fin.specific_dump(f, small); // end of "this" directory
	    // fin is a static constant variable of class directory,
	    // this hack avoids recurrent construction/destruction of a eod object.
    }

    void directory::recursive_update_sizes() const
    {
	if(!updated_sizes)
	{
	    directory *me = const_cast<directory *>(this);

	    if(me == NULL)
		throw SRC_BUG;
	    me->x_size = 0;
	    me->x_storage_size = 0;
	    list<nomme *>::const_iterator it = ordered_fils.begin();
	    const directory *f_dir = NULL;
	    const file *f_file = NULL;

	    while(it != ordered_fils.end())
	    {
		if(*it == NULL)
		    throw SRC_BUG;
		f_dir = dynamic_cast<const directory *>(*it);
		f_file = dynamic_cast<const file *>(*it);
		if(f_dir != NULL)
		{
			// recursion occurs here
			// by calling get_size() and get_storage_size() of child directories
			/// which in turn will call the recursive_update_sizes() of child objects
		    me->x_size += f_dir->get_size();
		    me->x_storage_size += f_dir->get_storage_size();
		}
		else if(f_file != NULL && f_file->get_saved_status() == s_saved)
		{
		    me->x_size += f_file->get_size();
		    if(f_file->get_storage_size() == 0)
			me->x_storage_size += f_file->get_size();
			// in old archives storage_size was set to zero to mean "no compression used"
		    else
			me->x_storage_size += f_file->get_storage_size();
		}

		++it;
	    }
	    me->updated_sizes = true;
	}
    }

    void directory::recursive_flag_size_to_update() const
    {
	directory *me = const_cast<directory *>(this);
	if(me == NULL)
	    throw SRC_BUG;
	me->updated_sizes = false;
	if(parent != NULL)
	    parent->recursive_flag_size_to_update();
    }

    void directory::add_children(nomme *r)
    {
	directory *d = dynamic_cast<directory *>(r);
	const nomme *ancien_nomme;

	if(r == NULL)
	    throw SRC_BUG;

	if(search_children(r->get_name(), ancien_nomme))  // same entry already present
	{
	    const directory *a_dir = dynamic_cast<const directory *>(ancien_nomme);

	    if(a_dir != NULL && d != NULL) // both directories : merging them
	    {
		a_dir = d; // updates the inode part, does not touch the directory specific part as defined in the directory::operator =
		list<nomme *>::iterator xit = d->ordered_fils.begin();
		while(xit != d->ordered_fils.end())
		{
		    const_cast<directory *>(a_dir)->add_children(*xit);
		    ++xit;
		}

		    // need to clear the lists of objects before destroying the directory objects itself
		    // to avoid the destructor destroyed the director children that have been merged to the a_dir directory
#ifdef LIBDAR_FAST_DIR
		d->fils.clear();
#endif
		d->ordered_fils.clear();
		delete r;
		r = NULL;
		d = NULL;
	    }
	    else // not directories: removing and replacing old entry
	    {
		if(ancien_nomme == NULL)
		    throw SRC_BUG;

		    // removing the old object
		remove(ancien_nomme->get_name());
		ancien_nomme = NULL;

		    // adding the new object
#ifdef LIBDAR_FAST_DIR
		fils[r->get_name()] = r;
#endif
		ordered_fils.push_back(r);
	    }
	}
	else // no conflict: adding
	{
#ifdef LIBDAR_FAST_DIR
	    fils[r->get_name()] = r;
#endif
	    ordered_fils.push_back(r);
	}

	if(d != NULL)
	    d->parent = this;

	recursive_flag_size_to_update();
    }

    void directory::reset_read_children() const
    {
	directory *moi = const_cast<directory *>(this);
	moi->it = moi->ordered_fils.begin();
    }

    void directory::end_read() const
    {
	directory *moi = const_cast<directory *>(this);
	moi->it = moi->ordered_fils.end();
    }

    bool directory::read_children(const nomme *&r) const
    {
	directory *moi = const_cast<directory *>(this);
	if(moi->it != moi->ordered_fils.end())
	{
	    r = *(moi->it);
	    ++(moi->it);
	    return true;
	}
	else
	    return false;
    }

    void directory::tail_to_read_children()
    {
#ifdef LIBDAR_FAST_DIR
	map<string, nomme *>::iterator dest;
	list<nomme *>::iterator ordered_dest = it;

	while(ordered_dest != ordered_fils.end())
	{
	    try
	    {
		if(*ordered_dest == NULL)
		    throw SRC_BUG;
		dest = fils.find((*ordered_dest)->get_name());
		fils.erase(dest);
		delete *ordered_dest;
		*ordered_dest = NULL;
		ordered_dest++;
	    }
	    catch(...)
	    {
		ordered_fils.erase(it, ordered_dest);
		throw;
	    }
	}
#endif
	ordered_fils.erase(it, ordered_fils.end());
	it = ordered_fils.end();
	recursive_flag_size_to_update();
    }

    void directory::remove(const string & name)
    {

	    // localizing old object in ordered_fils
	list<nomme *>::iterator ot = ordered_fils.begin();

	while(ot != ordered_fils.end() && *ot != NULL && (*ot)->get_name() != name)
	    ++ot;

	if(ot == ordered_fils.end())
	    throw Erange("directory::remove", tools_printf(gettext("Cannot remove nonexistent entry %S from catalogue"), &name));

	if(*ot == NULL)
	    throw SRC_BUG;


#ifdef LIBDAR_FAST_DIR
	    // localizing old object in fils
	map<string, nomme *>::iterator ut = fils.find(name);
	if(ut == fils.end())
	    throw SRC_BUG;

	    // sanity checks
	if(*ot != ut->second)
	    throw SRC_BUG;

	    // removing reference from fils
	fils.erase(ut);
#endif

	    // recoding the address of the object to remove
	nomme *obj = *ot;

	    // removing its reference from ordered_fils
	ordered_fils.erase(ot);

	    // destroying the object itself
	delete obj;
	reset_read_children();
	recursive_flag_size_to_update();
    }

    void directory::recursively_set_to_unsaved_data_and_FSA()
    {
	list<nomme *>::iterator it = ordered_fils.begin();
	directory *n_dir = NULL;
	inode *n_ino = NULL;
	mirage *n_mir = NULL;

	    // dropping info for the current directory
	set_saved_status(s_not_saved);
	if(ea_get_saved_status() == inode::ea_full)
	    ea_set_saved_status(inode::ea_partial);
	if(fsa_get_saved_status() == inode::fsa_full)
	    fsa_set_saved_status(inode::fsa_partial);

	    // doing the same for each entry found in that directory
	while(it != ordered_fils.end())
	{
	    if(*it == NULL)
		throw SRC_BUG;

	    n_dir = dynamic_cast<directory *>(*it);
	    n_ino = dynamic_cast<inode *>(*it);
	    n_mir = dynamic_cast<mirage *>(*it);

	    if(n_mir != NULL)
		n_ino = n_mir->get_inode();

	    if(n_dir != NULL)
		n_dir->recursively_set_to_unsaved_data_and_FSA();
	    else // nothing to do for directory the recursive call does the job
	    {
		if(n_ino != NULL)
		{
		    n_ino->set_saved_status(s_not_saved);
		    if(n_ino->ea_get_saved_status() == inode::ea_full)
			n_ino->ea_set_saved_status(ea_partial);
		    if(n_ino->fsa_get_saved_status() == inode::fsa_full)
			n_ino->fsa_set_saved_status(inode::fsa_partial);
		}
	    }

	    ++it;
	}
    }

    void directory::clear()
    {
	it = ordered_fils.begin();
	while(it != ordered_fils.end())
	{
	    if(*it == NULL)
		throw SRC_BUG;
	    delete *it;
	    *it = NULL;
	    ++it;
	}
#ifdef LIBDAR_FAST_DIR
	fils.clear();
#endif
	ordered_fils.clear();
	it = ordered_fils.begin();
	recursive_flag_size_to_update();
    }

    bool directory::search_children(const string &name, const nomme * & ptr) const
    {
#ifdef LIBDAR_FAST_DIR
	map<string, nomme *>::const_iterator ut = fils.find(name);

	if(ut != fils.end())
	{
	    if(ut->second == NULL)
		throw SRC_BUG;
	    ptr = ut->second;
	    if(ptr == NULL)
		throw SRC_BUG;
	}
	else
	    ptr = NULL;
#else
	list<nomme *>::const_iterator ot = ordered_fils.begin();

	while(ot != ordered_fils.end() && *ot != NULL && (*ot)->get_name() != name)
	    ++ot;

	if(ot != ordered_fils.end())
	{
	    ptr = *ot;
	    if(ptr == NULL)
		throw SRC_BUG;
	}
	else
	    ptr = NULL;
#endif
	return ptr != NULL;
    }

    bool directory::callback_for_children_of(user_interaction & dialog, const string & sdir, bool isolated) const
    {
	const directory *current = this;
	const nomme *next_nom = NULL;
	const directory *next_dir = NULL;
	const inode *next_ino = NULL;
	const detruit *next_detruit = NULL;
	const mirage *next_mir = NULL;
	string segment;
	bool loop = true;
	const nomme *tmp_nom;

	if(!dialog.get_use_listing())
	    throw Erange("directory::callback_for_children_of", gettext("listing() method must be given"));

	if(sdir != "")
	{
	    path dir = sdir;

	    if(!dir.is_relative())
		throw Erange("directory::callback_for_children_of", gettext("argument must be a relative path"));

		///////////////////////////
		// looking for the inner most directory (basename of given path)
		//

	    do
	    {
		if(!dir.pop_front(segment))
		{
		    segment = dir.display();
		    loop = false;
		}

		if(current->search_children(segment, tmp_nom))
		{
		    next_nom = const_cast<const nomme *>(tmp_nom);
		    next_mir = dynamic_cast<const mirage *>(next_nom);
		    if(next_mir != NULL)
			next_dir = dynamic_cast<const directory *>(next_mir->get_inode());
		    else
			next_dir = dynamic_cast<const directory *>(next_nom);

		    if(next_dir != NULL)
			current = next_dir;
		    else
			return false;
		}
		else
		    return false;
	    }
	    while(loop);
	}

	    ///////////////////////////
	    // calling listing() for each element of the "current" directory
	    //

	if(current == NULL)
	    throw SRC_BUG;

	loop = false; // loop now serves as returned value

	current->reset_read_children();
	while(current->read_children(next_nom))
	{
	    next_mir = dynamic_cast<const mirage *>(next_nom);
	    if(next_mir != NULL)
		next_ino = next_mir->get_inode();
	    else
		next_ino = dynamic_cast<const inode *>(next_nom);
	    next_detruit = dynamic_cast<const detruit *>(next_nom);
	    next_dir = dynamic_cast<const directory *>(next_ino);
	    if(next_ino != NULL)
	    {
		string a = local_perm(*next_ino, next_mir != NULL);
		string b = local_uid(*next_ino);
		string c = local_gid(*next_ino);
		string d = local_size(*next_ino);
		string e = local_date(*next_ino);
		string f = local_flag(*next_ino, isolated, false);
		string g = next_ino->get_name();
		dialog.listing(f,a,b,c,d,e,g, next_dir != NULL, next_dir != NULL && next_dir->has_children());
		loop = true;
	    }
	    else
		if(next_detruit != NULL)
		{
		    string a = next_detruit->get_name();
		    dialog.listing(REMOVE_TAG, "xxxxxxxxxx", "", "", "", "", a, false, false);
		    loop = true;
		}
		else
		    throw SRC_BUG; // unknown class
	}

	return loop;
    }

    void directory::recursive_has_changed_update() const
    {
	list<nomme *>::const_iterator it = ordered_fils.begin();

	const_cast<directory *>(this)->recursive_has_changed = false;
	while(it != ordered_fils.end())
	{
	    const directory *d = dynamic_cast<directory *>(*it);
	    const inode *ino = dynamic_cast<inode *>(*it);
	    if(d != NULL)
	    {
		d->recursive_has_changed_update();
		const_cast<directory *>(this)->recursive_has_changed |= d->get_recursive_has_changed();
	    }
	    if(ino != NULL && !recursive_has_changed)
		const_cast<directory *>(this)->recursive_has_changed |=
		    ino->get_saved_status() != s_not_saved
		    || ino->ea_get_saved_status() == ea_full
		    || ino->ea_get_saved_status() == ea_removed;
	    ++it;
	}
    }

    infinint directory::get_tree_size() const
    {
	infinint ret = ordered_fils.size();
	const directory *fils_dir = NULL;

	list<nomme *>::const_iterator ot = ordered_fils.begin();
	while(ot != ordered_fils.end())
	{
	    if(*ot == NULL)
		throw SRC_BUG;
	    fils_dir = dynamic_cast<const directory *>(*ot);
	    if(fils_dir != NULL)
		ret += fils_dir->get_tree_size();

	    ++ot;
	}

	return ret;
    }

    infinint directory::get_tree_ea_num() const
    {
	infinint ret = 0;

	list<nomme *>::const_iterator it = ordered_fils.begin();

	while(it != ordered_fils.end())
	{
	    const directory *fils_dir = dynamic_cast<const directory *>(*it);
	    const inode *fils_ino = dynamic_cast<const inode *>(*it);
	    const mirage *fils_mir = dynamic_cast<const mirage *>(*it);

	    if(fils_mir != NULL)
		fils_ino = fils_mir->get_inode();

	    if(fils_ino != NULL)
		if(fils_ino->ea_get_saved_status() != ea_none && fils_ino->ea_get_saved_status() != ea_removed)
		    ++ret;
	    if(fils_dir != NULL)
		ret += fils_dir->get_tree_ea_num();

	    ++it;
	}


	return ret;
    }

    infinint directory::get_tree_mirage_num() const
    {
	infinint ret = 0;

	list<nomme *>::const_iterator it = ordered_fils.begin();

	while(it != ordered_fils.end())
	{
	    const directory *fils_dir = dynamic_cast<const directory *>(*it);
	    const mirage *fils_mir = dynamic_cast<const mirage *>(*it);

	    if(fils_mir != NULL)
		++ret;

	    if(fils_dir != NULL)
		ret += fils_dir->get_tree_mirage_num();

	    ++it;
	}


	return ret;
    }

    void directory::get_etiquettes_found_in_tree(map<infinint, infinint> & already_found) const
    {
	list<nomme *>::const_iterator it = ordered_fils.begin();

	while(it != ordered_fils.end())
	{
	    const mirage *fils_mir = dynamic_cast<const mirage *>(*it);
	    const directory *fils_dir = dynamic_cast<const directory *>(*it);

	    if(fils_mir != NULL)
	    {
		map<infinint, infinint>::iterator tiq = already_found.find(fils_mir->get_etiquette());
		if(tiq == already_found.end())
		    already_found[fils_mir->get_etiquette()] = 1;
		else
		    already_found[fils_mir->get_etiquette()] = tiq->second + 1;
		    // due to st::map implementation, it is not recommanded to modify an entry directly
		    // using a "pair" structure (the one that holds .first and .second fields)
	    }

	    if(fils_dir != NULL)
		fils_dir->get_etiquettes_found_in_tree(already_found);

	    ++it;
	}
    }

    void directory::remove_all_mirages_and_reduce_dirs()
    {
	list<nomme *>::iterator curs = ordered_fils.begin();

	while(curs != ordered_fils.end())
	{
	    if(*curs == NULL)
		throw SRC_BUG;
	    directory *d = dynamic_cast<directory *>(*curs);
	    mirage *m = dynamic_cast<mirage *>(*curs);
	    nomme *n = dynamic_cast<nomme *>(*curs);

		// sanity check
	    if((m != NULL && n == NULL) || (d != NULL && n == NULL))
		throw SRC_BUG;

		// recursive call
	    if(d != NULL)
		d->remove_all_mirages_and_reduce_dirs();

	    if(m != NULL || (d != NULL && d->is_empty()))
	    {
#ifdef LIBDAR_FAST_DIR
		map<string, nomme *>::iterator monfils = fils.find(n->get_name());

		if(monfils == fils.end())
		    throw SRC_BUG;
		if(monfils->second != *curs)
		    throw SRC_BUG;
		fils.erase(monfils);
#endif
		curs = ordered_fils.erase(curs);
		    // curs now points to the next item
		delete n;
	    }
	    else
		++curs;
	}

	recursive_flag_size_to_update();
    }

    void directory::set_all_mirage_s_inode_dumped_field_to(bool val)
    {
	list<nomme *>::iterator curs = ordered_fils.begin();

	while(curs != ordered_fils.end())
	{
	    if(*curs == NULL)
		throw SRC_BUG;
	    directory *d = dynamic_cast<directory *>(*curs);
	    mirage *m = dynamic_cast<mirage *>(*curs);

		// recursive call
	    if(d != NULL)
		d->set_all_mirage_s_inode_dumped_field_to(val);

	    if(m != NULL)
		m->set_inode_dumped(val);

	    ++curs;
	}
    }


} // end of namespace

