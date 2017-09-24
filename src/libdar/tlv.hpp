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

    /// \file tlv.hpp
    /// \brief Generic Type Length Value data structures
    /// \ingroup Private

#ifndef TLV_HPP
#define TLV_HPP

#include "memory_file.hpp"
#include "storage.hpp"
#include "on_pool.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// Type Length Value Structure
	///
	/// this structure holds arbitrary type of data
	/// this is used in particular for the slice header
	/// \note a tlv is a memory_file, that way it holds the
	/// *V*alue and *L*ength of the data. Only the *T*ype field needs
	/// to be added to the memory_file datastructure
    class tlv : public memory_file
    {
    public:

	    // constructors & Co.

	tlv() { type = 0; };
	tlv(generic_file & f) { init(f); };
	tlv(const tlv & ref) = default;
	tlv & operator = (const tlv & ref) = default;
	~tlv() = default;

	    // methods (dump / setup tlv datastructure to/from file)

	void setup(generic_file & f); //< same as the constructor but on an existing object
	void dump(generic_file & f) const; //< dumps the tlv contents to file

	U_16 get_type() const { return type; };      //< get the TLV type
	void set_type(U_16 val) { type = val; };     //< set the TLV type

    private:
	U_16 type;

	void init(generic_file & f);
    };

	/// @}

} // end of namespace

#endif


