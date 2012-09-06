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

#include "../my_config.h"

#include "libdar.hpp"
#include "special_alloc.hpp"
#include "user_interaction.hpp"

#include <iostream>

using namespace libdar;
using namespace std;

int main()
{
    U_I maj, med, min;

    get_version(maj, med, min);

#ifdef LIBDAR_SPECIAL_ALLOC

    const int block_size = 1024;
    const int max = 66;
    void *ptr[max];

    try
    {

	for(int i = 0; i < max; ++i)
	    ptr[i] = special_alloc_new(block_size);
	for(int i = 0; i < max; ++i)
	    special_alloc_delete(ptr[i]);

	for(int i = 0; i < max; ++i)
	    ptr[i] = special_alloc_new(block_size);
	for(int i = 0; i < max; ++i)
	    special_alloc_delete(ptr[max-i-1]);

	for(int i = 0; i < max; ++i)
	    ptr[i] = special_alloc_new(block_size);

	for(int i = 0; i < max - 1; ++i)
	    special_alloc_delete(ptr[i]);

	for(int i = 0; i < max - 1; ++i)
	    ptr[i] = special_alloc_new(10*i);

	special_alloc_delete(ptr[max - 1]);
	for(int i = 0; i < max - 1; ++i)
	    special_alloc_delete(ptr[i]);

    }
    catch(Egeneric & e)
    {
	e.dump();
    }
    catch(...)
    {
	cout << "unknown exception caught" << endl;
    }
#endif

}
