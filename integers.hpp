/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: integers.hpp,v 1.1 2002/10/31 21:02:36 edrusb Rel $
//
/*********************************************************************/
//

#ifndef INTEGERS_HPP
#define INTEGERS_HPP

#if OS_BITS == 32
  #define U_16 unsigned short
  #define U_32 unsigned long
  #define U_I unsigned int 
  #define S_16 signed short
  #define S_32 signed long
  #define S_I signed int
#else
  #if OS_BITS == 64
    #define U_16 unsigned short
    #define U_32 unsigned int
    #define U_I  unsigned int
    #define S_16 signed short
    #define S_32 signed int
    #define S_I signed int
  #else
    #error // unknown OS_BITS value
    // #error does not exists and is here to force the compilation to fail
  #endif // OS_BITS == 64
#endif // OS_BITS == 32

#endif // ifndef
