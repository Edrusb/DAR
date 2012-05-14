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
// $Id: catalogue.hpp,v 1.16 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/

#ifndef CATALOGUE_HPP
#define CATALOGUE_HPP

#pragma interface

#include "../my_config.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <vector>
#include <map>
#include "infinint.hpp"
#include "path.hpp"
#include "generic_file.hpp"
#include "header_version.hpp"
#include "ea.hpp"
#include "compressor.hpp"
#include "integers.hpp"
#include "mask.hpp"
#include "special_alloc.hpp"

namespace libdar
{
    class file_etiquette;
    class entree;

    enum saved_status { s_saved, s_fake, s_not_saved };

    struct entree_stats
    {
        infinint num_x;                  // number of file referenced as destroyed since last backup
        infinint num_d;                  // number of directories
        infinint num_f;                  // number of plain files (hard link or not, thus file directory entries)
        infinint num_c;                  // number of char devices
        infinint num_b;                  // number of block devices
        infinint num_p;                  // number of named pipes
        infinint num_s;                  // number of unix sockets
        infinint num_l;                  // number of symbolic links
        infinint num_hard_linked_inodes; // number of inode that have more than one link (inode with "hard links")
        infinint num_hard_link_entries;  // total number of hard links (file directory entry pointing to an
            // inode already linked in the same or another directory (i.e. hard linked))
        infinint saved; // total number of saved inode (unix inode, not inode class) hard links do not count here
        infinint total; // total number of inode in archive (unix inode, not inode class) hard links do not count here
        void clear() { num_x = num_d = num_f = num_c = num_b = num_p
                           = num_s = num_l = num_hard_linked_inodes
                           = num_hard_link_entries = saved = total = 0; };
        void add(const entree *ref);
        void listing() const;
    };

    extern void catalogue_set_reading_version(const dar_version &ver);
    extern unsigned char mk_signature(unsigned char base, saved_status state);


    class entree
    {
    public :
        static entree *read(generic_file & f, entree_stats & stats);
        static void freemem() { corres.clear(); };

        virtual ~entree() {};
        virtual void dump(generic_file & f) const;
        virtual unsigned char signature() const = 0;
        virtual entree *clone() const = 0;

            // SPECIAL ALLOC not adapted here
            // because some inherited class object (eod) are
            // temporaneous
        
    private :
        static std::map <infinint, file_etiquette *> corres;
    };

    extern bool compatible_signature(unsigned char a, unsigned char b);

    class eod : public entree
    {
    public :
        eod() {};
        eod(generic_file & f) {};
            // dump defined by entree
        unsigned char signature() const { return 'z'; };
        entree *clone() const { return new eod(); };

            // eod are generally temporary object they are NOT
            // well adapted to "SPECIAL ALLOC"
    };

    class nomme : public entree
    {
    public :
        nomme(const std::string & name) { xname = name; };
        nomme(generic_file & f);
        void dump(generic_file & f) const;

        std::string get_name() const { return xname; };
        void change_name(const std::string &x) { xname = x; };
        bool same_as(const nomme & ref) const { return xname == ref.xname; };
            // no need to have a virtual method, as signature will differ in inherited classes (argument type changes)

            // signature() is kept as an abstract method
            // clone() is abstract

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(nomme);
#endif
   
    private :
        std::string xname;
    };

    class inode : public nomme
    {
    public:
        inode(U_16 xuid, U_16 xgid, U_16 xperm,
              const infinint & last_access,
              const infinint & last_modif,
              const std::string & xname);
        inode(generic_file & f, saved_status saved);
        inode(const inode & ref);
        ~inode();

        void dump(generic_file & f) const;
        U_16 get_uid() const { return uid; };
        U_16 get_gid() const { return gid; };
        U_16 get_perm() const { return perm; };
        infinint get_last_access() const { return *last_acc; };
        infinint get_last_modif() const { return *last_mod; };
        void set_last_access(const infinint & x_time) { *last_acc = x_time; };
        void set_last_modif(const infinint & x_time) { *last_mod = x_time; };    
        saved_status get_saved_status() const { return xsaved; };
        void set_saved_status(saved_status x) { xsaved = x; };
        
        bool same_as(const inode & ref) const;
        virtual bool is_more_recent_than(const inode & ref, const infinint & hourshift) const;
        virtual bool has_changed_since(const inode & ref, const infinint & hourshift) const;
            // signature() left as an abstract method
            // clone is abstract
        void compare(const inode &other, bool root_ea, bool user_ea) const;
            // throw Erange exception if a difference has been detected
            // this is not a symetrical comparison, but all what is present
            // in the current object is compared against the argument
            // which may contain supplementary informations



            //////////////////////////////////
            // EXTENDED ATTRIBUTS Methods
            //

        enum ea_status { ea_none, ea_partial, ea_full };
            // ea_none    : no EA present to this inode in filesystem
            // ea_partial : EA present in filesystem but not stored (ctime used to check changes)
            // ea_full    : EA present in filesystem and attached to this inode

            // I : to know whether EA data is present or not for this object
        void ea_set_saved_status(ea_status status); 
        ea_status ea_get_saved_status() const { return ea_saved; };

            // II : to associate EA list to an inode object (mainly for backup operation) #EA_FULL only#
        void ea_attach(ea_attributs *ref);
        const ea_attributs *get_ea() const;
        void ea_detach() const; //discards any future call to get_ea() !

            // III : to record where is dump the EA in the archive #EA_FULL only#
        void ea_set_offset(const infinint & pos) { *ea_offset = pos; };
        void ea_set_crc(const crc & val) { copy_crc(ea_crc, val); };

            // IV : to know/record if EA have been modified #EA_FULL or EA_PARTIAL#
        infinint get_last_change() const;
        void set_last_change(const infinint & x_time);

            ////////////////////////

        static void set_ignore_owner(bool mode) { ignore_owner = mode; };

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(inode);
#endif
    
    protected:
        virtual void sub_compare(const inode & other) const {};

    private :
        U_16 uid;
        U_16 gid;
        U_16 perm;
        infinint *last_acc, *last_mod;
        saved_status xsaved;
        ea_status ea_saved;
            //  the following is used only if ea_saved == full
        infinint *ea_offset;
        ea_attributs *ea;
            // the following is used if ea_saved == full or ea_saved == partial
        infinint *last_cha;
        crc ea_crc;

        static generic_file *storage; // where are stored EA
            // this is a static variable (shared amoung all objects)
            // because it is not efficient to have many copies of the same value
            // if in the future, all inode have their EA stored in their own
            // generic_file, this will be changed.
        static bool ignore_owner;
    };

    class file : public inode
    {
    public :
        file(U_16 xuid, U_16 xgid, U_16 xperm,
             const infinint & last_access,
             const infinint & last_modif,
             const std::string & src,
             const path & che,
             const infinint & taille);
        file(const file & ref);
        file(generic_file & f, saved_status saved);
        ~file() { detruit(); };
    
        void dump(generic_file & f) const;
        bool is_more_recent_than(const inode & ref, const infinint & hourshift) const;
        bool has_changed_since(const inode & ref, const infinint & hourshift) const;
        infinint get_size() const { return *size; };
        infinint get_storage_size() const { return *storage_size; };
        void set_storage_size(const infinint & s) { *storage_size = s; };
        generic_file *get_data() const; // return a newly alocated object in read_only mode
        void clean_data(); // partially free memory (but get_data() becomes disabled)
        void set_offset(const infinint & r);
        unsigned char signature() const { return mk_signature('f', get_saved_status()); };

        void set_crc(const crc &c) { copy_crc(check, c); };
        bool get_crc(crc & c) const;
        entree *clone() const { return new file(*this); };

        static void set_compression_algo_used(compression a) { algo = a; };
        static compression get_compression_algo_used() { return algo; };
        static void set_archive_localisation(generic_file *f) { loc = f; };

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(file);
#endif

    protected :
        void sub_compare(const inode & other) const;

    private :
        enum { empty, from_path, from_cat } status;
        path chemin;
        infinint *offset;
        infinint *size;
        infinint *storage_size;
        crc check;

        static generic_file *loc;
        static compression algo;

        void detruit();
    };

    class etiquette
    {
    public:
        virtual infinint get_etiquette() const = 0;
        virtual const file *get_inode() const = 0;

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(etiquette);
#endif
    };

    class file_etiquette : public file, public etiquette
    {
    public :
        file_etiquette(U_16 xuid, U_16 xgid, U_16 xperm,
                       const infinint & last_access,
                       const infinint & last_modif,
                       const std::string & src,
                       const path & che,
                       const infinint & taille);
        file_etiquette(const file_etiquette & ref);
        file_etiquette(generic_file & f, saved_status saved);
        ~file_etiquette() { if(etiquette != NULL) delete etiquette; };

        void dump(generic_file &f) const;
        unsigned char signature() const { return mk_signature('e', get_saved_status()); };
        entree *clone() const { return new file_etiquette(*this); };

        static void reset_etiquette_counter() { compteur_check(); *compteur = 0; };
    
            // inherited from etiquette
        infinint get_etiquette() const { return *etiquette; };
        void change_etiquette() { if(compteur == NULL) throw SRC_BUG; else *etiquette = (*compteur)++; };
        const file *get_inode() const { return this; };

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(file_etiquette);
#endif

    private :
        infinint *etiquette;

        static infinint *compteur;
        static void compteur_check();
    };

    class hard_link : public nomme, public etiquette
    {
    public :
        hard_link(const std::string & name, file_etiquette *ref);
        hard_link(generic_file & f, infinint & etiquette); // with etiquette, a call to set_reference() follows

        void dump(generic_file &f) const;
        unsigned char signature() const { return 'h'; };
        entree *clone() const { return new hard_link(*this); };
        void set_reference(file_etiquette *ref);

            // inherited from etiquette
        infinint get_etiquette() const { return x_ref->get_etiquette(); };
        const file *get_inode() const { return x_ref; };

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(hard_link);
#endif
    private :
        file_etiquette *x_ref;
    };


    class lien : public inode
    {
    public :
        lien(U_16 uid, U_16 gid, U_16 perm,
             const infinint & last_access,
             const infinint & last_modif,
             const std::string & name, const std::string & target);
        lien(generic_file & f, saved_status saved);

        void dump(generic_file & f) const;
        std::string get_target() const;
        void set_target(std::string x);

            // using the method is_more_recent_than() from inode
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('l', get_saved_status()); };
        entree *clone() const { return new lien(*this); };

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(lien);
#endif
    protected :
        void sub_compare(const inode & other) const;

    private :
        std::string points_to;
    };

    class directory : public inode
    {
    public :
        directory(U_16 xuid, U_16 xgid, U_16 xperm,
                  const infinint & last_access,
                  const infinint & last_modif,
                  const std::string & xname);
        directory(const directory &ref); // only the inode part is build, no children is duplicated (empty dir)
        directory(generic_file & f, saved_status saved, entree_stats & stats);
        ~directory(); // detruit aussi tous les fils et se supprime de son 'parent'

        void dump(generic_file & f) const;
        void add_children(nomme *r); // when r is a directory, 'parent' is set to 'this'
        void reset_read_children() const;
        bool read_children(const nomme * &r) const; // read the direct children of the directory, returns false if no more is available
        void listing(const mask &m = bool_mask(true), std::string marge = "") const;
        void tar_listing(const mask &m = bool_mask(true), const std::string & beginning = "") const;
        directory * get_parent() const { return parent; };
        bool search_children(const std::string &name, nomme *&ref);

            // using is_more_recent_than() from inode class
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('d', get_saved_status()); };
        entree *clone() const { return new directory(*this); };

        static void set_tar_listing_callback(void (*callback)(const std::string & flag, const std::string & perm, const std::string & uid,
                                                              const std::string & gid, const std::string & size, const std::string & date,
                                                              const std::string & filename))
            { tar_listing_callback = callback; };
        static void get_tar_listing_callback(void (*&callback)(const std::string & flag, const std::string & perm, const std::string & uid,
							       const std::string & gid, const std::string & size, const std::string & date,
							       const std::string & filename))
            { callback = tar_listing_callback; };

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(directory);
#endif
    private :
        directory *parent;
        std::vector<nomme *> fils;
        std::vector<nomme *>::iterator it;

        static void (*tar_listing_callback)(const std::string & flags, const std::string & perm, const std::string & uid, const std::string & gid,
                                            const std::string & size, const std::string & date, const std::string & filename);

        void clear();
    };

    class device : public inode
    {
    public :
        device(U_16 uid, U_16 gid, U_16 perm,
               const infinint & last_access,
               const infinint & last_modif,
               const std::string & name,
               U_16 major,
               U_16 minor);
        device(generic_file & f, saved_status saved);
    
        void dump(generic_file & f) const;
        int get_major() const { if(get_saved_status() != s_saved) throw SRC_BUG; else return xmajor; };
        int get_minor() const { if(get_saved_status() != s_saved) throw SRC_BUG; else return xminor; };
        void set_major(int x) { xmajor = x; };
        void set_minor(int x) { xminor = x; };

            // using method is_more_recent_than() from inode class
            // using method has_changed_since() from inode class
            // signature is left pure abstract

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(device);
#endif

    protected :
        void sub_compare(const inode & other) const;

    private :
        U_16 xmajor, xminor;
    };

    class chardev : public device
    {
    public:
        chardev(U_16 uid, U_16 gid, U_16 perm,
                const infinint & last_access,
                const infinint & last_modif,
                const std::string & name,
                U_16 major,
                U_16 minor) : device(uid, gid, perm, last_access,
                                     last_modif, name,
                                     major, minor) {};
        chardev(generic_file & f, saved_status saved) : device(f, saved) {};
    
            // using dump from device class
            // using method is_more_recent_than() from device class
            // using method has_changed_since() from device class
        unsigned char signature() const { return mk_signature('c', get_saved_status()); };
        entree *clone() const { return new chardev(*this); };

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(chardev);
#endif
    };

    class blockdev : public device
    {
    public:
        blockdev(U_16 uid, U_16 gid, U_16 perm,
                 const infinint & last_access,
                 const infinint & last_modif,
                 const std::string & name,
                 U_16 major,
                 U_16 minor) : device(uid, gid, perm, last_access,
                                      last_modif, name,
                                      major, minor) {};
        blockdev(generic_file & f, saved_status saved) : device(f, saved) {};
    
            // using dump from device class
            // using method is_more_recent_than() from device class
            // using method has_changed_since() from device class
        unsigned char signature() const { return mk_signature('b', get_saved_status()); };
        entree *clone() const { return new blockdev(*this); };

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(blockdev);
#endif
    };

    class tube : public inode
    {
    public :
        tube(U_16 xuid, U_16 xgid, U_16 xperm,
             const infinint & last_access,
             const infinint & last_modif,
             const std::string & xname) : inode(xuid, xgid, xperm, last_access, last_modif, xname) { set_saved_status(s_saved); };
        tube(generic_file & f, saved_status saved) : inode(f, saved) {};
    
            // using dump from inode class
            // using method is_more_recent_than() from inode class
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('p', get_saved_status()); };
        entree *clone() const { return new tube(*this); };

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(tube);
#endif
    };

    class prise : public inode
    {
    public :
        prise(U_16 xuid, U_16 xgid, U_16 xperm,
              const infinint & last_access,
              const infinint & last_modif,
              const std::string & xname) : inode(xuid, xgid, xperm, last_access, last_modif, xname) { set_saved_status(s_saved); };
        prise(generic_file & f, saved_status saved) : inode(f, saved) {};

            // using dump from inode class
            // using method is_more_recent_than() from inode class
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('s', get_saved_status()); };
        entree *clone() const { return new prise(*this); };

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(prise);
#endif
    };

    class detruit : public nomme
    {
    public :
        detruit(const std::string & name, unsigned char firm) : nomme(name) { signe = firm; };
        detruit(generic_file & f) : nomme(f) { if(f.read((char *)&signe, 1) != 1) throw Erange("detruit::detruit", "missing data to buid"); };

        void dump(generic_file & f) const { nomme::dump(f); f.write((char *)&signe, 1); };
        unsigned char get_signature() const { return signe; };
        void set_signature(unsigned char x) { signe = x; };
        unsigned char signature() const { return 'x'; };
        entree *clone() const { return new detruit(*this); };

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(detruit);
#endif
    private :
        unsigned char signe;
    };

    class ignored : public nomme
    {
    public :
        ignored(const std::string & name) : nomme(name) {};
        ignored(generic_file & f) : nomme(f) { throw SRC_BUG; };

        void dump(generic_file & f) const { throw SRC_BUG; };
        unsigned char signature() const { return 'i'; };
        entree *clone() const { return new ignored(*this); };
#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(ignored);
#endif
    };

    class ignored_dir : public inode
    {
    public:
        ignored_dir(const directory &target) : inode(target) {};
        ignored_dir(generic_file & f) : inode(f, s_not_saved) { throw SRC_BUG; };

        void dump(generic_file & f) const; // behaves like an empty directory
        unsigned char signature() const { return 'j'; }; 
        entree *clone() const { return new ignored_dir(*this); };
#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(ignored_dir);
#endif
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
        bool read_if_present(std::string *name, const nomme * & ref); 
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
        void listing(const mask &m = bool_mask(true), std::string marge = "") const;
        void tar_listing(const mask & m = bool_mask(true), const std::string & beginning = "") const;
        entree_stats get_stats() const { return stats; };

        const directory *get_contenu() const { return contenu; }; // used by data_tree

#ifdef SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(catalogue);
#endif

    private :
        directory *contenu;
        path out_compare;                 // stores the missing directory structure, when extracting
        directory *current_compare;       // points to the current directory when extracting
        directory *current_add;           // points to the directory where to add the next file with add_file;
        directory *current_read;          // points to the directory where the next item will be read
        path *sub_tree;                   // path to sub_tree
        signed int sub_count;             // count the depth in of read routine in the sub_tree
        entree_stats stats;               // statistics catalogue contents

        void partial_copy_from(const catalogue &ref);
        void detruire();

        static const eod r_eod;           // needed to return eod reference, without taking risk of saturating memory
    };

} // end of namespace

#endif
