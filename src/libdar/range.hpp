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

    /// \file range.hpp
    /// \brief class than provide a way to manipulate and represent range of integer numbers (infinint)
    /// \ingroup Private


#ifndef RANGE_HPP
#define RANGE_HPP

#include <string>
#include <set>
#include <list>

#include "../my_config.h"
#include "infinint.hpp"
#include "deci.hpp"

namespace libdar
{

    class range
    {
    public:
	range() { clear(); };
	range(const infinint & low, const infinint & high) { parts.push_back(segment(low, high)); };
	range(const range & ref) = default;
	range(range && ref) noexcept = default;
	range & operator = (const range & ref) = default;
	range & operator = (range && ref) noexcept = default;
	~range() = default;

	void operator += (const range & ref);
	range operator + (const range & ref) const { range ret = *this; ret += ref; return ret; };
	std::string display() const;

	    /// provides a way to read range contents segment by segment
	    ///
	    /// \note reset_read() is to be called once then read_next_segment()
	    /// will return true for each new segment giving in argument its low and high value
	    /// when no more segment are available it returns false, reset_read() can be call at
	    /// any time to reset the reading operation
	void reset_read() const { read_cursor = parts.begin(); };

	    /// read the next available segment
	    ///
	    /// \param[out] low when read_next_segment() returns true, contains the low value of the next segment
	    /// \param[out] high when read_next_segment() returns true, contains the high value of the next segment
	    /// \return true and set the low and high value when a next segment is available in the range, returns
	    /// false if all segment have been read low and high are not modified in that case.
	bool read_next_segment(infinint & low, infinint & high) const;

	void clear() { parts.clear(); };

    private:
	class segment
	{
	public:
	    segment(const infinint & x_low, const infinint & x_high) { low = x_low; high = x_high; };

	    const infinint & get_low() const { return low; };
	    const infinint & get_high() const { return high; };

	    bool overlaps_with(const segment & ref) const { return !(ref < *this) && !(ref > *this); };
	    void merge_with(const segment & ref); // only possible with a segment that overlaps with the current object

		// if two segment make < or > true they cannot be replaced by a single segment
	    bool operator < (const segment & ref) const { return high + 1 < ref.low; };
	    bool operator > (const segment & ref) const { return ref < *this; };
	    bool operator == (const segment & ref) const { return ref.high == high && ref.low == low; };
	    bool operator != (const segment & ref) const { return ! (*this == ref); };

		// if two segment make <= or >= true they can be replaced by a single (larger) segment
	    bool operator <= (const segment & ref) const { return ref.low < low && low <= ref.high + 1 && ref.high < high; };
	    bool operator >= (const segment &ref) const { return ref <= *this; };
	    bool contains(const segment & ref) const { return low <= ref.low && ref.high <= high; };

	    std::string display() const;

	private:
	    infinint low, high;
	};

	std::list<segment> parts;
	mutable std::list<segment>::const_iterator read_cursor;

    };


} // end of namespace

#endif
