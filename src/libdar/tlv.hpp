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
// $Id: tlv.hpp,v 1.2.2.2 2012/02/19 17:25:09 edrusb Exp $
//
/*********************************************************************/

    /// \file tlv.hpp
    /// \brief Generic Type Length Value data structures
    /// \ingroup Private

#ifndef TLV_HPP
#define TLV_HPP

#include "memory_file.hpp"
#include "storage.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// Type Length Value Structure

	/// this structure holds arbitrary type of data
	/// this is used in particular for the slice header
    class tlv
    {
    public:

	    // constructors & Co.

	tlv() { type = 0; value = NULL; };
	tlv(generic_file & f) { init(f); };
	tlv(const tlv & ref) { copy_from(ref); };
	~tlv() { detruit(); };

	const tlv & operator = (const tlv & ref) { detruit(); copy_from(ref); return *this; };

	    // methods (read / write tlv datastructure to file)

	void read(generic_file & f); //< same as the constructor but on an existing object
	void write(generic_file & f) const; //< dumps the tlv contents to file

	    // methods to fill the tlv object

	U_16 get_type() const { return type; };      //< get the TLV type
	void set_type(U_16 val) { type = val; };     //< set the TLV type
	void set_contents(const memory_file & contents);  //< the generic_file object is provided to dump data to the tlv object
        void get_contents(memory_file & contents) const;  //< the generic_file object is provided to read data from the tlv object

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(tlv);
#endif
    private:
	U_16 type;
	storage *value;

	void init(generic_file & f);
	void copy_from(const tlv & ref);
	void detruit() { if(value != NULL) { delete value; value = NULL; } };
    };

	/// @}

} // end of namespace

#endif


