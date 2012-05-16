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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: integers.hpp,v 1.7.4.2 2004/01/28 15:29:46 edrusb Rel $
//
/*********************************************************************/
//

#ifndef INTEGERS_HPP
#define INTEGERS_HPP

#include "../my_config.h"

#ifndef OS_BITS

#if HAVE_INTTYPES_H
extern "C"
{
#include <inttypes.h>
} // end extern "C"

namespace libdar
{
    typedef uint16_t U_16;
    typedef uint32_t U_32;
    typedef uint64_t U_64;
    typedef unsigned int U_I;
    typedef int16_t S_16;
    typedef int32_t S_32;
    typedef int64_t S_64;
    typedef signed int S_I;
}

#else // HAVE_INTTYPES_H
#error "Cannot determine interger types, use --enable-os-bits=... with the 'configure' script according to your system's CPU register size"
#endif // HAVE_INTTYPES_H

#else  //  OS_BITS not defined
#if OS_BITS == 32

namespace libdar
{

    typedef unsigned short U_16;
    typedef unsigned long U_32;
    typedef unsigned long long U_64;
    typedef unsigned int U_I;
    typedef signed short S_16;
    typedef signed long S_32;
    typedef signed long long S_64;
    typedef signed int S_I;

}

#else
#if OS_BITS == 64

namespace libdar
{
    typedef unsigned short U_16;
    typedef unsigned int U_32;
    typedef unsigned long long U_64;
    typedef unsigned int U_I;
    typedef signed short S_16;
    typedef signed int S_32;
    typedef signed long long S_64;
    typedef signed int S_I;

}

#else
#error "unknown value given to --enable-os-bits=... check the 'configure' script arguments"
    // unknown OS_BITS value ! use --enable-os-bits=... option to configure script
    //
    // the previous line should not compile, this is the expected behaviour

#endif // OS_BITS == 64
#endif // OS_BITS == 32
#endif // OS_BITS not defined

#endif // header file multiple inclusion protection
