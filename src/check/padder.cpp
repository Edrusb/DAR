/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <iostream>

using namespace std;

void padder(unsigned int size, string & number);

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
	cerr << "usage: " << argv[0] << " <padding size> <number>" << endl;
	return 1;
    }
    else
    {
	string tmp = argv[2];

	padder(atoi(argv[1]), tmp);
	cout << tmp << endl;

	return 0;
    }
}

void padder(unsigned int size, string & number)
{
    while(number.size() < size)
	number = "0" + number;
}
