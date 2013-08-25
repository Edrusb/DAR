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

    /// \file archive_version.hpp
    /// \brief class archive_version that rules which archive format to follow
    /// \ingroup Private

#ifndef ARCHIVE_VERSION_HPP
#define ARCHIVE_VERSION_HPP

#include "../my_config.h"

#include <string>

#include "integers.hpp"
#include "infinint.hpp"
#include "generic_file.hpp"

namespace libdar
{

	/// \ingroup Private
	/// @}

	/// class archive_version manages the version of the archive format
    class archive_version
    {
    public:
	    /// archive_version constructor

	    /// \param[in] x is the version number
	    /// \param[in] fix is a decimal-like
	    /// \note the fix argument must only be used when the current stable version need to be increased
	    /// due to a bug fix. This let the concurrent development version to keep the same version (usually x+1)
	    /// while having the stable version using a slightly different format to fix a bug.
	archive_version(U_16 x = 0, unsigned char fix = 0);

	bool operator < (const archive_version & ref) const { return value() < ref.value(); };
	bool operator >= (const archive_version & ref) const { return value() >= ref.value(); };
	bool operator == (const archive_version & ref) const { return value() == ref.value(); };
	bool operator != (const archive_version & ref) const { return value() != ref.value(); };
	bool operator > (const archive_version & ref) const { return value() > ref.value(); };
	bool operator <= (const archive_version & ref) const { return value() <= ref.value(); };

	void dump(generic_file & f) const;
	void read(generic_file & f);
	std::string display() const;

    private:
	U_16 version;
	unsigned char fix;

	U_I value() const { return (U_I)(version)*256 + fix; };
	static unsigned char to_digit(unsigned char val);
	static unsigned char to_char(unsigned char val);
    };

    extern const archive_version empty_archive_version();

	/// @}

} // end of namespace

#endif
