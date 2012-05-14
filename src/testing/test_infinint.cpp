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
// $Id: test_infinint.cpp,v 1.9 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <iostream>

#include "test_memory.hpp"
#include "integers.hpp"
#include "infinint.hpp"
#include "deci.hpp"
#include "cygwin_adapt.hpp"

using namespace libdar;
using namespace std;

static void routine1();

int main()
{
    MEM_BEGIN;
    MEM_IN;
    routine1();
    MEM_OUT;
    MEM_END;
}

static void routine1()
{
    infinint f1 = 123;
    infinint f2 = f1;
    infinint f3 = 0;
    
    deci d1 = f1;
    deci d2 = f2;
    deci d3 = f3;

    cout << d1.human() << " " << d2.human() << " " << d3.human() << endl;

    S_I fd = open("toto", O_RDWR | O_CREAT | O_TRUNC | O_BINARY, 0644);
    if(fd >= 0)
    {
        f1.dump(fd);
        close(fd);
        fd = open("toto", O_RDONLY|O_BINARY);
        if(fd >= 0)
        {
            f3 = infinint(&fd, NULL);
            d3 = deci(f3);
            cout << d3.human() << endl;
        }
        close(fd);
        fd = -1;
    }

    f1 += 3;
    d1 = deci(f1);
    cout << d1.human() << endl;

    f1 -= 2;
    d1 = deci(f1);
    cout << d1.human() << endl;

    f1 *= 10;
    d1 = deci(f1);
    cout << d1.human() << endl;

    f2 = f1;
    f1 /= 3;
    d1 = deci(f1);
    cout << d1.human() << endl;

    f2 %= 3;
    d2 = deci(f2);
    cout << d2.human() << endl;
    
    f2 >>= 12;
    d2 = deci(f2);
    cout << d2.human() << endl;

    f1 = 4;
    f2 >>= f1;
    d2 = deci(f2);
    cout << d2.human() << endl;

    f1 = 4+12;
    f2 = f3;
    f3 <<= f1;
    f2 <<= 4+12;
    d2 = deci(f2);
    cout << d2.human() << endl;
    d3 = deci(f3);
    cout << d3.human() << endl;
    

    f1 = 21;
    f2 = 1;
    try
    {
        for(f3 = 2; f3 <= f1; f3++)
        {
            d1 = deci(f1);
            d2 = deci(f2);
            d3 = deci(f3);
            cout << d1.human() << " " << d2.human() << " " << d3.human() << endl;
            f2 *= f3;
            d2 = deci(f2);
            cout << d2.human() << endl;
        }
    }
    catch(Elimitint & e)
    {
        cout << e.get_message() << endl;
    }
    d2 = deci(f2);
    d1 = deci(f1);
    cout << "factoriel(" <<d1.human() << ") = " << d2.human() << endl;
}
