/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: catalogue.cpp,v 1.30 2002/03/30 15:59:19 denis Rel $
//
/*********************************************************************/

#include <netinet/in.h>
#include <iostream.h>
#include <ctype.h>
#include <algorithm>
#include "tools.hpp"
#include "catalogue.hpp"
#include "tronc.hpp"
#include "user_interaction.hpp"
#include "deci.hpp"

static string local_perm(inode &ref);
static string local_uid(inode & ref);
static string local_gid(inode & ref);
static string local_size(inode & ref);
static string local_date(inode & ref);
static string local_flag(inode & ref);

entree *entree::read(generic_file & f)
{
    char type;
    bool saved;
    entree *ret = NULL;
    int lu =  f.read(&type, 1);

    if(lu == 0)
	return ret;
    saved = islower(type);
    if(!saved)
	type = tolower(type);

    switch(type)
    {
    case 'f':
	ret = new file(f, saved);
	break;
    case 'l':
	ret = new lien(f, saved);
	break;
    case 'c':
	ret = new chardev(f, saved);
	break;
    case 'b':
	ret = new blockdev(f, saved);
	break;
    case 'p':
	ret = new tube(f, saved);
	break;
    case 's':
	ret = new prise(f, saved);
	break;
    case 'd':
	ret = new directory(f, saved);
	break;
    case 'z':
	if(!saved)
	    throw Erange("entree::read", "corrupted file");
	ret = new eod(f);
	break;
    case 'x':
	if(!saved)
	    throw Erange("entree::read", "corrupted file");
	ret = new detruit(f);
	break;
    case 'i':
	if(!saved)
	    throw Erange("entree::read", "corrupted file");
	ret = new ignored(f);
    default :
	throw Erange("entree::read", "unknown type of data in catalogue");
    }

    return ret;
}

void entree::dump(generic_file & f) const 
{ 
    char s = signature();
    f.write(&s, 1);
}

nomme::nomme(generic_file & f)
{
    tools_read_string(f, xname);
}

void nomme::dump(generic_file & f) const
{
    entree::dump(f);
    tools_write_string(f, xname);
}

inode::inode(unsigned short int xuid, unsigned short int xgid, unsigned short int xperm, 
	     const infinint & last_access, 
	     const infinint & last_modif, 
	     const string & xname) : nomme(xname)
{
    uid = xuid;
    gid = xgid;
    perm = xperm;
    last_acc = last_access;
    last_mod = last_modif;
    xsaved = false;
}

inode::inode(generic_file & f, bool saved) : nomme(f)
{
    unsigned short int tmp;

    xsaved = saved;

    if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
	throw Erange("inode::inode", "missing data to build an inode");
    uid = ntohs(tmp);
    if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
	throw Erange("inode::inode", "missing data to build an inode");
    gid = ntohs(tmp);
    if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
	throw Erange("inode::inode", "missing data to build an inode");
    perm = ntohs(tmp);
    last_acc.read_from_file(f);
    last_mod.read_from_file(f);
}
 
bool inode::same_as(const inode & ref) const
{
    return nomme::same_as(ref) && (tolower(ref.signature()) == tolower(signature()));
}

bool inode::is_more_recent_than(const inode & ref) const
{
    if(same_as(ref))
	return ref.last_mod != last_mod || uid != ref.uid || gid != ref.gid || perm != ref.perm;
    else
	throw SRC_BUG; // not the same inode !
}
    
void inode::dump(generic_file & r) const
{
    unsigned short int tmp;

    nomme::dump(r);

    tmp = htons(uid);
    r.write((char *)&tmp, sizeof(tmp));
    tmp = htons(gid);
    r.write((char *)&tmp, sizeof(tmp));
    tmp = htons(perm);
    r.write((char *)&tmp, sizeof(tmp));
    last_acc.dump(r);
    last_mod.dump(r);
}

file::file(unsigned short int xuid, unsigned short int xgid, unsigned short int xperm, 
	   const infinint & last_access, 
	   const infinint & last_modif, 
	   const string & src, 
	   const path & che,
	   const infinint & taille) : inode(xuid, xgid, xperm, last_access, last_modif, src), chemin(che + src)
{
    from_path = true;
    offset = 0;
    size = taille;
    loc = NULL;
    set_saved_status(true);
}

file::file(generic_file & f, bool saved) : inode(f, saved), chemin("vide")
{
    from_path = false;
    size.read_from_file(f);
    if(saved)
	offset.read_from_file(f);
    else
	offset = 0;
    loc = &f;
}    

void file::dump(generic_file & f) const
{
    inode::dump(f);
    size.dump(f);
    if(get_saved_status())
	offset.dump(f);
}

bool file::is_more_recent_than(const inode & ref) const
{
    const file *tmp = dynamic_cast<const file *>(&ref);
    if(tmp != NULL)
	return inode::is_more_recent_than(*tmp) || size != tmp->size;
    else
	throw SRC_BUG;
}

generic_file *file::get_data() const
{
    generic_file *ret;

    if(!get_saved_status())
	throw Erange("file::get_data", "cannot provide data from a \"not saved\" file object");
    
    if(from_path)
	ret = new fichier(chemin, gf_read_only);
    else
	if(loc->get_mode() == gf_write_only)
	    throw SRC_BUG; // cannot get data from a write-only file !!!
	else
	    ret = new tronc(loc, offset, size, gf_read_only);

    if(ret == NULL)
	throw Ememory("file::get_data");
    else
	return ret;
}

void file::set_offset(const infinint & r)
{
    set_saved_status(true);
    offset = r;
}

lien::lien(short int uid, short int gid, short int perm, 
	   const infinint & last_access, 
	   const infinint & last_modif, 
	   const string & name, 
	   const string & target) : inode(uid, gid, perm, last_access, last_modif, name)
{
    points_to = target;
    set_saved_status(true);
}

lien::lien(generic_file & f, bool saved) : inode(f, saved)
{
    if(saved)
	tools_read_string(f, points_to);
}

string lien::get_target() const
{
    if(!get_saved_status())
	throw SRC_BUG;
    return points_to;
}

void lien::set_target(string x) 
{
    set_saved_status(true);
    points_to = x; 
}

void lien::dump(generic_file & f) const
{
    inode::dump(f);
    if(get_saved_status())
	tools_write_string(f, points_to);
}

directory::directory(unsigned short int xuid, unsigned short int xgid, unsigned short int xperm, 
		     const infinint & last_access, 
		     const infinint & last_modif, 
		     const string & xname) : inode(xuid, xgid, xperm, last_access, last_modif, xname)
{
    parent = NULL;
    fils.clear();
    it = fils.begin();
    set_saved_status(true);
}

directory::directory(const directory &ref) : inode(ref)
{
    parent = NULL;
    fils.clear();
    it = fils.begin();
}

directory::directory(generic_file & f, bool saved) : inode(f, saved)
{
    entree *p;
    nomme *t;
    directory *d;
    eod *fin = NULL;

    parent = NULL;
    fils.clear();
    it = fils.begin();
    
    try
    {
	while(fin == NULL)
	{
	    p = entree::read(f);
	    if(p != NULL)
	    {
		d = dynamic_cast<directory *>(p);
		fin = dynamic_cast<eod *>(p);
		t = dynamic_cast<nomme *>(p);
		
		if(t != NULL) // p is a "nomme"
		    fils.push_back(t);
		if(d != NULL) // p is a directory
		    d->parent = this;
		if(t == NULL && fin == NULL)
		    throw SRC_BUG; // neither an eod nor a nomme ! what's that ???
	    }
	    else
		throw Erange("directory::directory", "missing data to build a directory");
	}
	delete fin; // no nead to keep it
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

void directory::dump(generic_file & f) const
{
    vector<nomme *>::iterator x = const_cast<directory *>(this)->fils.begin();
    inode::dump(f);
    eod fin;
    
    while(x != fils.end())
	if(dynamic_cast<ignored *>(*x) != NULL)
	    x++; // "ignored" need not to be saved, they are only useful when updating_destroyed
	else
	    (*x++)->dump(f);

    fin.dump(f); // end of "this" directory   
}
	
void directory::add_children(nomme *r)
{
    directory *d = dynamic_cast<directory *>(r);
    nomme *ancien;
    
    if(search_children(r->get_name(), ancien))
    {
	directory *a_dir = const_cast<directory *>(dynamic_cast<const directory *>(ancien));
	vector<nomme *>::iterator pos = find(fils.begin(), fils.end(), ancien);
	if(pos == fils.end())
	    throw SRC_BUG; // ancien not found in fils !!!?

	if(a_dir != NULL && d != NULL)
	{
	    vector<nomme *>::iterator it = a_dir->fils.begin();
	    while(it != a_dir->fils.end())
		d->add_children(*it++);
	    a_dir->fils.clear();
	    delete a_dir;
	    *pos = r;
	}
	else
	{
	    delete ancien;
	    *pos = r;
	}
    }
    else
	fils.push_back(r);
    if(d != NULL)
	d->parent = this;
}

bool directory::read_children(nomme *&r)
{
    if(it != fils.end())
    {
	r = *it++;
	return true;
    }
    else
	return false;
}

void directory::clear()
{
    it = fils.begin();
    while(it != fils.end())
    {
	delete *it;
	it++;
    }
    fils.clear();
    it = fils.begin();
}

void directory::listing(ostream & flux, string marge)
{
    vector<nomme *>::iterator it = fils.begin();
    while(it != fils.end())
    {
	directory *d = dynamic_cast<directory *>(*it);
	detruit *det = dynamic_cast<detruit *>(*it);
	inode *ino = dynamic_cast<inode *>(*it);

	if(det != NULL)
	    flux << marge << "[ destroyed file or directory ]    " << (*it++)->get_name() << endl; 
	else
	    if(ino == NULL)
		throw SRC_BUG;
	    else
		flux <<  marge << local_perm(*ino)
		     << "   " << local_uid(*ino)
		     << "   " << local_gid(*ino)
		     << "   " << local_size(*ino) 
		     << "   " << local_date(*ino)
		     << "   " << local_flag(*ino)
		     << "   " << (*it++)->get_name() << endl; 
	if(d != NULL)
	{
	    d->listing(flux, marge + "|  ");
	    cout << marge << "+--- " <<endl;
	}
    }
}

bool directory::search_children(const string &name, nomme *&ref)
{
    vector<nomme *>::iterator ut = fils.begin();

    while(ut != fils.end() && (*ut)->get_name() != name)
	ut++;

    if(ut != fils.end())
    {
	ref = *ut;
	return true;
    }
    else
	return false;
}

device::device(short int uid, short int gid, short int perm, 
	       const infinint & last_access, 
	       const infinint & last_modif, 
	       const string & name, 
	       unsigned short int major, 
	       unsigned short int minor) : inode(uid, gid, perm, last_access, last_modif, name)
{
    xmajor = major;
    xminor = minor;
    set_saved_status(true);
}

device::device(generic_file & f, bool saved) : inode(f, saved)
{
    unsigned short int tmp;

    if(saved)
    {
	if(f.read((char *)&tmp, (size_t)sizeof(tmp)) != sizeof(tmp))
	    throw Erange("special::special", "missing data to build a special device");
	xmajor = ntohs(tmp);
	if(f.read((char *)&tmp, (size_t)sizeof(tmp)) != sizeof(tmp))
	    throw Erange("special::special", "missing data to build a special device");
	xminor = ntohs(tmp);
    }
}

void device::dump(generic_file & f) const
{
    unsigned short int tmp;

    inode::dump(f);
    if(get_saved_status())
    {	
	tmp = htons(xmajor);
	f.write((char *)&tmp, (size_t)sizeof(tmp));
	tmp = htons(xminor);
	f.write((char *)&tmp, (size_t)sizeof(tmp));
    }
}

void ignored_dir::dump(generic_file & f) const
{
    directory tmp = directory(get_uid(), get_gid(), get_perm(), get_last_access(), get_last_modif(), get_name());
    tmp.set_saved_status(get_saved_status());
    tmp.dump(f); // dump an empty directory 
}

catalogue::catalogue() : out_compare("/")
{
    contenu = new directory(0,0,0,0,0,"root");
    if(contenu == NULL)
	throw Ememory("catalogue::catalogue(path)");
    current_compare = contenu;
    current_add = contenu;
    current_read = contenu;
    sub_tree = NULL;
}

catalogue::catalogue(generic_file & f) : out_compare("/")
{
    string tmp;
    char a;

    f.read(&a, 1); // need to read the signature before constructing "contenu"
    if(tolower(a) != 'd')
	throw Erange("catalogue::catalogue(generic_file &)", "Incoherent catalogue structure");

    contenu = new directory(f, islower(a));
    if(contenu == NULL)
	throw Ememory("catalogue::catalogue(path)");
    current_compare = contenu;
    current_add = contenu;
    current_read = contenu;
    sub_tree = NULL;
}

catalogue & catalogue::operator = (const catalogue &ref)
{ 
    detruire(); 
    out_compare = ref.out_compare; 
    partial_copy_from(ref); 
    return *this; 
}

void catalogue::reset_read()
{
    current_read = contenu;
    contenu->reset_read_children();
}

void catalogue::skip_read_to_parent_dir()
{
    directory *tmp = current_read->get_parent();
    if(tmp == NULL)
	throw Erange("catalogue::skip_read_to_parent_dir", "root does not have a parent directory");
    current_read = tmp;
}

bool catalogue::read(const entree * & ref)
{
    nomme *tmp;

    if(current_read->read_children(tmp))
    {
	directory *dir = dynamic_cast<directory *>(tmp);
	if(dir != NULL)
	{
	    current_read = dir;
	    dir->reset_read_children();
	}
	ref = tmp;
	return true;
    }
    else
    {
	directory *papa = current_read->get_parent();
	ref = & r_eod;
	if(papa == NULL)
	    return false; // we reached end of root, no eod generation
	else
	{
	    current_read = papa;
	    return true;
	}
    }
}

bool catalogue::read_if_present(string *name, const nomme * & ref)
{
    nomme *tmp;

    if(current_read == NULL)
	throw Erange("catalogue::read_if_present", "no current directory defined");
    if(name == NULL) // we have to go to parent directory
    {
	if(current_read->get_parent() == NULL)
	    throw Erange("catalogue::read_if_present", "root directory has no parent directory");
	else
	    current_read = current_read->get_parent();
	ref = NULL;
	return true;
    }
    else // looking for a real filename
	if(current_read->search_children(*name, tmp))
	{
	    directory *d = dynamic_cast<directory *>(tmp);
	    if(d != NULL) // this is a directory need to chdir to it
		current_read = d;
	    ref = tmp;
	    return true;
	}
	else // filename not present in current dir
	    return false;
}

void catalogue::reset_sub_read(const path &sub)
{
    if(! sub.is_relative())
	throw SRC_BUG;
    
    if(sub_tree != NULL)
	delete sub_tree;
    sub_tree = new path(sub);
    if(sub_tree == NULL)
	throw Ememory("catalogue::reset_sub_read");
    sub_count = -1; // must provide the path to subtree;
    reset_read();
}

bool catalogue::sub_read(const entree * &ref)
{
    string tmp;

    if(sub_tree == NULL)
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
	    ref = NULL;
	    delete sub_tree;
	    sub_tree = NULL;
	    sub_count = -2;
	    return false;
	}
    case -2: // reading is finished
	return false;
    case -1: // providing path to sub_tree
	if(sub_tree->read_subdir(tmp))
	{
	    nomme *xtmp;

	    if(current_read->search_children(tmp, xtmp))
	    {
		ref = xtmp;
		directory *dir = dynamic_cast<directory *>(xtmp);
		
		if(dir != NULL)
		{
		    current_read = dir;
		    return true;
		}
		else
		    if(sub_tree->read_subdir(tmp)) 
		    {
			user_interaction_warning(sub_tree->display() + " is not present in the archive");
			delete sub_tree;
			sub_tree = NULL;
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
		user_interaction_warning(sub_tree->display() + " is not present in the archive");
		delete sub_tree;
		sub_tree = NULL;
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
	    const directory *dir = dynamic_cast<const directory *>(ref);
	    const eod *fin = dynamic_cast<const eod *>(ref);
	    
	    if(dir != NULL)
		sub_count++;
	    if(fin != NULL)
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

void catalogue::add(entree *ref)
{
    if(current_add == NULL)
	throw SRC_BUG;

    eod *f = dynamic_cast<eod *>(ref);

    if(f == NULL) // ref is not eod
    {
	nomme *n = dynamic_cast<nomme *>(ref);
	directory *t = dynamic_cast<directory *>(ref);

	if(n == NULL)
	    throw SRC_BUG; // unknown type neither "eod" nor "nomme"
	current_add->add_children(n);
	if(t != NULL) // ref is a directory
	    current_add = t;
    }
    else // ref is an eod
    {
	directory *parent = current_add->get_parent();
	delete ref; // all data given throw add becomes owned by the catalogue object
	if(parent == NULL)
	    throw Erange("catalogue::add_file", "root has no parent directory, cannot change to it");
	else
	    current_add = parent;
    }
}

void catalogue::add_in_current_read(nomme *ref)
{
    if(current_read == NULL)
	throw SRC_BUG; // current read directory does not exists
    current_read->add_children(ref);
}

void catalogue::reset_compare()
{
    current_compare = contenu;
    out_compare = "/";
}

bool catalogue::compare(const entree * target, const entree * & extracted)
{
    const directory *dir = dynamic_cast<const directory *>(target);
    const eod *fin = dynamic_cast<const eod *>(target);
    const nomme *nom = dynamic_cast<const nomme *>(target);
	
    if(out_compare.degre() > 1) // actually scanning a inexisting directory
    {
	if(dir != NULL)
	    out_compare += dir->get_name();
	else
	    if(fin != NULL)
	    {
		string tmp_s;

		if(!out_compare.pop(tmp_s))
		    if(out_compare.is_relative())
			throw SRC_BUG; // should not be a relative path !!!
		    else // both cases are bugs, but need to know which case is generating a bug
			throw SRC_BUG; // out_compare.degre() > 0 but cannot pop !
	    }
	
	return false;
    }
    else
    {
	nomme *found;

	if(fin != NULL)
	{
	    directory *tmp = current_compare->get_parent();
	    if(tmp == NULL)
		throw Erange("catalogue::compare", "root has no parent directory");
	    current_compare = tmp;
	    extracted = target;
	    return true;
	}
	    
	if(nom == NULL)
	    throw SRC_BUG; // ref, is neither a eod nor a nomme ! what's that ???

	if(current_compare->search_children(nom->get_name(), found))
	{
	    const detruit *src_det = dynamic_cast<const detruit *>(nom);
	    const detruit *dst_det = dynamic_cast<const detruit *>(found);
	    const inode *src_ino = dynamic_cast<const inode *>(nom);
	    const inode *dst_ino = dynamic_cast<const inode *>(found);
	 
		// updating internal structure to follow directory tree :
	    if(dir != NULL)
	    {
		directory *d_ext = dynamic_cast<directory *>(found);
		if(d_ext != NULL)
		    current_compare = d_ext;
		else
		    out_compare += dir->get_name();
	    }

		// now comparing the objects :
	    if(src_ino != NULL) // thus dst_ino != NULL but let's double check :
		if(dst_ino != NULL)
		{
		    if(!src_ino->same_as(*dst_ino))
			return false;
		}
		else
		    return false;
	    else
		if(src_det != NULL)
		    if(dst_det != NULL)
		    {
			if(!dst_det->same_as(*dst_det))
			    return false;
		    }
		    else
			return false;
		else
		    throw SRC_BUG; // src_det == NULL && src_ino == NULL, thus a nomme which is neither detruit nor inode !
	    
	    extracted = found;
	    return true;
	}
	else
	{
	    if(dir != NULL)
		out_compare += dir->get_name();
	    return false;
	}
    }
}

infinint catalogue::update_destroyed_with(catalogue & ref)
{
    directory *current = contenu;
    nomme *ici;
    const entree *projo;
    const eod *pro_eod;
    const directory *pro_dir;
    const detruit *pro_det;
    const nomme *pro_nom;
    infinint count = 0;
    
    ref.reset_read();
    while(ref.read(projo))
    {
	pro_eod = dynamic_cast<const eod *>(projo);
	pro_dir = dynamic_cast<const directory *>(projo);
	pro_det = dynamic_cast<const detruit *>(projo);
	pro_nom = dynamic_cast<const nomme *>(projo);
	
	if(pro_eod != NULL)
	{
	    directory *tmp = current->get_parent();
	    if(tmp == NULL)
		throw SRC_BUG; // reached root for "contenu", and not yet for "ref";
	    current = tmp;
	    continue;
	}
	
	if(pro_det != NULL)
	    continue;

	if(pro_nom == NULL)
	    throw SRC_BUG; // neither an eod nor a nomme ! what's that ?

	if(!current->search_children(pro_nom->get_name(), ici))
	{
	    current->add_children(new detruit(pro_nom->get_name(), pro_nom->signature()));
	    count++;
	    if(pro_dir != NULL)
		ref.skip_read_to_parent_dir();
	}
	else
	    if(pro_dir != NULL)
	    {
		directory *ici_dir = dynamic_cast<directory *>(ici);

		if(ici_dir != NULL)
		    current = ici_dir;
		else
		    ref.skip_read_to_parent_dir();
	    }
    }

    return count;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: catalogue.cpp,v 1.30 2002/03/30 15:59:19 denis Rel $";
    dummy_call(id);
}

void catalogue::dump(generic_file & ref) const
{
    contenu->dump(ref);
}

void catalogue::partial_copy_from(const catalogue & ref)
{
    if(ref.contenu == NULL)
	throw SRC_BUG;
    contenu = new directory(*ref.contenu);
    if(contenu == NULL)
	throw Ememory("catalogue::catalogue(const catalogue &)");
    current_compare = contenu;
    current_add = contenu;
    current_read = contenu;
    if(ref.sub_tree != NULL)
	sub_tree = new path(*ref.sub_tree);
    else
	sub_tree = NULL;
    sub_count = ref.sub_count;
}

void catalogue::detruire()
{
    if(contenu != NULL)
	delete contenu;
    if(sub_tree != NULL)
	delete sub_tree;
}

const eod catalogue::r_eod;


static string local_perm(inode &ref)
{
    string ret = "";
    unsigned int perm = ref.get_perm();

    char type = tolower(ref.signature());
    if(type == 'f')
	type = '-';
    ret += type;
    if((perm & 0400) != 0)
	ret += 'r';
    else
	ret += '-';
    if((perm & 0200) != 0)
	ret += 'w';
    else
	ret += '-';
    if((perm & 0100) != 0)
	if((perm & 04000) != 0)
	    ret += 's';
	else
	    ret += 'x';
    else
	if((perm & 04000) != 0)
	    ret += 'S';
	else
	    ret += '-';
    if((perm & 040) != 0)
	ret += 'r';
    else
	ret += '-';
    if((perm & 020) != 0)
	ret += 'w';
    else
	ret += '-';
    if((perm & 010) != 0)
	if((perm & 02000) != 0)
	    ret += 's';
	else
	    ret += 'x';
    else
	if((perm & 02000) != 0)
	    ret += 'S';
	else
	    ret += '-';
    if((perm & 04) != 0)
	ret += 'r';
    else
	ret += '-';
    if((perm & 02) != 0)
	ret += 'w';
    else
	ret += '-';
    if((perm & 01) != 0)
	if((perm & 01000) != 0)
	    ret += 't';
	else
	    ret += 'x';
    else
	if((perm & 01000) != 0)
	    ret += 'T';
	else
	    ret += '-';
    
    return ret;
}

static string local_uid(inode & ref)
{
    string ret = "";
    infinint tmp = ref.get_uid();
    deci d = tmp;
    ret = d.human();

    return ret;
}

static string local_gid(inode & ref)
{
    string ret = "";
    infinint tmp = ref.get_gid();
    deci d = tmp;
    ret = d.human();

    return ret;
}

static string local_size(inode & ref)
{
    string ret;

    file *fic = dynamic_cast<file *>(&ref);
    if(fic != NULL)
    {
	deci d = fic->get_size();
	ret = d.human();
    }
    else
	ret = "0";

    return ret;
}

static string local_date(inode & ref)
{
    string ret;
    infinint quand = ref.get_last_modif();
    unsigned int pas = 0;
 
    quand.unstack(pas);
    ret = ctime((time_t *) &pas);

    return string(ret.begin(), ret.end() - 1);
}
    
static string local_flag(inode & ref)
{
    string ret;

    if(ref.get_saved_status())
	ret = "[Saved]";
    else
	ret = "[     ]";

    return ret;
}


