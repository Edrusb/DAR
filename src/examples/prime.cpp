/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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

extern "C"
{
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
}

#include <iostream>

#include "infinint.hpp"
#include "deci.hpp"
#include "integers.hpp"
#include "../dar_suite/dar_suite.hpp"

using namespace libdar;
using namespace std;

int little_main(shared_ptr<user_interaction> & ui, S_I argc, char * const argv[], const char **env);

int main(S_I argc, char * const argv[], const char **env)
{
    return dar_suite_global(argc,
			    argv,
			    env,
			    "",
			    nullptr,
			    '\0',
			    &little_main);
}

int little_main(shared_ptr<user_interaction> & ui, S_I argc, char * const argv[], const char **env)
{
    if(argc != 2)
    {
        cout << "usage : " << argv[0] <<  " <number>" << endl;
        exit(1);
    }

    libdar::deci x = string(argv[1]);
    cout << "converting string to infinint... " << endl;
    infinint num = x.computer();
    cout << "checking whether the number is a prime factor... " << endl;
    infinint max = (num / 2) + 1;
    infinint i = 2;
    while(i < max)
        if(num%i == 0)
            break;
        else
            ++i;

    if(i < max)
        cout << argv[1] << " is NOT prime" << endl;
    else
        cout << argv[1] << " is PRIME" << endl;

    return EXIT_OK;
}
