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
// $Id: test_memory.cpp,v 1.6 2002/03/18 11:00:54 denis Rel $
//
/*********************************************************************/

#include <list>
#include "test_memory.hpp"
#include "erreurs.hpp"

#ifdef TEST_MEMORY

using namespace std;

struct my_alloc
{
    void *address;
    unsigned long size;

    bool operator == (const struct my_alloc & x) { return x.address == address; };
};

static unsigned long total_size = 0;
static list <my_alloc> liste;

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

unsigned long get_total_alloc_size() 
{ 
    Egeneric::clear_last_destroyed();
    return total_size; 
}

static void dummy_call(char *x)
{
    static char id[]="$Id: test_memory.cpp,v 1.6 2002/03/18 11:00:54 denis Rel $";
    dummy_call(id);
}

void all_delete_done()
{
    if(liste.size() > 0)
	cout << liste.size() << " memory allocation(s) not released ! " << " total size = " << total_size << endl;

}

void memory_check(unsigned long ref, const char *fichier, int ligne)
{
    Egeneric::clear_last_destroyed();
    
    unsigned long current = get_total_alloc_size();
    signed long diff = current - ref;
    if(diff != 0)
	cout << "file " << fichier << " line " << ligne << " : memory leakage detected : initial = " << ref << "   final = " << current << "  DELTA =  " << diff << endl;
}

#endif
