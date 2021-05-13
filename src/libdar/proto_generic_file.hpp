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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file proto_generic_file.hpp
    /// \brief precursor class of generic_file used to avoid cyclic dependencies with storage and infinint
    /// \ingroup Private
    /// \note API included module due to dependencies

#ifndef PROTO_GENERIC_FILE_HPP
#define PROTO_GENERIC_FILE_HPP


#include "../my_config.h"

extern "C"
{
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"

#include "integers.hpp"
#include "erreurs.hpp"

#include <string>

namespace libdar
{

	/// \addtogroup Private
        /// @{

	/// ancestor class of generic_file

	/// this class exist to avoid cyclic dependency between generic_file and infinint
    class proto_generic_file
    {
    public :
	proto_generic_file() {};

	    /// copy constructor
	proto_generic_file(const proto_generic_file &ref) = default;

	    /// move constructor
	proto_generic_file(proto_generic_file && ref) noexcept = default;

	    /// assignment operator
	proto_generic_file & operator = (const proto_generic_file & ref) = default;

	    /// move operator
	proto_generic_file & operator = (proto_generic_file && ref) noexcept = default;

	    /// virtual destructor

	    /// \note this let inherited destructor to be called even from a proto_generic_file pointer to an inherited class
        virtual ~proto_generic_file() noexcept(false) {};


	    /// read data from the proto_generic_file

	    /// \param[in, out] a is where to put the data to read
	    /// \param[in] size is how much data to read
	    /// \return the exact number of byte read.
	    /// \note read as much as requested data, unless EOF is met (only EOF can lead to reading less than requested data)
	    /// \note EOF is met if read() returns less than size
        virtual U_I read(char *a, U_I size) = 0;

	    /// write data to the proto_generic_file

	    /// \note throws a exception if not all data could be written as expected
        virtual void write(const char *a, U_I size) = 0;
    };

	/// @}

} // end of namespace

#endif
