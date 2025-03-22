/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
#include "archive_version.hpp"
#include "proto_compressor.hpp"
#include "pile_descriptor.hpp"
#include "smart_pointer.hpp"
#include "entree_stats.hpp"
#include "list_entry.hpp"
#include "slice_layout.hpp"

#include <memory>

namespace libdar
{
    class cat_etoile;

	/// \addtogroup Private
	/// @{


	/// the root class from all other inherite for any entry in the catalogue

    class cat_entree
    {
    public :
	    /// read and create an object of inherited class of class cat_entree

	    /// \param[in] dialog for user interaction
	    /// \param[in] f where from to read data in order to create the object
	    /// \param[in] reading_ver archive version format to use for reading
	    /// \param[in,out] stats updated statistical fields
	    /// \param[in,out] corres used to setup hard links
	    /// \param[in] default_algo default compression algorithm
	    /// \param[in] lax whether to use relax mode
	    /// \param[in] only_detruit whether to only consider detruit objects (in addition to the directory tree)
	    /// \param[in] small whether the dump() to read has been done with the small argument set
        static cat_entree *read(const std::shared_ptr<user_interaction> & dialog,
				const smart_pointer<pile_descriptor> & f,
				const archive_version & reading_ver,
				entree_stats & stats,
				std::map <infinint, cat_etoile *> & corres,
				compression default_algo,
				bool lax,
				bool only_detruit,
				bool small);

	    /// setup an object when read from filesystem
	cat_entree(saved_status val): xsaved(val) {};

	    /// setup an object when read from an archive

	    /// \param[in] pdesc points to an existing stack that will be read from to setup fields of inherited classes,
	    /// this pointed to pile object must survive the whole life of the cat_entree object
	    /// \param[in] small whether a small or a whole read is to be read, (inode has been dump() with small set to true)
	    /// \param[in] val saved_status to assign the the new object
	cat_entree(const smart_pointer<pile_descriptor> & pdesc, bool small, saved_status val);

	    // copy constructor is fine as we only copy the address of pointers
	cat_entree(const cat_entree & ref) = default;

	    // move constructor
	cat_entree(cat_entree && ref) noexcept = default;

	    // assignment operator is fine too for the same reason
	cat_entree & operator = (const cat_entree & ref) = default;

	    // move assignment operator
	cat_entree & operator = (cat_entree && ref) = default;

	    /// destructor
        virtual ~cat_entree() noexcept(false) {};

	    /// returns true if the two object are the same
	virtual bool operator == (const cat_entree & ref) const = 0;
	bool operator != (const cat_entree & ref) const { return ! (*this == ref); };

	    /// return true of the two objects would generate the same entry on filsystem

	    /// for a and b two cat_entree, if a == b, then a.same_as(b) is true also.
	    /// But the opposit may be wrong if for example "a" is a hardlink pointing to an inode
	    /// while b is a normal inode, restoring both could lead to having the same entry in
	    /// filsystem while a and b are different for libdar: here they are objects of two
	    /// different classes.
	bool same_as(const cat_entree & ref) const { return true; };

	    /// write down the object information to a stack

	    /// \param[in,out] pdesc is the stack where to write the data to
	    /// \param[in] small defines whether to do a small or normal dump
        void dump(const pile_descriptor & pdesc, bool small) const;

	    /// this call gives an access to inherited_dump

	    /// \param[in,out] pdesc is the stack where to write the data to
	    /// \param[in] small defines whether to do a small or normal dump
	void specific_dump(const pile_descriptor & pdesc, bool small) const { inherited_dump(pdesc, small); };

	    /// let inherited classes build object's data after CRC has been read from file in small read mode

	    /// \param[in] pdesc stack to read the data from
	    /// \note used from cat_entree::read to complete small read
	    /// \note this method is called by cat_entree::read and mirage::post_constructor only when contructing an object with small set to true
	virtual void post_constructor(const pile_descriptor & pdesc) {};

	    /// inherited class signature
        virtual unsigned char signature() const = 0;

	    /// inherited class designation
	virtual std::string get_description() const = 0;

	    /// a way to copy the exact type of an object even if pointed to by a parent class pointer
        virtual cat_entree *clone() const = 0;

	    /// for archive merging, will let the object drop EA, FSA and Data
	    /// to an alternate stack than the one it has been read from

	    /// \note this is used when cloning an object from a catalogue to provide a
	    /// merged archive. Such cloned object must point
	    /// the stack of the archive under construction, so we use this call for that need,
	    /// \note this is also used when opening a catalogue if an isolated
	    /// catalogue in place of the internal catalogue of an archive
	    /// \note this method is virtual in order for cat_directory to
	    /// overwrite it and propagate the change to all entries of the directory tree
	    /// as well for mirage to propagate the change to the hard linked inode
	virtual void change_location(const smart_pointer<pile_descriptor> & pdesc);

	    /// obtain the saved status of the object
	saved_status get_saved_status() const { return xsaved; };

	    /// modify the saved_status of the object
        void set_saved_status(saved_status x) { xsaved = x; };

	    /// setup a list_entry object relative to the current cat_entree object

	    /// \param[in] sly the slice layout that shall be used for file location in slices
	    /// \param[in] fetch_ea whether to fetch EA and fill these values into the generated list_entry
	    /// \param[out] ent the list_entry that will be setup by the call
	    /// \note if sly is set to nullptr, no slice location is performed, this
	    /// brings some speed improvement when this information is not required
	void set_list_entry(const slice_layout *sly,
			    bool fetch_ea,
			    list_entry & ent) const;

    protected:
	    /// inherited class may overload this method but shall first call the parent's inherited_dump() in the overloaded method
	virtual void inherited_dump(const pile_descriptor & pdesc, bool small) const;


	    /// stack used to read object from (nullptr is returned for object created from filesystem)
	pile *get_pile() const { return pdesc.is_null() ? nullptr : pdesc->stack; };

	    /// compressor generic_file relative methods

	    /// \note CAUTION: the pointer to object is member of the get_pile() stack and may be managed by another thread
	    /// all precaution like get_pile()->flush_read_above(get_compressor_layer() shall be take to avoid
	    /// concurrent access to the proto_compressor object by the current thread and the thread managing this object
	proto_compressor *get_compressor_layer() const { return pdesc.is_null() ? nullptr : pdesc->compr; };

	    /// escape generic_file relative methods

	    /// \note CAUTION: the pointer to object is member of the get_pile() stack and may be managed by another thread
	    /// all precaution like get_pile()->flush_read_above(get_escape_layer() shall be take to avoid
	    /// concurrent access to the compressor object by the current thread and the thread managing this object
	escape *get_escape_layer() const { return pdesc.is_null() ? nullptr : pdesc->esc; };

	    /// return the adhoc layer in the stack to read from the catalogue objects (except the EA, FSA or Data part)
	generic_file *get_read_cat_layer(bool small) const;

    private:
	static const U_I ENTREE_CRC_SIZE;

	saved_status xsaved;     ///< inode data status, this field is stored with signature() in the cat_signature field -> must be managed by cat_entree
	smart_pointer<pile_descriptor> pdesc;
    };

	/// convert a signature char to a human readable string
    extern const char *cat_entree_signature2string(unsigned char sign);

	/// @}

} // end of namespace

#endif
