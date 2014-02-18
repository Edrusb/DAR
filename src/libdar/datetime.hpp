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
	enum time_unit { tu_seconde };

	datetime(const infinint & value = 0, time_unit unit = tu_seconde) { val = value; uni = unit; };
	datetime(generic_file &x, archive_version ver) { read(x, ver); };

	    // comparison operators

	bool operator < (const datetime & ref) const;
	bool operator == (const datetime & ref) const;
	bool operator != (const datetime & ref) const { return ! (*this == ref); };
	bool operator >= (const datetime & ref) const { return ! (*this < ref); };
	bool operator > (const datetime & ref) const { return ref < *this; };
	bool operator <= (const datetime & ref) const { ref >= *this; };

	    // arithmetic on time
	datetime operator - (const datetime & ref) const;
	datetime operator + (const datetime & ref) const;

	    /// tell wether the time can be fully specified in the given time unit
	    /// \param[in] unit time unit to check
	    /// \return true if the time is an integer number of the time unit
	bool is_integer_value_of(time_unit unit) const;

	    /// tell which largest unit can fully store the date
	time_unit reduce_to_largest_uni() const;

	infinint get_value(time_unit unit = tu_seconde) const;

	    /// return a time in second as in the time_t argument

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

	static time_unit min(time_unit a, time_unit b);
	static const char time_unit_to_char(time_unit a);
	static time_unit char_to_time_unit(const char a);
    };

	/// converts dar_manager database version to dar archive version in order to properly read time fields
    extern archive_version db2archive_version(unsigned char db_version);


	/// @}

} // end of namespace

#endif
