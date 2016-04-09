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

    /// \cat_storage.hpp
    /// \brief class to handle archive layers where catalogue object read their data from
    /// \ingroup Private

#ifndef CAT_STORAGE_HPP
#define CAT_STORAGE_HPP

#include "../my_config.h"

#include "on_pool.hpp"
#include "smart_pointer.hpp"
#include "pile_descriptor.hpp"

namespace libdar
{

    class cat_storage: public virtual on_pool
    {
    public:
	    /// constructor
	    ///
	    /// \param[in] pdesc points to an existing stack that will be read from to setup fields of inherited classes,
	    /// this pointed to pile object must survive the whole life of the cat_entree object
	    /// \param[in] small whether a small or a whole read is to be done, (inode has been dump() with small set to true)
	cat_storage(const smart_pointer<pile_descriptor> & pdesc, bool small);

	    /// constructor "null"
	cat_storage() {};

	    /// for archive merging, will let the object drop EA, FSA and Data to an alternate stack than the one it has been read from
	    ///
	    /// \note this is used when cloning an object from a catalogue to provide a merged archive. Such cloned object must point
	    /// the stack of the archive under construction, so we use this call for that need,
	    /// \note this is also used when opening a catalogue if an isolated catalogue in place of the internal catalogue of an archive
	    /// \note this method is virtual for cat_directory to overwrite it and propagate the change to all entries of the directory tree
	    /// as well for mirage to propagate the change to the hard linked inode
	void change_location(const smart_pointer<pile_descriptor> & x_pdesc);

    protected:
	    /// stack used to read object from (nullptr is returned for object created from filesystem)
	pile *get_pile() const { if(pdesc.is_null()) throw SRC_BUG; return pdesc->stack; };

	    /// compressor generic_file relative methods
	    ///
	    /// \note CAUTION: the pointer to object is member of the get_pile() stack and may be managed by another thread
	    /// all precaution like get_pile()->flush_read_above(get_compressor_layer() shall be take to avoid
	    /// concurrent access to the compressor object by the current thread and the thread managing this object
	compressor *get_compressor_layer() const { if(pdesc.is_null()) throw SRC_BUG; return pdesc->compr; };

	    /// escape generic_file relative methods
	    ///
	    /// \note CAUTION: the pointer to object is member of the get_pile() stack and may be managed by another thread
	    /// all precaution like get_pile()->flush_read_above(get_escape_layer() shall be take to avoid
	    /// concurrent access to the compressor object by the current thread and the thread managing this object
	escape *get_escape_layer() const { if(pdesc.is_null()) throw SRC_BUG; return pdesc->esc; };

	    /// return the adhoc layer in the stack to read from the catalogue objects (except the EA, FSA or Data part)
	generic_file *get_read_cat_layer(bool small) const;

    private:
	smart_pointer<pile_descriptor> pdesc;

    };

} // end of namespace

#endif
