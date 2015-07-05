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

    static const infinint one_thousand = 1000;
    static const infinint one_million = one_thousand*one_thousand;
    static const infinint one_billion = one_million*one_thousand;

    bool datetime::operator < (const datetime & ref) const
    {
	if(sec < ref.sec)
	    return true;
	if(sec > ref.sec)
	    return false;

	    // sec == ref.sec
	if(uni == ref.uni)
	    return frac < ref.frac;
	else
	{
		// using the unit having the less precision to perform the comparison
	    time_unit c = max(uni, ref.uni);
	    return get_subsecond_value(c) < ref.get_subsecond_value(c);
	}
    }

    bool datetime::operator == (const datetime & ref) const
    {
	if(sec != ref.sec)
	    return false;
	else
	{
		// using the unit having the less precision to perform the comparison
	    time_unit c = max(uni, ref.uni);
	    return get_subsecond_value(c) == ref.get_subsecond_value(c);
	}
    }

    datetime datetime::operator - (const datetime & ref) const
    {
	datetime val = *this;

	if(*this < ref)
	    throw SRC_BUG; // negative date would result of the operation

	val.sec -= ref.sec;

	    // using the most precised unit to avoid loosing accuracy
	val.uni = min(uni, ref.uni);
	infinint me_frac = get_subsecond_value(val.uni);
	infinint ref_frac = ref.get_subsecond_value(val.uni);

	if(me_frac >= ref_frac)
	    val.frac = me_frac - ref_frac;
	else
	{
	    --val.sec; // removing 1 second
	    val.frac = me_frac + how_much_to_make_1_second(val.uni);
	    val.frac -= ref_frac;
	}

	return val;
    }

    datetime datetime::operator + (const datetime & ref) const
    {
	datetime val = *this;

	val.sec += ref.sec;

	    // using the most precised unit to avoid loosing accuracy
	val.uni = min(uni, ref.uni);
	infinint me_frac = get_subsecond_value(val.uni);
	infinint ref_frac = ref.get_subsecond_value(val.uni);

	val.frac = me_frac + ref_frac;
	if(val.frac >= how_much_to_make_1_second(val.uni))
	{
	    ++val.sec;
	    val.frac -= how_much_to_make_1_second(val.uni);
	}

	return val;
    }

    datetime datetime::loose_diff(const datetime & ref) const
    {
	datetime val = *this;
	time_unit max_capa = tu_second;

	if(*this < ref)
	    throw SRC_BUG; // negative date would result of the operation

	val.sec -= ref.sec;

	    // using the less precised unit to avoid loosing accuracy
	val.uni = max(uni, ref.uni);
#if LIBDAR_MICROSECOND_READ_ACCURACY && LIBDAR_MICROSECOND_WRITE_ACCURACY
	max_capa = tu_microsecond;
#endif
	if(val.uni < max_capa)
	    val.uni = max_capa;
	infinint me_frac = get_subsecond_value(val.uni);
	infinint ref_frac = ref.get_subsecond_value(val.uni);

	if(me_frac >= ref_frac)
	    val.frac = me_frac - ref_frac;
	else
	{
	    --val.sec; // removing 1 second
	    val.frac = me_frac + how_much_to_make_1_second(val.uni);
	    val.frac -= ref_frac;
	}

	return val;
    }

    bool datetime::is_subsecond_an_integer_value_of(time_unit target, infinint & newval) const
    {
	if(target <= uni)
	{
	    newval = frac * get_scaling_factor(uni, target);
	    return true;
	}
	else
	{
	    infinint f = get_scaling_factor(target, uni);
		// target = f*uni
	    infinint  r;
	    euclide(frac, f, newval, r);
		// frac = f*newval + r
	    return r.is_zero();
	}
    }

    void datetime::reduce_to_largest_unit() const
    {
	infinint newval;
	datetime *me = const_cast<datetime *>(this);

	if(me == nullptr)
	    throw SRC_BUG;

	switch(uni)
	{
	case tu_nanosecond:
	    if(!is_subsecond_an_integer_value_of(tu_microsecond, newval))
		break; // cannot reduce the unit further
	    else
	    {
		me->frac = newval;
		me->uni = tu_microsecond;
	    }
		/* no break ! */
	case tu_microsecond:
	    if(!is_subsecond_an_integer_value_of(tu_second, newval))
		break; // cannot reduce the unit further
	    else
	    {
		me->frac = newval;
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

    infinint datetime::get_subsecond_value(time_unit unit) const
    {
	infinint ret;
	(void) is_subsecond_an_integer_value_of(unit, ret);
	return ret;
    }

    bool datetime::get_value(time_t & second, time_t & subsecond, time_unit unit) const
    {
	infinint tmp;

	second = 0;
	tmp = sec;
	tmp.unstack(second);
	if(!tmp.is_zero())
	    return false;

	if(unit < tu_second && uni < tu_second && !frac.is_zero())
	{
	    (void) is_subsecond_an_integer_value_of(unit, tmp);
	    subsecond = 0;
	    tmp.unstack(subsecond);
	    return tmp.is_zero();
	}
	else
	{
	    subsecond = 0;
	    return true;
	}
    }

    void datetime::dump(generic_file &x) const
    {
	char tmp;

	reduce_to_largest_unit();
	tmp = time_unit_to_char(uni);
	x.write(&tmp, 1);
	sec.dump(x);
	if(uni < tu_second)
	    frac.dump(x);
    }

    void datetime::read(generic_file &f, archive_version ver)
    {
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
	    frac.read(f);
	else
	    frac = 0;
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
	    throw SRC_BUG;
	}
    }

    infinint datetime::get_scaling_factor(time_unit source, time_unit dest)
    {
	infinint factor = 1;

	if(dest > source)
	    throw SRC_BUG;

	switch(source)
	{
	case tu_second:
	    if(dest == tu_second)
		break;
	    else
		factor *= one_million; // to reach microsecond
		/* no break ! */
	case tu_microsecond:
	    if(dest == tu_microsecond)
		break;
	    factor *= one_thousand; // to reach nanosecond
		/* no break ! */
	case tu_nanosecond:
	    if(dest == tu_nanosecond)
		break;
	    throw SRC_BUG; // unknown dest unit!
	default:
	    throw SRC_BUG;
	}

	return factor;
    }

    infinint datetime::how_much_to_make_1_second(time_unit unit)
    {
	switch(unit)
	{
	case tu_nanosecond:
	    return one_billion;
	case tu_microsecond:
	    return one_million;
	case tu_second:
	    return 1;
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
