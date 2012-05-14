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
// $Id: infinint.hpp,v 1.8 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/
//

#ifndef INFININT_HPP
#define INFININT_HPP

#ifndef MODE
#include "real_infinint.hpp"
#else
#if MODE == 32
#include "integers.hpp"
#include "limitint.hpp"
namespace libdar
{
    typedef limitint<U_32> infinint;
}
# else
#if MODE == 64
#include "integers.hpp"
#include "limitint.hpp"
namespace libdar
{
    typedef limitint<U_64> infinint;
}
#else
#include "real_infinint.hpp"
#endif
#endif
#endif
     
#endif
