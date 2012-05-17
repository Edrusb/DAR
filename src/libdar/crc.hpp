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
// $Id: crc.hpp,v 1.8 2011/02/11 20:23:42 edrusb Rel $
//
/*********************************************************************/

    /// \file crc.hpp
    /// \brief class crc definition, used to handle Cyclic Redundancy Checks
    /// \ingroup Private

#ifndef CRC_HPP
#define CRC_HPP

#include "../my_config.h"

#include <string>
#include <list>

#include "integers.hpp"
#include "storage.hpp"
#include "infinint.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class crc
    {
    public:
	static const U_I OLD_CRC_SIZE = 2;

	crc(U_I width = 1);
	crc(const infinint & width);
	crc(const crc & ref) : i_cyclic(1) { copy_from(ref); };
	const crc & operator = (const crc & ref) { destroy(); copy_from(ref); return *this; };
	~crc() { destroy(); };

	bool operator == (const crc & ref) const;
	bool operator != (const crc & ref) const { return ! (*this == ref); };

	void compute(const infinint & offset, const char *buffer, U_I length);
	void compute(const char *buffer, U_I length); // for sequential read only
	void clear();
	void dump(generic_file & f) const;
	void read(generic_file & f);
	void old_read(generic_file & f);
	std::string crc2str() const;
	void resize(const infinint & width);
	void resize(U_I width) { resize_non_infinint(width); };
	void resize_based_on(const crc & ref) { resize(ref.get_size()); };
	infinint get_size() const { return infinint_mode ? i_size : n_size; };

	static void set_crc_pointer(crc * & dst, const crc *src);

    private:

	bool infinint_mode;                           //< true for infinint mode
                                                      // ---
	infinint i_size;                              //< size of the checksum (infinint mode)
	storage::iterator i_pointer;                  //< points to the next byte to modify (infinint mode)
	storage i_cyclic;                             //< the checksum storage (infinint mode)
	                                              // ---
	U_I n_size;                                   //< size of checksum (non infinint mode)
	unsigned char *n_pointer;                     //< points to the next byte to modify (non infinint mode)
	unsigned char *n_cyclic;                      //< the checksum storage (non infinint mode)

	void copy_from(const crc & ref);
	void destroy();
	void resize_infinint(const infinint & width);
	void resize_non_infinint(U_I width);
    };


	/// @}

} // end of namespace


#endif
