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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

#include <iostream>
#include <fstream>
#include <string>

extern "C"
{
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
}

using namespace std;

void show_usage(const string & cmd);
void modify(const string & arg);

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
	show_usage(argv[0]);
	return 1;
    }
    else
    {
	try
	{
	    modify(argv[1]);
	}
	catch(...)
	{
	    cerr << "Error met during treatment" << endl;
	    return 2;
	}
	return 0;
    }
}

void show_usage(const string & cmd)
{
    cout << "usage: " << cmd << " <filename>" << endl;
}

void modify(const string & arg)
{
    ofstream fic;
    fic.open(arg.c_str(), ios::out);

    while(1) // loop that never ends, unless program is killed
    {
	usleep(50000); // sleep 50 ms
	int rnd = rand();
	cout << "seeking at : " << rnd << endl;
	fic.seekp(rnd); // seek at a random place
	fic << "coucou " << endl;
    }
}
