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
// $Id: test_memory.cpp,v 1.7.4.1 2012/01/26 20:09:08 edrusb Exp $
//
/*********************************************************************/

#include "../my_config.h"

#include <list>
#include <iostream>

#include "test_memory.hpp"
#include "erreurs.hpp"

#ifdef TEST_MEMORY

using namespace std;

namespace libdar
{

    struct my_alloc
    {
        void *address;
        U_32 size;

        bool operator == (const struct my_alloc & x) { return x.address == address; };
    };

    static U_32 total_size = 0;
    static U_32 initial_offset = 0;
    static U_32 initial_size = 0;
    static list <my_alloc> liste;


    U_32 get_total_alloc_size()
    {
        return total_size;
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: test_memory.cpp,v 1.7.4.1 2012/01/26 20:09:08 edrusb Exp $";
        dummy_call(id);
    }

    void record_offset()
    {
        initial_offset = liste.size();
        initial_size = total_size;
    }

    void all_delete_done()
    {
        if(liste.size() != initial_offset || initial_size != total_size)
            cerr << (liste.size() - initial_offset) << " memory allocation(s) not released ! Total size = " << total_size - initial_size << endl;
        else
            if(liste.size() < initial_offset)
                throw SRC_BUG;
    }

    void memory_check(U_32 ref, const char *fichier, S_I ligne)
    {
        U_32 current = get_total_alloc_size();
        S_32 diff = current - ref;
        if(diff != 0)
            cerr << "file " << fichier << " line " << ligne << " : memory leakage detected : initial = " << ref << "    final = " << current << "    DELTA = " << diff << endl;
    }

} // end of namespace

using namespace libdar;

void * operator new(size_t size) throw (bad_alloc)
{
    void *ret = malloc(size);

    if(ret != NULL)
    {
	my_alloc al;
	al.address = ret;
	al.size = size;
	liste.push_back(al);
	total_size += size;
    }

    return ret;
}

void operator delete(void *p) throw ()
{
    list<my_alloc>::iterator it = liste.begin();
    while(it != liste.end() && it->address != p)
	it++;

    if(it != liste.end())
    {
	total_size -= it->size;
	liste.remove(*it);
    }
    else
	throw SRC_BUG;

    free(p);
}


#endif
