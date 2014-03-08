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

    /// \file datetime.hpp
    /// \brief this file contains the definition of class datetime that stores unix times in a portable way
    /// \ingroup Private

#ifndef DATETIME_HPP
#define DATETIME_HPP

#include "../my_config.h"
#include "on_pool.hpp"
#include "infinint.hpp"
#include "archive_version.hpp"

namespace libdar
{
    class archive; // needed to be able to use pointer on archive object.

	/// \addtogroup Private
	/// @{

    class datetime : public on_pool
    {
    public:
	    // time units must be sorted: the first is the smallest step, last is the largest increment.
	    // this makes the comparison operators (<, >, <=, >=,...) become naturally defined on that type
	enum time_unit { tu_microsecond, tu_second };

	datetime(const infinint & value = 0, time_unit unit = tu_second) { val = value; uni = unit; };
	datetime(generic_file &x, archive_version ver) { read(x, ver); };

	    // comparison operators

	bool operator < (const datetime & ref) const;
	bool operator == (const datetime & ref) const;
	bool operator != (const datetime & ref) const { return ! (*this == ref); };
	bool operator >= (const datetime & ref) const { return ! (*this < ref); };
	bool operator > (const datetime & ref) const { return ref < *this; };
	bool operator <= (const datetime & ref) const { return ref >= *this; };

	    // arithmetic on time
	datetime operator - (const datetime & ref) const;
	datetime operator + (const datetime & ref) const;

	    /// tells wether the time can be fully specified in the given time unit
	    ///
	    /// \param[in] target time unit to get the value in
	    /// \param[out] newval value of the datetime in the target unit
	    /// \return true if the time is an integer number of the target time unit,
	    /// newval is then the exact time value expressed in that unit. Else, false
	    /// is returned and newval is rounded down time value to the integer
	    /// value just below, in the target unit
	bool is_integer_value_of(time_unit target, infinint & newval) const;

	time_unit get_unit() const { return uni; };
	infinint get_raw_value() const { return val; };

	infinint get_value(time_unit unit = tu_second) const;

	    /// return a time in second as in the time_t argument
	    ///
	    /// \param[out] val the time value in second
	    /// \return false if the value cannot cast in a time_t variable
	bool get_value(time_t & val) const;

	void dump(generic_file &x) const;
	void read(generic_file &f, archive_version ver);

	bool is_null() const { return val == 0; };

	infinint get_storage_size() const { return val.get_storage_size() + 1; };

    private:
	infinint val;
	time_unit uni;

	    /// reduce the value to the largest unit possible
	void reduce_to_largest_unit() const;


	static time_unit min(time_unit a, time_unit b);
	static time_unit max(time_unit a, time_unit b);
	static const char time_unit_to_char(time_unit a);
	static time_unit char_to_time_unit(const char a);

	    /// return the factor between two units
	    ///
	    /// \note "from" must be larger than "to" (from >= to), else an exception is thrown
	    /// \return the factor f, which makes the following to be true: from = f*to
	static infinint get_scaling_factor(time_unit source, time_unit dest);
    };

	/// converts dar_manager database version to dar archive version in order to properly read time fields
    extern archive_version db2archive_version(unsigned char db_version);


	/// @}

} // end of namespace

#endif
