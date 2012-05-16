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
// $Id: tronc.cpp,v 1.7.2.2 2004/07/25 20:38:04 edrusb Exp $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif
} // end extern "C"

#include "tronc.hpp"

namespace libdar
{

    tronc::tronc(generic_file *f, const infinint & offset, const infinint &size) : generic_file(f->get_mode())
    {
        ref = f;
        sz = size;
        start = offset;
        current = size; // forces skipping the first time
    }

    tronc::tronc(generic_file *f, const infinint & offset, const infinint &size, gf_mode mode) : generic_file(mode)
    {
        ref = f;
        sz = size;
        start = offset;
        current = size; // forces skipping the firt time
    }

    bool tronc::skip(const infinint & pos)
    {
        if(current == pos)
            return true;

        if(pos > sz)
        {
            current = sz;
            ref->skip(start + sz);
            return false;
        }
        else
        {
            current = pos;
            return ref->skip(start + pos);
        }
    }

    bool tronc::skip_to_eof()
    {
        current = sz;
        return ref->skip(start + sz);
    }

    bool tronc::skip_relative(S_I x)
    {
        if(x < 0)
            if(current < -x)
            {
                ref->skip(start);
                current = 0;
                return false;
            }
            else
            {
                bool r = ref->skip_relative(x);
                if(r)
                    current -= -x;
                else
                {
                    ref->skip(start);
                    current = 0;
                }
                return r;
            }

        if(x > 0)
            if(current + x >= sz)
            {
                current = sz;
                ref->skip(start+sz);
                return false;
            }
            else
            {
                bool r = ref->skip_relative(x);
                if(r)
                    current += x;
                else
                {
                    ref->skip(start+sz);
                    current = sz;
                }
                return r;
            }

        return true;
    }


    static void dummy_call(char *x)
    {
        static char id[]="$Id: tronc.cpp,v 1.7.2.2 2004/07/25 20:38:04 edrusb Exp $";
        dummy_call(id);
    }

    S_I tronc::inherited_read(char *a, size_t size)
    {
        infinint avail = sz - current;
        U_32 macro_pas = 0, micro_pas;
        U_32 lu = 0;
        S_I ret;

        do
        {
            avail.unstack(macro_pas);
            micro_pas = size - lu > macro_pas ? macro_pas : size - lu;
            if(micro_pas > 0)
            {
                ret = ref->read(a+lu, micro_pas);
                if(ret > 0)
                {
                    lu += ret;
                    macro_pas -= ret;
                }
            }
            else
                ret = 0;
        }
        while(ret > 0);
        current += lu;

        return lu;
    }

    S_I tronc::inherited_write(char *a, size_t size)
    {
        infinint avail = sz - current;
        U_32 macro_pas = 0, micro_pas;
        U_32 wrote = 0;
        S_I ret;

        ref->skip(start + current);
        do
        {
            avail.unstack(macro_pas);
            if(macro_pas == 0 && wrote < size)
                throw Erange("tronc::inherited_write", "tried to write out of size limited file");
            micro_pas = size - wrote > macro_pas ? macro_pas : size - wrote;
            ret = ref->write(a+wrote, micro_pas);
            if( ret > 0)
            {
                wrote += ret;
                macro_pas -= ret;
            }
        }
        while(ret > 0);
        current += wrote;

        return wrote;
    }

} // end of namespace
