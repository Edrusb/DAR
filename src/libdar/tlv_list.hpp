/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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

    /// \file tlv_list.hpp
    /// \brief List of Generic Type Length Value data structures
    /// \ingroup Private

#ifndef TLV_LIST_HPP
#define TLV_LIST_HPP

#include "tlv.hpp"
#include "generic_file.hpp"

#include <deque>

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class tlv_list
    {
    public:
	tlv_list() = default;                     ///< builds an empty list
	tlv_list(generic_file & f) { init(f); };  ///< builds a list from a file
	tlv_list(const tlv_list & ref) = default;
	tlv_list(tlv_list && ref) noexcept = default;
	tlv_list & operator = (const tlv_list & ref) = default;
	tlv_list & operator = (tlv_list && ref) noexcept = default;
	~tlv_list() = default;

	void dump(generic_file & f) const;        ///< dump tlv_list to file
	void read(generic_file & f) { init(f); }; ///< erase and read a list from a file
	U_I size() const { return contents.size(); };
	tlv & operator[] (U_I item) const;
	void clear() { contents.clear(); };
	void add(const tlv & next) { contents.push_back(next); };

    private:
	std::deque<tlv> contents;

	void init(generic_file & f);
    };

	/// @}

} // end of namespace


#endif
