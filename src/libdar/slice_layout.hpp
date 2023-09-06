/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

    /// \file slice_layout.hpp
    /// \brief object describing the slicing of an archive
    /// \ingroup Private


#ifndef SLICE_LAYOUT_HPP
#define SLICE_LAYOUT_HPP

#include <string>
#include <set>

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{

    class slice_layout
    {
    public:
	slice_layout() { older_sar_than_v8 = false; }; // fields of type "infinint" are objects initialized by the class default constructor
	slice_layout(const slice_layout & ref) = default;
	slice_layout(slice_layout && ref) noexcept = default;
	slice_layout & operator = (const slice_layout & ref) = default;
	slice_layout & operator = (slice_layout && ref) noexcept = default;
	~slice_layout() = default;

	    // field still exposed (slice_layout was a struct before being a class)
	    // we keep these fields as is for now

	infinint first_size;         ///< size of the first slice
	infinint other_size;         ///< maximum size of other slices
	infinint first_slice_header; ///< size of the slice header in the first slice
	infinint other_slice_header; ///< size of the slice header in the other slices
	bool older_sar_than_v8;      ///< true if the archive format is older than version 8

	void read(generic_file & f);
	void write(generic_file & f) const;
	void clear();

	    /// given a slice_layout and a archive offset, provides the corresponding slice and slice offset

	    /// \param[in] offset input offset as if all slices were sticked toghether
	    /// \param[out] slice_num slice number where to find the given offset
	    /// \param[out] slice_offset offset in that slice where is the given offset
	void which_slice(const infinint & offset,
			 infinint & slice_num,
			 infinint & slice_offset) const;

    private:
	static const char OLDER_THAN_V8 = '7';
	static const char V8 = '8';
    };

	/// @}

} // end of namespace

#endif
