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

    /// \file elastic.hpp
    /// \brief here is defined the elastic class
    /// \ingroup Private

#ifndef ELASTIC_HPP
#define ELASTIC_HPP

#include "../my_config.h"

#include "integers.hpp"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "archive_version.hpp"

namespace libdar
{

	/// \ingroup Private
	/// @}


    enum elastic_direction { elastic_forward, elastic_backward };

	/// the elastic buffer class

	/// the elastic class makes possible to insert arbritrary bytes beside information bytes, and to
	/// retreive later without any other knowledge which bytes are information and which byte are from the
	/// elastic buffer. The main purpose is for strong encryption
	/// \ingroup Private
    class elastic
    {
    public:
	elastic(U_32 size);
	elastic(const unsigned char *buffer, U_32 size, elastic_direction dir, const archive_version & reading_ver);
	elastic(generic_file &f, elastic_direction dir, const archive_version & reading_ver);
	elastic(const elastic & ref) = default;
	elastic(elastic && ref) noexcept = default;
	elastic & operator = (const elastic & ref) = default;
	elastic & operator = (elastic && ref) noexcept = default;
	~elastic() = default;

	U_32 dump(unsigned char *buffer, U_32 size) const;
	U_32 get_size() const { return taille; };

	static U_I max_length() { return (U_I)(254)*254*254*254 - 1; };

    private:
	U_32 taille; // max size of elastic buffer is 4GB which is large enough

	void randomize(unsigned char *a) const;
	U_I base_from_version(const archive_version & reading_ver) const;
	unsigned char get_low_mark(const archive_version & reading_ver) const;
	unsigned char get_high_mark(const archive_version & reading_ver) const;
	unsigned char get_low_mark() const { return 255; };
	unsigned char get_high_mark() const { return 254; };

    };

	/// @}

} // end of namespace

#endif
