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
// $Id: scrambler.cpp,v 1.5 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

#include "scrambler.hpp"

using namespace std;

namespace libdar
{

    scrambler::scrambler(const string & pass, generic_file & hidden_side) : generic_file(hidden_side.get_mode())
    {
        if(pass == "")
            throw Erange("scrambler::scrambler", "key cannot be an empty string");
        key = pass;
        len = pass.size();
        ref = & hidden_side;
        buffer = NULL;
        buf_size = 0;
    }

    S_I scrambler::inherited_read(char *a, size_t size)
    {
        unsigned char *ptr = (unsigned char *)a;
        if(ref == NULL)
            throw SRC_BUG;

        U_32 index = ref->get_position() % len;
        S_I ret = ref->read(a, size);

        for(register S_I i = 0; i < ret; i++)
        {
            ptr[i] = ((S_I)(ptr[i]) - (unsigned char)(key[index])) % 256;
            index = (index + 1)%len;
        }
        return ret;
    }

    S_I scrambler::inherited_write(const char *a, size_t size)
    {
        unsigned char *ptr = (unsigned char *)a;
        if(ref == NULL)
            throw SRC_BUG;

        U_32 index = ref->get_position() % len;
        if(size > buf_size)
        {
            if(buffer != NULL)
            {
                delete buffer;
                buffer = NULL;
            }
            buffer = new unsigned char[size];
            if(buffer != NULL)
                buf_size = size;
            else
            {
                buf_size = 0;
                throw Ememory("scramble::inherited_write");
            }
        }

        for(register size_t i = 0; i < size; i++)
        {
            buffer[i] = (ptr[i] + (unsigned char)(key[index])) % 256;
            index = (index + 1)%len;
        }

        return ref->write((char *)buffer, size);
    }


    static void dummy_call(char *x)
    {
        static char id[]="$Id: scrambler.cpp,v 1.5 2003/10/18 14:43:07 edrusb Rel $";
        dummy_call(id);
    }

} // end of namespace
