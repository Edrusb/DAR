/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

    /// \file infinint.hpp
    /// \brief switch module to limitint (32 ou 64 bits integers) or infinint
    /// \ingroup API


#ifndef INFININT_HPP
#define INFININT_HPP

#ifndef LIBDAR_MODE
#include "real_infinint.hpp"
#else
#if LIBDAR_MODE == 32
#define INFININT_BASE_TYPE U_32
#include "integers.hpp"
#include "limitint.hpp"
# else
#if LIBDAR_MODE == 64
#define INFININT_BASE_TYPE U_64
#include "integers.hpp"
#include "limitint.hpp"
#else
#include "real_infinint.hpp"
#endif
#endif
#endif

#endif
