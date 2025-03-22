//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
// to contact the author, see the AUTHOR file
/*********************************************************************/

    /// \file nls_swap.hpp
    /// \brief provides a set of macro to change the NLS from user application domaine to libdar domain and viceversa
    /// \ingroup Private

#ifndef NLS_SWAP_HPP
#define NLS_SWAP_HPP

#include "../my_config.h"

#include <string>

#ifdef ENABLE_NLS

    /// \addtogroup Private
    /// @{


#define NLS_SWAP_IN                             \
    std::string nls_swap_tmp;			\
    if(textdomain(nullptr) != nullptr)		\
    {						\
	nls_swap_tmp = textdomain(nullptr);	\
	textdomain(PACKAGE);			\
    }						\
    else					\
	nls_swap_tmp = ""


#define NLS_SWAP_OUT				\
    if(nls_swap_tmp != "")			\
	textdomain(nls_swap_tmp.c_str())

    /// @}

#else

#define NLS_SWAP_IN //
#define NLS_SWAP_OUT //

#endif



#endif
