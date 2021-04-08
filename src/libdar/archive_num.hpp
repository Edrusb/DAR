/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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

    /// \file archive_num.hpp
    /// \brief class storing the position of an archive inside a database
    /// \ingroup API


#ifndef ARCHIVE_NUM_HPP
#define ARCHIVE_NUM_HPP

#include "../my_config.h"

#include "integers.hpp"
#include "erreurs.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

	// no need to dig into this class from the API
    class generic_file;

	/// class archive_num stores the position of an archive inside a dar_manager database

    class archive_num
    {
    public:
	archive_num(U_16 arg = 0) { set(arg); };
	archive_num(const archive_num & ref) = default;
	archive_num(archive_num && ref) noexcept = default;
	archive_num & operator = (const archive_num & ref) = default;
	archive_num & operator = (archive_num && ref) = default;
	~archive_num() = default;

	    /// this operator makes an object of that class convertible to an 16 bits integer

	    /// \note this is the only call you should need, just use an archive_num
	    /// implictely of explicitely as an integer
	operator U_16() const { return val; };
	U_16 operator = (U_16 arg) { set(arg); return arg; };
	archive_num & operator++() { set(val+1); return *this; };

	    // no need of order operator (<, <=, >, >=, ==, !=)
	    // thanks to the implicit conversion to U_16 defined
	    // above

	void read_from_file(generic_file &f);
	void write_to_file(generic_file &f) const;

    private:
	static constexpr U_I val_size = sizeof(U_16);
	static constexpr U_I MAX = 65534;

	U_16 val;

	inline void set(U_16 arg)
	{
	    if(arg >= MAX)
		throw SRC_BUG;
	    val = arg;
	}
    };

	/// @}

} // end of namespace

#endif
