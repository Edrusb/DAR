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
// $Id: prime.cpp,v 1.7 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"
#include <iostream>

extern "C"
{
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
} // extern "C"

#include "infinint.hpp"
#include "deci.hpp"
#include "integers.hpp"

using namespace libdar;
using namespace std;

int main(S_I argc, char *argv[])
{
    if(argc != 2)
    {
        cout << "usage : " << argv[0] <<  " <number>" << endl;
        exit(1);
    }

    deci x = string(argv[1]);
    cout << "converting string to infinint... " << endl;
    infinint num = x.computer();
    cout << "checking if the number is a prime factor... " << endl;
    infinint max = (num / 2) + 1;
    infinint i = 2;
    while(i < max)
        if(num%i == 0)
            break;
        else
            i++;

    if(i < max)
        cout << argv[1] << " is NOT prime" << endl;
    else
        cout << argv[1] << " is PRIME" << endl;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: prime.cpp,v 1.7 2003/10/18 14:43:07 edrusb Rel $";
    dummy_call(id);
}
