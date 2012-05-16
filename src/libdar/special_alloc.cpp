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
// $Id: special_alloc.cpp,v 1.4.4.1 2004/01/28 15:29:47 edrusb Rel $
//
/*********************************************************************/

#include "erreurs.hpp"
#include "special_alloc.hpp"
#include "user_interaction.hpp"

#include <list>

#ifdef LIBDAR_SPECIAL_ALLOC

using namespace std;

namespace libdar
{

    const unsigned long CHUNK_SIZE = 1048576; // must stay greater than storage.cpp:storage::alloc_size


    struct segment
    {
        char *alloc;
        char *ptr;
        unsigned long reste;
        unsigned long ref;
    };

    list<struct segment> alloc;

    void *special_alloc_new(size_t taille)
    {
        void *ret = NULL;
        
        if(alloc.size() == 0 || alloc.back().reste < taille)
        {
            struct segment seg;
            
            seg.alloc = ::new char[CHUNK_SIZE];
            if(seg.alloc == NULL)
                throw Ememory("special_alloc_new");
            seg.ptr = seg.alloc;
            seg.reste = CHUNK_SIZE;
            seg.ref = 0;
            
            alloc.push_back(seg);

            if(alloc.size() == 0)
                throw SRC_BUG;

            if(alloc.back().reste < taille)
            {
                ui_printf("requested chunk = %d\n", taille);
                throw SRC_BUG;
            }
        }

        ret = alloc.back().ptr;
        alloc.back().ptr += taille;
        alloc.back().reste -= taille;
        alloc.back().ref++;

        return ret;
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: special_alloc.cpp,v 1.4.4.1 2004/01/28 15:29:47 edrusb Rel $";
        dummy_call(id);
    }
   
    void special_alloc_delete(void *ptr)
    {
        list<struct segment>::iterator rit = alloc.begin();

        while(rit != alloc.end() && (ptr < rit->alloc || ptr >= rit->alloc+CHUNK_SIZE))
            rit++;

        if(rit != alloc.end())
        {
            (rit->ref)--;
            if(rit->ref == 0)
            {
                ::delete rit->alloc;
                alloc.erase(rit);
            }
        }
        else
            throw SRC_BUG; // unknown memory, not allocated here !?
    }

} // end of namespace

#endif
