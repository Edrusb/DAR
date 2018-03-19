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

#ifdef STDC_HEADERS
#include <ctype.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif
} // end extern "C"

#include <typeinfo>
#include <algorithm>
#include <map>
#include "catalogue.hpp"
#include "tools.hpp"
#include "tronc.hpp"
#include "user_interaction.hpp"
#include "deci.hpp"
#include "header.hpp"
#include "defile.hpp"
#include "pile.hpp"
#include "sparse_file.hpp"
#include "fichier_local.hpp"
#include "macro_tools.hpp"
#include "null_file.hpp"
#include "range.hpp"
#include "cat_all_entrees.hpp"
#include "cat_tools.hpp"

using namespace std;

namespace libdar
{

    static bool local_check_dirty_seq(escape *ptr);

    static inline string yes_no(bool val) { return (val ? "yes" : "no"); }

    catalogue::catalogue(const std::shared_ptr<user_interaction> & ui,
			 const datetime & root_last_modif,
			 const label & data_name): mem_ui(ui), out_compare("/")
    {
	contenu = nullptr;

	try
	{
	    contenu = new (nothrow) cat_directory(0,0,0,datetime(0),root_last_modif,datetime(0),"root",0);
	    if(contenu == nullptr)
		throw Ememory("catalogue::catalogue(path)");
	    current_compare = contenu;
	    current_add = contenu;
	    current_read = contenu;
	    sub_tree = nullptr;
	    ref_data_name = data_name;
	}
	catch(...)
	{
	    if(contenu != nullptr)
		delete contenu;
	    throw;
	}

	stats.clear();
    }

    catalogue::catalogue(const std::shared_ptr<user_interaction> & ui,
			 const pile_descriptor &pdesc,
			 const archive_version & reading_ver,
			 compression default_algo,
			 bool lax,
			 const label & lax_layer1_data_name,
			 bool only_detruit): mem_ui(ui), out_compare("/")
    {
	string tmp;
	saved_status st;
	unsigned char base;
	map <infinint, cat_etoile *> corres;
	crc *calc_crc = nullptr;
	crc *read_crc = nullptr;
	contenu = nullptr;

	pdesc.check(false);

	try
	{
	    pdesc.stack->reset_crc(CAT_CRC_SIZE);
	    try
	    {
		if(reading_ver > 7)
		{
			// we first need to read the ref_data_name
		    try
		    {
			ref_data_name.read(*pdesc.stack);
		    }
		    catch(Erange & e)
		    {
			throw Erange("catalogue::catalogue(generic_file &)", gettext("incoherent catalogue structure"));
		    }

		}
		else
		    ref_data_name.clear(); // a cleared data_name is emulated for older archives

		if(lax)
		{
		    if(ref_data_name != lax_layer1_data_name && !lax_layer1_data_name.is_cleared())
		    {
			get_ui().message(gettext("LAX MODE: catalogue label does not match archive label, as if it was an extracted catalogue, assuming data corruption occurred and fixing the catalogue to be considered an a plain internal catalogue"));
			ref_data_name = lax_layer1_data_name;
		    }
		}

		cat_signature cat_sig(*pdesc.stack, reading_ver);

		if(!cat_sig.get_base_and_status(base, st) && !lax)
		    throw Erange("catalogue::catalogue(generic_file &)", gettext("incoherent catalogue structure"));
		if(base != 'd' && !lax)
		    throw Erange("catalogue::catalogue(generic_file &)", gettext("incoherent catalogue structure"));

		stats.clear();
		smart_pointer<pile_descriptor> spdesc(new (nothrow) pile_descriptor(pdesc));
		if(spdesc.is_null())
		    throw Ememory("catalogue::catalogue");
		contenu = new (nothrow) cat_directory(ui, spdesc, reading_ver, st, stats, corres, default_algo, lax, only_detruit, false);
		if(contenu == nullptr)
		    throw Ememory("catalogue::catalogue(path)");
		if(only_detruit)
		    contenu->remove_all_mirages_and_reduce_dirs();
		current_compare = contenu;
		current_add = contenu;
		current_read = contenu;
		sub_tree = nullptr;
	    }
	    catch(...)
	    {
		calc_crc = pdesc.stack->get_crc(); // keeping "f" in coherent status
		if(calc_crc != nullptr)
		{
		    delete calc_crc;
		    calc_crc = nullptr;
		}
		throw;
	    }
	    calc_crc = pdesc.stack->get_crc(); // keeping "f" incoherent status in any case
	    if(calc_crc == nullptr)
		throw SRC_BUG;

	    if(reading_ver > 7)
	    {
		bool force_crc_failure = false;

		try
		{
		    read_crc = create_crc_from_file(*pdesc.stack, nullptr);
		}
		catch(Egeneric & e)
		{
		    force_crc_failure = true;
		}

		if(force_crc_failure || read_crc == nullptr || calc_crc == nullptr || read_crc->get_size() != calc_crc->get_size() || *read_crc != *calc_crc)
		{
		    if(!lax)
			throw Erange("catalogue::catalogue(generic_file &)", gettext("CRC failed for table of contents (aka \"catalogue\")"));
		    else
			get_ui().pause(gettext("LAX MODE: CRC failed for catalogue, the archive contents is corrupted. This may even lead dar to see files in the archive that never existed, but this will most probably lead to other failures in restoring files. Shall we proceed anyway?"));
		}
	    }
	}
	catch(...)
	{
	    if(contenu != nullptr)
		delete contenu;
	    if(calc_crc != nullptr)
		delete calc_crc;
	    if(read_crc != nullptr)
		delete read_crc;
	    throw;
	}
	    // "contenu" must not be destroyed under normal terminaison!
	if(calc_crc != nullptr)
	    delete calc_crc;
	if(read_crc != nullptr)
	    delete read_crc;
    }

    catalogue & catalogue::operator = (const catalogue & ref)
    {
	detruire();

	    // now copying the catalogue's data

	out_compare = ref.out_compare;
	partial_copy_from(ref);

	return *this;
    }

    void catalogue::reset_read() const
    {
	current_read = contenu;
	contenu->reset_read_children();
    }

    void catalogue::end_read() const
    {
	current_read = contenu;
	contenu->end_read();
    }

    void catalogue::skip_read_to_parent_dir() const
    {
	cat_directory *tmp = current_read->get_parent();

	if(tmp == nullptr)
	    throw Erange("catalogue::skip_read_to_parent_dir", gettext("root does not have a parent directory"));
	current_read = tmp;
    }

    bool catalogue::read(const cat_entree * & ref) const
    {
	const cat_nomme *tmp;

	if(current_read->read_children(tmp))
	{
	    const cat_directory *dir = dynamic_cast<const cat_directory *>(tmp);
	    if(dir != nullptr)
	    {
		current_read = const_cast<cat_directory *>(dir);
		dir->reset_read_children();
	    }
	    ref = tmp;
	    return true;
	}
	else
	{
	    cat_directory *papa = current_read->get_parent();
	    ref = &r_eod;
	    if(papa == nullptr)
		return false; // we reached end of root, no cat_eod generation
	    else
	    {
		current_read = papa;
		return true;
	    }
	}
    }

    bool catalogue::read_if_present(string *name, const cat_nomme * & ref) const
    {
	const cat_nomme *tmp;

	if(current_read == nullptr)
	    throw Erange("catalogue::read_if_present", gettext("no current directory defined"));
	if(name == nullptr) // we have to go to parent directory
	{
	    if(current_read->get_parent() == nullptr)
		throw Erange("catalogue::read_if_present", gettext("root directory has no parent directory"));
	    else
		current_read = current_read->get_parent();
	    ref = nullptr;
	    return true;
	}
	else // looking for a real filename
	    if(current_read->search_children(*name, tmp))
	    {
		cat_directory *d = dynamic_cast<cat_directory *>(const_cast<cat_nomme *>(tmp));
		if(d != nullptr) // this is a directory need to chdir to it
		    current_read = d;
		ref = tmp;
		return true;
	    }
	    else // filename not present in current dir
		return false;
    }

    void catalogue::remove_read_entry(std::string & name)
    {
	if(current_read == nullptr)
	    throw Erange("catalogue::remove_read_entry", gettext("no current reading directory defined"));
	current_read->remove(name);
    }

    void catalogue::tail_catalogue_to_current_read()
    {
	while(current_read != nullptr)
	{
	    current_read->tail_to_read_children();
	    current_read = current_read->get_parent();
	}

	current_read = contenu;
    }

    void catalogue::reset_sub_read(const path &sub)
    {
	if(! sub.is_relative())
	    throw SRC_BUG;

	if(sub_tree != nullptr)
	    delete sub_tree;
	sub_tree = new (nothrow) path(sub);
	if(sub_tree == nullptr)
	    throw Ememory("catalogue::reset_sub_read");
	sub_count = -1; // must provide the path to subtree;
	reset_read();
    }

    bool catalogue::sub_read(user_interaction & ui,
			     const cat_entree * &ref)
    {
	string tmp;

	if(sub_tree == nullptr)
	    throw SRC_BUG; // reset_sub_read

	switch(sub_count)
	{
	case 0 : // sending oed to go back to the root
	    if(sub_tree->pop(tmp))
	    {
		ref = &r_eod;
		return true;
	    }
	    else
	    {
		ref = nullptr;
		delete sub_tree;
		sub_tree = nullptr;
		sub_count = -2;
		return false;
	    }
	case -2: // reading is finished
	    return false;
	case -1: // providing path to sub_tree
	    if(sub_tree->read_subdir(tmp))
	    {
		const cat_nomme *xtmp;

		if(current_read->search_children(tmp, xtmp))
		{
		    ref = xtmp;
		    const cat_directory *dir = dynamic_cast<const cat_directory *>(xtmp);

		    if(dir != nullptr)
		    {
			current_read = const_cast<cat_directory *>(dir);
			return true;
		    }
		    else
			if(sub_tree->read_subdir(tmp))
			{
			    ui.message(sub_tree->display() + gettext(" is not present in the archive"));
			    delete sub_tree;
			    sub_tree = nullptr;
			    sub_count = -2;
			    return false;
			}
			else // subdir is a single file (no tree))
			{
			    sub_count = 0;
			    return true;
			}
		}
		else
		{
		    ui.message(sub_tree->display() + gettext(" is not present in the archive"));
		    delete sub_tree;
		    sub_tree = nullptr;
		    sub_count = -2;
		    return false;
		}
	    }
	    else
	    {
		sub_count = 1;
		current_read->reset_read_children();
		    // now reading the sub_tree
		    // no break !
	    }

	default:
	    if(read(ref) && sub_count > 0)
	    {
		const cat_directory *dir = dynamic_cast<const cat_directory *>(ref);
		const cat_eod *fin = dynamic_cast<const cat_eod *>(ref);

		if(dir != nullptr)
		    sub_count++;
		if(fin != nullptr)
		    sub_count--;

		return true;
	    }
	    else
		throw SRC_BUG;
	}
    }

    void catalogue::reset_add()
    {
	current_add = contenu;
    }

    void catalogue::add(cat_entree *ref)
    {
	if(current_add == nullptr)
	    throw SRC_BUG;

	cat_eod *f = dynamic_cast<cat_eod *>(ref);

	if(f == nullptr) // ref is not cat_eod
	{
	    cat_nomme *n = dynamic_cast<cat_nomme *>(ref);
	    cat_directory *t = dynamic_cast<cat_directory *>(ref);

	    if(n == nullptr)
		throw SRC_BUG; // unknown type neither "cat_eod" nor "cat_nomme"
	    current_add->add_children(n);
	    if(t != nullptr) // ref is a directory
		current_add = t;
	    stats.add(ref);
	}
	else // ref is an cat_eod
	{
	    cat_directory *parent = current_add->get_parent();
	    if(parent == nullptr)
		throw SRC_BUG; // root has no parent directory, cannot change to it
	    else
		current_add = parent;
	    delete ref; // all data given throw add becomes owned by the catalogue object
	}
    }

    void catalogue::re_add_in(const string &subdirname)
    {
	const cat_nomme *sub = nullptr;

	if(current_add->search_children(subdirname, sub))
	{
	    const cat_directory *subdir = dynamic_cast<const cat_directory *>(sub);
	    if(subdir != nullptr)
		current_add = const_cast<cat_directory *>(subdir);
	    else
		throw Erange("catalogue::re_add_in", gettext("Cannot recurs in a non directory entry"));
	}
	else
	    throw Erange("catalogue::re_add_in", gettext("The entry to recurs in does not exist, cannot add further entry to that absent subdirectory"));
    }

    void catalogue::re_add_in_replace(const cat_directory &dir)
    {
	if(dir.has_children())
	    throw Erange("catalogue::re_add_in_replace", "Given argument must be an empty dir");
	re_add_in(dir.get_name());
	*current_add = dir; // the directory's 'operator =' method does preverse existing children of the left (assigned) operand
    }


    void catalogue::add_in_current_read(cat_nomme *ref)
    {
	if(current_read == nullptr)
	    throw SRC_BUG; // current read directory does not exists
	current_read->add_children(ref);
    }

    void catalogue::reset_compare() const
    {
	if(contenu == nullptr)
	    throw SRC_BUG;
	current_compare = contenu;
	out_compare = "/";
    }

    bool catalogue::compare(const cat_entree * target, const cat_entree * & extracted) const
    {
	const cat_mirage *mir = dynamic_cast<const cat_mirage *>(target);
	const cat_directory *dir = dynamic_cast<const cat_directory *>(target);
	const cat_eod *fin = dynamic_cast<const cat_eod *>(target);
	const cat_nomme *nom = dynamic_cast<const cat_nomme *>(target);

	if(mir != nullptr)
	    dir = dynamic_cast<const cat_directory *>(mir->get_inode());

	if(out_compare.degre() > 1) // actually scanning a nonexisting directory
	{
	    if(dir != nullptr)
		out_compare += dir->get_name();
	    else
		if(fin != nullptr)
		{
		    string tmp_s;

		    if(!out_compare.pop(tmp_s))
		    {
			if(out_compare.is_relative())
			    throw SRC_BUG; // should not be a relative path !!!
			else // both cases are bugs, but need to know which case is generating a bug
			    throw SRC_BUG; // out_compare.degre() > 0 but cannot pop !
		    }
		}

	    return false;
	}
	else // scanning an existing directory
	{
	    const cat_nomme *found;

	    if(fin != nullptr)
	    {
		cat_directory *tmp = current_compare->get_parent();
		if(tmp == nullptr)
		    throw Erange("catalogue::compare", gettext("root has no parent directory"));
		current_compare = tmp;
		extracted = target;
		return true;
	    }

	    if(nom == nullptr)
		throw SRC_BUG; // ref, is neither a cat_eod nor a cat_nomme ! what's that ???

	    if(current_compare->search_children(nom->get_name(), found))
	    {
		const cat_detruit *src_det = dynamic_cast<const cat_detruit *>(nom);
		const cat_detruit *dst_det = dynamic_cast<const cat_detruit *>(found);
		const cat_inode *src_ino = dynamic_cast<const cat_inode *>(nom);
		const cat_inode *dst_ino = dynamic_cast<const cat_inode *>(found);
		const cat_mirage *src_mir = dynamic_cast<const cat_mirage *>(nom);
		const cat_mirage *dst_mir = dynamic_cast<const cat_mirage *>(found);

		    // extracting cat_inode from hard links
		if(src_mir != nullptr)
		    src_ino = src_mir->get_inode();
		if(dst_mir != nullptr)
		    dst_ino = dst_mir->get_inode();

		    // updating internal structure to follow directory tree :
		if(dir != nullptr)
		{
		    const cat_directory *d_ext = dynamic_cast<const cat_directory *>(dst_ino);
		    if(d_ext != nullptr)
			current_compare = const_cast<cat_directory *>(d_ext);
		    else
			out_compare += dir->get_name();
		}

		    // now comparing the objects :
		if(src_ino != nullptr)
		    if(dst_ino != nullptr)
		    {
			if(!src_ino->same_as(*dst_ino))
			    return false;
		    }
		    else
			return false;
		else
		    if(src_det != nullptr)
			if(dst_det != nullptr)
			{
			    if(!dst_det->same_as(*dst_det))
				return false;
			}
			else
			    return false;
		    else
			throw SRC_BUG; // src_det == nullptr && src_ino == nullptr, thus a cat_nomme which is neither cat_detruit nor cat_inode !

		if(dst_mir != nullptr)
		    extracted = dst_mir->get_inode();
		else
		    extracted = found;
		return true;
	    }
	    else
	    {
		if(dir != nullptr)
		    out_compare += dir->get_name();
		return false;
	    }
	}
    }

    infinint catalogue::update_destroyed_with(const catalogue & ref)
    {
	cat_directory *current = contenu;
	const cat_nomme *ici;
	const cat_entree *projo;
	const cat_eod *pro_eod;
	const cat_directory *pro_dir;
	const cat_detruit *pro_det;
	const cat_nomme *pro_nom;
	const cat_mirage *pro_mir;
	infinint count = 0;

	ref.reset_read();
	while(ref.read(projo))
	{
	    pro_eod = dynamic_cast<const cat_eod *>(projo);
	    pro_dir = dynamic_cast<const cat_directory *>(projo);
	    pro_det = dynamic_cast<const cat_detruit *>(projo);
	    pro_nom = dynamic_cast<const cat_nomme *>(projo);
	    pro_mir = dynamic_cast<const cat_mirage *>(projo);

	    if(pro_eod != nullptr)
	    {
		cat_directory *tmp = current->get_parent();
		if(tmp == nullptr)
		    throw SRC_BUG; // reached root for "contenu", and not yet for "ref";
		current = tmp;
		continue;
	    }

	    if(pro_det != nullptr)
		continue;

	    if(pro_nom == nullptr)
		throw SRC_BUG; // neither an cat_eod nor a cat_nomme ! what's that ?

	    if(!current->search_children(pro_nom->get_name(), ici))
	    {
		unsigned char firm;

		if(pro_mir != nullptr)
		    firm = pro_mir->get_inode()->signature();
		else
		    firm = pro_nom->signature();

		cat_detruit *det_tmp = new (nothrow) cat_detruit(pro_nom->get_name(), firm, current->get_last_modif());
		if(det_tmp == nullptr)
		    throw Ememory("catalogue::update_destroyed_with");
		try
		{
		    current->add_children(det_tmp);
		}
		catch(...)
		{
		    delete det_tmp;
		    throw;
		}

		count++;
		if(pro_dir != nullptr)
		    ref.skip_read_to_parent_dir();
	    }
	    else
		if(pro_dir != nullptr)
		{
		    const cat_directory *ici_dir = dynamic_cast<const cat_directory *>(ici);

		    if(ici_dir != nullptr)
			current = const_cast<cat_directory *>(ici_dir);
		    else
			ref.skip_read_to_parent_dir();
		}
	}

	return count;
    }

    void catalogue::update_absent_with(const catalogue & ref, infinint aborting_next_etoile)
    {
	cat_directory *current = contenu;
	const cat_nomme *ici;
	const cat_entree *projo;
	const cat_eod *pro_eod;
	const cat_directory *pro_dir;
	const cat_detruit *pro_det;
	const cat_nomme *pro_nom;
	const cat_inode *pro_ino;
	const cat_mirage *pro_mir;
	map<infinint, cat_etoile *> corres_clone;
	    // for each etiquette from the reference catalogue
	    // gives an cloned or original cat_etoile object
	    // in the current catalogue

	ref.reset_read();
	while(ref.read(projo))
	{
	    pro_eod = dynamic_cast<const cat_eod *>(projo);
	    pro_dir = dynamic_cast<const cat_directory *>(projo);
	    pro_det = dynamic_cast<const cat_detruit *>(projo);
	    pro_nom = dynamic_cast<const cat_nomme *>(projo);
	    pro_ino = dynamic_cast<const cat_inode *>(projo);
	    pro_mir = dynamic_cast<const cat_mirage *>(projo);

	    if(pro_eod != nullptr)
	    {
		cat_directory *tmp = current->get_parent();
		if(tmp == nullptr)
		    throw SRC_BUG; // reached root for "contenu", and not yet for "ref";
		current = tmp;
		continue;
	    }

	    if(pro_det != nullptr)
		continue;

	    if(pro_nom == nullptr)
		throw SRC_BUG; // neither an cat_eod nor a cat_nomme! what's that?

	    if(pro_mir != nullptr)
		pro_ino = pro_mir->get_inode();
		// warning: the returned cat_inode's name is undefined
		// one must use the mirage's own name

	    if(pro_ino == nullptr)
		throw SRC_BUG; // a nome that is not an cat_inode nor a cat_detruit!? What's that?

	    if(!current->search_children(pro_nom->get_name(), ici))
	    {
		cat_entree *clo_ent = nullptr;
		cat_inode *clo_ino = nullptr;
		cat_directory *clo_dir = nullptr;
		cat_mirage *clo_mir = nullptr;
		cat_etoile *clo_eto = nullptr;

		try
		{
		    clo_ent = pro_ino->clone();
		    clo_ino = dynamic_cast<cat_inode *>(clo_ent);
		    clo_dir = dynamic_cast<cat_directory *>(clo_ent);

			// sanity checks

		    if(clo_ino == nullptr)
			throw SRC_BUG; // clone of an cat_inode is not an cat_inode???
		    if((clo_dir != nullptr) ^ (pro_dir != nullptr))
			throw SRC_BUG; // both must be nullptr or both must be non nullptr

			// converting cat_inode to unsaved entry

		    clo_ino->set_saved_status(saved_status::not_saved);
		    if(clo_ino->ea_get_saved_status() != ea_saved_status::none)
		    {
			if(clo_ino->ea_get_saved_status() == ea_saved_status::removed)
			    clo_ino->ea_set_saved_status(ea_saved_status::none);
			else
			    clo_ino->ea_set_saved_status(ea_saved_status::partial);
		    }

			// handling hard links

		    if(pro_mir != nullptr)
		    {
			try
			{
			    map<infinint, cat_etoile *>::iterator it = corres_clone.find(pro_mir->get_etiquette());
			    if(it == corres_clone.end())
			    {
				clo_eto = new (nothrow) cat_etoile(clo_ino, aborting_next_etoile++);
				if(clo_eto == nullptr)
				    throw Ememory("catalogue::update_absent_with");
				else
				    clo_ent = nullptr; // object now managed by clo_eto

				try
				{
				    corres_clone[pro_mir->get_etiquette()] = clo_eto;
				    clo_mir = new (nothrow) cat_mirage(pro_mir->get_name(), clo_eto);
				    if(clo_mir == nullptr)
					throw Ememory("catalogue::update_absent_with");
				}
				catch(...)
				{
				    if(clo_eto != nullptr)
					delete clo_eto;
				    throw;
				}
			    }
			    else // mapping already exists (a star already shines)
			    {
				    // we have cloned the cat_inode but we do not need it as
				    // an hard linked structure already exists
				delete clo_ent;
				clo_ent = nullptr;

				    // so we add a new reference to the existing hard linked structure
				clo_mir = new (nothrow) cat_mirage(pro_mir->get_name(), it->second);
				if(clo_mir == nullptr)
				    throw Ememory("catalogue::update_absent_with");
			    }

				// adding it to the catalogue

			    current->add_children(clo_mir);

			}
			catch(...)
			{
			    if(clo_mir != nullptr)
			    {
				delete clo_mir;
				clo_mir = nullptr;
			    }
			    throw;
			}
		    }
		    else // not a hard link entry
		    {
			    // adding it to the catalogue

			current->add_children(clo_ino);
			clo_ent = nullptr; // object now managed by the current catalogue
		    }

			// recusing in case of directory

		    if(clo_dir != nullptr)
		    {
			if(current->search_children(pro_ino->get_name(), ici))
			{
			    if((void *)ici != (void *)clo_dir)
				throw SRC_BUG;  // we have just added the entry we were looking for, but could find another one!?!
			    current = clo_dir;
			}
			else
			    throw SRC_BUG; // cannot find the entry we have just added!!!
		    }
		}
		catch(...)
		{
		    if(clo_ent != nullptr)
		    {
			delete clo_ent;
			clo_ent = nullptr;
		    }
		    throw;
		}
	    }
	    else // entry found in the current catalogue
	    {
		if(pro_dir != nullptr)
		{
		    const cat_directory *ici_dir = dynamic_cast<const cat_directory *>(ici);

		    if(ici_dir != nullptr)
			current = const_cast<cat_directory *>(ici_dir);
		    else
			ref.skip_read_to_parent_dir();
		}

		if(pro_mir != nullptr)
		{
		    const cat_mirage *ici_mir = dynamic_cast<const cat_mirage *>(ici);

		    if(ici_mir != nullptr && corres_clone.find(pro_mir->get_etiquette()) == corres_clone.end())
		    {
			    // no correspondance found
			    // so we add a one to the map

			corres_clone[pro_mir->get_etiquette()] = ici_mir->get_etoile();
		    }
		}
	    }
	}
    }

    void catalogue::listing(user_interaction & ui,
			    bool isolated,
			    const mask &selection,
			    const mask &subtree,
			    bool filter_unsaved,
			    bool list_ea,
			    bool sizes_in_bytes,
			    string marge) const
    {
	const cat_entree *e = nullptr;
	thread_cancellation thr;
	const string marge_plus = " |  ";
	const U_I marge_plus_length = marge_plus.size();
	defile juillet = FAKE_ROOT;
	const cat_eod tmp_eod;

	ui.message(tools_printf(gettext("Access mode    | User | Group | Size  |          Date                 | [Data ][D][ EA  ][FSA][Compr][S]|   Filename")));
	ui.message(tools_printf("---------------+------+-------+-------+-------------------------------+---------------------------------+-----------"));
	if(filter_unsaved)
	    contenu->recursive_has_changed_update();

	reset_read();
	while(read(e))
	{
	    const cat_eod *e_eod = dynamic_cast<const cat_eod *>(e);
	    const cat_directory *e_dir = dynamic_cast<const cat_directory *>(e);
	    const cat_detruit *e_det = dynamic_cast<const cat_detruit *>(e);
	    const cat_inode *e_ino = dynamic_cast<const cat_inode *>(e);
	    const cat_mirage *e_hard = dynamic_cast<const cat_mirage *>(e);
	    const cat_nomme *e_nom = dynamic_cast<const cat_nomme *>(e);

	    thr.check_self_cancellation();
	    juillet.enfile(e);

	    if(e_eod != nullptr)
	    {
		    // descendre la marge
		U_I length = marge.size();
		if(length >= marge_plus_length)
		    marge.erase(length - marge_plus_length, marge_plus_length);
		else
		    throw SRC_BUG;
		ui.message(tools_printf("%S +---", &marge));
	    }
	    else
		if(e_nom == nullptr)
		    throw SRC_BUG; // not an cat_eod nor a cat_nomme, what's that?
		else
		    if(subtree.is_covered(juillet.get_path()) && (e_dir != nullptr || selection.is_covered(e_nom->get_name())))
		    {
			if(e_det != nullptr)
			{
			    string tmp = e_nom->get_name();
			    string tmp_date = !e_det->get_date().is_null() ? tools_display_date(e_det->get_date()) : "Unknown date";
			    saved_status poub;
			    char type;
			    cat_signature cat_sig(e_det->get_signature(), e_det->get_saved_status());

			    if(!cat_sig.get_base_and_status((unsigned char &)type, poub))
				type = '?';
			    if(type == 'f')
				type = '-';
			    ui.message(tools_printf(gettext("%S [%c] [ REMOVED ENTRY ] (%S)  %S"), &marge, type, &tmp_date, &tmp));
			}
			else
			{
			    if(e_hard != nullptr)
				e_ino = e_hard->get_inode();

			    if(e_ino == nullptr)
				throw SRC_BUG;
			    else
				if(!filter_unsaved
				   || e_ino->get_saved_status() != saved_status::not_saved
				   || (e_ino->ea_get_saved_status() == ea_saved_status::full || e_ino->ea_get_saved_status() == ea_saved_status::fake)
				   || (e_dir != nullptr && e_dir->get_recursive_has_changed()))
				{
				    bool dirty_seq = local_check_dirty_seq(get_escape_layer());
				    string a = local_perm(*e_ino, e_hard != nullptr);
				    string b = local_uid(*e_ino);
				    string c = local_gid(*e_ino);
				    string d = local_size(*e_ino, sizes_in_bytes);
				    string e = local_date(*e_ino);
				    string f = local_flag(*e_ino, isolated, dirty_seq);
				    string g = e_nom->get_name();
				    if(list_ea && e_hard != nullptr)
				    {
					infinint tiq = e_hard->get_etiquette();
					g += tools_printf(" [%i] ", &tiq);
				    }

				    ui.message(tools_printf("%S%S\t%S\t%S\t%S\t%S\t%S %S", &marge, &a, &b, &c, &d, &e, &f, &g));
				    if(list_ea)
					local_display_ea(ui, e_ino, marge + gettext("      Extended Attribute: ["), "]");

				    if(e_dir != nullptr)
					marge += marge_plus;
				}
				else // not saved, filtered out
				{
				    if(e_dir != nullptr)
				    {
					skip_read_to_parent_dir();
					juillet.enfile(&tmp_eod);
				    }
				}
			}
		    }
		    else // filtered out
			if(e_dir != nullptr)
			{
			    skip_read_to_parent_dir();
			    juillet.enfile(&tmp_eod);
			}
		// else what was skipped is not a directory, nothing to do.
	}
    }

    void catalogue::tar_listing(user_interaction & ui,
				catalogue_listing_callback callback,
				void *context,
				bool isolated,
				const mask &selection,
				const mask &subtree,
				bool filter_unsaved,
				bool list_ea,
				bool sizes_in_bytes,
				string beginning) const
    {
	const cat_entree *e = nullptr;
	thread_cancellation thr;
	defile juillet = FAKE_ROOT;
	const cat_eod tmp_eod;

	if(callback == nullptr)
	{
	    ui.message(tools_printf(gettext("[Data ][D][ EA  ][FSA][Compr][S]| Permission | User  | Group | Size  |          Date                 |    filename")));
	    ui.message(tools_printf("--------------------------------+------------+-------+-------+-------+-------------------------------+------------"));
	}
	if(filter_unsaved)
	    contenu->recursive_has_changed_update();

	reset_read();
	while(read(e))
	{
	    string sep = beginning == "" ? "" : "/";
	    const cat_eod *e_eod = dynamic_cast<const cat_eod *>(e);
	    const cat_directory *e_dir = dynamic_cast<const cat_directory *>(e);
	    const cat_detruit *e_det = dynamic_cast<const cat_detruit *>(e);
	    const cat_inode *e_ino = dynamic_cast<const cat_inode *>(e);
	    const cat_mirage *e_hard = dynamic_cast<const cat_mirage *>(e);
	    const cat_nomme *e_nom = dynamic_cast<const cat_nomme *>(e);

	    thr.check_self_cancellation();
	    juillet.enfile(e);

	    if(e_eod != nullptr)
	    {
		string tmp;

		    // removing the last directory of the path contained in beginning
		path p = beginning;
		if(p.pop(tmp))
		    beginning = p.display();
		else
		    if(p.degre() == 1)
			beginning = "";
		    else
			throw SRC_BUG;
	    }
	    else
		if(e_nom == nullptr)
		    throw SRC_BUG;
		else
		{
		    if(subtree.is_covered(juillet.get_path()) && (e_dir != nullptr || selection.is_covered(e_nom->get_name())))
		    {
			if(e_det != nullptr)
			{
			    string tmp = e_nom->get_name();
			    string tmp_date = ! e_det->get_date().is_null() ? tools_display_date(e_det->get_date()) : "Unknown date";
			    if(callback != nullptr)
				(*callback)(context, REMOVE_TAG, "xxxxxxxxxx", "", "", "", tmp_date, beginning+sep+tmp, false, false);
			    else
			    {
				saved_status poub;
				char type;
				cat_signature cat_sig(e_det->get_signature(), e_det->get_saved_status());

				if(!cat_sig.get_base_and_status((unsigned char &)type, poub))
				    type = '?';
				if(type == 'f')
				    type = '-';
				ui.message(tools_printf("%s (%S) [%c] %S%S%S", REMOVE_TAG, &tmp_date, type,  &beginning, &sep, &tmp));
			    }
			}
			else
			{
			    if(e_hard != nullptr)
				e_ino = e_hard->get_inode();

			    if(e_ino == nullptr)
				throw SRC_BUG;
			    else
			    {
				string nom = e_nom->get_name();

				if(!filter_unsaved
				   || e_ino->get_saved_status() != saved_status::not_saved
				   || (e_ino->ea_get_saved_status() == ea_saved_status::full || e_ino->ea_get_saved_status() == ea_saved_status::fake)
				   || (e_dir != nullptr && e_dir->get_recursive_has_changed()))
				{
				    bool dirty_seq = local_check_dirty_seq(get_escape_layer());
				    string a = local_perm(*e_ino, e_hard != nullptr);
				    string b = local_uid(*e_ino);
				    string c = local_gid(*e_ino);
				    string d = local_size(*e_ino, sizes_in_bytes);
				    string e = local_date(*e_ino);
				    string f = local_flag(*e_ino, isolated, dirty_seq);

				    if(list_ea && e_hard != nullptr)
				    {
					infinint tiq = e_hard->get_etiquette();
					nom += tools_printf(" [%i] ", &tiq);
				    }

				    if(callback != nullptr)
					(*callback)(context, f, a, b, c, d, e, beginning+sep+nom, e_dir != nullptr, e_dir != nullptr && e_dir->has_children());
				    else
					ui.message(tools_printf("%S %S   %S\t%S\t%S\t%S\t%S%S%S", &f, &a, &b, &c, &d, &e, &beginning, &sep, &nom));
				    if(list_ea)
					local_display_ea(ui, e_ino, gettext("      Extended Attribute: ["), "]");

				    if(e_dir != nullptr)
					beginning += sep + nom;
				}
				else // not saved, filtered out
				{
				    if(e_dir != nullptr)
				    {
					juillet.enfile(&tmp_eod);
					skip_read_to_parent_dir();
				    }
				}
			    }
			}
		    }
		    else // excluded, filtered out
			if(e_dir != nullptr)
			{
			    juillet.enfile(&tmp_eod);
			    skip_read_to_parent_dir();
			}
			// else what was skipped is not a directory, nothing to do.
		}
	}
    }

    void catalogue::xml_listing(user_interaction &ui,
				bool isolated,
				const mask &selection,
				const mask &subtree,
				bool filter_unsaved,
				bool list_ea,
				bool sizes_in_bytes,
				string beginning) const
    {
	const cat_entree *e = nullptr;
	thread_cancellation thr;
	defile juillet = FAKE_ROOT;

	ui.message("<?xml version=\"1.0\" ?>");
	ui.message("<!DOCTYPE Catalog SYSTEM \"dar-catalog.dtd\">");
	ui.message("<Catalog format=\"1.2\">");
	if(filter_unsaved)
	    contenu->recursive_has_changed_update();

	reset_read();
	while(read(e))
	{
	    const cat_eod *e_eod = dynamic_cast<const cat_eod *>(e);
	    const cat_directory *e_dir = dynamic_cast<const cat_directory *>(e);
	    const cat_detruit *e_det = dynamic_cast<const cat_detruit *>(e);
	    const cat_inode *e_ino = dynamic_cast<const cat_inode *>(e);
	    const cat_mirage *e_hard = dynamic_cast<const cat_mirage *>(e);
	    const cat_lien *e_sym = dynamic_cast<const cat_lien *>(e);
	    const cat_device *e_dev = dynamic_cast<const cat_device *>(e);
	    const cat_nomme *e_nom = dynamic_cast<const cat_nomme *>(e);
	    const cat_eod tmp_eod;

	    thr.check_self_cancellation();
	    juillet.enfile(e);

	    if(e_eod != nullptr)
	    {
		U_I length = beginning.size();

		if(length > 0)
		    beginning.erase(length - 1, 1); // removing the last tab character
		else
		    throw SRC_BUG;
		ui.message(tools_printf("%S</Directory>", &beginning));
	    }
	    else
		if(e_nom == nullptr)
		    throw SRC_BUG;
		else
		{
		    if(subtree.is_covered(juillet.get_path()) && (e_dir != nullptr || selection.is_covered(e_nom->get_name())))
		    {
			string name = tools_output2xml(e_nom->get_name());

			if(e_det != nullptr)
			{
			    unsigned char sig = e_nom->signature();
			    string data = "deleted";
			    string metadata = "absent";

			    switch(sig)
			    {
			    case 'd':
				ui.message(tools_printf("%S<Directory name=\"%S\">", &beginning, &name));
				xml_listing_attributes(ui, beginning, data, metadata);
				ui.message(tools_printf("%S</Directory>", &beginning));
				break;
			    case 'f':
			    case 'h':
			    case 'e':
				ui.message(tools_printf("%S<File name=\"%S\">", &beginning, &name));
				xml_listing_attributes(ui, beginning, data, metadata);
				ui.message(tools_printf("%S</File>", &beginning));
				break;
			    case 'l':
				ui.message(tools_printf("%S<Symlink name=\"%S\">", &beginning, &name));
				xml_listing_attributes(ui, beginning, data, metadata);
				ui.message(tools_printf("%S</Symlink>", &beginning));
				break;
			    case 'c':
				ui.message(tools_printf("%S<Device name=\"%S\" type=\"character\">", &beginning, &name));
				xml_listing_attributes(ui, beginning, data, metadata);
				ui.message(tools_printf("%S</Device>", &beginning));
				break;
			    case 'b':
				ui.message(tools_printf("%S<Device name=\"%S\" type=\"block\">", &beginning, &name));
				xml_listing_attributes(ui, beginning, data, metadata);
				ui.message(tools_printf("%S</Device>", &beginning));
				break;
			    case 'p':
				ui.message(tools_printf("%S<Pipe name=\"%S\">", &beginning, &name));
				xml_listing_attributes(ui, beginning, data, metadata);
				ui.message(tools_printf("%S</Pipe>", &beginning));
				break;
			    case 's':
				ui.message(tools_printf("%S<Socket name=\"%S\">", &beginning, &name));
				xml_listing_attributes(ui, beginning, data, metadata);
				ui.message(tools_printf("%S</Socket>", &beginning));
				break;
			    default:
				throw SRC_BUG;
			    }
			}
			else // other than cat_detruit object
			{
			    if(e_hard != nullptr)
			    {
				e_ino = e_hard->get_inode();
				e_sym = dynamic_cast<const cat_lien *>(e_ino);
				e_dev = dynamic_cast<const cat_device *>(e_ino);
			    }

			    if(e_ino == nullptr)
				throw SRC_BUG; // this is a cat_nomme which is neither a cat_detruit nor an cat_inode

			    if(!filter_unsaved
			       || e_ino->get_saved_status() != saved_status::not_saved
			       || (e_ino->ea_get_saved_status() == ea_saved_status::full || e_ino->ea_get_saved_status() == ea_saved_status::fake)
			       || (e_dir != nullptr && e_dir->get_recursive_has_changed()))
			    {
				const cat_file * f_ino = dynamic_cast<const cat_file *>(e_ino);
				string data, metadata, maj, min, chksum, chkbasesum, chkresultsum, target, delta_sig;
				string dirty, sparse;
				string size = local_size(*e_ino, sizes_in_bytes);
				string stored = local_storage_size(*e_ino);
				const cat_file *reg = dynamic_cast<const cat_file *>(e_ino); // ino is no more it->second (if it->second was a cat_mirage)
				const crc *crc_tmp = nullptr;
				const crc *crc_patch_base = nullptr;
				const crc *crc_patch_result = nullptr;

				unsigned char sig = e_ino->signature();
				saved_status data_st = e_ino->get_saved_status();
				ea_saved_status ea_st = e_ino->ea_get_saved_status();

				if(isolated)
				{
				    if(data_st == saved_status::saved
				       && (f_ino == nullptr
					   || !f_ino->is_dirty()))
					data_st = saved_status::fake;
				    ea_st = ea_saved_status::fake;
				}

				if(stored == "0" && (reg == nullptr || !reg->get_sparse_file_detection_read()))
				    stored = size;

				    // defining "data" string

				switch(data_st)
				{
				case saved_status::saved:
				    data = "saved";
				    break;
				case saved_status::fake:
				case saved_status::not_saved:
				    data = "referenced";
				    break;
				case saved_status::delta:
				    data = "patch";
				    break;
				case saved_status::inode_only:
				    data = "inode-only";
				    break;
				default:
				    throw SRC_BUG;
				}

				    // defining "metadata" string

				switch(ea_st)
				{
				case ea_saved_status::full:
				    metadata = "saved";
				    break;
				case ea_saved_status::partial:
				case ea_saved_status::fake:
				    metadata = "referenced";
				    break;
				case ea_saved_status::none:
				case ea_saved_status::removed:
				    metadata = "absent";
				    break;
				default:
				    throw SRC_BUG;
				}

				    // building entry for each type of cat_inode

				switch(sig)
				{
				case 'd': // directories
				    ui.message(tools_printf("%S<Directory name=\"%S\">", &beginning, &name));
				    xml_listing_attributes(ui, beginning, data, metadata, e, list_ea);
				    beginning += "\t";
				    break;
				case 'h': // hard linked files
				case 'e': // hard linked files
				    throw SRC_BUG; // no more used in dynamic data
				case 'f': // plain files
				    if(data_st == saved_status::saved)
				    {
					if(f_ino == nullptr)
					    throw SRC_BUG;
					dirty = yes_no(f_ino->is_dirty());
					sparse= yes_no(f_ino->get_sparse_file_detection_read());
				    }
				    else
				    {
					stored = "";
					dirty = "";
					sparse = "";
				    }

				    if(reg == nullptr)
					throw SRC_BUG; // f is signature for plain files
				    if(reg->get_crc(crc_tmp) && crc_tmp != nullptr)
					chksum = crc_tmp->crc2str();
				    else
					chksum = "";

				    delta_sig = yes_no(reg->has_delta_signature_available());

				    if(reg->has_patch_base_crc() && reg->get_patch_base_crc(crc_patch_base) && crc_patch_base != nullptr)
					chkbasesum = crc_patch_base->crc2str();
				    else
					chkbasesum = "";

				    if(reg->has_patch_result_crc() && reg->get_patch_result_crc(crc_patch_result) && crc_patch_result != nullptr)
					chkresultsum = crc_patch_result->crc2str();
				    else
					chkresultsum = "";

				    ui.message(tools_printf("%S<File name=\"%S\" size=\"%S\" stored=\"%S\" crc=\"%S\" dirty=\"%S\" sparse=\"%S\" delta_sig=\"%S\" patch_base_crc=\"%S\" patch_result_crc=\"%S\">",
							    &beginning, &name, &size, &stored, &chksum, &dirty, &sparse, &delta_sig, &chkbasesum, &chkresultsum));
				    xml_listing_attributes(ui, beginning, data, metadata, e, list_ea);
				    ui.message(tools_printf("%S</File>", &beginning));
				    break;
				case 'l': // soft link
				    if(data_st == saved_status::saved)
					target = tools_output2xml(e_sym->get_target());
				    else
					target = "";
				    ui.message(tools_printf("%S<Symlink name=\"%S\" target=\"%S\">",
							    &beginning, &name, &target));
				    xml_listing_attributes(ui, beginning, data, metadata, e, list_ea);
				    ui.message(tools_printf("%S</Symlink>", &beginning));
				    break;
				case 'c':
				case 'b':
					// this is maybe less performant, to have both 'c' and 'b' here and
					// make an additional test, but this has the advantage to not duplicate
					// very similar code, which would obviously evoluate the same way.
					// Experience shows that two identical codes even when driven by the same need
					// are an important source of bugs, as one may forget to update both of them, the
					// same way...

				    if(sig == 'c')
					target = "character";
				    else
					target = "block";
					// we re-used target variable which is not used for the current cat_inode

				    if(data_st == saved_status::saved)
				    {
					maj = tools_uword2str(e_dev->get_major());
					min = tools_uword2str(e_dev->get_minor());
				    }
				    else
					maj = min = "";

				    ui.message(tools_printf("%S<Device name=\"%S\" type=\"%S\" major=\"%S\" minor=\"%S\">",
							    &beginning, &name, &target, &maj, &min));
				    xml_listing_attributes(ui, beginning, data, metadata, e, list_ea);
				    ui.message(tools_printf("%S</Device>", &beginning));
				    break;
				case 'p':
				    ui.message(tools_printf("%S<Pipe name=\"%S\">", &beginning, &name));
				    xml_listing_attributes(ui, beginning, data, metadata, e, list_ea);
				    ui.message(tools_printf("%S</Pipe>", &beginning));
				    break;
				case 's':
				    ui.message(tools_printf("%S<Socket name=\"%S\">", &beginning, &name));
				    xml_listing_attributes(ui, beginning, data, metadata, e, list_ea);
				    ui.message(tools_printf("%S</Socket>", &beginning));
				    break;
				default:
				    throw SRC_BUG;
				}
			    }
			    else // not saved, filtered out
			    {
				if(e_dir != nullptr)
				{
				    skip_read_to_parent_dir();
				    juillet.enfile(&tmp_eod);
				}
			    }
			}
		    }  // end of filter check
		    else // filtered out
			if(e_dir != nullptr)
			{
			    skip_read_to_parent_dir();
			    juillet.enfile(&tmp_eod);
			}
			// else what was skipped is not a directory, nothing to do.
		}
	}

	ui.message("</Catalog>");
    }

    void catalogue::slice_listing(user_interaction & ui,
				  bool isolated,
				  const mask & selection,
				  const mask & subtree,
				  const slice_layout & slicing) const
    {
	const cat_entree *e = nullptr;
	thread_cancellation thr;
	defile juillet = FAKE_ROOT;
	const cat_eod tmp_eod;
	range all_slices;
	range file_slices;

	ui.message("Slice(s)|[Data ][D][ EA  ][FSA][Compr][S]|Permission| Filemane");
	ui.message("--------+--------------------------------+----------+-----------------------------");
	reset_read();
	while(read(e))
	{
	    const cat_eod *e_eod = dynamic_cast<const cat_eod *>(e);
	    const cat_directory *e_dir = dynamic_cast<const cat_directory *>(e);
	    const cat_inode *e_ino = dynamic_cast<const cat_inode *>(e);
	    const cat_mirage *e_hard = dynamic_cast<const cat_mirage *>(e);
	    const cat_nomme *e_nom = dynamic_cast<const cat_nomme *>(e);
	    const cat_detruit *e_det = dynamic_cast<const cat_detruit *>(e);

	    thr.check_self_cancellation();
	    juillet.enfile(e);

	    if(e_eod != nullptr)
		continue;

	    if(e_nom == nullptr)
		throw SRC_BUG;
	    else
	    {
		if(subtree.is_covered(juillet.get_path()) && (e_dir != nullptr || selection.is_covered(e_nom->get_name())))
		{
		    file_slices = macro_tools_get_slices(e_nom, slicing);
		    all_slices += file_slices;

		    if(e_det != nullptr)
			ui.message(tools_printf("%s\t %s%s", file_slices.display().c_str(), REMOVE_TAG, juillet.get_string().c_str()));
		    else
		    {
			if(e_hard != nullptr)
			    e_ino = e_hard->get_inode();

			if(e_ino == nullptr)
			    throw SRC_BUG;
			else
			{
			    bool dirty_seq = local_check_dirty_seq(get_escape_layer());
			    string a = local_perm(*e_ino, e_hard != nullptr);
			    string f = local_flag(*e_ino, isolated, dirty_seq);

			    ui.message(tools_printf("%s\t %S%S %s", file_slices.display().c_str(), &f, &a, juillet.get_string().c_str()));
			}
		    }
		}
		else // excluded, filtered out
		    if(e_dir != nullptr)
		    {
			juillet.enfile(&tmp_eod);
			skip_read_to_parent_dir();
		    }
		    // else what was skipped is not a directory, nothing to do.
	    }
	}

	ui.message("-----");
	ui.message(tools_printf("All displayed files have their data in slice range [%s]", all_slices.display().c_str()));
	ui.message("-----");
    }

    void catalogue::drop_all_non_detruits()
    {
	cat_directory *ptr = contenu;
	const cat_nomme *e = nullptr;
	const cat_directory *e_dir = nullptr;
	const cat_detruit *e_det = nullptr;

	ptr->reset_read_children();
	while(ptr != nullptr)
	{
	    if(ptr->read_children(e))
	    {
		e_dir = dynamic_cast<const cat_directory *>(e);
		e_det = dynamic_cast<const cat_detruit *>(e);
		if(e_dir != nullptr)
		{
		    ptr = const_cast<cat_directory *>(e_dir);
		    ptr->reset_read_children();
		}
		else if(e_det == nullptr)
		    ptr->remove(e->get_name());
	    }
	    else // finished reading the current directory
	    {
		cat_directory *parent = ptr->get_parent();

		if(parent != nullptr && !ptr->has_children())
		{
		    parent->remove(ptr->get_name());
		    ptr = parent;
		}
		else
		    ptr = parent;
	    }
	}
    }

    bool catalogue::is_subset_of(const catalogue & ref) const
    {
	bool ret = true;
	const cat_entree *moi = nullptr;
	const cat_entree *toi = nullptr;

	reset_read();
	ref.reset_compare();

	try
	{
	    while(ret && !read(moi))
	    {
		if(moi == nullptr)
		    throw SRC_BUG;

		if(!ref.compare(moi, toi))
		    ret = false;
		else
		{
		    if(toi == nullptr)
			throw SRC_BUG;
		    if(*toi != *moi)
			ret = false;
		}
	    }
	}
	catch(Edata & e)
	{
	    ret = false;
		// no rethrow
	}
	catch(Erange & e)
	{
	    ret = false;
		// no rethrow
	}

	return ret;
    }


    void catalogue::reset_dump() const
    {
	if(contenu == nullptr)
	    throw SRC_BUG;
	contenu->set_all_mirage_s_inode_dumped_field_to(false);
    }

    void catalogue::dump(const pile_descriptor & pdesc) const
    {
	crc *tmp = nullptr;

	pdesc.check(false);
	if(pdesc.compr->is_compression_suspended())
	{
	    pdesc.stack->sync_write_above(pdesc.compr);
	    pdesc.compr->resume_compression();
	}
	else
	{
	    pdesc.stack->sync_write_above(pdesc.compr);
	    pdesc.compr->sync_write(); // required to reset the compression engine and be able to uncompress from that position later on
	}

	try
	{
	    pdesc.stack->reset_crc(CAT_CRC_SIZE);
	    try
	    {
		ref_data_name.dump(*pdesc.stack);
		contenu->dump(pdesc, false);
	    }
	    catch(...)
	    {
		tmp = pdesc.stack->get_crc();
		throw;
	    }
	    tmp = pdesc.stack->get_crc();
	    if(tmp == nullptr)
		throw SRC_BUG;
	    tmp->dump(*pdesc.stack);
	}
	catch(...)
	{
	    if(tmp != nullptr)
		delete tmp;
	    throw;
	}
	if(tmp != nullptr)
	    delete tmp;
    }

    void catalogue::reset_all()
    {
	out_compare = "/";
	current_compare = contenu;
	current_add = contenu;
	current_read = contenu;
	if(sub_tree != nullptr)
	{
	    delete sub_tree;
	    sub_tree = nullptr;
	}
    }

    void catalogue::transfer_delta_signatures(const pile_descriptor & destination,
					      bool sequential_read,
					      bool build,
					      const mask & delta_mask,
					      const infinint & delta_sig_min_size)
    {
	const cat_entree *ent = nullptr;
	const cat_file *ent_file = nullptr;
	const cat_inode *ent_inode = nullptr;
	const cat_mirage *ent_mir = nullptr;
	memory_file mem;
	const crc *my_crc = nullptr;
	defile juillet = FAKE_ROOT;
	null_file trash = gf_write_only;
	generic_file *data = nullptr;

	if(destination.compr == nullptr || destination.stack == nullptr)
	    throw SRC_BUG;
	else
	{
	    destination.stack->sync_write_above(destination.compr);
	    destination.compr->sync_write();
	    destination.compr->suspend_compression();
	}

	set_all_mirage_s_inode_wrote_field_to(false);
	reset_read();
	while(read(ent))
	{
	    ent_file = dynamic_cast<const cat_file *>(ent);
	    ent_inode = dynamic_cast<const cat_inode *>(ent);
	    ent_mir = dynamic_cast<const cat_mirage *>(ent);

	    juillet.enfile(ent);

	    if(ent_mir != nullptr)
	    {
		if(!ent_mir->is_inode_wrote())
		{
		    ent_inode = ent_mir->get_inode();
		    ent_file = dynamic_cast<const cat_file *>(ent_inode);
		    ent_mir->set_inode_wrote(true);
		}
	    }

	    if(ent_file != nullptr)
	    {
		cat_file *e_file = const_cast<cat_file *>(ent_file);
		if(e_file == nullptr)
		    throw SRC_BUG;

		if(sequential_read)
		{
		    const crc * tmp = nullptr;
		    (void)e_file->get_crc(tmp);
		}

		if(ent_file->has_delta_signature_structure())
		{
			// delta signature field found, we may have to either:
			// - drop the global delta_signature structure
			// - keep the global delta_signature structure but drop the delta_signature data
			// - keep the global detla_signature structure and keep the delta_signature data, dumping it to the target archive
		    if(!build  // we keep existing delta_signature as is
		       ||
		       (delta_mask.is_covered(juillet.get_string()) // or we have to build/transfer delta sig if they match size and mask criteria
			&& e_file->get_size() >= delta_sig_min_size))
		    {
			memory_file *sig_ptr = nullptr;

			try
			{
			    ent_file->read_delta_signature(sig_ptr);
			    if(sig_ptr != nullptr)
				e_file->dump_delta_signature(*sig_ptr, *(destination.compr), false);
			    else
				e_file->dump_delta_signature(*(destination.compr), false);
			}
			catch(...)
			{
			    if(sig_ptr != nullptr)
				delete sig_ptr;
			    throw;
			}
			if(sig_ptr != nullptr)
			    delete sig_ptr;
		    }
		    else // we need to remove the delta signature, but not the delta signature structure when status is saved_status::delta
			if(e_file->get_saved_status() == saved_status::delta)
			{
			    memory_file *sig_ptr = nullptr;

			    e_file->read_delta_signature(sig_ptr);
			    if(sig_ptr != nullptr)
			    {
				delete sig_ptr;
				sig_ptr = nullptr;
				e_file->clear_delta_signature_only();
			    }
				// no need to drop the signature_structure structure outside the catalogue
				// only the delta_signature_data will stay outside the catalogue not the associated CRC
				// they will only be kept inside the (isolated) catalogue
			}
			else
			    e_file->clear_delta_signature_structure();
		}
		else // no delta signature found we may have to calculate them
		{
		    if(build
		       && delta_mask.is_covered(juillet.get_string())
		       && e_file->get_size() >= delta_sig_min_size)
		    {
			const crc **checksum = nullptr;

			if(!e_file->has_crc() && !sequential_read)
			{
				// this is an old archive, we will add a data crc on-fly

			    checksum = new (nothrow)(const crc *);
			    if(checksum == nullptr)
				throw Ememory("catalogue::transfer_delta_signatures");
			    *checksum = nullptr;
			}

			try
			{
			    switch(e_file->get_saved_status())
			    {
			    case saved_status::saved:
				data = e_file->get_data(cat_file::plain, &mem, nullptr, checksum);

				if(data == nullptr)
				    throw SRC_BUG;

				try
				{
				    data->copy_to(trash);
				}
				catch(...)
				{
				    delete data;
				    throw;
				}
				delete data;

				if(checksum != nullptr)
				{
				    if(*checksum != nullptr)
					e_file->set_crc(**checksum);
				    else
					throw SRC_BUG;
				}

				if(!e_file->get_crc(my_crc))
				    throw SRC_BUG;
				if(my_crc == nullptr)
				    throw SRC_BUG;
				e_file->will_have_delta_signature_available();
				e_file->set_patch_base_crc(*my_crc);
				e_file->set_patch_result_crc(*my_crc);
				e_file->dump_delta_signature(mem, *(destination.compr), false);
				break;
			    case saved_status::fake:
			    case saved_status::not_saved:
				break;
			    case saved_status::delta:
				    // reading the crc from the archive in sequential read mode
				if(sequential_read)
				    e_file->get_crc(my_crc);
				break;
			    default:
				throw SRC_BUG;
			    }
			}
			catch(...)
			{
			    if(checksum != nullptr)
			    {
				if(*checksum != nullptr)
				    delete *checksum;
				delete checksum;
			    }
			    throw;
			}
			if(checksum != nullptr)
			{
			    if(*checksum != nullptr)
				delete *checksum;
			    delete checksum;
			}
		    }
		     // no signature and no need to add some for that entry
		}
	    }

	    if(ent_inode != nullptr && sequential_read)
	    {
		    // EA

		if(ent_inode->ea_get_saved_status() == ea_saved_status::full)
		{
		    (void)ent_inode->get_ea();
		    ent_inode->ea_detach();
		}

		    // FSA

		if(ent_inode->fsa_get_saved_status() == fsa_saved_status::full)
		{
		    (void)ent_inode->get_fsa();
		    ent_inode->fsa_detach();
		}
	    }
	}
    }

    void catalogue::drop_delta_signatures()
    {
	const cat_entree *ent = nullptr;
	const cat_file *ent_file = nullptr;
	const cat_mirage *ent_mir = nullptr;

	reset_read();
	while(read(ent))
	{
	    ent_file = dynamic_cast<const cat_file *>(ent);
	    ent_mir = dynamic_cast<const cat_mirage *>(ent);

	    if(ent_mir != nullptr)
		ent_file = dynamic_cast<const cat_file *>(ent_mir->get_inode());

	    if(ent_file != nullptr)
	    {
		if(ent_file->has_delta_signature_available())
		{
		    cat_file *e_file = const_cast<cat_file *>(ent_file);
		    e_file->clear_delta_signature_only();
		}
	    }
	}
    }

    void catalogue::change_location(const pile_descriptor & pdesc)
    {
	smart_pointer<pile_descriptor> tmp(new (nothrow) pile_descriptor(pdesc));
	if(tmp.is_null())
	    throw Ememory("catalogue::change_location");
	contenu->change_location(tmp);
    }

    void catalogue::copy_detruits_from(const catalogue & ref)
    {
	const cat_entree *ent;

	ref.reset_read();
	reset_add();

	while(ref.read(ent))
	{
	    const cat_detruit *ent_det = dynamic_cast<const cat_detruit *>(ent);
	    const cat_directory *ent_dir = dynamic_cast<const cat_directory *>(ent);
	    const cat_eod *ent_eod = dynamic_cast<const cat_eod *>(ent);

	    if(ent_dir != nullptr)
		re_add_in(ent_dir->get_name());
	    if(ent_eod != nullptr)
	    {
		cat_eod *tmp = new (nothrow) cat_eod();
		if(tmp == nullptr)
		    throw Ememory("catalogue::copy_detruits_from");
		try
		{
		    add(tmp);
		}
		catch(...)
		{
		    delete tmp;
		    throw;
		}
	    }
	    if(ent_det != nullptr)
	    {
		cat_detruit *cp = new (nothrow) cat_detruit(*ent_det);
		if(cp == nullptr)
		    throw Ememory("catalogue::copy_detruits_from");
		try
		{
		    add(cp);
		}
		catch(...)
		{
		    delete cp;
		    throw;
		}
	    }
	}
    }

    void catalogue::swap_stuff(catalogue & ref)
    {
	    // swapping contenu
	cat_directory *tmp = contenu;
	contenu = ref.contenu;
	ref.contenu = tmp;
	tmp = nullptr;

	    // swapping stats
	entree_stats tmp_st = stats;
	stats = ref.stats;
	ref.stats = tmp_st;

	    // swapping label
	label tmp_lab;
	tmp_lab = ref_data_name;
	ref_data_name = ref.ref_data_name;
	ref.ref_data_name = tmp_lab;

	    // avoid pointers to point to the now other's object tree
	reset_all();
	ref.reset_all();
    }

    void catalogue::partial_copy_from(const catalogue & ref)
    {
	contenu = nullptr;
	sub_tree = nullptr;

	try
	{
	    if(ref.contenu == nullptr)
		throw SRC_BUG;
	    contenu = new (nothrow) cat_directory(*ref.contenu);
	    if(contenu == nullptr)
		throw Ememory("catalogue::catalogue(const catalogue &)");
	    current_compare = contenu;
	    current_add = contenu;
	    current_read = contenu;
	    if(ref.sub_tree != nullptr)
	    {
		sub_tree = new (nothrow) path(*ref.sub_tree);
		if(sub_tree == nullptr)
		    throw Ememory("catalogue::partial_copy_from");
	    }
	    else
		sub_tree = nullptr;
	    sub_count = ref.sub_count;
	    stats = ref.stats;
	}
	catch(...)
	{
	    if(contenu != nullptr)
	    {
		delete contenu;
		contenu = nullptr;
	    }
	    if(sub_tree != nullptr)
	    {
		delete sub_tree;
		sub_tree = nullptr;
	    }
	    throw;
	}
    }

    void catalogue::detruire()
    {
	if(contenu != nullptr)
	{
	    delete contenu;
	    contenu = nullptr;
	}
	if(sub_tree != nullptr)
	{
	    delete sub_tree;
	    sub_tree = nullptr;
	}
    }


    const cat_eod catalogue::r_eod;
    const U_I catalogue::CAT_CRC_SIZE = 4;

    static bool local_check_dirty_seq(escape *ptr)
    {
	bool ret;

	if(ptr != nullptr)
	{
	    bool already_set = ptr->is_unjumpable_mark(escape::seqt_file);

	    if(!already_set)
		ptr->add_unjumpable_mark(escape::seqt_file);
	    ret = ptr != nullptr && ptr->skip_to_next_mark(escape::seqt_dirty, true);
	    if(!already_set)
		ptr->remove_unjumpable_mark(escape::seqt_file);
	}
	else
	    ret = false;

	return ret;
    }


} // end of namespace

