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

    static const infinint one_unit = 1;
    static const infinint one_thousand = 1000;
    static const infinint one_million = one_thousand*one_thousand;
    static const infinint one_billion = one_million*one_thousand;

    datetime::datetime(time_t second, time_t subsec, time_unit unit)
    {
	build(infinint(second), infinint(subsec), unit);
    }

    bool datetime::operator < (const datetime & ref) const
    {
	if(uni <= ref.uni && val < ref.val)
	    return true;

	if(uni < ref.uni)
	{
	    infinint newval, reste;
	    euclide(val, get_scaling_factor(ref.uni, uni), newval, reste);
	    return newval < ref.val;
	}

	if(uni == ref.uni)
	    return val < ref.val;
	else
	{
		// uni > ref.uni
	    infinint newval, reste;
	    euclide(ref.val, get_scaling_factor(uni, ref.uni), newval, reste);
	    return (val == newval && !reste.is_zero())
		|| val < newval;
	}
    }

    bool datetime::operator == (const datetime & ref) const
    {
	return uni == ref.uni && val == ref.val;
	    // fields are always reduced to the larger possible unit
    }

    void datetime::operator -= (const datetime & ref)
    {
	if(ref.uni < uni)
	{
	    val *= get_scaling_factor(uni, ref.uni);
	    uni = ref.uni;
	}

	if(ref.uni == uni)
	{
	    if(val < ref.val)
		throw SRC_BUG;
	    val -= ref.val;
	}
	else // ref.uni > uni
	{
	    infinint tmp = ref.val * get_scaling_factor(ref.uni, uni);
	    if(tmp > val)
		throw SRC_BUG;
	    val -= tmp;
	}

	reduce_to_largest_unit();
    }

    void datetime::operator += (const datetime & ref)
    {
	if(ref.uni < uni)
	{
	    val *= get_scaling_factor(uni, ref.uni);
	    uni = ref.uni;
	}

	if(ref.uni == uni)
	    val += ref.val;
	else // ref.uni > uni
	{
	    infinint tmp = ref.val * get_scaling_factor(ref.uni, uni);
	    val += tmp;
	}

	reduce_to_largest_unit();
    }

    bool datetime::loose_equal(const datetime & ref) const
    {
	if(uni == ref.uni)
	    return val == ref.val;
	else
	{
	    time_unit tu = max(uni, ref.uni);
	    infinint val1, val2;

	    if(uni < tu)
		val1 = val / get_scaling_factor(tu, uni);
	    else
		val1 = val;

	    if(ref.uni < tu)
		val2 = ref.val / get_scaling_factor(tu, ref.uni);
	    else
		val2 = ref.val;

	    return val1 == val2;
	}
    }

    datetime datetime::loose_diff(const datetime & ref) const
    {
#if LIBDAR_MICROSECOND_READ_ACCURACY && LIBDAR_MICROSECOND_WRITE_ACCURACY
	static const time_unit max_capa = tu_microsecond;
#else
	static const time_unit max_capa = tu_second;
#endif
	datetime ret;
	infinint aux;

	    // using the less precised unit to avoid loosing accuracy
	ret.uni = max(uni, ref.uni);

	if(ret.uni < max_capa)
	    ret.uni = max_capa;

	if(uni < ret.uni)
	    ret.val = val / get_scaling_factor(ret.uni, uni);
	else
	    ret.val = val;

	if(ref.uni < ret.uni)
	    aux = ref.val / get_scaling_factor(ret.uni, ref.uni);
	else
	    aux = ref.val;

	if(ret.val < aux)
	    throw SRC_BUG; // negative date would result of the operation
	ret.val -= aux;
	ret.reduce_to_largest_unit();

	return ret;
    }

    infinint datetime::get_storage_size() const
    {
	infinint sec, sub, size;
	get_value(sec, sub, uni);

	size = sec.get_storage_size();
	if(uni < tu_second)
	    size += sub.get_storage_size() + 1;

	return size;
    }

    void datetime::reduce_to_largest_unit() const
    {
	infinint newval, reste;
	datetime *me = const_cast<datetime *>(this);

	if(me == nullptr)
	    throw SRC_BUG;

	if(val.is_zero())
	{
	    if(uni != tu_second)
		me->uni = tu_second;
	}
	else
	{
	    switch(uni)
	    {
	    case tu_nanosecond:
		euclide(val, get_scaling_factor(tu_microsecond, uni), newval, reste);
		if(!reste.is_zero())
		    break; // cannot reduce the unit further
		else
		{
		    me->val = newval;
		    me->uni = tu_microsecond;
		}
		    /* no break ! */
	    case tu_microsecond:
		euclide(val, get_scaling_factor(tu_second, uni), newval, reste);
		if(!reste.is_zero())
		    break; // cannot reduce the unit further
		else
		{
		    me->val = newval;
		    me->uni = tu_second;
		}
		    /* no break ! */
	    case tu_second:
		    // cannot reduce further as
		    // this is the largest known time unit
		    // so we break here
		break;
	    default:
		throw SRC_BUG;
	    }
	}
    }

    void datetime::get_value(infinint & sec, infinint & sub, time_unit unit) const
    {
	euclide(val, get_scaling_factor(tu_second, uni), sec, sub);
	if(unit < uni)
	    sub *= get_scaling_factor(uni, unit);
	if(unit > uni)
	    sub /= get_scaling_factor(unit, uni);
    }

    void datetime::build(const infinint & sec, const infinint & sub, time_unit unit)
    {
	bool loop = false;
	infinint subsec = sub;

	do
	{
	    try
	    {
		if(tu_second == unit)
		    val = sec;
		    // this saves an infinint multiplication and fixes
		    // a bug when reading an archive generated by dar 2.4.x or
		    // below that included a file for which the system returned
		    // a negative date, which was read by libdar as a huge positive
		    // number, that much that multiplying it by 1 triggers the
		    // limiting overflow mecanism
		else
		    val = sec*get_scaling_factor(tu_second, unit) + subsec;
		uni = unit;
		loop = false;
	    }
	    catch(Elimitint& e)
	    {
		switch(unit)
		{
		case tu_nanosecond:
		    unit = tu_microsecond;
		    subsec = subsec/1000;
		    break;
		case tu_microsecond:
		    unit = tu_second;
		    subsec = subsec/1000;
		    break;
		case tu_second:
		    throw;
		default:
		    throw SRC_BUG;
		}
		loop = true;
	    }
	}
	while(loop);
	reduce_to_largest_unit();
    }


    infinint datetime::get_subsecond_value(time_unit unit) const
    {
	infinint ret, tmp;

	get_value(tmp, ret, unit);

	return ret;
    }

    bool datetime::get_value(time_t & second, time_t & subsecond, time_unit unit) const
    {
	infinint sub, sec;

	get_value(sec, sub, unit);

	second = 0;
	sec.unstack(second);
	if(!sec.is_zero())
	    return false;

	subsecond = 0;
	sub.unstack(subsecond);
	if(!sub.is_zero())
	    return false;

	return true;
    }

    void datetime::dump(generic_file &x) const
    {
	char tmp;
	infinint sec, sub;

	get_value(sec, sub, uni);
	tmp = time_unit_to_char(uni);

	    // we keep storing:
	    // - a first flag telling the unit
	    // - an infinint telling the amount of seconds
	    // - an other infinint telling the amount of subsecond additional time expressed in the unit of the flag
	x.write(&tmp, 1);
	sec.dump(x);
	if(uni < tu_second)
	    sub.dump(x);
    }

    void datetime::read(generic_file &f, archive_version ver)
    {
	infinint sec, sub;

	if(ver < 9)
	    uni = tu_second;
	else
	{
	    char tmp;
	    f.read(&tmp, 1);
	    uni = char_to_time_unit(tmp);
	}

	sec.read(f);
	if(uni < tu_second)
	    sub.read(f);
	else
	    sub = 0;
	build(sec, sub, uni);
    }

    datetime::time_unit datetime::min(time_unit a, time_unit b)
    {
	if(a < b)
	    return a;
	else
	    return b;
    }

    datetime::time_unit datetime::max(time_unit a, time_unit b)
    {
	if(a < b)
	    return b;
	else
	    return a;
    }

    const char datetime::time_unit_to_char(time_unit a)
    {
	switch(a)
	{
	case tu_nanosecond:
	    return 'n';
	case tu_microsecond:
	    return 'u';
	case tu_second:
	    return 's';
	default:
	    throw SRC_BUG;
	}
    }

    datetime::time_unit datetime::char_to_time_unit(const char a)
    {
	switch(a)
	{
	case 'n':
	    return tu_nanosecond;
	case 's':
	    return tu_second;
	case 'u':
	    return tu_microsecond;
	default:
	    throw Erange("datetime::time_unit", gettext("Unknown time unit"));
	}
    }

    const infinint & datetime::get_scaling_factor(time_unit source, time_unit dest)
    {
	if(dest > source)
	    throw SRC_BUG;

	switch(source)
	{
	case tu_second:
	    if(dest == tu_second)
		return one_unit;
	    else if(dest == tu_microsecond)
		return one_million;
	    else if(dest == tu_nanosecond)
		return one_billion;
	    else
		throw SRC_BUG; // unknown dest unit!
	case tu_microsecond:
	    if(dest == tu_microsecond)
		return one_unit;
	    else if(dest == tu_nanosecond)
		return one_thousand;
	    else
		throw SRC_BUG; // unknown dest unit!
	case tu_nanosecond:
	    if(dest == tu_nanosecond)
		return one_unit;
	    else
		throw SRC_BUG; // unknown dest unit!
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
