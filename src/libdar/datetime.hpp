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

extern "C"
{
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UTIME_H
#include <utime.h>
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

} // end extern "C"

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
	enum time_unit { tu_nanosecond, tu_microsecond, tu_second };

	    /// constructor based on the number of second ellasped since the end of 1969
	datetime(const infinint & value = 0) { sec = value; frac = 0; uni = tu_second; };

	    /// general constructor
	    ///
	    /// \param[in] sec the number of second since the dawn of computer time (1970)
	    /// \param[in] subsec the fraction of the time below 1 second expressed in the time unit given as next argument
	    /// \param[in] the time unit in which is expressed the previous argument
	datetime(time_t second, time_t subsec, time_unit unit) { sec = second; frac = subsec; uni = unit; if(uni == tu_second && subsec != 0) throw SRC_BUG; };

	    /// constructor reading data dump() into a generic_file
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

	    /// at the difference of operator - provides the difference using the less precise unit used between the two elements
	datetime loose_diff(const datetime & ref) const;

	    /// return the integer number of second
	infinint get_second_value() const { return sec; };

	    /// return the subsecond time fraction expressed in the given time unit
	infinint get_subsecond_value(time_unit unit) const;

	    /// returns the time unit used internally to store the subsecond time fraction
	time_unit get_unit() const { return uni; };

	    /// return a time as time_t arguments
	    ///
	    /// \param[out] second the time value in second
	    /// \param[out] subsecond is the remaining time fraction as expressed in the unit given as next argument
	    /// \param[in] unit the unit of the subsecond fraction of the timestamp
	    /// \return true upon success, false if the value cannot be represented by system types (overflow)
	bool get_value(time_t & second, time_t & subsecond, time_unit unit) const;


	    /// write down this to file
	void dump(generic_file &x) const;

	    /// read this from file
	void read(generic_file &f, archive_version ver);

	    /// return true if the datetime is exactly January 1st, 1970, 0 h 0 mn 0 s
	bool is_null() const { return sec.is_zero() && frac.is_zero(); };

	    /// return true if the datetime is an integer number of second (subsecond part is zero)
	bool is_integer_second() const { return frac.is_zero(); };

	    /// return the storage it would require to dump this object
	infinint get_storage_size() const { return sec.get_storage_size() + frac.get_storage_size() + 1; };

    private:
	    // the date must not be stored as a single integer
	    // to avoid reducing the possible addressable dates
	    // when compiling using  32 or 64 bits integer in place
	    // of infinint. The fraction cannot handle smaller unit
	    // than nanosecond if using 32 bits integer.

	infinint sec;  //< the date as number of second ellapsed since 1969
	infinint frac; //< the fraction of the date expressed in the unit defined by the "uni" field
	time_unit uni; //< the time unit used to store the subsecond fraction of the timestamp.

	    /// reduce the value to the largest unit possible
	void reduce_to_largest_unit() const;

	    /// tells wether the time can be fully specified in the given time unit
	    ///
	    /// \param[in] target time unit to get the subsecond time fraction in
	    /// \param[out] newval value of the subsecond time fracion in the target unit
	    /// \return true if the time is an integer number of the target time unit,
	    /// newval is then the exact subsecond time fraction expressed in that unit. Else, false
	    /// is returned and newval is rounded down time value to the integer
	    /// value just below, what is the correct subsecond time fraction expressed in the target unit
	bool is_subsecond_an_integer_value_of(time_unit target, infinint & newval) const;


	static time_unit min(time_unit a, time_unit b);
	static time_unit max(time_unit a, time_unit b);
	static const char time_unit_to_char(time_unit a);
	static time_unit char_to_time_unit(const char a);

	    /// return the factor between two units
	    ///
	    /// \note "from" must be larger than "to" (from >= to), else an exception is thrown
	    /// \return the factor f, which makes the following to be true: from = f*to
	static const infinint & get_scaling_factor(time_unit source, time_unit dest);

	    /// return the max subsecond value that makes exactly one second for the given unit
	static infinint how_much_to_make_1_second(time_unit unit);
    };

	/// converts dar_manager database version to dar archive version in order to properly read time fields
    extern archive_version db2archive_version(unsigned char db_version);


	/// @}

} // end of namespace

#endif
