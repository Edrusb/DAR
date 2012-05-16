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
// $Id: test_special_alloc.cpp,v 1.2.4.1 2004/01/28 15:29:47 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

#include "special_alloc.hpp"
#include "user_interaction.hpp"
#include "test_memory.hpp"
#include "shell_interaction.hpp"

#include <iostream>

using namespace libdar;

int main()
{
#ifdef LIBDAR_SPECIAL_ALLOC
    const int max = 3;
    void *ptr[max];
#endif

    shell_interaction_init(&cout, &cerr);

    MEM_BEGIN;
    MEM_IN;

#ifdef LIBDAR_SPECIAL_ALLOC

    for(int i = 0; i < max; i++)
        ptr[i] = special_alloc_new(1000);
    for(int i = 0; i < max; i++)
        special_alloc_delete(ptr[i]);

    MEM_OUT;

    for(int i = 0; i < max; i++)
        ptr[i] = special_alloc_new(1000);
    for(int i = 0; i < max - 1; i++)
        special_alloc_delete(ptr[i]);
    for(int i = 0; i < max - 1; i++)
        ptr[i] = special_alloc_new(10*i);
    special_alloc_delete(ptr[max - 1]);
    for(int i = 0; i < max - 1; i++)
        special_alloc_delete(ptr[i]);

#endif

    MEM_OUT;
    MEM_END;

    shell_interaction_close();
}
