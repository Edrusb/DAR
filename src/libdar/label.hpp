/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

    /// \file label.hpp
    /// \brief define the datastructure "label" used to identify slice membership to an archive
    /// \ingroup Private

#ifndef LABEL_HPP
#define LABEL_HPP

#include "../my_config.h"

#include "integers.hpp"
#include "generic_file.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// manage label data structure used in archive slice headers

    class label
    {
    public:
	label(); // builds a label equal to 'zero'
	label(const label & ref) { copy_from(ref); };
	label(label && ref) noexcept { move_from(std::move(ref)); };
	label & operator = (const label & ref) { copy_from(ref); return *this; };
	label & operator = (label && ref) noexcept { move_from(std::move(ref)); return *this; };
	~label() = default;

	bool operator == (const label & ref) const;
	bool operator != (const label & ref) const { return ! ((*this) == ref); };

	void clear();
	bool is_cleared() const;

	void generate_internal_filename();

	void read(generic_file & f);
	void dump(generic_file & f) const;

	void invert_first_byte() { val[0] = ~val[0]; };

	    // avoid using these two calls, only here for backward compatibility
	    // where the cost to move to object is really too heavy than
	    // sticking with a char array.
	U_I size() const { return LABEL_SIZE; };
	char *data() { return (char *)&val; };
	const char *data() const { return (char *)&val; };

	static U_I common_size() { return LABEL_SIZE; };

    private:
	static constexpr U_I LABEL_SIZE = 10;

	char val[LABEL_SIZE];

	void copy_from(const label & ref);
	void move_from(label && ref) noexcept;
    };


    extern const label label_zero;

	/// @}

} // end of namespace

#endif
