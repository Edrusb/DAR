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

#include "../my_config.h"


#include "datetime.hpp"

using namespace std;

namespace libdar
{

    bool datetime::operator < (const datetime & ref) const
    {
	time_unit c = min(uni, ref.uni);

	return get_value(c) < ref.get_value(c);
    }


    bool datetime::operator == (const datetime & ref) const
    {
	time_unit c = min(uni, ref.uni);

	return get_value(c) == ref.get_value(c);
    }


    datetime datetime::operator - (const datetime & ref) const
    {
	datetime ret;
	ret.uni = min(uni, ref.uni);

	if(*this < ref)
	    throw SRC_BUG; // negative date would result of the operation
	ret.val = get_value(ret.uni) - ref.get_value(ret.uni);

	return ret;
    }

    datetime datetime::operator + (const datetime & ref) const
    {
	datetime ret;

	ret.uni = min(uni, ref.uni);
	ret.val = get_value(ret.uni) + ref.get_value(ret.uni);

	return ret;
    }

    bool datetime::is_integer_value_of(time_unit unit) const
    {
	if(unit == uni)
	    return true;
	else
	    throw Efeature("convertion from a time unit to another");
    }

    datetime::time_unit datetime::reduce_to_largest_uni() const
    {
	switch(uni)
	{
	case tu_seconde:
	    break;
	default:
	    throw SRC_BUG;
	}

	return uni;
    }

    infinint datetime::get_value(time_unit unit) const
    {
	if(unit == uni)
	    return val;
	else
	    throw Efeature("Time Unit Conversion");
    }

    bool datetime::get_value(time_t & val) const
    {
	infinint tmp = get_value(tu_seconde);

	val = 0;
	tmp.unstack(val);

	return tmp == 0;
    }

    void datetime::dump(generic_file &x) const
    {
	const char tmp = time_unit_to_char(uni);
	x.write(&tmp, 1);
	val.dump(x);
    }

    void datetime::read(generic_file &f, archive_version ver)
    {
	if(ver < 9)
	    uni = tu_seconde;
	else
	{
	    char tmp;
	    f.read(&tmp, 1);
	    uni = char_to_time_unit(tmp);
	}

	val.read(f);
    }


    datetime::time_unit datetime::min(time_unit a, time_unit b)
    {
	if(a < b)
	    return a;
	else
	    return b;
    }

    const char datetime::time_unit_to_char(time_unit a)
    {
	switch(a)
	{
	case tu_seconde:
	    return 's';
	default:
	    throw SRC_BUG;
	}
    }

    datetime::time_unit datetime::char_to_time_unit(const char a)
    {
	switch(a)
	{
	case 's':
	    return tu_seconde;
	default:
	    throw SRC_BUG;
	}
    }

    archive_version db2archive_version(unsigned char db_version)
    {
	    // the class datetime read() method is based on dar archive version.
	    // here we know the database version (dar_manager). Starting with version 4 (release 2.5.0)
	    // time is no more stored as an integer. This correspond to dar archive version 9
	    // and above (release 2.5.0 too), wherefrom this hack below:
	return db_version > 3 ? archive_version(9,0) : archive_version(8,0);
    }


} // end of namespace
