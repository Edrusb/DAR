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

    /// \file cat_entree.hpp
    /// \brief base class for all object contained in a catalogue
    /// \ingroup Private

#ifndef CAT_ENTREE_HPP
#define CAT_ENTREE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "infinint.hpp"
#include "user_interaction.hpp"
#include "pile.hpp"
#include "escape.hpp"
#include "on_pool.hpp"
#include "archive_version.hpp"
#include "compressor.hpp"
#include "pile_descriptor.hpp"

namespace libdar
{
    class cat_etoile;
    class cat_entree;

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
        void add(const cat_entree *ref);
        void listing(user_interaction & dialog) const;
    };

	/// the root class from all other inherite for any entry in the catalogue
    class cat_entree : public on_pool
    {
    public :
	    /// read and create an object of inherited class of class cat_entree
	    ///
	    /// \param[in] dialog for user interaction
	    /// \param[in] pool for memory allocation (nullptr if special_alloc not activated)
	    /// \param[in] f where from to read data in order to create the object
	    /// \param[in] reading_ver archive version format to use for reading
	    /// \param[in,out] stats updated statistical fields
	    /// \param[in,out] corres used to setup hard links
	    /// \param[in] default_algo default compression algorithm
	    /// \param[in] lax whether to use relax mode
	    /// \param[in] only_detruit whether to only consider detruit objects (in addition to the directory tree)
	    /// \param[in] small whether the dump() to read has been done with the small argument set
        static cat_entree *read(user_interaction & dialog,
				memory_pool *pool,
				const pile_descriptor & f,
				const archive_version & reading_ver,
				entree_stats & stats,
				std::map <infinint, cat_etoile *> & corres,
				compression default_algo,
				bool lax,
				bool only_detruit,
				bool small);

	    /// setup an object when read from an archive
	    ///
	    /// \param[in] pdesc points to an existing stack that will be read from to setup fields of inherited classes,
	    /// this pointed to pile object must survive the whole life of the cat_entree object
	    /// \param[in] small whether a small or a whole read is to be read, (inode has been dump() with small set to true)
	cat_entree(const pile_descriptor & pdesc, bool small) { change_location(pdesc, small); };

	    // copy constructor is fine as we only copy the address of pointers

	    // assignment operator is fine too for the same reason

	    /// setup an object when read from filesystem
	cat_entree() {};

	    /// destructor
        virtual ~cat_entree() throw(Ebug) {};

	    /// returns true if the two object have the same content
	virtual bool operator == (const cat_entree & ref) const { return true; };
	bool operator != (const cat_entree & ref) const { return ! (*this == ref); };

	    /// write down the object information to a stack
	    ///
	    /// \param[in,out] pdesc is the stack where to write the data to
	    /// \param[in] small defines whether to do a small or normal dump
        void dump(const pile_descriptor & pdesc, bool small) const;

	    /// this call gives an access to inherited_dump
	    ///
	    /// \param[in,out] pdesc is the stack where to write the data to
	    /// \param[in] small defines whether to do a small or normal dump
	void specific_dump(const pile_descriptor & pdesc, bool small) const { inherited_dump(pdesc, small); };

	    /// let inherited classes build object's data after CRC has been read from file in small read mode
	    ///
	    /// \param[in] pdesc stack to read the data from
	    /// \note used from cat_entree::read to complete small read
	    /// \note this method is called by cat_entree::read and mirage::post_constructor only when contructing an object with small set to true
	virtual void post_constructor(const pile_descriptor & pdesc) {};

	    /// inherited class signature
        virtual unsigned char signature() const = 0;

	    /// a way to copy the exact type of an object even if pointed to by a parent class pointer
        virtual cat_entree *clone() const = 0;

	    /// for archive merging, will let the object drop EA, FSA and Data to an alternate stack than the one it has been read from
	    ///
	    /// \note this is used when cloning an object from a catalogue to provide a merged archive. Such cloned object must point
	    /// the stack of the archive under construction, so we use this call for that need,
	    /// \note this is also used when opening a catalogue if an isolated catalogue in place of the internal catalogue of an archive
	    /// \note this method is virtual for cat_directory to overwrite it and propagate the change to all entries of the directory tree
	    /// as well for mirage to propagate the change to the hard linked inode
	virtual void change_location(const pile_descriptor & pdesc, bool small);


    protected:
	    /// inherited class may overload this method but shall first call the parent's inherited_dump() in the overloaded method
	virtual void inherited_dump(const pile_descriptor & pdesc, bool small) const;


	    /// stack used to read object from (nullptr is returned for object created from filesystem)
	pile *get_pile() const { return pdesc.stack; };

	    /// compressor generic_file relative methods
	    ///
	    /// \note CAUTION: the pointer to object is member of the get_pile() stack and may be managed by another thread
	    /// all precaution like get_pile()->flush_read_above(get_compressor_layer() shall be take to avoid
	    /// concurrent access to the compressor object by the current thread and the thread managing this object
	compressor *get_compressor_layer() const { return pdesc.compr; };

	    /// escape generic_file relative methods
	    ///
	    /// \note CAUTION: the pointer to object is member of the get_pile() stack and may be managed by another thread
	    /// all precaution like get_pile()->flush_read_above(get_escape_layer() shall be take to avoid
	    /// concurrent access to the compressor object by the current thread and the thread managing this object
	escape *get_escape_layer() const { return pdesc.esc; };

	    /// return the adhoc layer in the stack to read from the catalogue objects (except the EA, FSA or Data part)
	generic_file *get_read_cat_layer(bool small) const;

    private:
	static const U_I ENTREE_CRC_SIZE;

	pile_descriptor pdesc;
    };

    extern bool compatible_signature(unsigned char a, unsigned char b);
    extern unsigned char mk_signature(unsigned char base, saved_status state);
    extern unsigned char get_base_signature(unsigned char a);

	/// @}

} // end of namespace

#endif
