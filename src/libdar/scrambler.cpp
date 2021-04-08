/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

#include "scrambler.hpp"

using namespace std;

namespace libdar
{

    scrambler::scrambler(const secu_string & pass, generic_file & hidden_side) : generic_file(hidden_side.get_mode())
    {
        if(pass.get_size() == 0)
            throw Erange("scrambler::scrambler", gettext("Key cannot be an empty string"));
        key = pass;
        len = key.get_size();
        ref = & hidden_side;
        buffer = nullptr;
        buf_size = 0;
    }

    U_I scrambler::inherited_read(char *a, U_I size)
    {
        if(ref == nullptr)
            throw SRC_BUG;

        unsigned char *ptr = (unsigned char *)a;
        U_32 index = ref->get_position() % len;
        U_I ret = ref->read(a, size);

        for(U_I i = 0; i < ret; ++i)
        {
            ptr[i] = ((S_I)(ptr[i]) - (unsigned char)(key[index])) % 256;
            index = (index + 1)%len;
        }
        return ret;
    }

    void scrambler::inherited_write(const char *a, U_I size)
    {
        const unsigned char *ptr = (const unsigned char *)a;
        if(ref == nullptr)
            throw SRC_BUG;

        U_32 index = ref->get_position() % len;
        if(size > buf_size)
        {
            if(buffer != nullptr)
            {
                delete [] buffer;
                buffer = nullptr;
            }
            buffer = new (nothrow) unsigned char[size];
            if(buffer != nullptr)
                buf_size = size;
            else
            {
                buf_size = 0;
                throw Ememory("scramble::inherited_write");
            }
        }

        for(U_I i = 0; i < size; ++i)
        {
            buffer[i] = (ptr[i] + (unsigned char)(key[index])) % 256;
            index = (index + 1)%len;
        }

        ref->write((char *)buffer, size);
    }

} // end of namespace
