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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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

	// static field of class cat_directory

    const cat_eod cat_directory::fin;

	// methods of class cat_directory

    cat_directory::cat_directory(const infinint & xuid, const infinint & xgid, U_16 xperm,
				 const datetime & last_access,
				 const datetime & last_modif,
				 const datetime & last_change,
				 const string & xname,
				 const infinint & fs_device) : cat_inode(xuid, xgid, xperm, last_access, last_modif, last_change, xname, fs_device)
    {
	parent = nullptr;
#ifdef LIBDAR_FAST_DIR
	fils.clear();
#endif
	ordered_fils.clear();
	it = ordered_fils.begin();
	set_saved_status(s_saved);
	recursive_has_changed = true;
	updated_sizes = false;
    }

    cat_directory::cat_directory(const cat_directory &ref) : cat_inode(ref)
    {
	parent = nullptr;
#ifdef LIBDAR_FAST_DIR
	fils.clear();
#endif
	ordered_fils.clear();
	it = ordered_fils.begin();
	recursive_has_changed = ref.recursive_has_changed;
	updated_sizes = false;
    }

    const cat_directory & cat_directory::operator = (const cat_directory & ref)
    {
	const cat_inode *ref_ino = &ref;
	cat_inode * this_ino = this;

	*this_ino = *ref_ino; // this assigns the inode part of the object
	    // we don't modify the existing subfiles or subdirectories nor we copy them from the reference cat_directory
	recursive_flag_size_to_update();
	return *this;
    }


    cat_directory::cat_directory(user_interaction & dialog,
				 const smart_pointer<pile_descriptor> & pdesc,
				 const archive_version & reading_ver,
				 saved_status saved,
				 entree_stats & stats,
				 std::map <infinint, cat_etoile *> & corres,
				 compression default_algo,
				 bool lax,
				 bool only_detruit,
				 bool small) : cat_inode(dialog, pdesc, reading_ver, saved, small)
    {
	cat_entree *p;
	cat_nomme *t;
	cat_directory *d;
	cat_detruit *x;
	cat_mirage *m;
	cat_eod *fin = nullptr;
	bool lax_end = false;

	parent = nullptr;
#ifdef LIBDAR_FAST_DIR
	fils.clear();
#endif
	ordered_fils.clear();
	recursive_has_changed = true; // need to call recursive_has_changed_update() first if this fields has to be used
	updated_sizes = false;

	try
	{
	    while(fin == nullptr && !lax_end)
	    {
		try
		{
		    p = cat_entree::read(dialog, get_pool(), pdesc, reading_ver, stats, corres, default_algo, lax, only_detruit, small);
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
			p = nullptr;
		    }
		}

		if(p != nullptr)
		{
		    d = dynamic_cast<cat_directory *>(p);
		    fin = dynamic_cast<cat_eod *>(p);
		    t = dynamic_cast<cat_nomme *>(p);
		    x = dynamic_cast<cat_detruit *>(p);
		    m = dynamic_cast<cat_mirage *>(p);

		    if(!only_detruit || d != nullptr || x != nullptr || fin != nullptr || m != nullptr)
		    {
			    // we must add the cat_mirage object, else
			    // we will trigger an incoherent catalogue structure
			    // as the cat_mirage without inode cannot link to the cat_mirage with inode
			    // carring the same etiquette if we destroy them right now.
			if(t != nullptr) // p is a "cat_nomme"
			{
#ifdef LIBDAR_FAST_DIR
			    fils[t->get_name()] = t;
#endif
			    ordered_fils.push_back(t);
			}
			if(d != nullptr) // p is a cat_directory
			    d->parent = this;
			if(t == nullptr && fin == nullptr)
			    throw SRC_BUG; // neither an cat_eod nor a cat_nomme ! what's that ???
		    }
		    else
		    {
			delete p;
			p = nullptr;
			d = nullptr;
			fin = nullptr;
			t = nullptr;
			x = nullptr;
		    }
		}
		else
		    if(!lax)
			throw Erange("cat_directory::cat_directory", gettext("missing data to build a cat_directory"));
		    else
			lax_end = true;
	    }
	    if(fin != nullptr)
	    {
		delete fin; // no need to keep it
		fin = nullptr;
	    }

	    it = ordered_fils.begin();
	}
	catch(Egeneric & e)
	{
	    clear();
	    throw;
	}
    }

    cat_directory::~cat_directory()
    {
	clear();
    }

    bool cat_directory::operator == (const cat_entree & ref) const
    {
	const cat_directory *ref_dir = dynamic_cast<const cat_directory *>(&ref);

	if(ref_dir == nullptr)
	    return false;
	else
	    return cat_inode::operator == (ref);
    }

    void cat_directory::inherited_dump(const pile_descriptor & pdesc, bool small) const
    {
	list<cat_nomme *>::const_iterator x = ordered_fils.begin();

	cat_inode::inherited_dump(pdesc, small);
	if(!small)
	{
	    while(x != ordered_fils.end())
	    {
		if(*x == nullptr)
		    throw SRC_BUG;
		if(dynamic_cast<cat_ignored *>(*x) != nullptr)
		    ++x; // "cat_ignored" need not to be saved, they are only useful when updating_destroyed
		else
		{
		    (*x)->specific_dump(pdesc, small);
		    ++x;
		}
	    }
	}
	    // else in small mode, we do not dump any children
	    // an inode may have children while small dump is asked
	    // when performing a merging operation

	fin.specific_dump(pdesc, small); // end of "this" cat_directory
	    // fin is a static constant variable of class cat_directory,
	    // this hack avoids recurrent construction/destruction of a cat_eod object.
    }

    void cat_directory::recursive_update_sizes() const
    {
	if(!updated_sizes)
	{
	    cat_directory *me = const_cast<cat_directory *>(this);

	    if(me == nullptr)
		throw SRC_BUG;
	    me->x_size = 0;
	    me->x_storage_size = 0;
	    list<cat_nomme *>::const_iterator it = ordered_fils.begin();
	    const cat_directory *f_dir = nullptr;
	    const cat_file *f_file = nullptr;

	    while(it != ordered_fils.end())
	    {
		if(*it == nullptr)
		    throw SRC_BUG;
		f_dir = dynamic_cast<const cat_directory *>(*it);
		f_file = dynamic_cast<const cat_file *>(*it);
		if(f_dir != nullptr)
		{
			// recursion occurs here
			// by calling get_size() and get_storage_size() of child directories
			/// which in turn will call the recursive_update_sizes() of child objects
		    me->x_size += f_dir->get_size();
		    me->x_storage_size += f_dir->get_storage_size();
		}
		else if(f_file != nullptr && f_file->get_saved_status() == s_saved)
		{
		    me->x_size += f_file->get_size();
		    if(!f_file->get_storage_size().is_zero() || f_file->get_sparse_file_detection_read())
			me->x_storage_size += f_file->get_storage_size();
		    else
			me->x_storage_size += f_file->get_size();
			// in very first archive formats, storage_size was set to zero to
			// indicate "no compression used"
			// the only way to have zero as storage_size is either file size is
			// zero or file is a sparse_file with only zeroed bytes. Sparse file
			// were not taken into account in that old archive that set storage_size
			// to zero to indicate the absence of compression
		}

		++it;
	    }
	    me->updated_sizes = true;
	}
    }

    void cat_directory::recursive_flag_size_to_update() const
    {
	cat_directory *me = const_cast<cat_directory *>(this);
	if(me == nullptr)
	    throw SRC_BUG;
	me->updated_sizes = false;
	if(parent != nullptr)
	    parent->recursive_flag_size_to_update();
    }

    void cat_directory::add_children(cat_nomme *r)
    {
	cat_directory *d = dynamic_cast<cat_directory *>(r);
	const cat_nomme *ancien_nomme;

	if(r == nullptr)
	    throw SRC_BUG;

	if(search_children(r->get_name(), ancien_nomme))  // same entry already present
	{
	    const cat_directory *a_dir = dynamic_cast<const cat_directory *>(ancien_nomme);

	    if(a_dir != nullptr && d != nullptr) // both directories : merging them
	    {
		a_dir = d; // updates the inode part, does not touch the cat_directory specific part as defined in the cat_directory::operator =
		list<cat_nomme *>::iterator xit = d->ordered_fils.begin();
		while(xit != d->ordered_fils.end())
		{
		    const_cast<cat_directory *>(a_dir)->add_children(*xit);
		    ++xit;
		}

		    // need to clear the lists of objects before destroying the cat_directory objects itself
		    // to avoid the destructor destroyed the director children that have been merged to the a_dir cat_directory
#ifdef LIBDAR_FAST_DIR
		d->fils.clear();
#endif
		d->ordered_fils.clear();
		delete r;
		r = nullptr;
		d = nullptr;
	    }
	    else // not directories: removing and replacing old entry
	    {
		if(ancien_nomme == nullptr)
		    throw SRC_BUG;

		    // removing the old object
		remove(ancien_nomme->get_name());
		ancien_nomme = nullptr;

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

	if(d != nullptr)
	    d->parent = this;

	recursive_flag_size_to_update();
    }

    void cat_directory::reset_read_children() const
    {
	cat_directory *moi = const_cast<cat_directory *>(this);
	moi->it = moi->ordered_fils.begin();
    }

    void cat_directory::end_read() const
    {
	cat_directory *moi = const_cast<cat_directory *>(this);
	moi->it = moi->ordered_fils.end();
    }

    bool cat_directory::read_children(const cat_nomme *&r) const
    {
	cat_directory *moi = const_cast<cat_directory *>(this);
	if(moi->it != moi->ordered_fils.end())
	{
	    r = *(moi->it);
	    ++(moi->it);
	    return true;
	}
	else
	    return false;
    }

    void cat_directory::tail_to_read_children()
    {
#ifdef LIBDAR_FAST_DIR
	map<string, cat_nomme *>::iterator dest;
	list<cat_nomme *>::iterator ordered_dest = it;

	while(ordered_dest != ordered_fils.end())
	{
	    try
	    {
		if(*ordered_dest == nullptr)
		    throw SRC_BUG;
		dest = fils.find((*ordered_dest)->get_name());
		fils.erase(dest);
		delete *ordered_dest;
		*ordered_dest = nullptr;
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

    void cat_directory::remove(const string & name)
    {

	    // locating old object in ordered_fils
	list<cat_nomme *>::iterator ot = ordered_fils.begin();

	while(ot != ordered_fils.end() && *ot != nullptr && (*ot)->get_name() != name)
	    ++ot;

	if(ot == ordered_fils.end())
	    throw Erange("cat_directory::remove", tools_printf(gettext("Cannot remove nonexistent entry %S from catalogue"), &name));

	if(*ot == nullptr)
	    throw SRC_BUG;


#ifdef LIBDAR_FAST_DIR
	    // localizing old object in fils
	map<string, cat_nomme *>::iterator ut = fils.find(name);
	if(ut == fils.end())
	    throw SRC_BUG;

	    // sanity checks
	if(*ot != ut->second)
	    throw SRC_BUG;

	    // removing reference from fils
	fils.erase(ut);
#endif

	    // recording the address of the object to remove
	cat_nomme *obj = *ot;

	    // removing its reference from ordered_fils
	    // and having "it" pointing to the entry following the
	    // removed one, if it would have been the next entry to be read
	if(it == ot)
	    it = ordered_fils.erase(ot);
	else
	    (void)ordered_fils.erase(ot);

	    // destroying the object itself
	delete obj;
	recursive_flag_size_to_update();
    }

    void cat_directory::recursively_set_to_unsaved_data_and_FSA()
    {
	list<cat_nomme *>::iterator it = ordered_fils.begin();
	cat_directory *n_dir = nullptr;
	cat_inode *n_ino = nullptr;
	cat_mirage *n_mir = nullptr;

	    // dropping info for the current cat_directory
	set_saved_status(s_not_saved);
	if(ea_get_saved_status() == cat_inode::ea_full)
	    ea_set_saved_status(cat_inode::ea_partial);
	if(fsa_get_saved_status() == cat_inode::fsa_full)
	    fsa_set_saved_status(cat_inode::fsa_partial);

	    // doing the same for each entry found in that cat_directory
	while(it != ordered_fils.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;

	    n_dir = dynamic_cast<cat_directory *>(*it);
	    n_ino = dynamic_cast<cat_inode *>(*it);
	    n_mir = dynamic_cast<cat_mirage *>(*it);

	    if(n_mir != nullptr)
		n_ino = n_mir->get_inode();

	    if(n_dir != nullptr)
		n_dir->recursively_set_to_unsaved_data_and_FSA();
	    else // nothing to do for cat_directory the recursive call does the job
	    {
		if(n_ino != nullptr)
		{
		    n_ino->set_saved_status(s_not_saved);
		    if(n_ino->ea_get_saved_status() == cat_inode::ea_full)
			n_ino->ea_set_saved_status(ea_partial);
		    if(n_ino->fsa_get_saved_status() == cat_inode::fsa_full)
			n_ino->fsa_set_saved_status(cat_inode::fsa_partial);
		}
	    }

	    ++it;
	}
    }

    void cat_directory::change_location(const smart_pointer<pile_descriptor> & pdesc)
    {
	list<cat_nomme *>::iterator tmp_it = ordered_fils.begin();

	cat_nomme::change_location(pdesc);
	while(tmp_it != ordered_fils.end())
	{
	    if(*tmp_it == nullptr)
		throw SRC_BUG;

	    (*tmp_it)->change_location(pdesc);
	    ++tmp_it;
	}
    }

    void cat_directory::clear()
    {
	it = ordered_fils.begin();
	while(it != ordered_fils.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    delete *it;
	    *it = nullptr;
	    ++it;
	}
#ifdef LIBDAR_FAST_DIR
	fils.clear();
#endif
	ordered_fils.clear();
	it = ordered_fils.begin();
	recursive_flag_size_to_update();
    }

    bool cat_directory::search_children(const string &name, const cat_nomme * & ptr) const
    {
#ifdef LIBDAR_FAST_DIR
	map<string, cat_nomme *>::const_iterator ut = fils.find(name);

	if(ut != fils.end())
	{
	    if(ut->second == nullptr)
		throw SRC_BUG;
	    ptr = ut->second;
	    if(ptr == nullptr)
		throw SRC_BUG;
	}
	else
	    ptr = nullptr;
#else
	list<cat_nomme *>::const_iterator ot = ordered_fils.begin();

	while(ot != ordered_fils.end() && *ot != nullptr && (*ot)->get_name() != name)
	    ++ot;

	if(ot != ordered_fils.end())
	{
	    ptr = *ot;
	    if(ptr == nullptr)
		throw SRC_BUG;
	}
	else
	    ptr = nullptr;
#endif
	return ptr != nullptr;
    }

    bool cat_directory::callback_for_children_of(user_interaction & dialog, const string & sdir, bool isolated) const
    {
	const cat_directory *current = this;
	const cat_nomme *next_nom = nullptr;
	const cat_directory *next_dir = nullptr;
	const cat_inode *next_ino = nullptr;
	const cat_detruit *next_detruit = nullptr;
	const cat_mirage *next_mir = nullptr;
	string segment;
	bool loop = true;
	const cat_nomme *tmp_nom;

	if(!dialog.get_use_listing())
	    throw Erange("cat_directory::callback_for_children_of", gettext("listing() method must be given"));

	if(sdir != "")
	{
	    path dir = sdir;

	    if(!dir.is_relative())
		throw Erange("cat_directory::callback_for_children_of", gettext("argument must be a relative path"));

		///////////////////////////
		// looking for the inner most cat_directory (basename of given path)
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
		    next_nom = const_cast<const cat_nomme *>(tmp_nom);
		    next_mir = dynamic_cast<const cat_mirage *>(next_nom);
		    if(next_mir != nullptr)
			next_dir = dynamic_cast<const cat_directory *>(next_mir->get_inode());
		    else
			next_dir = dynamic_cast<const cat_directory *>(next_nom);

		    if(next_dir != nullptr)
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
	    // calling listing() for each element of the "current" cat_directory
	    //

	if(current == nullptr)
	    throw SRC_BUG;

	loop = false; // loop now serves as returned value

	current->reset_read_children();
	while(current->read_children(next_nom))
	{
	    next_mir = dynamic_cast<const cat_mirage *>(next_nom);
	    if(next_mir != nullptr)
		next_ino = next_mir->get_inode();
	    else
		next_ino = dynamic_cast<const cat_inode *>(next_nom);
	    next_detruit = dynamic_cast<const cat_detruit *>(next_nom);
	    next_dir = dynamic_cast<const cat_directory *>(next_ino);
	    if(next_ino != nullptr)
	    {
		string a = local_perm(*next_ino, next_mir != nullptr);
		string b = local_uid(*next_ino);
		string c = local_gid(*next_ino);
		string d = local_size(*next_ino, false);
		string e = local_date(*next_ino);
		string f = local_flag(*next_ino, isolated, false);
		string g = next_ino->get_name();
		dialog.listing(f,a,b,c,d,e,g, next_dir != nullptr, next_dir != nullptr && next_dir->has_children());
		loop = true;
	    }
	    else
		if(next_detruit != nullptr)
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

    void cat_directory::recursive_has_changed_update() const
    {
	list<cat_nomme *>::const_iterator it = ordered_fils.begin();

	const_cast<cat_directory *>(this)->recursive_has_changed = false;
	while(it != ordered_fils.end())
	{
	    const cat_directory *d = dynamic_cast<cat_directory *>(*it);
	    const cat_inode *ino = dynamic_cast<cat_inode *>(*it);
	    if(d != nullptr)
	    {
		d->recursive_has_changed_update();
		const_cast<cat_directory *>(this)->recursive_has_changed |= d->get_recursive_has_changed();
	    }
	    if(ino != nullptr && !recursive_has_changed)
		const_cast<cat_directory *>(this)->recursive_has_changed |=
		    ino->get_saved_status() != s_not_saved
		    || ino->ea_get_saved_status() == ea_full
		    || ino->ea_get_saved_status() == ea_removed;
	    ++it;
	}
    }

    infinint cat_directory::get_tree_size() const
    {
	infinint ret = ordered_fils.size();
	const cat_directory *fils_dir = nullptr;

	list<cat_nomme *>::const_iterator ot = ordered_fils.begin();
	while(ot != ordered_fils.end())
	{
	    if(*ot == nullptr)
		throw SRC_BUG;
	    fils_dir = dynamic_cast<const cat_directory *>(*ot);
	    if(fils_dir != nullptr)
		ret += fils_dir->get_tree_size();

	    ++ot;
	}

	return ret;
    }

    infinint cat_directory::get_tree_ea_num() const
    {
	infinint ret = 0;

	list<cat_nomme *>::const_iterator it = ordered_fils.begin();

	while(it != ordered_fils.end())
	{
	    const cat_directory *fils_dir = dynamic_cast<const cat_directory *>(*it);
	    const cat_inode *fils_ino = dynamic_cast<const cat_inode *>(*it);
	    const cat_mirage *fils_mir = dynamic_cast<const cat_mirage *>(*it);

	    if(fils_mir != nullptr)
		fils_ino = fils_mir->get_inode();

	    if(fils_ino != nullptr)
		if(fils_ino->ea_get_saved_status() != ea_none && fils_ino->ea_get_saved_status() != ea_removed)
		    ++ret;
	    if(fils_dir != nullptr)
		ret += fils_dir->get_tree_ea_num();

	    ++it;
	}


	return ret;
    }

    infinint cat_directory::get_tree_mirage_num() const
    {
	infinint ret = 0;

	list<cat_nomme *>::const_iterator it = ordered_fils.begin();

	while(it != ordered_fils.end())
	{
	    const cat_directory *fils_dir = dynamic_cast<const cat_directory *>(*it);
	    const cat_mirage *fils_mir = dynamic_cast<const cat_mirage *>(*it);

	    if(fils_mir != nullptr)
		++ret;

	    if(fils_dir != nullptr)
		ret += fils_dir->get_tree_mirage_num();

	    ++it;
	}


	return ret;
    }

    void cat_directory::get_etiquettes_found_in_tree(map<infinint, infinint> & already_found) const
    {
	list<cat_nomme *>::const_iterator it = ordered_fils.begin();

	while(it != ordered_fils.end())
	{
	    const cat_mirage *fils_mir = dynamic_cast<const cat_mirage *>(*it);
	    const cat_directory *fils_dir = dynamic_cast<const cat_directory *>(*it);

	    if(fils_mir != nullptr)
	    {
		map<infinint, infinint>::iterator tiq = already_found.find(fils_mir->get_etiquette());
		if(tiq == already_found.end())
		    already_found[fils_mir->get_etiquette()] = 1;
		else
		    already_found[fils_mir->get_etiquette()] = tiq->second + 1;
		    // due to st::map implementation, it is not recommanded to modify an entry directly
		    // using a "pair" structure (the one that holds .first and .second fields)
	    }

	    if(fils_dir != nullptr)
		fils_dir->get_etiquettes_found_in_tree(already_found);

	    ++it;
	}
    }

    void cat_directory::remove_all_mirages_and_reduce_dirs()
    {
	list<cat_nomme *>::iterator curs = ordered_fils.begin();

	while(curs != ordered_fils.end())
	{
	    if(*curs == nullptr)
		throw SRC_BUG;
	    cat_directory *d = dynamic_cast<cat_directory *>(*curs);
	    cat_mirage *m = dynamic_cast<cat_mirage *>(*curs);
	    cat_nomme *n = dynamic_cast<cat_nomme *>(*curs);

		// sanity check
	    if((m != nullptr && n == nullptr) || (d != nullptr && n == nullptr))
		throw SRC_BUG;

		// recursive call
	    if(d != nullptr)
		d->remove_all_mirages_and_reduce_dirs();

	    if(m != nullptr || (d != nullptr && d->is_empty()))
	    {
#ifdef LIBDAR_FAST_DIR
		map<string, cat_nomme *>::iterator monfils = fils.find(n->get_name());

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

    void cat_directory::set_all_mirage_s_inode_wrote_field_to(bool val)
    {
	list <cat_nomme *>::iterator curs = ordered_fils.begin();
	cat_mirage *mir = nullptr;
	cat_directory *dir = nullptr;


	while(curs != ordered_fils.end())
	{
	    mir = dynamic_cast<cat_mirage *>(*curs);
	    dir = dynamic_cast<cat_directory *>(*curs);

	    if(mir != nullptr)
		mir->set_inode_wrote(val);
	    if(dir != nullptr)
		dir->set_all_mirage_s_inode_wrote_field_to(val);

	    ++curs;
	}
    }

    void cat_directory::set_all_mirage_s_inode_dumped_field_to(bool val)
    {
	list<cat_nomme *>::iterator curs = ordered_fils.begin();

	while(curs != ordered_fils.end())
	{
	    if(*curs == nullptr)
		throw SRC_BUG;
	    cat_directory *d = dynamic_cast<cat_directory *>(*curs);
	    cat_mirage *m = dynamic_cast<cat_mirage *>(*curs);

		// recursive call
	    if(d != nullptr)
		d->set_all_mirage_s_inode_dumped_field_to(val);

	    if(m != nullptr)
		m->set_inode_dumped(val);

	    ++curs;
	}
    }


} // end of namespace
