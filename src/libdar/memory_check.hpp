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

#ifndef MEMORY_CHECK_HPP
#define MEMORY_CHECK_HPP

#include "../my_config.h"

// this module redefines the two global operators new and delete
// no need to include this header anywhere as these operatore will
// be used in any case as replacement of the default ones

    /// adds dump of currently allocated blocks to the debug_memory_output
extern void memory_check_snapshot();

    /// log special_alloc_operations
extern void memory_check_special_report_new(const void *ptr, unsigned long taille);
extern void memory_check_special_report_delete(const void *ptr);
extern void memory_check_special_new_sized(unsigned long taille);

#else

#define memory_check_snapshot(null)
#define memory_check_special_report_new(ptr, taille)
#define memory_check_special_report_delete(ptr)
#define memory_check_special_new_sized(taille)

#endif
