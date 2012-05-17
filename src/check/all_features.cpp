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

#include "libdar.hpp"
#include <cstdlib>
using namespace libdar;
using namespace std;


int main()
{
	// initializing libdar

    U_I major;
    U_I medium;
    U_I minor;

    try
    {
	get_version(major, medium, minor);
    }
    catch(...)
    {
	cerr << "libdar library error, cannot initialize library" << endl;
	exit(1);
    }

    try
    {
	if(major != LIBDAR_COMPILE_TIME_MAJOR || medium < LIBDAR_COMPILE_TIME_MEDIUM)
	{
	    cerr << "all_feature program has is not evaluating the expected libdar library, aborting" << endl;
	    throw Efeature("version");
	}

	if(!compile_time::libz())
	{
	    cerr << "MISSING GZIP COMPRESSION SUPPORT TO BE ABLE TO PERFORM ALL TESTS, ABORTING" << endl;
	    throw Efeature("libz");
	}

	if(!compile_time::libbz2())
	{
	    cerr << "MISSING BZIP2 COMPRESSION SUPPORT TO BE ABLE TO PERFORM ALL TESTS, ABORTING" << endl;
	    throw Efeature("libz2");
	}

	if(!compile_time::liblzo())
	{
	    cerr << "MISSING LZO COMPRESSION SUPPORT TO BE ABLE TO PERFORM ALL TESTS, ABORTING5D" << endl;
	    throw Efeature("lzo2");
	}

	if(!compile_time::libgcrypt())
	{
	    cerr << "MISSING STRONG ENCRYPTION SUPPORT TO BE ABLE TO PERFORM ALL TESTS, ABORTING" << endl;
	    throw Efeature("libz2");
	}
    }
    catch(...)
    {
	close_and_clean();
	exit(2);
    }

    close_and_clean();
    exit(0);
}


