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
// $Id: test_memory.hpp,v 1.5 2002/03/18 11:00:54 denis Rel $
//
/*********************************************************************/

#ifndef TEST_MEMORY_HPP
#define TEST_MEMORY_HPP

#include <stdlib.h>

#ifdef TEST_MEMORY

#define MEM_IN unsigned long local_total_alloc_size = get_total_alloc_size()
#define MEM_OUT memory_check(local_total_alloc_size, __FILE__, __LINE__)

#define MEM_END all_delete_done()

extern unsigned long get_total_alloc_size();
extern void all_delete_done();
extern void memory_check(unsigned long ref, const char *fichier, int ligne);

#else

#define MEM_IN  // does nothing 
#define MEM_OUT // does nothing
#define MEM_END // does nothing

#endif

#endif
