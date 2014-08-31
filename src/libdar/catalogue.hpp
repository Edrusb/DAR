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
#include "user_interaction.hpp"
#include "label.hpp"
#include "escape.hpp"
#include "on_pool.hpp"
#include "filesystem_specific_attribute.hpp"
#include "datetime.hpp"
#include "slice_layout.hpp"

namespace libdar
{
    class etoile;
    class entree;

	/// \addtogroup Private
	/// @{

    enum saved_status
    {
	s_saved,      //< inode is saved in the archive
	s_fake,       //< inode is not saved in the archive but is in the archive of reference (isolation context) s_fake is no more used in archive format "08" and above: isolated catalogue do keep the data pointers and s_saved stays a valid status in isolated catalogues.
	s_not_saved   //< inode is not saved in the archive
    };

	/// holds the statistics contents of a catalogue
    struct entree_stats
    {
        infinint num_x;                  //< number of file referenced as destroyed since last backup
        infinint num_d;                  //< number of directories
        infinint num_f;                  //< number of plain files (hard link or not, thus file directory entries)
	infinint num_c;                  //< number of char devices
	infinint num_b;                  //< number of block devices
	infinint num_p;                  //< number of named pipes
	infinint num_s;                  //< number of unix sockets
	infinint num_l;                  //< number of symbolic links
	infinint num_D;                  //< number of Door
	infinint num_hard_linked_inodes; //< number of inode that have more than one link (inode with "hard links")
        infinint num_hard_link_entries;  //< total number of hard links (file directory entry pointing to \an
            //< inode already linked in the same or another directory (i.e. hard linked))
        infinint saved; //< total number of saved inode (unix inode, not inode class) hard links do not count here
        infinint total; //< total number of inode in archive (unix inode, not inode class) hard links do not count here
        void clear() { num_x = num_d = num_f = num_c = num_b = num_p
                = num_s = num_l = num_D = num_hard_linked_inodes
                = num_hard_link_entries = saved = total = 0; };
        void add(const entree *ref);
        void listing(user_interaction & dialog) const;
    };

	/// the root class from all other inherite for any entry in the catalogue
    class entree : public on_pool
    {
    public :
        static entree *read(user_interaction & dialog,
			    memory_pool *pool,
			    generic_file & f, const archive_version & reading_ver,
			    entree_stats & stats,
			    std::map <infinint, etoile *> & corres,
			    compression default_algo,
			    generic_file *data_loc,
			    compressor *efsa_loc,
			    bool lax,
			    bool only_detruit,
			    escape *ptr);

        virtual ~entree() {};

	    /// write down the object information to a generic_file

	    /// \param[in,out] f is the file where to write the data to
	    /// \param[in] small defines whether to do a small or normal dump
	    /// \note small dump are used beside escape sequence marks they can be done
	    /// before the a file's data or EA has took its place within the archive
	    /// while normal dump are used with catalogue dump at the end of the archive
	    /// creation
        void dump(generic_file & f, bool small) const;

	    /// this call gives an access to inherited_dump

	    /// \param[in,out] f is the file where to write the data to
	    /// \param[in] small defines whether to do a small or normal dump
	    /// \note this method is to avoid having class mirage and class directory being
	    /// a friend of class entree. Any other class may use it, sure, but neither
	    /// class mirage nor class directory has not access to class entree's private
	    /// data, only to what it needs.
	void specific_dump(generic_file & f, bool small) const { inherited_dump(f, small); };

	    /// called by entree::read and mirage::post_constructor, let inherited classes builds object's data after CRC has been read from file

	    /// \param[in,out] f is the file where to write the data to
	    /// \note only used when an non NULL escape pointer is given to entree::read (reading a small dump).
	virtual void post_constructor(generic_file & f) {};

        virtual unsigned char signature() const = 0;
        virtual entree *clone() const = 0;


    protected:
	virtual void inherited_dump(generic_file & f, bool small) const;


    private:
	static const U_I ENTREE_CRC_SIZE;

    };

    extern bool compatible_signature(unsigned char a, unsigned char b);
    extern unsigned char mk_signature(unsigned char base, saved_status state);
    extern unsigned char get_base_signature(unsigned char a);

	/// the End of Directory entry class
    class eod : public entree
    {
    public :
        eod() {};
        eod(generic_file & f) {};
            // dump defined by entree
        unsigned char signature() const { return 'z'; };
        entree *clone() const { return new (get_pool()) eod(); };

    };

	/// the base class for all entry that have a name
    class nomme : public entree
    {
    public:
        nomme(const std::string & name) { xname = name; };
        nomme(generic_file & f);
	virtual bool operator == (const nomme & ref) const { return xname == ref.xname; };
	virtual bool operator < (const nomme & ref) const { return xname < ref.xname; };

        const std::string & get_name() const { return xname; };
        void change_name(const std::string & x) { xname = x; };
        bool same_as(const nomme & ref) const { return xname == ref.xname; };
            // no need to have a virtual method, as signature will differ in inherited classes (argument type changes)

            // signature() is kept as an abstract method
            // clone() is abstract


    protected:
        void inherited_dump(generic_file & f, bool small) const;

    private:
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

        inode(const infinint & xuid,
	      const infinint & xgid,
	      U_16 xperm,
              const datetime & last_access,
              const datetime & last_modif,
	      const datetime & last_change,
              const std::string & xname,
	      const infinint & device);
        inode(user_interaction & dialog,
	      generic_file & f,
	      const archive_version & reading_ver,
	      saved_status saved,
	      compressor *efsa_loc,
	      escape *ptr);      // if ptr is not NULL, reading a partial dump(), which was done with "small" set to true
        inode(const inode & ref);
	const inode & operator = (const inode & ref);
        ~inode();

        const infinint & get_uid() const { return uid; };
        const infinint & get_gid() const { return gid; };
        U_16 get_perm() const { return perm; };
        datetime get_last_access() const { return last_acc; };
        datetime get_last_modif() const { return last_mod; };
        void set_last_access(const datetime & x_time) { last_acc = x_time; };
        void set_last_modif(const datetime & x_time) { last_mod = x_time; };
        saved_status get_saved_status() const { return xsaved; };
        void set_saved_status(saved_status x) { xsaved = x; };
	infinint get_device() const { if(fs_dev == NULL) throw SRC_BUG; return *fs_dev; };

        bool same_as(const inode & ref) const;
        bool is_more_recent_than(const inode & ref, const infinint & hourshift) const;
	    // used for RESTORATION
        virtual bool has_changed_since(const inode & ref, const infinint & hourshift, comparison_fields what_to_check) const;
            // signature() left as an abstract method
            // clone is abstract too
	    // used for INCREMENTAL BACKUP
        void compare(const inode &other,
		     const mask & ea_mask,
		     comparison_fields what_to_check,
		     const infinint & hourshift,
		     bool symlink_date,
		     const fsa_scope & scope,
		     bool isolated_mode) const; //< do not try to compare pointed to data, EA of FSA (suitable for isolated catalogue)

            // throw Erange exception if a difference has been detected
            // this is not a symetrical comparison, but all what is present
            // in the current object is compared against the argument
            // which may contain supplementary informations
	    // used for DIFFERENCE



            //////////////////////////////////
            // EXTENDED ATTRIBUTES Methods
            //

        enum ea_status { ea_none, ea_partial, ea_fake, ea_full, ea_removed };
            // ea_none    : no EA  present for this inode in filesystem
            // ea_partial : EA present in filesystem but not stored (ctime used to check changes)
	    // ea_fake    : EA present in filesystem but not attached to this inode (isolation context) no more used in archive version "08" and above, ea_partial or ea_full stays a valid status in isolated catalogue because pointers to EA and data are no more removed during isolation process.
            // ea_full    : EA present in filesystem and attached to this inode
	    // ea_removed : EA were present in the reference version, but not present anymore

            // I : to know whether EA data is present or not for this object
        void ea_set_saved_status(ea_status status);
        ea_status ea_get_saved_status() const { return ea_saved; };

            // II : to associate EA list to an inode object (mainly for backup operation) #EA_FULL only#
        void ea_attach(ea_attributs *ref);
        const ea_attributs *get_ea() const;              //   #<-- EA_FULL *and* EA_REMOVED# for this call only
        void ea_detach() const; //discards any future call to get_ea() !
	infinint ea_get_size() const; //returns the size of EA (still valid if ea have been detached) mainly used to define CRC width

            // III : to record where is dump the EA in the archive #EA_FULL only#
        void ea_set_offset(const infinint & pos);
	bool ea_get_offset(infinint & pos) const;
        void ea_set_crc(const crc & val);
	void ea_get_crc(const crc * & ptr) const; //< the argument is set to point to an allocated crc object owned by this "inode" object, this reference stays valid while the "inode" object exists and MUST NOT be deleted by the caller in any case
	bool ea_get_crc_size(infinint & val) const; //< returns true if crc is know and puts its width in argument

            // IV : to know/record if EA and FSA have been modified # any EA status# and FSA status #
        datetime get_last_change() const;
        void set_last_change(const datetime & x_time);
	bool has_last_change() const { return last_cha != NULL; };
	    // old format did provide last_change only when EA were present, since archive
	    // format 8, this field is always present even in absence of EA. Thus it is
	    // still necessary to check if the inode has a last_change() before
	    // using get_last_change() (depends on the version of the archive read).


	    // V : for archive migration (merging) EA and FSA concerned
        void change_efsa_location(compressor *loc) { storage = loc; };


            //////////////////////////////////
            // FILESYSTEM SPECIFIC ATTRIBUTES Methods
            //
	    // there is not "remove status for FSA, either the inode contains
	    // full copy of FSA or only remembers the families of FSA found in the unchanged inode
	    // FSA none is used when the file has no FSA because:
	    //  - either the underlying filesystem has no known FSA
	    //  - or the underlying filesystem FSA support has not been activated at compilation time
	    //  - or the fsa_scope requested at execution time exclude the filesystem FSA families available here
	enum fsa_status { fsa_none, fsa_partial, fsa_full };

	    // I : which FSA are present
	void fsa_set_saved_status(fsa_status status);
	fsa_status fsa_get_saved_status() const { return fsa_saved; };
	    /// gives the set of FSA family recorded for that inode
	fsa_scope fsa_get_families() const { if(fsa_families == NULL) throw SRC_BUG; return infinint_to_fsa_scope(*fsa_families); };



	    // II : add or drop FSA list to the inode
	void fsa_attach(filesystem_specific_attribute_list *ref);
	const filesystem_specific_attribute_list *get_fsa() const; // #<-- FSA_FULL only
	void fsa_detach() const; // discard any future call to get_fsa() !
	infinint fsa_get_size() const; // returns the size of FSA (still valid if fsal has been detached) / mainly used to define CRC size

	    // III : to record where FSA are dumped in the archive (only if fsa_status not empty !)
	void fsa_set_offset(const infinint & pos);
	bool fsa_get_offset(infinint & pos) const;
	void fsa_set_crc(const crc & val);
	void fsa_get_crc(const crc * & ptr) const;
	bool fsa_get_crc_size(infinint & val) const;

    protected:
        virtual void sub_compare(const inode & other, bool isolated_mode) const {};

	    /// escape generic_file relative methods
	escape *get_escape_layer() const { return esc; };

        void inherited_dump(generic_file & f, bool small) const;

    private :
        infinint uid;            //< inode owner's user ID
        infinint gid;            //< inode owner's group ID
        U_16 perm;               //< inode's permission
        datetime last_acc;       //< last access time (atime)
	datetime last_mod;       //< last modification time (mtime)
        datetime *last_cha;
      //< last inode meta data change (ctime)
        saved_status xsaved;     //< inode data status
        ea_status ea_saved;      //< inode Extended Attribute status
	fsa_status fsa_saved;    //< inode Filesystem Specific Attribute status

            //  the following is used only if ea_saved == full
        infinint *ea_offset;     //< offset in archive where to find EA
        ea_attributs *ea;        //< Extended Attributes read or to be written down
	infinint *ea_size;       //< storage size required by EA
            // the following is used if ea_saved == full or ea_saved == partial or
        crc *ea_crc;             //< CRC computed on EA

	infinint *fsa_families; //< list of FSA families present for that inode (set to NULL in fsa_none mode)
	infinint *fsa_offset;    //< offset in archive where to find FSA  # always allocated (to be reviewed)
	filesystem_specific_attribute_list *fsal; //< Filesystem Specific Attributes read or to be written down # only allocated if fsa_saved if set to FULL
	infinint *fsa_size;      //< storage size required for FSA
	crc *fsa_crc;            //< CRC computed on FSA
	    //
	infinint *fs_dev;        //< filesystem ID on which resides the inode (only used when read from filesystem)
	compressor *storage;     //< where are stored EA and FSA
	archive_version edit;    //< need to know EA and FSA format used in archive file

	escape *esc;  // if not NULL, the object is partially build from archive (at archive generation, dump() was called with small set to true)

	void nullifyptr();
	void destroy();
	void copy_from(const inode & ref);

	static const ea_attributs empty_ea;
    };

	/// the hard link implementation (etoile means star in French, seen a star as a point from which are thrown many ray of light)
    class etoile : public on_pool
    {
    public:

	    /// build a object

	    ///\param[in] host is an inode, it must not be a directory (this would throw an Erange exception)
	    ///\param[in] etiquette_number is the identifier of this multiply linked structure
	    ///\note given inode is now managed by the etoile object
	etoile(inode *host, const infinint & etiquette_number);
	etoile(const etoile & ref) { throw SRC_BUG; }; // copy constructor not allowed for this class
	const etoile & operator = (const etoile & ref) { throw SRC_BUG; }; // assignment not allowed for this class
	~etoile() { delete hosted; };

	void add_ref(void *ref);
	void drop_ref(void *ref);
	infinint get_ref_count() const { return refs.size(); };
	inode *get_inode() const { return hosted; };
	infinint get_etiquette() const { return etiquette; };
	void change_etiquette(const infinint & new_val) { etiquette = new_val; };


	bool is_counted() const { return tags.counted; };
	bool is_wrote() const { return tags.wrote; };
	bool is_dumped() const { return tags.dumped; };
	void set_counted(bool val) { tags.counted = val ? 1 : 0; };
	void set_wrote(bool val) { tags.wrote = val ? 1 : 0; };
	void set_dumped(bool val) { tags.dumped = val ? 1 : 0; };

	    // return the address of the first mirage that triggered the creation of this mirage
	    // if this object is destroyed afterward this call returns NULL
	const void *get_first_ref() const { if(refs.size() == 0) throw SRC_BUG; return refs.front(); };


    private:
	struct bool_tags
	{
	    unsigned counted : 1; //< whether the inode has been counted
	    unsigned wrote : 1;   //< whether the inode has its data copied to archive
	    unsigned dumped : 1;  //< whether the inode information has been dumped in the catalogue
	    unsigned : 5;         //< padding to get byte boundary and reserved for future use.

	    bool_tags() { counted = wrote = dumped = 0; };
	};

	std::list<void *> refs; //< list of pointers to the mirages objects, in the order of their creation
	inode *hosted;
	infinint etiquette;
	bool_tags tags;
    };

	/// the hard link implementation, mirage is the named entry owned by a directory it points to a common "etoile class"

	/// well, mirage is those fake apparition of water in a desert... I guess you get the picture now... :-)
    class mirage : public nomme
    {
    public:
	enum mirage_format {fmt_mirage,           //< new format
			    fmt_hard_link,        //< old dual format
			    fmt_file_etiquette }; //< old dual format

	mirage(const std::string & name, etoile *ref) : nomme(name) { star_ref = ref; if(ref == NULL) throw SRC_BUG; star_ref->add_ref(this); };
	mirage(user_interaction & dialog,
	       generic_file & f,
	       const archive_version & reading_ver,
	       saved_status saved,
	       entree_stats & stats,
	       std::map <infinint, etoile *> & corres,
	       compression default_algo,
	       generic_file *data_loc,
	       compressor *efsa_loc,
	       mirage_format fmt,
	       bool lax,
	       escape *ptr);
	mirage(user_interaction & dialog,
	       generic_file & f,
	       const archive_version & reading_ver,
	       saved_status saved,
	       entree_stats & stats,
	       std::map <infinint, etoile *> & corres,
	       compression default_algo,
	       generic_file *data_loc,
	       compressor *efsa_loc,
	       bool lax,
	       escape *ptr);
	mirage(const mirage & ref) : nomme (ref) { star_ref = ref.star_ref; if(star_ref == NULL) throw SRC_BUG; star_ref->add_ref(this); };
	const mirage & operator = (const mirage & ref);
	~mirage() { star_ref->drop_ref(this); };

	unsigned char signature() const { return 'm'; };
	entree *clone() const { return new (get_pool()) mirage(*this); };

	inode *get_inode() const { if(star_ref == NULL) throw SRC_BUG; return star_ref->get_inode(); };
	infinint get_etiquette() const { return star_ref->get_etiquette(); };
	infinint get_etoile_ref_count() const { return star_ref->get_ref_count(); };
	etoile *get_etoile() const { return star_ref; };

	bool is_inode_counted() const { return star_ref->is_counted(); };
	bool is_inode_wrote() const { return star_ref->is_wrote(); };
	bool is_inode_dumped() const { return star_ref->is_dumped(); };
	void set_inode_counted(bool val) const { star_ref->set_counted(val); };
	void set_inode_wrote(bool val) const { star_ref->set_wrote(val); };
	void set_inode_dumped(bool val) const { star_ref->set_dumped(val); };

	void post_constructor(generic_file & f);

	    /// whether we are the mirage that triggered this hard link creation
	bool is_first_mirage() const { return star_ref->get_first_ref() == this; };


    protected:
	void inherited_dump(generic_file & f, bool small) const;

    private:
	etoile *star_ref;

	void init(user_interaction & dialog,
		  generic_file & f,
		  const archive_version & reading_ver,
		  saved_status saved,
		  entree_stats & stats,
		  std::map <infinint, etoile *> & corres,
		  compression default_algo,
		  generic_file *data_loc,
		  compressor *efsa_loc,
		  mirage_format fmt,
		  bool lax,
		  escape *ptr);
    };


	/// the plain file class
    class file : public inode
    {
    public :
	enum get_data_mode
	{
	    keep_compressed, //< provide access to compressed data
	    keep_hole,       //< provide access to uncompressed data but sparse_file datastructure
	    normal,          //< provide access to full data (uncompressed, uses skip() to restore holes)
	    plain            //< provide access to plain data, no skip to restore holes, provide instead zeroed bytes
	};

	static const U_8 FILE_DATA_WITH_HOLE = 0x01; //< file's data contains hole datastructure
	static const U_8 FILE_DATA_IS_DIRTY = 0x02;  //< data modified while being saved

        file(const infinint & xuid, const infinint & xgid, U_16 xperm,
             const datetime & last_access,
             const datetime & last_modif,
	     const datetime & last_change,
             const std::string & src,
             const path & che,
             const infinint & taille,
	     const infinint & fs_device,
	     bool x_furtive_read_mode);
        file(const file & ref);
        file(user_interaction & dialog,
	     generic_file & f,
	     const archive_version & reading_ver,
	     saved_status saved,
	     compression default_algo,
	     generic_file *data_loc,
	     compressor *efsa_loc,
	     escape *ptr);
        ~file() { detruit(); };

        bool has_changed_since(const inode & ref, const infinint & hourshift, inode::comparison_fields what_to_check) const;
        infinint get_size() const { return *size; };
	void change_size(const infinint & s) const { *size = s; };
        infinint get_storage_size() const { return *storage_size; };
        void set_storage_size(const infinint & s) { *storage_size = s; };
        virtual generic_file *get_data(get_data_mode mode) const; // returns a newly allocated object in read_only mode
        void clean_data(); // partially free memory (but get_data() becomes disabled)
        void set_offset(const infinint & r);
	const infinint & get_offset() const;
        unsigned char signature() const { return mk_signature('f', get_saved_status()); };

        void set_crc(const crc &c);
        bool get_crc(const crc * & c) const; //< the argument is set the an allocated crc object the owned by the "file" object, its stay valid while this "file" object exists and MUST NOT be deleted by the caller in any case
	bool has_crc() const { return check != NULL; };
	bool get_crc_size(infinint & val) const; //< returns true if crc is know and puts its width in argument
	void drop_crc() { if(check != NULL) { delete check; check = NULL; } };

	    // whether the plain file has to detect sparse file
	void set_sparse_file_detection_read(bool val) { if(status == from_cat) throw SRC_BUG; if(val) file_data_status_read |= FILE_DATA_WITH_HOLE; else file_data_status_read &= ~FILE_DATA_WITH_HOLE; };

	void set_sparse_file_detection_write(bool val) { if(val) file_data_status_write |= FILE_DATA_WITH_HOLE; else file_data_status_write &= ~FILE_DATA_WITH_HOLE; };

	    // whether the plain file is stored with a sparse_file datastructure in the archive
	bool get_sparse_file_detection_read() const { return (file_data_status_read & FILE_DATA_WITH_HOLE) != 0; };
	bool get_sparse_file_detection_write() const { return (file_data_status_write & FILE_DATA_WITH_HOLE) != 0; };

        entree *clone() const { return new (get_pool()) file(*this); };

        compression get_compression_algo_read() const { return algo_read; };

	compression get_compression_algo_write() const { return algo_write; };

	    // object migration methods (merging)
	void change_compression_algo_write(compression x) { algo_write = x; };
	void change_location(generic_file *x) { loc = x; };

	    // dirtiness

	bool is_dirty() const { return dirty; };
	void set_dirty(bool value) { dirty = value; };

    protected:
        void sub_compare(const inode & other, bool isolated_mode) const;
        void inherited_dump(generic_file & f, bool small) const;
	void post_constructor(generic_file & f);

        enum { empty, from_path, from_cat } status;

    private:
	std::string chemin;     //< path to the data (when read from filesystem)
        infinint *offset;       //< start location of the data in 'loc'
        infinint *size;         //< size of the data (uncompressed)
        infinint *storage_size; //< how much data used in archive (after compression)
        crc *check;
	bool dirty;     //< true when a file has been modified at the time it was saved

        generic_file *loc;      //< where to find data (eventually compressed) at the recorded offset and for storage_size length
        compression algo_read;  //< which compression algorithm to use to read the file's data
	compression algo_write; //< which compression algorithm to use to write down (merging) the file's data
	bool furtive_read_mode; // used only when status equals "from_path"
	char file_data_status_read; // defines the datastructure to use when reading the data
	char file_data_status_write; // defines the datastructure to apply when writing down the data

        void detruit();
    };

	/// the class for Door IPC (mainly for Solaris)
    class door : public file
    {
    public:
        door(const infinint & xuid,
	     const infinint & xgid,
	     U_16 xperm,
             const datetime & last_access,
             const datetime & last_modif,
             const datetime & last_change,
             const std::string & src,
             const path & che,
             const infinint & fs_device) : file(xuid, xgid, xperm, last_access, last_modif,
						last_change, src, che, 0, fs_device, false) {};
        door(user_interaction & dialog,
             generic_file & f,
             const archive_version & reading_ver,
             saved_status saved,
             compression default_algo,
             generic_file *data_loc,
             compressor *efsa_loc,
             escape *ptr) : file(dialog, f, reading_ver, saved, default_algo, data_loc, efsa_loc, ptr) {};

        unsigned char signature() const { return mk_signature('o', get_saved_status()); };

        generic_file *get_data(get_data_mode mode) const; // inherited from class file

    };

	/// the symbolic link inode class
    class lien : public inode
    {
    public :
        lien(const infinint & uid, const infinint & gid, U_16 perm,
             const datetime & last_access,
             const datetime & last_modif,
	     const datetime & last_change,
             const std::string & name,
	     const std::string & target,
	     const infinint & fs_device);
        lien(user_interaction & dialog,
	     generic_file & f,
	     const archive_version & reading_ver,
	     saved_status saved,
	     compressor *efsa_loc,
	     escape *ptr);

        const std::string & get_target() const;
        void set_target(std::string x);

            // using the method is_more_recent_than() from inode
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('l', get_saved_status()); };
        entree *clone() const { return new (get_pool()) lien(*this); };

    protected :
        void sub_compare(const inode & other, bool isolated_mode) const;
        void inherited_dump(generic_file & f, bool small) const;


    private :
        std::string points_to;
    };

	/// the directory inode class
    class directory : public inode
    {
    public :
        directory(const infinint & xuid,
		  const infinint & xgid,
		  U_16 xperm,
                  const datetime & last_access,
                  const datetime & last_modif,
		  const datetime & last_change,
                  const std::string & xname,
		  const infinint & device);
        directory(const directory &ref); // only the inode part is build, no children is duplicated (empty dir)
	const directory & operator = (const directory & ref); // set the inode part *only* no subdirectories/subfiles are copies or removed.
        directory(user_interaction & dialog,
		  generic_file & f,
		  const archive_version & reading_ver,
		  saved_status saved,
		  entree_stats & stats,
		  std::map <infinint, etoile *> & corres,
		  compression default_algo,
		  generic_file *data_loc,
		  compressor *efsa_loc,
		  bool lax,
		  bool only_detruit, // objects of other class than detruit and directory are not built in memory
		  escape *ptr);
        ~directory(); // detruit aussi tous les fils et se supprime de son 'parent'

        void add_children(nomme *r); // when r is a directory, 'parent' is set to 'this'
	bool has_children() const { return !ordered_fils.empty(); };
        void reset_read_children() const;
	void end_read() const;
        bool read_children(const nomme * &r) const; // read the direct children of the directory, returns false if no more is available
	    // remove all entry not yet read by read_children
	void tail_to_read_children();

	void remove(const std::string & name); // remove the given entry from the catalogue
	    // as side effect the reset_read_children() method must be called.

        directory * get_parent() const { return parent; };
        bool search_children(const std::string &name, const nomme *&ref) const;
	bool callback_for_children_of(user_interaction & dialog, const std::string & sdir, bool isolated = false) const;

            // using is_more_recent_than() from inode class
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('d', get_saved_status()); };

	    /// detemine whether some data has changed since archive of reference in this directory or subdirectories
	bool get_recursive_has_changed() const { return recursive_has_changed; };

	    /// ask recursive update for the recursive_has_changed field
	void recursive_has_changed_update() const;

	    /// get then number of "nomme" entry contained in this directory and subdirectories (recursive call)
	infinint get_tree_size() const;

	    /// get the number of entry having some EA set in the directory tree (recursive call)
	infinint get_tree_ea_num() const;

	    /// get the number of entry that are hard linked inode (aka mirage in dar implementation) (recursive call)
	infinint get_tree_mirage_num() const;

	    // for each mirage found (hard link implementation) in the directory tree, add its etiquette to the returned
	    // list with the number of reference that has been found in the tree. (map[etiquette] = number of occurence)
	    // from outside of class directory, the given argument is expected to be an empty map.
	void get_etiquettes_found_in_tree(std::map<infinint, infinint> & already_found) const;

	    /// whether this directory is empty or not
	bool is_empty() const { return ordered_fils.empty(); };

	    /// recursively remove all mirage entries
	void remove_all_mirages_and_reduce_dirs();

	    /// set the value of inode_dumped for all mirage (recusively)
	void set_all_mirage_s_inode_dumped_field_to(bool val);

        entree *clone() const { return new (get_pool()) directory(*this); };

	const infinint & get_size() const { recursive_update_sizes(); return x_size; };
	const infinint & get_storage_size() const { recursive_update_sizes(); return x_storage_size; };

	void recursively_set_to_unsaved_data_and_FSA();

    protected:
        void inherited_dump(generic_file & f, bool small) const;

    private:
	static const eod fin;

	infinint x_size;
	infinint x_storage_size;
	bool updated_sizes;
        directory *parent;
#ifdef LIBDAR_FAST_DIR
        std::map<std::string, nomme *> fils; // used for fast lookup
#endif
	std::list<nomme *> ordered_fils;
        std::list<nomme *>::iterator it;
	bool recursive_has_changed;

	void clear();
	void recursive_update_sizes() const;
	void recursive_flag_size_to_update() const;
    };

	/// the special device root class
    class device : public inode
    {
    public :
        device(const infinint & uid, const infinint & gid, U_16 perm,
               const datetime & last_access,
               const datetime & last_modif,
	       const datetime &last_change,
               const std::string & name,
               U_16 major,
               U_16 minor,
	       const infinint & fs_device);
        device(user_interaction & dialog,
	       generic_file & f,
	       const archive_version & reading_ver,
	       saved_status saved,
	       compressor *efsa_loc,
	       escape *ptr);

        int get_major() const { if(get_saved_status() != s_saved) throw SRC_BUG; else return xmajor; };
        int get_minor() const { if(get_saved_status() != s_saved) throw SRC_BUG; else return xminor; };
        void set_major(int x) { xmajor = x; };
        void set_minor(int x) { xminor = x; };

            // using method is_more_recent_than() from inode class
            // using method has_changed_since() from inode class
            // signature is left pure abstract

    protected :
        void sub_compare(const inode & other, bool isolated_mode) const;
        void inherited_dump(generic_file & f, bool small) const;

    private :
        U_16 xmajor, xminor;
    };

	/// the char device class
    class chardev : public device
    {
    public:
        chardev(const infinint & uid, const infinint & gid, U_16 perm,
                const datetime & last_access,
                const datetime & last_modif,
		const datetime & last_change,
                const std::string & name,
                U_16 major,
                U_16 minor,
		const infinint & fs_device) : device(uid, gid, perm,
						     last_access,
						     last_modif,
						     last_change,
						     name,
						     major, minor, fs_device) {};
        chardev(user_interaction & dialog,
		generic_file & f,
		const archive_version & reading_ver,
		saved_status saved,
		compressor *efsa_loc,
		escape *ptr) : device(dialog, f, reading_ver, saved, efsa_loc, ptr) {};

            // using dump from device class
            // using method is_more_recent_than() from device class
            // using method has_changed_since() from device class
        unsigned char signature() const { return mk_signature('c', get_saved_status()); };
        entree *clone() const { return new (get_pool()) chardev(*this); };
    };

	/// the block device class
    class blockdev : public device
    {
    public:
        blockdev(const infinint & uid, const infinint & gid, U_16 perm,
                 const datetime & last_access,
                 const datetime & last_modif,
		 const datetime & last_change,
                 const std::string & name,
                 U_16 major,
                 U_16 minor,
		 const infinint & fs_device) : device(uid, gid, perm, last_access,
						      last_modif, last_change, name,
						      major, minor, fs_device) {};
        blockdev(user_interaction & dialog,
		 generic_file & f,
		 const archive_version & reading_ver,
		 saved_status saved,
		 compressor *efsa_loc,
		 escape *ptr) : device(dialog, f, reading_ver, saved, efsa_loc, ptr) {};

            // using dump from device class
            // using method is_more_recent_than() from device class
            // using method has_changed_since() from device class
        unsigned char signature() const { return mk_signature('b', get_saved_status()); };
        entree *clone() const { return new (get_pool()) blockdev(*this); };
    };

	/// the named pipe class
    class tube : public inode
    {
    public :
        tube(const infinint & xuid, const infinint & xgid, U_16 xperm,
             const datetime & last_access,
             const datetime & last_modif,
	     const datetime & last_change,
             const std::string & xname,
	     const infinint & fs_device) : inode(xuid, xgid, xperm, last_access, last_modif, last_change, xname, fs_device) { set_saved_status(s_saved); };
        tube(user_interaction & dialog,
	     generic_file & f,
	     const archive_version & reading_ver,
	     saved_status saved,
 	     compressor *efsa_loc,
	     escape *ptr) : inode(dialog, f, reading_ver, saved, efsa_loc, ptr) {};

            // using dump from inode class
            // using method is_more_recent_than() from inode class
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('p', get_saved_status()); };
        entree *clone() const { return new (get_pool()) tube(*this); };
    };

	/// the Unix socket inode class
    class prise : public inode
    {
    public :
        prise(const infinint & xuid, const infinint & xgid, U_16 xperm,
              const datetime & last_access,
              const datetime & last_modif,
	      const datetime & last_change,
              const std::string & xname,
	      const infinint & fs_device) : inode(xuid, xgid, xperm, last_access, last_modif, last_change, xname, fs_device) { set_saved_status(s_saved); };
        prise(user_interaction & dialog,
	      generic_file & f,
	      const archive_version & reading_ver,
	      saved_status saved,
	      compressor *efsa_loc,
	      escape *ptr) : inode(dialog, f, reading_ver, saved, efsa_loc, ptr) {};

            // using dump from inode class
            // using method is_more_recent_than() from inode class
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('s', get_saved_status()); };
        entree *clone() const { return new (get_pool()) prise(*this); };
    };

	/// the deleted file entry
    class detruit : public nomme
    {
    public :
        detruit(const std::string & name, unsigned char firm, const datetime & date) : nomme(name) , del_date(date) { signe = firm; };
        detruit(generic_file & f, const archive_version & reading_ver);
	detruit(const nomme &ref) : nomme(ref.get_name()), del_date(0) { signe = ref.signature(); };

        unsigned char get_signature() const { return signe; };
        void set_signature(unsigned char x) { signe = x; };
        unsigned char signature() const { return 'x'; };
        entree *clone() const { return new (get_pool()) detruit(*this); };

	const datetime & get_date() const { return del_date; };
	void set_date(const datetime & ref) { del_date = ref; };

    protected:
        void inherited_dump(generic_file & f, bool small) const;

    private :
        unsigned char signe;
	datetime del_date;
    };

	/// the present file to ignore (not to be recorded as deleted later)
    class ignored : public nomme
    {
    public :
        ignored(const std::string & name) : nomme(name) {};
        ignored(generic_file & f) : nomme(f) { throw SRC_BUG; };

        unsigned char signature() const { return 'i'; };
        entree *clone() const { return new (get_pool()) ignored(*this); };

    protected:
        void inherited_dump(generic_file & f, bool small) const { throw SRC_BUG; };

    };

	/// the ignored directory class, to be promoted later as empty directory if needed
    class ignored_dir : public inode
    {
    public:
        ignored_dir(const directory &target) : inode(target) {};
        ignored_dir(user_interaction & dialog,
		    generic_file & f,
		    const archive_version & reading_ver,
		    compressor *efsa_loc,
		    escape *ptr) : inode(dialog, f, reading_ver, s_not_saved, efsa_loc, ptr) { throw SRC_BUG; };

        unsigned char signature() const { return 'j'; };
        entree *clone() const { return new (get_pool()) ignored_dir(*this); };

    protected:
        void inherited_dump(generic_file & f, bool small) const; // behaves like an empty directory

    };

	/// the catalogue class which gather all objects contained in a give archive
    class catalogue : protected mem_ui, public on_pool
    {
    public :
        catalogue(user_interaction & dialog,
		  const datetime & root_last_modif,
		  const label & data_name);
        catalogue(user_interaction & dialog,
		  generic_file & f,
		  const archive_version & reading_ver,
		  compression default_algo,
		  generic_file *data_loc,
		  compressor *efsa_loc,
		  bool lax,
		  const label & lax_layer1_data_name, //< ignored unless in lax mode, in lax mode unless it is a cleared label, forces the catalogue label to be equal to the lax_layer1_data_name for it be considered a plain internal catalogue, even in case of corruption
		  bool only_detruit = false); //< if set to true, only directories and detruit objects are read from the archive
        catalogue(const catalogue & ref) : mem_ui(ref), out_compare(ref.out_compare) { partial_copy_from(ref); };
        const catalogue & operator = (const catalogue &ref);
        virtual ~catalogue() { detruire(); };


	    // reading methods. The reading is iterative and uses the current_read directory pointer

        virtual void reset_read() const; // set the reading cursor to the beginning of the catalogue
	virtual void end_read() const; // set the reading cursor to the end of the catalogue
        virtual void skip_read_to_parent_dir() const;
            // skip all items of the current dir and of any subdir, the next call will return
            // next item of the parent dir (no eod to exit from the current dir !)
        virtual bool read(const entree * & ref) const;
            // sequential read (generates eod) and return false when all files have been read
        virtual bool read_if_present(std::string *name, const nomme * & ref) const;
            // pseudo-sequential read (reading a directory still
            // implies that following read are located in this subdirectory up to the next EOD) but
            // it returns false if no entry of this name are present in the current directory
            // a call with NULL as first argument means to set the current dir the parent directory
	void remove_read_entry(std::string & name);
	    // in the currently read directory, removes the entry which name is given in argument
	const directory & get_current_reading_dir() const { if(current_read == NULL) throw SRC_BUG; return *current_read; };
	    // remove from the catalogue all the entries that have not yet been read
	    // by read().
	void tail_catalogue_to_current_read();


        void reset_sub_read(const path &sub); // initialise sub_read to the given directory
        bool sub_read(const entree * &ref); // sequential read of the catalogue, ignoring all that
            // is not part of the subdirectory specified with reset_sub_read
            // the read include the inode leading to the sub_tree as well as the pending eod

	    // return true if the last read entry has already been read
	    // and has not to be counted again. This is never the case for catalogue but may occure
	    // with escape_catalogue (where from the 'virtual').
	    // last this method gives a valid result only if the last read() entry is a directory as
	    // only directory may be read() twice.
	virtual bool read_second_time_dir() const { return false; };


	    // Additions methods. The addition is also iterative but uses its specific current_add directory pointer

        void reset_add();

	    /// catalogue extension routines for escape sequence
	    // real implementation is only needed in escape_catalogue class, here there nothing to be done
	virtual void pre_add(const entree *ref) const {};
	virtual void pre_add_ea(const entree *ref) const {};
	virtual void pre_add_crc(const entree *ref) const {};
	virtual void pre_add_dirty() const {};
	virtual void pre_add_ea_crc(const entree *ref) const {};
	virtual void pre_add_waste_mark() const {};
	virtual void pre_add_failed_mark() const {};
	virtual void pre_add_fsa(const entree *ref) const {};
	virtual void pre_add_fsa_crc(const entree *ref) const {};
	virtual escape *get_escape_layer() const { return NULL; };

        void add(entree *ref); // add at end of catalogue (sequential point of view)
	void re_add_in(const std::string &subdirname); // return into an already existing subdirectory for further addition
	void re_add_in_replace(const directory &dir); // same as re_add_in but also set the properties of the existing directory to those of the given argument
        void add_in_current_read(nomme *ref); // add in currently read directory
	const directory & get_current_add_dir() const { if(current_add == NULL) throw SRC_BUG; return *current_add; };



	    // Comparison methods. The comparision is here also iterative and uses its specific current_compare directory pointer

        void reset_compare() const;
        bool compare(const entree * name, const entree * & extracted) const;
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



	    // non interative methods

        infinint update_destroyed_with(const catalogue & ref);
            // ref must have the same root, else the operation generates an exception

	void update_absent_with(const catalogue & ref, infinint aborting_next_etoile);
	    // in case of abortion, completes missing files as if what could not be
	    // inspected had not changed since the reference was done
	    // aborting_last_etoile is the highest etoile reference withing "this" current object.

	    /// before dumping the catalogue, need to set all hardlinked inode they have not been saved once
	void reset_dump() const;

	    /// write down the whole catalogue to file
        void dump(generic_file & f) const;

        void listing(bool isolated,
		     const mask &selection,
		     const mask & subtree,
		     bool filter_unsaved,
		     bool list_ea,
		     std::string marge) const;
        void tar_listing(bool isolated,
			 const mask & selection,
			 const mask & subtree,
			 bool filter_unsaved,
			 bool list_ea,
			 std::string beginning) const;
        void xml_listing(bool isolated,
			 const mask & selection,
			 const mask & subtree,
			 bool filter_unsaved,
			 bool list_ea,
			 std::string beginning) const;
	void slice_listing(bool isolated,
			   const mask & selection,
			   const mask & subtree,
			   const slice_layout & slicing) const;

        entree_stats get_stats() const { return stats; };

	    /// whether the catalogue is empty or not
	bool is_empty() const { if(contenu == NULL) throw SRC_BUG; return contenu->is_empty(); };

        const directory *get_contenu() const { return contenu; }; // used by data_tree

	const label & get_data_name() const { return ref_data_name; };
	datetime get_root_dir_last_modif() const { return contenu->get_last_modif(); };

	    /// recursive evaluation of directories that have changed (make the directory::get_recurisve_has_changed() method of entry in this catalogue meaningful)
	void launch_recursive_has_changed_update() const { contenu->recursive_has_changed_update(); };

	datetime get_root_mtime() const { return contenu->get_last_modif(); };

	    /// reset all pointers to the root (a bit better than reset_add() + reset_read() + reset_compare() + reset_sub_read())
	void reset_all();

	void set_to_unsaved_data_and_FSA() { if(contenu == NULL) throw SRC_BUG; contenu->recursively_set_to_unsaved_data_and_FSA(); };


    protected:
	entree_stats & access_stats() { return stats; };
	void set_data_name(const label & val) { ref_data_name = val; };
	void copy_detruits_from(const catalogue & ref); // needed for escape_catalogue implementation only.

	const eod * get_r_eod_address() const { return & r_eod; }; // eod are never stored in the catalogue
	    // however it is sometimes required to return such a reference to a valid object
	    // owned by the catalogue.


	    /// invert the data tree memory management responsibility pointed to by "contenu" pointers between the current
	    /// catalogue and the catalogue given in argument.
	void swap_stuff(catalogue & ref);

    private :
        directory *contenu;               ///< catalogue contents
        path out_compare;                 ///< stores the missing directory structure, when extracting
        directory *current_compare;       ///< points to the current directory when extracting
        directory *current_add;           ///< points to the directory where to add the next file with add_file;
        directory *current_read;          ///< points to the directory where the next item will be read
        path *sub_tree;                   ///< path to sub_tree
        signed int sub_count;             ///< count the depth in of read routine in the sub_tree
        entree_stats stats;               ///< statistics catalogue contents
	label ref_data_name;              ///< name of the archive where is located the data

        void partial_copy_from(const catalogue &ref);
        void detruire();

        static const eod r_eod;           // needed to return eod reference, without taking risk of saturating memory
	static const U_I CAT_CRC_SIZE;
    };



	/// @}

} // end of namespace

#endif
