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

    /// \file zapette_protocol.hpp
    /// \brief protocol management between archive and libdar_slave classes
    /// \ingroup Private

#include "../my_config.h"

#ifndef ZAPETTE_PROTOCOL_HPP
#define ZAPETTE_PROTOCOL_HPP

extern "C"
{
} // end extern "C"

#include <string>
#include <new>
#include "integers.hpp"
#include "infinint.hpp"
#include "generic_file.hpp"

namespace libdar
{

	/// \addtogroup Private
        /// @{

    constexpr unsigned char ANSWER_TYPE_DATA = 'D';
    constexpr unsigned char ANSWER_TYPE_INFININT = 'I';

    constexpr U_I REQUEST_SIZE_SPECIAL_ORDER = 0;
    constexpr U_I REQUEST_OFFSET_END_TRANSMIT = 0;
    constexpr U_I REQUEST_OFFSET_GET_FILESIZE = 1;
    constexpr U_I REQUEST_OFFSET_CHANGE_CONTEXT_STATUS = 2;
    constexpr U_I REQUEST_IS_OLD_START_END_ARCHIVE = 3;
    constexpr U_I REQUEST_GET_DATA_NAME = 4;
    constexpr U_I REQUEST_FIRST_SLICE_HEADER_SIZE = 5;
    constexpr U_I REQUEST_OTHER_SLICE_HEADER_SIZE = 6;

    struct request
    {
        char serial_num;
        U_16 size; // size or REQUEST_SIZE_SPECIAL_ORDER
        infinint offset; // offset or REQUEST_OFFSET_END_TRANSMIT or REQUEST_OFFSET_GET_FILESIZE, REQUEST_OFFSET_* ...
	std::string info; // new contextual_status

        void write(generic_file *f); // master side
        void read(generic_file *f);  // slave side
    };

    struct answer
    {
        char serial_num;
        char type;
        U_16 size;
        infinint arg;

        void write(generic_file *f, char *data); // slave side
        void read(generic_file *f, char *data, U_16 max);  // master side
    };

	/// @}

} // end of namespace


#endif
