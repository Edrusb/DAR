//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

#include "../my_config.h"

#include "integers.hpp"
#include "erreurs.hpp"
#include "tools.hpp"

namespace libdar
{

	// template to check the width of a particular integer type

    template<class T> void integer_check_width(const char *type_name, T x, unsigned int expected)
    {
	if(sizeof(T) != expected)
	    throw Ehardware("interger_check_width", tools_printf(gettext("%s type length is not %d byte(s) but %d"), type_name, expected, sizeof(T)));
    }

    template<class T> void integer_check_sign(const char *type_name, T x, bool expected_signed)
    {
	x = 0;
	--x;
	if(x > 0 && expected_signed) //	"x > 0" to avoid compilation warning when T is unsigned
	    throw Ehardware("integer_check_sign", tools_printf(gettext("%s type is not a signed type as expected"), type_name));
	if(x < 1 && !expected_signed) // "x < 1" to avoid compilation warning when T is unsigned we compare against 1
	    throw Ehardware("integer_check_sign", tools_printf(gettext("%s type is not an unsigned type as expected"), type_name));
    }

    template<class T> bool is_unsigned_big_endian(const char *type_name, T x)
    {
	unsigned int size = sizeof(x);
	unsigned char *ptr = (unsigned char *)(& x);
	unsigned int i = 0;

	    // given type should be unsigned

	try
	{
	    integer_check_sign(type_name, x, false);
	}
	catch(Ehardware & e)
	{
	    throw SRC_BUG;
	}

	    // setting x to the following serie of bytes values: "1|2|3|..|size"

	x = 0;
	i = 1;
	while(i <= size)
	{
	    x *= 256;
	    x += i%256;
	    ++i;
	}

	    // looking for a litte endian sequence

	i = 0;
	while(i < size && ptr[i] == (size - i)%256)
	    ++i;

	if(i == size) // full litte endian sequence found
	    return false;

	if(i > size)
	    throw SRC_BUG;

	    // looking for a big endian sequence

	i = 0;
	while(i < size && ptr[i] == (i+1)%256)
	    ++i;

	if(i == size)  // full big endian sequence found
	    return true;
	else
	    if(i > size)
		throw SRC_BUG;
	    else // i < size, thus CPU/system uses neither a little nor a big endian sequence!!!
		throw Ehardware("is_unsigned_big_endian", tools_printf(gettext("type %s is neither big nor little endian! Do not know how to handle integer in a portable manner on this host, aborting"), type_name));
    }

    void integer_check()
    {
	U_8 u8 = 0;
	U_16 u16 = 0;
	U_32 u32 = 0;
	U_64 u64 = 0;
	U_I ui = 0;
	S_8 s8 = 0;
	S_16 s16 = 0;
	S_32 s32 = 0;
	S_64 s64 = 0;
	S_I si = 0;

	    // checking integer type width

	integer_check_width("U_8", u8, 1);
	integer_check_width("U_16", u16, 2);
	integer_check_width("U_32", u32, 4);
	integer_check_width("U_64", u64, 8);
	integer_check_width("S_8", s8, 1);
	integer_check_width("S_16", s16, 2);
	integer_check_width("S_32", s32, 4);
	integer_check_width("S_64", s64, 8);

	    // checking signed types and unsigned types is as expected

	integer_check_sign("U_8", u8, false);
	integer_check_sign("U_16", u16, false);
	integer_check_sign("U_32", u32, false);
	integer_check_sign("U_64", u64, false);
	integer_check_sign("U_I", ui, false);
	integer_check_sign("S_8", s8, true);
	integer_check_sign("S_16", s16, true);
	integer_check_sign("S_32", s32, true);
	integer_check_sign("S_64", s64, true);
	integer_check_sign("S_I", si, true);
    }

    bool integers_system_is_big_endian()
    {
	U_16 u16 = 0;
	U_32 u32 = 0;
	U_64 u64 = 0;
	U_I ui = 0;
	bool ref;

	integer_check();
	ref = is_unsigned_big_endian("U_16", u16);

	if(ref != is_unsigned_big_endian("U_32", u32))
	    throw Ehardware("integers_system_is_big_endian", gettext("incoherent endian between U_16 and U_32"));
	if(ref != is_unsigned_big_endian("U_64", u64))
	    throw Ehardware("integers_system_is_big_endian", gettext("incoherent endian between U_16 and U_64"));
	if(ref != is_unsigned_big_endian("U_I", ui))
	    throw Ehardware("integers_system_is_big_endian", gettext("incoherent endian between U_16 and U_I"));

	return ref;
    }

}
