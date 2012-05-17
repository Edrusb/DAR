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
// $Id: catalogue.hpp,v 1.48.2.6 2010/09/12 16:32:51 edrusb Rel $
//
/*********************************************************************/

    /// \file catalogue.hpp
    /// \brief here is defined the many classed which is build of the catalogue
    /// \ingroup Private

#ifndef CATALOGUE_HPP
#define CATALOGUE_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"

#include <vector>
#include <map>
#include "infinint.hpp"
#include "generic_file.hpp"
#include "path.hpp"
#include "header_version.hpp"
#include "ea.hpp"
#include "compressor.hpp"
#include "integers.hpp"
#include "mask.hpp"
#include "special_alloc.hpp"
#include "user_interaction.hpp"

namespace libdar
{
    class file_etiquette;
    class entree;

	/// \addtogroup Private
	/// @{

    enum saved_status
    {
	s_saved,      //< inode is saved in the archive
	s_fake,       //< inode is not saved in the archive but is in the archive of reference (isolation context)
	s_not_saved   //< inode is not saved in the archive
    };

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
        void listing(user_interaction & dialog) const;
    };

    extern unsigned char mk_signature(unsigned char base, saved_status state);
    extern void unmk_signature(unsigned char sig, unsigned char & base, saved_status & state);

	/// the root class from all other inherite for any entry in the catalogue
    class entree
    {
    public :
        static entree *read(user_interaction & dialog,
			    generic_file & f, const dar_version & reading_ver,
			    entree_stats & stats,
			    std::map <infinint, file_etiquette *> & corres,
			    compression default_algo,
			    generic_file *data_loc,
			    generic_file *ea_loc);

        virtual ~entree() {};
        virtual void dump(user_interaction & dialog, generic_file & f) const;
        virtual unsigned char signature() const = 0;
        virtual entree *clone() const = 0;

            // SPECIAL ALLOC not adapted here
            // because some inherited class object (eod) are
            // temporary
    };

    extern bool compatible_signature(unsigned char a, unsigned char b);

	/// the End of Directory entry class
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

	/// the base class for all entry that have a name
    class nomme : public entree
    {
    public :
        nomme(const std::string & name) { xname = name; };
        nomme(generic_file & f);
        void dump(user_interaction & dialog, generic_file & f) const;

        const std::string & get_name() const { return xname; };
        void change_name(const std::string & x) { xname = x; };
        bool same_as(const nomme & ref) const { return xname == ref.xname; };
            // no need to have a virtual method, as signature will differ in inherited classes (argument type changes)

            // signature() is kept as an abstract method
            // clone() is abstract

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(nomme);
#endif

    private :
        std::string xname;
    };

	/// the root class for all inode
    class inode : public nomme
    {
    public:

	    /// flag used to only consider certain fields when comparing/restoring inodes

	enum comparison_fields
	{
	    cf_all,          //< consider any available field for comparing inodes
	    cf_ignore_owner, //< consider any available field except ownership fields
	    cf_mtime,        //< consider any available field except ownership and permission fields
	    cf_inode_type    //< only consider the file type
	};

        inode(U_16 xuid, U_16 xgid, U_16 xperm,
              const infinint & last_access,
              const infinint & last_modif,
              const std::string & xname, const infinint & device);
        inode(user_interaction & dialog,
	      generic_file & f,
	      const dar_version & reading_ver,
	      saved_status saved,
	      generic_file *ea_loc);
        inode(const inode & ref);
        ~inode();

        void dump(user_interaction & dialog, generic_file & f) const;
        U_16 get_uid() const { return uid; };
        U_16 get_gid() const { return gid; };
        U_16 get_perm() const { return perm; };
        infinint get_last_access() const { return *last_acc; };
        infinint get_last_modif() const { return *last_mod; };
        void set_last_access(const infinint & x_time) { *last_acc = x_time; };
        void set_last_modif(const infinint & x_time) { *last_mod = x_time; };
        saved_status get_saved_status() const { return xsaved; };
        void set_saved_status(saved_status x) { xsaved = x; };
	infinint get_device() const { return *fs_dev; };

        bool same_as(const inode & ref) const;
        bool is_more_recent_than(const inode & ref, const infinint & hourshift) const;
	    // used for RESTORATION
        virtual bool has_changed_since(const inode & ref, const infinint & hourshift, comparison_fields what_to_check) const;
            // signature() left as an abstract method
            // clone is abstract too
	    // used for INCREMENTAL BACKUP
        void compare(user_interaction & dialog,
		     const inode &other,
		     const mask & ea_mask,
		     comparison_fields what_to_check,
		     const infinint & hourshift) const;
            // throw Erange exception if a difference has been detected
            // this is not a symetrical comparison, but all what is present
            // in the current object is compared against the argument
            // which may contain supplementary informations
	    // used for DIFFERENCE



            //////////////////////////////////
            // EXTENDED ATTRIBUTS Methods
            //

        enum ea_status { ea_none, ea_partial, ea_fake, ea_full };
            // ea_none    : no EA present for this inode in filesystem
            // ea_partial : EA present in filesystem but not stored (ctime used to check changes)
	    // ea_fake    : EA present in filesystem but not attached to this inode (isolation context)
            // ea_full    : EA present in filesystem and attached to this inode

            // I : to know whether EA data is present or not for this object
        void ea_set_saved_status(ea_status status);
        ea_status ea_get_saved_status() const { return ea_saved; };

            // II : to associate EA list to an inode object (mainly for backup operation) #EA_FULL only#
        void ea_attach(ea_attributs *ref);
        const ea_attributs *get_ea(user_interaction & dialog) const;
        void ea_detach() const; //discards any future call to get_ea() !

            // III : to record where is dump the EA in the archive #EA_FULL only#
        void ea_set_offset(const infinint & pos) { *ea_offset = pos; };
        void ea_set_crc(const crc & val) { copy_crc(ea_crc, val); };
	void ea_get_crc(crc & val) const { copy_crc(val, ea_crc); };

            // IV : to know/record if EA have been modified #EA_FULL,  EA_PARTIAL or EA_FAKE#
        infinint get_last_change() const;
        void set_last_change(const infinint & x_time);

	    // V : for archive migration (merging)
        void change_ea_location(generic_file *loc) { storage = loc; };

            ////////////////////////

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(inode);
#endif

    protected:
        virtual void sub_compare(user_interaction & dialog, const inode & other) const {};

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
	infinint *fs_dev;
	generic_file *storage; // where are stored EA
	dar_version edit;   // need to know EA format used in archive file
    };

	/// the plain file class
    class file : public inode
    {
    public :
        file(U_16 xuid, U_16 xgid, U_16 xperm,
             const infinint & last_access,
             const infinint & last_modif,
             const std::string & src,
             const path & che,
             const infinint & taille,
	     const infinint & fs_device);
        file(const file & ref);
        file(user_interaction & dialog,
	     generic_file & f,
	     const dar_version & reading_ver,
	     saved_status saved,
	     compression default_algo,
	     generic_file *data_loc,
	     generic_file *ea_loc);
        ~file() { detruit(); };

        void dump(user_interaction & dialog, generic_file & f) const;
        bool has_changed_since(const inode & ref, const infinint & hourshift, inode::comparison_fields what_to_check) const;
        infinint get_size() const { return *size; };
        infinint get_storage_size() const { return *storage_size; };
        void set_storage_size(const infinint & s) { *storage_size = s; };
        generic_file *get_data(user_interaction & dialog, bool keep_compressed = false) const; // return a newly alocated object in read_only mode
        void clean_data(); // partially free memory (but get_data() becomes disabled)
        void set_offset(const infinint & r);
        unsigned char signature() const { return mk_signature('f', get_saved_status()); };

        void set_crc(const crc &c) { copy_crc(check, c); };
        bool get_crc(crc & c) const;
        entree *clone() const { return new file(*this); };

        compression get_compression_algo_used() const { return algo; };

	    // object migration methods (merging)
	void change_compression_algo_used(compression x) { algo = x; };
	void change_location(generic_file *x) { loc = x; };


#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(file);
#endif

    protected :
        void sub_compare(user_interaction & dialog, const inode & other) const;

    private :
        enum { empty, from_path, from_cat } status;
        path chemin;
        infinint *offset;
        infinint *size;
        infinint *storage_size;

	bool available_crc;
        crc check;

        generic_file *loc;
        compression algo;

        void detruit();
    };

	/// the hard link managment interface class (pure virtual class)
    class etiquette
    {
    public:
        virtual infinint get_etiquette() const = 0;
        virtual const file_etiquette *get_inode() const = 0;
	virtual ~etiquette() {};

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(etiquette);
#endif
    };

	/// the hard linked plain file
    class file_etiquette : public file, public etiquette
    {
    public :
        file_etiquette(U_16 xuid, U_16 xgid, U_16 xperm,
                       const infinint & last_access,
                       const infinint & last_modif,
                       const std::string & src,
                       const path & che,
                       const infinint & taille,
		       const infinint & fs_device,
		       const infinint & etiquette_number);
        file_etiquette(const file_etiquette & ref);
        file_etiquette(user_interaction & dialog,
		       generic_file & f,
		       const dar_version & reading_ver,
		       saved_status saved,
		       compression default_algo,
		       generic_file *data_loc,
		       generic_file *ea_loc);

        void dump(user_interaction & dialog, generic_file &f) const;
        unsigned char signature() const { return mk_signature('e', get_saved_status()); };
        entree *clone() const { return new file_etiquette(*this); };

	void change_etiquette(const infinint & new_val) { etiquette = new_val; };

            // inherited from etiquette
        infinint get_etiquette() const { return etiquette; };
        const file_etiquette *get_inode() const { return this; };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(file_etiquette);
#endif

    private :
        infinint etiquette;
    };

	/// the secondary reference to a hard linked inode
    class hard_link : public nomme, public etiquette
    {
    public :
        hard_link(const std::string & name, file_etiquette *ref);
        hard_link(generic_file & f, infinint & etiquette); // with etiquette, a call to set_reference() follows

        void dump(user_interaction & dialog, generic_file &f) const;
        unsigned char signature() const { return 'h'; };
        entree *clone() const { return new hard_link(*this); };
        void set_reference(file_etiquette *ref);

            // inherited from etiquette
        infinint get_etiquette() const;
        const file_etiquette *get_inode() const { return x_ref; };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(hard_link);
#endif
    private :
        file_etiquette *x_ref;
    };

	/// the symbolic link inode class
    class lien : public inode
    {
    public :
        lien(U_16 uid, U_16 gid, U_16 perm,
             const infinint & last_access,
             const infinint & last_modif,
             const std::string & name,
	     const std::string & target,
	     const infinint & fs_device);
        lien(user_interaction & dialog,
	     generic_file & f,
	     const dar_version & reading_ver,
	     saved_status saved,
	     generic_file *ea_loc);

        void dump(user_interaction & dialog, generic_file & f) const;
        const std::string & get_target() const;
        void set_target(std::string x);

            // using the method is_more_recent_than() from inode
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('l', get_saved_status()); };
        entree *clone() const { return new lien(*this); };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(lien);
#endif
    protected :
        void sub_compare(user_interaction & dialog, const inode & other) const;

    private :
        std::string points_to;
    };

	/// the directory inode class
    class directory : public inode
    {
    public :
        directory(U_16 xuid, U_16 xgid, U_16 xperm,
                  const infinint & last_access,
                  const infinint & last_modif,
                  const std::string & xname,
		  const infinint & device);
        directory(const directory &ref); // only the inode part is build, no children is duplicated (empty dir)
        directory(user_interaction & dialog,
		  generic_file & f,
		  const dar_version & reading_ver,
		  saved_status saved,
		  entree_stats & stats,
		  std::map <infinint, file_etiquette *> & corres,
		  compression default_algo,
		  generic_file *data_loc,
		  generic_file *ea_loc);
        ~directory(); // detruit aussi tous les fils et se supprime de son 'parent'

        void dump(user_interaction & dialog, generic_file & f) const;
        void add_children(nomme *r); // when r is a directory, 'parent' is set to 'this'
	bool has_children() const { return fils.size() != 0; };
        void reset_read_children() const;
        bool read_children(const nomme * &r) const; // read the direct children of the directory, returns false if no more is available
        void listing(user_interaction & dialog,
		     const mask &m = bool_mask(true), bool filter_unsaved = false, const std::string & marge = "") const;
        void tar_listing(user_interaction & dialog,
			 const mask &m = bool_mask(true), bool filter_unsaved = false, const std::string & beginning = "") const;
	void xml_listing(user_interaction & dialog,
			 const mask &m = bool_mask(true), bool filter_unsaved = false, const std::string & beginning = "") const;
        directory * get_parent() const { return parent; };
        bool search_children(const std::string &name, nomme *&ref);
	bool callback_for_children_of(user_interaction & dialog, const std::string & sdir) const;

            // using is_more_recent_than() from inode class
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('d', get_saved_status()); };

	    // some data has changed since archive of reference in this directory or subdirectories
	bool get_recursive_has_changed() const { return recursive_has_changed; };
	    // update the recursive_has_changed field
	void recursive_has_changed_update() const;

        entree *clone() const { return new directory(*this); };

 #ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(directory);
#endif
    private :
        directory *parent;
        std::vector<nomme *> fils;
        std::vector<nomme *>::iterator it;
	bool recursive_has_changed;

	void clear();
    };

	/// the special device root class
    class device : public inode
    {
    public :
        device(U_16 uid, U_16 gid, U_16 perm,
               const infinint & last_access,
               const infinint & last_modif,
               const std::string & name,
               U_16 major,
               U_16 minor,
	       const infinint & fs_device);
        device(user_interaction & dialog,
	       generic_file & f,
	       const dar_version & reading_ver,
	       saved_status saved,
	       generic_file *ea_loc);

        void dump(user_interaction & dialog, generic_file & f) const;
        int get_major() const { if(get_saved_status() != s_saved) throw SRC_BUG; else return xmajor; };
        int get_minor() const { if(get_saved_status() != s_saved) throw SRC_BUG; else return xminor; };
        void set_major(int x) { xmajor = x; };
        void set_minor(int x) { xminor = x; };

            // using method is_more_recent_than() from inode class
            // using method has_changed_since() from inode class
            // signature is left pure abstract

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(device);
#endif

    protected :
        void sub_compare(user_interaction & dialog, const inode & other) const;

    private :
        U_16 xmajor, xminor;
    };

	/// the char device class
    class chardev : public device
    {
    public:
        chardev(U_16 uid, U_16 gid, U_16 perm,
                const infinint & last_access,
                const infinint & last_modif,
                const std::string & name,
                U_16 major,
                U_16 minor,
		const infinint & fs_device) : device(uid, gid, perm, last_access,
                                     last_modif, name,
                                     major, minor, fs_device) {};
        chardev(user_interaction & dialog,
		generic_file & f,
		const dar_version & reading_ver,
		saved_status saved,
		generic_file *ea_loc) : device(dialog, f, reading_ver, saved, ea_loc) {};

            // using dump from device class
            // using method is_more_recent_than() from device class
            // using method has_changed_since() from device class
        unsigned char signature() const { return mk_signature('c', get_saved_status()); };
        entree *clone() const { return new chardev(*this); };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(chardev);
#endif
    };

	/// the block device class
    class blockdev : public device
    {
    public:
        blockdev(U_16 uid, U_16 gid, U_16 perm,
                 const infinint & last_access,
                 const infinint & last_modif,
                 const std::string & name,
                 U_16 major,
                 U_16 minor,
		 const infinint & fs_device) : device(uid, gid, perm, last_access,
						      last_modif, name,
						      major, minor, fs_device) {};
        blockdev(user_interaction & dialog,
		 generic_file & f,
		 const dar_version & reading_ver,
		 saved_status saved,
		 generic_file *ea_loc) : device(dialog, f, reading_ver, saved, ea_loc) {};

            // using dump from device class
            // using method is_more_recent_than() from device class
            // using method has_changed_since() from device class
        unsigned char signature() const { return mk_signature('b', get_saved_status()); };
        entree *clone() const { return new blockdev(*this); };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(blockdev);
#endif
    };

	/// the named pipe class
    class tube : public inode
    {
    public :
        tube(U_16 xuid, U_16 xgid, U_16 xperm,
             const infinint & last_access,
             const infinint & last_modif,
             const std::string & xname,
	     const infinint & fs_device) : inode(xuid, xgid, xperm, last_access, last_modif, xname, fs_device) { set_saved_status(s_saved); };
        tube(user_interaction & dialog,
	     generic_file & f,
	     const dar_version & reading_ver,
	     saved_status saved,
 	     generic_file *ea_loc) : inode(dialog, f, reading_ver, saved, ea_loc) {};

            // using dump from inode class
            // using method is_more_recent_than() from inode class
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('p', get_saved_status()); };
        entree *clone() const { return new tube(*this); };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(tube);
#endif
    };

	/// the Unix socket inode class
    class prise : public inode
    {
    public :
        prise(U_16 xuid, U_16 xgid, U_16 xperm,
              const infinint & last_access,
              const infinint & last_modif,
              const std::string & xname,
	      const infinint & fs_device) : inode(xuid, xgid, xperm, last_access, last_modif, xname, fs_device) { set_saved_status(s_saved); };
        prise(user_interaction & dialog,
	      generic_file & f,
	      const dar_version & reading_ver,
	      saved_status saved,
	      generic_file *ea_loc) : inode(dialog, f, reading_ver, saved, ea_loc) {};

            // using dump from inode class
            // using method is_more_recent_than() from inode class
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('s', get_saved_status()); };
        entree *clone() const { return new prise(*this); };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(prise);
#endif
    };

	/// the deleted file entry
    class detruit : public nomme
    {
    public :
        detruit(const std::string & name, unsigned char firm) : nomme(name) { signe = firm; };
        detruit(generic_file & f) : nomme(f) { if(f.read((char *)&signe, 1) != 1) throw Erange("detruit::detruit", gettext("missing data to build")); };

        void dump(user_interaction & dialog, generic_file & f) const { nomme::dump(dialog, f); f.write((char *)&signe, 1); };
        unsigned char get_signature() const { return signe; };
        void set_signature(unsigned char x) { signe = x; };
        unsigned char signature() const { return 'x'; };
        entree *clone() const { return new detruit(*this); };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(detruit);
#endif
    private :
        unsigned char signe;
    };

	/// the present file to ignore (not to be recorded as deleted later)
    class ignored : public nomme
    {
    public :
        ignored(const std::string & name) : nomme(name) {};
        ignored(generic_file & f) : nomme(f) { throw SRC_BUG; };

        void dump(user_interaction & dialog, generic_file & f) const { throw SRC_BUG; };
        unsigned char signature() const { return 'i'; };
        entree *clone() const { return new ignored(*this); };
#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(ignored);
#endif
    };

	/// the ignored directory class, to be promoted later as empty directory if needed
    class ignored_dir : public inode
    {
    public:
        ignored_dir(const directory &target) : inode(target) {};
        ignored_dir(user_interaction & dialog,
		    generic_file & f,
		    const dar_version & reading_ver,
		    generic_file *ea_loc) : inode(dialog, f, reading_ver, s_not_saved, ea_loc) { throw SRC_BUG; };

        void dump(user_interaction & dialog, generic_file & f) const; // behaves like an empty directory
        unsigned char signature() const { return 'j'; };
        entree *clone() const { return new ignored_dir(*this); };
#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(ignored_dir);
#endif
    };

	/// the catalogue class which gather all objects contained in a give archive
    class catalogue
    {
    public :
        catalogue(user_interaction & dialog);
        catalogue(user_interaction & dialog,
		  generic_file & f,
		  const dar_version & reading_ver,
		  compression default_algo,
		  generic_file *data_loc,
		  generic_file *ea_loc);
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
            // ref must have the same root, else the operation generates an exception

	void update_absent_with(catalogue & ref);
	    // in case of abortion, complete missing files as if what could not be
	    // inspected had not changed since the reference was done

        void dump(generic_file & ref) const;
        void listing(const mask &m = bool_mask(true), bool filter_unsaved = false, const std::string & marge = "") const;
        void tar_listing(const mask & m = bool_mask(true), bool filter_unsaved = false, const std::string & beginning = "") const;
        void xml_listing(const mask & m = bool_mask(true), bool filter_unsaved = false, const std::string & beginning = "") const;
        entree_stats get_stats() const { return stats; };

        const directory *get_contenu() const { return contenu; }; // used by data_tree

#ifdef LIBDAR_SPECIAL_ALLOC
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

	user_interaction *cat_ui;

        void partial_copy_from(const catalogue &ref);
        void detruire();

        static const eod r_eod;           // needed to return eod reference, without taking risk of saturating memory
    };

	/// @}

} // end of namespace

#endif
