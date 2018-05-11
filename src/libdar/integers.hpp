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
    typedef unsigned char U_8;
    typedef uint16_t U_16;
    typedef uint32_t U_32;
    typedef uint64_t U_64;
    typedef size_t U_I;
	// configure will define size_t as "unsigned int" if it not defined by system headers
	// thus using U_I we are sure we can compare buffer sizes with SSIZE_MAX
    typedef signed char S_8;
    typedef int16_t S_16;
    typedef int32_t S_32;
    typedef int64_t S_64;
    typedef signed int S_I;

}

#else // HAVE_INTTYPES_H
#error "Cannot determine interger types, use --enable-os-bits=... with the 'configure' script according to your system's CPU register size"
#endif // HAVE_INTTYPES_H

#else  //  OS_BITS is defined
#if OS_BITS == 32

namespace libdar
{
    typedef unsigned char U_8;
    typedef unsigned short U_16;
    typedef unsigned long U_32;
    typedef unsigned long long U_64;
    typedef size_t U_I;
    typedef signed char S_8;
    typedef signed short S_16;
    typedef signed long S_32;
    typedef signed long long S_64;
    typedef signed int S_I;

}

#else // OS_BITS != 32
#if OS_BITS == 64

namespace libdar
{
    typedef unsigned char U_8;
    typedef unsigned short U_16;
    typedef unsigned int U_32;
    typedef unsigned long long U_64;
    typedef size_t U_I;
    typedef signed char S_8;
    typedef signed short S_16;
    typedef signed int S_32;
    typedef signed long long S_64;
    typedef signed int S_I;

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
