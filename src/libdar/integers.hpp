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

    /// \file integers.hpp
    /// \brief are defined here basic integer types that tend to be portable
    /// \ingroup API


#ifndef INTEGERS_HPP
#define INTEGERS_HPP

#include "../my_config.h"
#include <string>

    /// \addtogroup API
    /// @{

#ifndef OS_BITS

#if HAVE_INTTYPES_H
extern "C"
{
#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#if HAVE_LIMITS_H
#include <limits.h>
#endif
} // end extern "C"

namespace libdar
{
    using U_8 = unsigned char;
    using U_16 = uint16_t;
    using U_32 = uint32_t;
    using U_64 = uint64_t;
    using U_I = size_t;
	// configure will define size_t as "unsigned int" if it not defined by system headers
	// thus using U_I we are sure we can compare buffer sizes with SSIZE_MAX
    using S_8 =  signed char;
    using S_16 = int16_t;
    using S_32 = int32_t;
    using S_64 = int64_t;
    using S_I = signed int;

}

#else // HAVE_INTTYPES_H
#error "Cannot determine interger types, use --enable-os-bits=... with the 'configure' script according to your system's CPU register size"
#endif // HAVE_INTTYPES_H

#else  //  OS_BITS is defined
#if OS_BITS == 32

namespace libdar
{
    using U_8 = unsigned char;
    using U_16 = unsigned short;
    using U_32 = unsigned long;
    using U_64 = unsigned long long;
    using U_I = size_t;
    using S_8 = signed char;
    using S_16 = signed short;
    using S_32 = signed long;
    using S_64 = signed long long;
    using S_I = signed int;

}

#else // OS_BITS != 32
#if OS_BITS == 64

namespace libdar
{
    using U_8 = unsigned char;
    using U_16 = unsigned short;
    using U_32 = unsigned int;
    using U_64 = unsigned long long;
    using U_I = size_t;
    using S_8 = signed char;
    using S_16 = signed short;
    using S_32 = signed int;
    using S_64 = signed long long;
    using S_I = signed int;

}

#else // OS_BITS != 32 and OS_BITS != 64
#error "unknown value given to --enable-os-bits=... check the 'configure' script arguments"
    // unknown OS_BITS value ! use --enable-os-bits=... option to configure script
    //
    // the previous line should not compile, this is the expected behaviour

#endif // OS_BITS == 64
#endif // OS_BITS == 32
#endif // OS_BITS not defined

namespace libdar
{


        /// checks sign and width of integer types

        /// \note this call may throws an Ehardware exception
    void integer_check();


        /// returns true if the system is big endian, false else

        /// \note this call may throw an Ehardware() exception if the
        /// system is not coherent for all integer types
    bool integers_system_is_big_endian();

}

    /// @}


#endif // header file multiple inclusion protection
