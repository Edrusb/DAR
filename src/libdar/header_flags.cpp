/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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

#include "../my_config.h"

extern "C"
{
} // end of extern "C"


#include "header_flags.hpp"
#include "erreurs.hpp"

using namespace std;

namespace libdar
{

    void header_flags::set_bits(U_I bitfield)
    {
	if(has_an_lsb_set(bitfield))
	    throw SRC_BUG;
	bits |= bitfield;
    }

    void header_flags::unset_bits(U_I bitfield)
    {
	if(has_an_lsb_set(bitfield))
	    throw SRC_BUG;
	bits &= ~bitfield;
    }

    bool header_flags::is_set(U_I bitfield) const
    {
	if(has_an_lsb_set(bitfield))
	    throw SRC_BUG;
	return (bits & bitfield) == bitfield;
    }

    void header_flags::read(generic_file & f)
    {
	bits = 0;
	unsigned char a;

	do
	{
	    if(f.read((char *)&a, 1) != 1)
		throw Erange("header_glags::read", gettext("Reached End of File while reading flag field"));
	    if(((bits << 8) >> 8) == bits)
	    {
		bits <<= 8;
		bits |= a & 0xFE;
	    }
	    else
		throw Erange("header_flags::read", gettext("tool large flag field for this implementation, either data corruption occured or you need to upgrade your software"));
	}
	while((a & 0x01) > 0);
    }

    void header_flags::dump(generic_file & f) const
    {
	U_I bit_size = 8;
	U_I bitfield = bits;
	unsigned char tmp;

	    // placing as needed 0x01 bits to keep trace of the width of the bitfield

	while((bitfield >> bit_size) > 0)
	{
	    bitfield |= (0x01 << bit_size);
	    bit_size += 8;
	}

	    // writing down the most significant bits first

	while(bit_size > 0)
	{
	    bit_size -= 8;
	    tmp = ((bitfield >> bit_size) & 0xFF);

	    f.write((char *)(& tmp), 1);
	}
    }

    bool header_flags::has_an_lsb_set(U_I bitfield)
    {
	while(bitfield > 0)
	    if((bitfield & 0x01) > 0)
		return true;
	    else
		bitfield >>= 8;

	return false;
    }

} // end of namespace
