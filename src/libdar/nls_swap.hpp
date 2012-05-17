//*********************************************************************/
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

    /// \file nls_swap.hpp
    /// \brief provides a set of macro to change the NLS from user application domaine to libdar domain and viceversa
    /// \ingroup Private

#ifndef NLS_SWAP_HPP
#define NLS_SWAP_HPP

#include <string>

#include "../my_config.h"
#include "tools.hpp"

#ifdef ENABLE_NLS

    /// \addtogroup Private
    /// @{


#define NLS_SWAP_IN                             \
         std::string nls_swap_tmp;              \
         if(textdomain(NULL) != NULL)           \
         {                                      \
	     nls_swap_tmp = textdomain(NULL);   \
             textdomain(PACKAGE);               \
	 }                                      \
         else                                   \
             nls_swap_tmp = ""


#define NLS_SWAP_OUT                                             \
        if(nls_swap_tmp != "")                                   \
            textdomain(nls_swap_tmp.c_str())

#else

#define NLS_SWAP_IN //
#define NLS_SWAP_OUT //

#endif

    /// @}



#endif

