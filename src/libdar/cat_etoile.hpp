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

    /// \file cat_etoile.hpp
    /// \brief class holding an cat_inode object that get pointed by multiple mirage objects (smart pointers) to record hard links in a catalogue
    /// \ingroup Private

#ifndef CAT_ETOILE_HPP
#define CAT_ETOILE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include <list>
#include "cat_inode.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{


	/// the hard link implementation (etoile means star in French, seen a star as a point from which are thrown many ray of light)
    class cat_etoile
    {
    public:

	    /// build an object

	    ///\param[in] host is an inode, it must not be a directory (this would throw an Erange exception)
	    ///\param[in] etiquette_number is the identifier of this multiply linked structure
	    ///\note the given cat_inode is now managed by the cat_etoile object
	cat_etoile(cat_inode *host, const infinint & etiquette_number);
	cat_etoile(const cat_etoile & ref) = delete; // copy constructor not allowed for this class
	cat_etoile & operator = (const cat_etoile & ref) = delete; // assignment not allowed for this class
	~cat_etoile() { delete hosted; };

	void add_ref(void *ref);
	void drop_ref(void *ref);
	infinint get_ref_count() const { return refs.size(); };
	cat_inode *get_inode() const { return hosted; };
	infinint get_etiquette() const { return etiquette; };
	void change_etiquette(const infinint & new_val) { etiquette = new_val; };
	void disable_reduction_to_normal_inode() { tags.reduceable = 0; };
	bool cannot_reduce_to_normal_inode() const { return tags.reduceable == 0; };

	bool is_counted() const { return tags.counted; };
	bool is_wrote() const { return tags.wrote; };
	bool is_dumped() const { return tags.dumped; };
	void set_counted(bool val) { tags.counted = val ? 1 : 0; };
	void set_wrote(bool val) { tags.wrote = val ? 1 : 0; };
	void set_dumped(bool val) { tags.dumped = val ? 1 : 0; };

	    // return the address of the first mirage that triggered the creation of this mirage
	    // if this object is destroyed afterward this call returns nullptr
	const void *get_first_ref() const { if(refs.size() == 0) throw SRC_BUG; return refs.front(); };


    private:
	struct bool_tags
	{
	    unsigned counted : 1;    //< whether the inode has been counted
	    unsigned wrote : 1;      //< whether the inode has its data copied to archive
	    unsigned dumped : 1;     //< whether the inode information has been dumped in the catalogue
	    unsigned reduceable : 1; //< whether the inode can be reduce to normal inode
	    unsigned : 4;            //< padding to get byte boundary and reserved for future use.

	    bool_tags() { counted = wrote = dumped = 0; reduceable = 1; };
	};

	std::list<void *> refs; //< list of pointers to the mirages objects, in the order of their creation
	cat_inode *hosted;
	infinint etiquette;
	bool_tags tags;
    };


	/// @}

} // end of namespace

#endif
