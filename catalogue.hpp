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
// $Id: catalogue.hpp,v 1.27 2002/03/30 15:59:19 denis Rel $
//
/*********************************************************************/

#ifndef CATALOGUE_HPP
#define CATALOGUE_HPP

#include <unistd.h>
#include <vector>
#include "infinint.hpp"
#include "path.hpp"
#include "generic_file.hpp"

class entree
{
public :
    static entree *read(generic_file & f);
    virtual ~entree() {};
    virtual void dump(generic_file & f) const;
    virtual unsigned char signature() const = 0;

};

class eod : public entree
{
public :
    eod() {};
    eod(generic_file & f) {};
	// dump defined by entree
    unsigned char signature() const { return 'z'; };
};

class nomme : public entree
{
public :
    nomme(const string & name) { xname = name; };
    nomme(generic_file & f);
    void dump(generic_file & f) const;

    string get_name() const { return xname; };
    bool same_as(const nomme & ref) const { return xname == ref.xname; };
	// no need to have a virtual method, as signature will differ in inherited classes (argument type changes)

	// signature() is kept as an abstract method

private :
    string xname;
};

class inode : public nomme
{
public:
    inode(unsigned short int xuid, unsigned short int xgid, unsigned short int xperm, 
	  const infinint & last_access, 
	  const infinint & last_modif, 
	  const string & xname);
    inode(generic_file & f, bool saved);

    void dump(generic_file & f) const;
    short int get_uid() const { return uid; };
    short int get_gid() const { return gid; };
    short int get_perm() const { return perm; };
    infinint get_last_access() const { return last_acc; };
    infinint get_last_modif() const { return last_mod; };
    void set_last_access(const infinint & x_time) { last_acc = x_time; };
    void set_last_modif(const infinint & x_time) { last_mod = x_time; };
    
    bool get_saved_status() const { return xsaved; };
    void set_saved_status(bool x) { xsaved = x; };
	
    bool same_as(const inode & ref) const;
    virtual bool is_more_recent_than(const inode & ref) const;
	// signature left as an abstract method

private :
    unsigned short int uid;
    unsigned short int gid;
    unsigned short int perm;
    infinint last_acc, last_mod;
    bool xsaved;
};

class file : public inode
{
public :
    file(unsigned short int xuid, unsigned short int xgid, unsigned short int xperm, 
	 const infinint & last_access, 
	 const infinint & last_modif, 
	 const string & src,
	 const path & che,
	 const infinint & taille);
    file(generic_file & f, bool saved);
    
    void dump(generic_file & f) const;
    bool is_more_recent_than(const inode & ref) const;
    infinint get_size() const { return size; };
    generic_file *get_data() const; // return a newly alocated object in read_only mode
    void set_offset(const infinint & r);
    unsigned char signature() const { return get_saved_status() ? 'f' : 'F'; };

private :	
    bool from_path;
    path chemin;
    infinint offset;
    infinint size;
    generic_file *loc;
};

class lien : public inode
{
public :
    lien(short int uid, short int gid, short int perm, 
	 const infinint & last_access, 
	 const infinint & last_modif, 
	 const string & name, const string & target);
    lien(generic_file & f, bool saved);

    void dump(generic_file & f) const;
    string get_target() const;
    void set_target(string x);

	// using the method is_more_recent_than() from inode
    unsigned char signature() const { return get_saved_status() ? 'l' : 'L'; };

private :
    string points_to;
};

class directory : public inode
{
public :
    directory(unsigned short int xuid, unsigned short int xgid, unsigned short int xperm, 
	      const infinint & last_access, 
	      const infinint & last_modif, 
	      const string & xname);
    directory(const directory &ref);
    directory(generic_file & f, bool saved);
    ~directory(); // detruit aussi tous les fils et se supprime de son 'parent'

    void dump(generic_file & f) const;
    void add_children(nomme *r); // when r is a directory, 'parent' is set to 'this'
    void reset_read_children() { it = fils.begin(); }; 
    bool read_children(nomme * &r); // read the direct children of the directory, returns false if no more is available
    void listing(ostream & flux, string marge = "");
    directory * get_parent() const { return parent; };
    bool search_children(const string &name, nomme *&ref);

	// using is_more_recent_than() from inode class
    unsigned char signature() const { return get_saved_status() ? 'd' : 'D'; };

private :
    directory *parent;
    vector<nomme *> fils;
    vector<nomme *>::iterator it;

    void clear();
};

class device : public inode
{
public :
    device(short int uid, short int gid, short int perm, 
	   const infinint & last_access, 
	   const infinint & last_modif, 
	   const string & name, 
	   unsigned short int major, 
	   unsigned short int minor);
    device(generic_file & f, bool saved);
    
    void dump(generic_file & f) const;
    int get_major() const { if(!get_saved_status()) throw SRC_BUG; else return xmajor; };
    int get_minor() const { if(!get_saved_status()) throw SRC_BUG; else return xminor; };
    void set_major(int x) { xmajor = x; };
    void set_minor(int x) { xminor = x; };

	// using method is_more_recent_than() from inode class
	// signature is left pure abstract

private :
    unsigned short int xmajor, xminor;
};

class chardev : public device
{
public:
    chardev(short int uid, short int gid, short int perm, 
	    const infinint & last_access, 
	    const infinint & last_modif, 
	    const string & name, 
	    unsigned short int major, 
	    unsigned short int minor) : device(uid, gid, perm, last_access, 
					       last_modif, name,
					       major, minor) {};
    chardev(generic_file & f, bool saved) : device(f, saved) {};

	// using dump from device class
	// using method is_more_recent_than() from device class
    unsigned char signature() const { return get_saved_status() ? 'c' : 'C'; };
};

class blockdev : public device
{
public:
    blockdev(short int uid, short int gid, short int perm, 
	     const infinint & last_access, 
	     const infinint & last_modif, 
	     const string & name, 
	     unsigned short int major, 
	     unsigned short int minor) : device(uid, gid, perm, last_access, 
						last_modif, name,
						major, minor) {};
    blockdev(generic_file & f, bool saved) : device(f, saved) {};
    
	// using dump from device class
	// using method is_more_recent_than() from device class
    unsigned char signature() const { return get_saved_status() ? 'b' : 'B'; };
};

class tube : public inode
{
public :
    tube(unsigned short int xuid, unsigned short int xgid, unsigned short int xperm, 
	 const infinint & last_access, 
	 const infinint & last_modif, 
	 const string & xname) : inode(xuid, xgid, xperm, last_access, last_modif, xname) { set_saved_status(true); };
    tube(generic_file & f, bool saved) : inode(f, saved) {};
    
    	// using dump from inode class
	// using method is_more_recent_than() from inode class
    unsigned char signature() const { return get_saved_status() ? 'p' : 'P'; };
};

class prise : public inode
{
public :
    prise(unsigned short int xuid, unsigned short int xgid, unsigned short int xperm, 
	  const infinint & last_access, 
	  const infinint & last_modif, 
	  const string & xname) : inode(xuid, xgid, xperm, last_access, last_modif, xname) { set_saved_status(true); };
    prise(generic_file & f, bool saved) : inode(f, saved) {};

    	// using dump from inode class
	// using method is_more_recent_than() from inode class
    unsigned char signature() const { return get_saved_status() ? 's' : 'S'; };
};

class detruit : public nomme
{
public :
    detruit(const string & name, unsigned char firm) : nomme(name) { signe = firm; };
    detruit(generic_file & f) : nomme(f) { if(f.read((char *)&signe, 1) != 1) throw Erange("detruit::detruit", "missing data to buid"); };

    void dump(generic_file & f) const { nomme::dump(f); f.write((char *)&signe, 1); };
    unsigned char get_signature() const { return signe; };
    void set_signature(unsigned char x) { signe = x; };
    unsigned char signature() const { return 'x'; };

private :
    unsigned char signe;
};

class ignored : public nomme
{
public :
    ignored(const string & name) : nomme(name) {};
    ignored(generic_file & f) : nomme(f) { throw SRC_BUG; };

    void dump(generic_file & f) const { throw SRC_BUG; };
    unsigned char signature() const { return 'i'; };
};

class ignored_dir : public inode
{
public:
    ignored_dir(const directory &target) : inode(target) {};
    ignored_dir(generic_file & f) : inode(f, false) { throw SRC_BUG; };

    void dump(generic_file & f) const; // behaves like an empty directory
    unsigned char signature() const { return 'j'; }; 
};

class catalogue
{
public :
    catalogue();
    catalogue(generic_file & f);
    catalogue(const catalogue & ref) : out_compare(ref.out_compare) { partial_copy_from(ref); };
    catalogue & operator = (const catalogue &ref);
    ~catalogue() { detruire(); };

    void reset_read();
    void skip_read_to_parent_dir(); 
	// skip all items of the current dir and of any subdir, the next call will return 
	// next item of the parent dir (no eod to exit from the current dir !)
    bool read(const entree * & ref); 
	// sequential read (generates eod) and return false when all files have been read
    bool read_if_present(string *name, const nomme * & ref); 
	// pseudo-sequential read (reading a directory still 
	// implies that following read are located in this subdirectory up to the next EOD) but
	// it returns false if no entry of this name are present in the current directory 
	// a call with NULL as first argument means to set the current dir the parent directory

    void reset_sub_read(const path &sub); // return false if the path do not exists in catalogue 
    bool sub_read(const entree * &ref); // sequential read of the catalogue, ignoring all that
	// is not part of the subdirectory specified with reset_sub_read
	// the read include the inode leading to the sub_tree as well as the pending eod

    void reset_add();
    void add(entree *ref); // add at end of catalogue (sequential point of view)
    void add_in_current_read(nomme *ref); // add in currently read directory

    void reset_compare();
    bool compare(const entree * name, const entree * & extracted);
	// returns true if the ref exists, and gives it back in second argument as it is in the current catalogue.
	// returns false is no entry of that nature exists in the catalogue (in the current directory)
	// if ref is a directory, the operation is normaly relative to the directory itself, but 
	// such a call implies a chdir to that directory. thus, a call with an EOD is necessary to 
	// change to the parent directory.
	// note :
	// if a directory is not present, returns false, but records the inexistant subdirectory 
	// structure defined by the following calls to this routine, this to be able to know when 
	// the last available directory is back the current one when changing to parent directory, 
	// and then proceed with normal comparison of inode. In this laps of time, the call will
	// always return false, while it temporary stores the missing directory structure

    bool direct_read(const path & ref, const nomme * &ret);
            
    infinint update_destroyed_with(catalogue & ref);
	// ref must have the same root, else the operation generates a exception
    
    void dump(generic_file & ref) const;
    void listing(ostream & flux, string marge = "") { contenu->listing(flux, marge); };

private :
    directory *contenu;
    path out_compare;                 // stores the missing directory structure, when extracting
    directory *current_compare;       // points to the current directory when extracting
    directory *current_add;           // points to the directory where to add the next file with add_file;
    directory *current_read;          // points to the directory where the next item will be read
    path *sub_tree;                   // path to sub_tree
    signed int sub_count;             // count the depth in of read routine in the sub_tree

    void partial_copy_from(const catalogue &ref);
    void detruire();

    static const eod r_eod;           // needed to return eod reference, without taking risk of saturating memory
};

#endif

