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
// $Id: test_memory.hpp,v 1.7 2004/11/07 18:21:39 edrusb Rel $
//
/*********************************************************************/

    /// \file test_memory.hpp
    /// \brief wrapper around new and delete operator to detect memory leakage
    ///
    /// activating this feature is only for debugging purpose, it makes
    /// the resulting executable very very very very slow.


#ifndef TEST_MEMORY_HPP
#define TEST_MEMORY_HPP

#include "../my_config.h"

extern "C"
{
#if STDC_HEADERS
#include <stdlib.h>
#endif
} // end extern "C"

#include "integers.hpp"

#ifdef TEST_MEMORY

#define MEM_BEGIN record_offset()
#define MEM_IN U_32 local_total_alloc_size = get_total_alloc_size()
#define MEM_OUT memory_check(local_total_alloc_size, __FILE__, __LINE__)
#define MEM_END all_delete_done()

namespace libdar
{
    extern void record_offset();
    extern libdar::U_32 get_total_alloc_size();
    extern void all_delete_done();
    extern void memory_check(libdar::U_32 ref, const char *fichier, libdar::S_I ligne);
}

#else

#define MEM_BEGIN // does nothing
#define MEM_IN    // does nothing
#define MEM_OUT   // does nothing
#define MEM_END   // does nothing

#endif

#endif
