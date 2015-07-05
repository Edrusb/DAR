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

#include "scrambler.hpp"

using namespace std;

namespace libdar
{

    scrambler::scrambler(const secu_string & pass, generic_file & hidden_side) : generic_file(hidden_side.get_mode())
    {
        if(pass.size() == 0)
            throw Erange("scrambler::scrambler", gettext("Key cannot be an empty string"));
        key = pass.c_str();
	    // here aboce we've convert a secured data to unsecured data, but due to the weakness of the scrambling algo,
	    // this does not open any security breach...
        len = pass.size();
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
                meta_delete(buffer);
                buffer = nullptr;
            }
            meta_new(buffer, size);
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
