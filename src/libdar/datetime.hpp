/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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

    /// \file datetime.hpp
    /// \brief this file contains the definition of class datetime that stores unix times in a portable way
    /// \ingroup API

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
#include "infinint.hpp"

namespace libdar
{
	/// \addtogroup API
	/// @{

	/// no need to dig into these classes from the API
    class archive_version;
    class generic_file;

	/// stores time information
    class datetime
    {
    public:
	    // time units must be sorted: the first is the smallest step, last is the largest increment.
	    // this makes the comparison operators (<, >, <=, >=,...) become naturally defined on that type
	enum time_unit { tu_nanosecond, tu_microsecond, tu_second };

	    /// constructor based on the number of second ellasped since the end of 1969
	datetime(const infinint & value = 0) { val = value; uni = tu_second; };

	    /// general constructor

	    /// \param[in] second the number of second since the dawn of computer time (1970)
	    /// \param[in] subsec the fraction of the time below 1 second expressed in the time unit given as next argument
	    /// \param[in] unit the time unit in which is expressed the previous argument
	datetime(time_t second, time_t subsec, time_unit unit);

	    /// constructor reading data dump() into a generic_file
	datetime(generic_file &x, archive_version ver);

	datetime(const datetime & ref) = default;
	datetime(datetime && ref) noexcept = default;
	datetime & operator = (const datetime & ref) = default;
	datetime & operator = (datetime && ref) noexcept = default;
	~datetime() = default;


	    // comparison operators

	bool operator < (const datetime & ref) const;
	bool operator == (const datetime & ref) const;
	bool operator != (const datetime & ref) const { return ! (*this == ref); };
	bool operator >= (const datetime & ref) const { return ! (*this < ref); };
	bool operator > (const datetime & ref) const { return ref < *this; };
	bool operator <= (const datetime & ref) const { return ref >= *this; };

	    // arithmetic on time
	void operator -= (const datetime & ref);
	void operator += (const datetime & ref);
	datetime operator - (const datetime & ref) const { datetime tmp(*this); tmp -= ref; return tmp; };
	datetime operator + (const datetime & ref) const { datetime tmp(*this); tmp += ref; return tmp; };

	    /// equivalent to operator == but if compared object use different time unit, do the comparison rounding up the values to the largest unit
	bool loose_equal(const datetime & ref) const;

	    /// at the difference of operator - provides the difference using the less precise unit used between the two elements
	datetime loose_diff(const datetime & ref) const;

	    /// return the integer number of second
	infinint get_second_value() const { infinint sec, sub; get_value(sec, sub, uni); return sec; };

	    /// return the subsecond time fraction expressed in the given time unit
	infinint get_subsecond_value(time_unit unit) const;

	    /// returns the time unit used internally to store the subsecond time fraction
	time_unit get_unit() const { return uni; };

	    /// return a time as time_t arguments

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
	bool is_null() const { return val.is_zero(); };

	    /// return true if the datetime is an integer number of second (subsecond part is zero)
	bool is_integer_second() const { return (uni == tu_second); };

	    /// return the storage it would require to dump this object
	infinint get_storage_size() const;

	    /// set to null (zero)
	void nullify() { val = 0; uni = tu_second ; };

	    /// return the SI symbol of the time unit (s, ms, us (should be mu greek letter), ns)
	static std::string unit_symbol(time_unit tu);

    private:
	    // the date must not be stored as a single integer
	    // to avoid reducing the possible addressable dates
	    // when compiling using  32 or 64 bits integer in place
	    // of infinint. The fraction cannot handle smaller unit
	    // than nanosecond if using 32 bits integer.

	infinint val;  //< the date expressed in the "uni" time unit
	time_unit uni; //< the time unit used to store the subsecond fraction of the timestamp.

	    /// reduce the value to the largest unit possible
	void reduce_to_largest_unit() const;
	void get_value(infinint & sec, infinint & sub, time_unit unit) const;
	void build(const infinint & sec, const infinint & sub, time_unit unit);

	static time_unit min(time_unit a, time_unit b);
	static time_unit max(time_unit a, time_unit b);
	static char time_unit_to_char(time_unit a);
	static time_unit char_to_time_unit(const char a);

	    /// return the factor between two units

	    /// \note "source" must be larger than "dest" (source >= dest), else an exception is thrown
	    /// \return the factor f, which makes the following to be true: source = f*dest
	static const infinint & get_scaling_factor(time_unit source, time_unit dest);

    };

	/// converts dar_manager database version to dar archive version in order to properly read time fields
    extern archive_version db2archive_version(unsigned char db_version);


	/// @}

} // end of namespace

#endif
