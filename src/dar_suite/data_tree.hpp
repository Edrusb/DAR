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
// $Id: data_tree.hpp,v 1.10.2.1 2003/12/14 13:57:54 edrusb Rel $
//
/*********************************************************************/

#ifndef DATA_TREE_HPP
#define DATA_TREE_HPP

#include "../my_config.h"

#include <map>
#include <string>
#include <list>
#include "generic_file.hpp"
#include "infinint.hpp"
#include "catalogue.hpp"
#include "special_alloc.hpp"

using namespace libdar;
using namespace std;

typedef U_16 archive_num;
#define ARCHIVE_NUM_MAX  65534

class data_tree
{
public:

    data_tree(const string &name);
    data_tree(generic_file &f);
    virtual ~data_tree() {};

    virtual void dump(generic_file & f) const;
    string get_name() const { return filename; };
    bool get_data(archive_num & archive) const; // return the archive where to find the most recent version
    bool get_EA(archive_num & archive) const; // if EA has been saved alone later, returns in which version
    bool read_data(archive_num num, infinint & val) const; // return the date of last modification within this archive
    bool read_EA(archive_num num, infinint & val) const; // return the date of last inode change

    void set_data(const archive_num & archive, const infinint & date) { last_mod[archive] = date; };
    void set_EA(const archive_num & archive, const infinint & date) { last_change[archive] = date; };
    virtual bool remove_all_from(const archive_num & archive); // return true if the corresponding file
        // is no more located by any archive (thus, the object is no more usefull)
    void listing() const; // list where is saved this file
    virtual void apply_permutation(archive_num src, archive_num dst);
    virtual void skip_out(archive_num num); // decrement archive numbers above num
    virtual void compute_most_recent_stats(vector<infinint> & data, vector<infinint> & ea,
                                           vector<infinint> & total_data, vector<infinint> & total_ea) const;
    
    virtual char obj_signature() const { return signature(); };
    static char signature() { return 't'; };

#ifdef SPECIAL_ALLOC
    void *operator new(size_t taille) { return special_alloc_new(taille); };
    void operator delete(void *ptr) { special_alloc_delete(ptr); };
#endif
private:
    string filename;
    map<archive_num, infinint> last_mod; // key is archive number ; value is last_mod time
    map<archive_num, infinint> last_change; // key is archive number ; value is last_change time
};

class data_dir : public data_tree
{
public:
    data_dir(const string &name);
    data_dir(generic_file &f);
    data_dir(const data_dir & ref);
    data_dir(const data_tree & ref);
    ~data_dir();

    void dump(generic_file & f) const;

    void add(const inode *entry, const archive_num & archive);
    const data_tree *read_child(const string & name) const;

    bool remove_all_from(const archive_num & archive); 
    void show(archive_num num, string marge = "") const; 
        // list the most recent files owned by that archive
        // (or by any archive if num == 0)
    void apply_permutation(archive_num src, archive_num dst);
    void skip_out(archive_num num);
    void compute_most_recent_stats(vector<infinint> & data, vector<infinint> & ea,
                                   vector<infinint> & total_data, vector<infinint> & total_ea) const;

    char obj_signature() const { return signature(); };
    static char signature() { return 'd'; };
    
#ifdef SPECIAL_ALLOC
    void *operator new(size_t taille) { return special_alloc_new(taille); };
    void operator delete(void *ptr) { special_alloc_delete(ptr); };
#endif
    
private:
    list<data_tree *> rejetons;

    void add_child(data_tree *fils); // "this" is now responsible of "fils" disalocation
    void remove_child(const string & name);
};

extern data_dir *data_tree_read(generic_file & f);
extern bool data_tree_find(path chemin, const data_dir & racine, const data_tree *& ptr);
extern void data_tree_update_with(const directory *dir, archive_num archive, data_dir *racine);
extern archive_num data_tree_permutation(archive_num src, archive_num dst, archive_num x);

#endif
