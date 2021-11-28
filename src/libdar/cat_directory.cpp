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
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "cat_all_entrees.hpp"
#include "tools.hpp"

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
	set_saved_status(saved_status::saved);
	recursive_has_changed = true;
	updated_sizes = false;
    }

    cat_directory::cat_directory(const cat_directory & ref) : cat_inode(ref)
    {
	init();
	recursive_has_changed = ref.recursive_has_changed;
    }

    cat_directory::cat_directory(cat_directory && ref) noexcept: cat_inode(move(ref))
    {
	init();
	recursive_has_changed = move(ref.recursive_has_changed);
    }

    cat_directory & cat_directory::operator = (const cat_directory & ref)
    {
	    // this assigns the inode part of the object
 	    // we don't modify the existing subfiles or subdirectories nor we copy them from the reference cat_directory
	cat_inode::operator = (ref);

	recursive_flag_size_to_update();
	return *this;
    }

    cat_directory & cat_directory::operator = (cat_directory && ref) noexcept
    {
	    // this assigns the inode part of the object
 	    // we don't modify the existing subfiles or subdirectories nor we copy them from the reference cat_directory
	    // to stay coherent with the copy_from operation
	cat_inode::operator = (move(ref));

	recursive_flag_size_to_update();
	return *this;
    }

    cat_directory::cat_directory(const shared_ptr<user_interaction> & dialog,
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

	if(only_detruit)
	{
		// only detruit is used in sequential read mode
		// when the in-lined metadata has been completely
		// read. Then catalogue at end of file is read
		// using "only_detruit" to build a directory tree
		// only containing the detruit objects (file removed
		// since backup of reference was made).
		//
		// if sequential read mode is used over a pipe
		// we must drop the EA and FSA of the directories,
		// to avoid having the calling filtre_restore routine
		// to try restoring them. First they have already been
		// restored during the in-lined data/metadata reading
		// of the backup and doing so here would lead the
		// fitre_restore to ask to skip backward in the archive
		// to fetch EA and FSA
		//
		// if the underlying structure is a pipe, this will
		// fail, leading dar warning that skipping backward
		// on a pipe is not possible.
		//
		// for that reason, in "only_detruit" mode the catalogue
		// need to drop all EA and FSA of its directories
		// this is what is done here:

	    if(ea_get_saved_status() == ea_saved_status::full)
		ea_set_saved_status(ea_saved_status::partial);

	    if(fsa_get_saved_status() == fsa_saved_status::full)
		fsa_set_saved_status(fsa_saved_status::partial);
	}

	try
	{
	    while(fin == nullptr && !lax_end)
	    {
		try
		{
		    p = cat_entree::read(dialog, pdesc, reading_ver, stats, corres, default_algo, lax, only_detruit, small);
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
			dialog->message(string(gettext("LAX MODE: Error met building a catalogue entry, skipping this entry and continuing. Skipped error is: ")) + e.get_message());
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

    cat_directory::~cat_directory() noexcept(false)
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
	deque<cat_nomme *>::const_iterator x = ordered_fils.begin();

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
	    x_size = 0;
	    x_storage_size = 0;
	    deque<cat_nomme *>::const_iterator it = ordered_fils.begin();
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
		    x_size += f_dir->get_size();
		    x_storage_size += f_dir->get_storage_size();
		}
		else if(f_file != nullptr && (f_file->get_saved_status() == saved_status::saved || f_file->get_saved_status() == saved_status::delta))
		{
		    x_size += f_file->get_size();
		    if(!f_file->get_storage_size().is_zero() || f_file->get_sparse_file_detection_read())
			x_storage_size += f_file->get_storage_size();
		    else
			x_storage_size += f_file->get_size();
			// in very first archive formats, storage_size was set to zero to
			// indicate "no compression used"
			// the only way to have zero as storage_size is either file size is
			// zero or file is a sparse_file with only zeroed bytes. Sparse file
			// were not taken into account in that old archive that set storage_size
			// to zero to indicate the absence of compression
		}

		++it;
	    }
	    updated_sizes = true;
	}
    }

    void cat_directory::recursive_flag_size_to_update() const
    {
	if(updated_sizes)
	{
	    updated_sizes = false;
	    if(parent != nullptr)
		parent->recursive_flag_size_to_update();
	}
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
		deque<cat_nomme *>::iterator xit = d->ordered_fils.begin();
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
	it = ordered_fils.begin();
    }

    void cat_directory::end_read() const
    {
	    // "moi" is necessary to avoid assigning a const_iterator to an iterator
	cat_directory *moi = const_cast<cat_directory *>(this);
	moi->it = moi->ordered_fils.end();
    }

    bool cat_directory::read_children(const cat_nomme *&r) const
    {
	if(it != ordered_fils.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    r = *it;
	    ++it;
	    return true;
	}
	else
	    return false;
    }

    void cat_directory::erase_ordered_fils(deque<cat_nomme *>::const_iterator debut, deque<cat_nomme *>::const_iterator fin)
    {
	for(deque<cat_nomme *>::const_iterator ut = debut;
	    ut != fin;
	    ++ut)
	    if(*ut != nullptr)
		delete *ut;
	ordered_fils.erase(debut, fin);
    }


    void cat_directory::tail_to_read_children()
    {
#ifdef LIBDAR_FAST_DIR
	map<string, cat_nomme *>::iterator dest;
	deque<cat_nomme *>::const_iterator ordered_dest = it;

	while(ordered_dest != ordered_fils.end())
	{
	    try
	    {
		if(*ordered_dest == nullptr)
		    throw SRC_BUG;
		dest = fils.find((*ordered_dest)->get_name());
		if(dest == fils.end())
		    throw SRC_BUG;
		fils.erase(dest);
		ordered_dest++;
	    }
	    catch(...)
	    {
		erase_ordered_fils(it, ordered_dest);
		it = ordered_fils.end();
		throw;
	    }
	}
#endif
	erase_ordered_fils(it, ordered_fils.end());
	it = ordered_fils.end();
	recursive_flag_size_to_update();
    }

    void cat_directory::remove(const string & name)
    {

	    // locating old object in ordered_fils
	deque<cat_nomme *>::iterator ot = ordered_fils.begin();

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
	else // not removing the next to read object (the one pointed to by 'it')
	{
	    (void)ordered_fils.erase(ot);

		// however if we remove the last item of the deque
		// we must set 'it' to end() to avoid 'it' becoming
		// a dangling pointer:
	    if(ordered_fils.empty())
		it = ordered_fils.end();
	}

	    // destroying the object itself
	delete obj;
	recursive_flag_size_to_update();
    }

    void cat_directory::recursively_set_to_unsaved_data_and_FSA()
    {
	deque<cat_nomme *>::iterator it = ordered_fils.begin();
	cat_directory *n_dir = nullptr;
	cat_inode *n_ino = nullptr;
	cat_mirage *n_mir = nullptr;

	    // dropping info for the current cat_directory
	set_saved_status(saved_status::not_saved);
	if(ea_get_saved_status() == ea_saved_status::full)
	    ea_set_saved_status(ea_saved_status::partial);
	if(fsa_get_saved_status() == fsa_saved_status::full)
	    fsa_set_saved_status(fsa_saved_status::partial);

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
		    n_ino->set_saved_status(saved_status::not_saved);
		    if(n_ino->ea_get_saved_status() == ea_saved_status::full)
			n_ino->ea_set_saved_status(ea_saved_status::partial);
		    if(n_ino->fsa_get_saved_status() == fsa_saved_status::full)
			n_ino->fsa_set_saved_status(fsa_saved_status::partial);
		}
	    }

	    ++it;
	}
    }

    void cat_directory::change_location(const smart_pointer<pile_descriptor> & pdesc)
    {
	deque<cat_nomme *>::iterator tmp_it = ordered_fils.begin();

	cat_nomme::change_location(pdesc);
	while(tmp_it != ordered_fils.end())
	{
	    if(*tmp_it == nullptr)
		throw SRC_BUG;

	    (*tmp_it)->change_location(pdesc);
	    ++tmp_it;
	}
    }

    void cat_directory::init() noexcept
    {
    	parent = nullptr;
#ifdef LIBDAR_FAST_DIR
	fils.clear();
#endif
	ordered_fils.clear();
	it = ordered_fils.begin();
	updated_sizes = false;
    }

    void cat_directory::clear()
    {
#ifdef LIBDAR_FAST_DIR
	fils.clear();
#endif
	erase_ordered_fils(ordered_fils.begin(), ordered_fils.end());
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
	deque<cat_nomme *>::const_iterator ot = ordered_fils.begin();

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

    void cat_directory::recursive_has_changed_update() const
    {
	deque<cat_nomme *>::const_iterator it = ordered_fils.begin();

	recursive_has_changed = false;
	while(it != ordered_fils.end())
	{
	    const cat_directory *d = dynamic_cast<cat_directory *>(*it);
	    const cat_inode *ino = dynamic_cast<cat_inode *>(*it);
	    if(d != nullptr)
	    {
		d->recursive_has_changed_update();
		recursive_has_changed |= d->get_recursive_has_changed();
	    }
	    if(ino != nullptr && !recursive_has_changed)
		recursive_has_changed |=
		    ino->get_saved_status() != saved_status::not_saved
		    || ino->ea_get_saved_status() == ea_saved_status::full
		    || ino->ea_get_saved_status() == ea_saved_status::removed;
	    ++it;
	}
    }

    infinint cat_directory::get_tree_size() const
    {
	infinint ret = ordered_fils.size();
	const cat_directory *fils_dir = nullptr;

	deque<cat_nomme *>::const_iterator ot = ordered_fils.begin();
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

	deque<cat_nomme *>::const_iterator it = ordered_fils.begin();

	while(it != ordered_fils.end())
	{
	    const cat_directory *fils_dir = dynamic_cast<const cat_directory *>(*it);
	    const cat_inode *fils_ino = dynamic_cast<const cat_inode *>(*it);
	    const cat_mirage *fils_mir = dynamic_cast<const cat_mirage *>(*it);

	    if(fils_mir != nullptr)
		fils_ino = fils_mir->get_inode();

	    if(fils_ino != nullptr)
		if(fils_ino->ea_get_saved_status() != ea_saved_status::none
		   && fils_ino->ea_get_saved_status() != ea_saved_status::removed)
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

	deque<cat_nomme *>::const_iterator it = ordered_fils.begin();

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
	deque<cat_nomme *>::const_iterator it = ordered_fils.begin();

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
	deque<cat_nomme *>::iterator curs = ordered_fils.begin();

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

    void cat_directory::set_all_mirage_s_inode_wrote_field_to(bool val) const
    {
	deque <cat_nomme *>::const_iterator curs = ordered_fils.begin();
	const cat_mirage *mir = nullptr;
	const cat_directory *dir = nullptr;


	while(curs != ordered_fils.end())
	{
	    mir = dynamic_cast<const cat_mirage *>(*curs);
	    dir = dynamic_cast<const cat_directory *>(*curs);

	    if(mir != nullptr)
		mir->set_inode_wrote(val);
	    if(dir != nullptr)
		dir->set_all_mirage_s_inode_wrote_field_to(val);

	    ++curs;
	}
    }

    void cat_directory::set_all_mirage_s_inode_dumped_field_to(bool val) const
    {
	deque<cat_nomme *>::const_iterator curs = ordered_fils.begin();

	while(curs != ordered_fils.end())
	{
	    if(*curs == nullptr)
		throw SRC_BUG;
	    const cat_directory *d = dynamic_cast<const cat_directory *>(*curs);
	    const cat_mirage *m = dynamic_cast<const cat_mirage *>(*curs);

		// recursive call
	    if(d != nullptr)
		d->set_all_mirage_s_inode_dumped_field_to(val);

	    if(m != nullptr)
		m->set_inode_dumped(val);

	    ++curs;
	}
    }


} // end of namespace
