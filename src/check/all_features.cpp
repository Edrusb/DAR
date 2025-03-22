/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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

#include "libdar.hpp"
#include <cstdlib>
using namespace libdar;
using namespace std;

    ///////////////
    // strange() is an unused function that invokes three symbols defined in <libintl.h>
    // without this trick for some reason I still cannot understand, the linking fails on Cygwin
    // having libdar missing _libintl_bindtextdomain symbols to link with.
    // The point that is strange is that invoking these three symbols here do not lead to
    // define any associated _libintl_ prefixed symbols here, just to refer to them one more time,
    // so the linking of all_features should still fails! (while here it succeeds...)
    // Looking at <libintl.h> header files, it seems that three modes are available:
    // 1:  _INTL_REDIRECT_INLINE
    // 2:  _INTL_REDIRECT_MACROS
    // 3:  _INTL_REDIRECT_ASM
    // seen the output of g++ -E the option selected under Cygwin is the asm one when
    // compiling all_features.cpp as well as while compiling libdar, all three methods
    // make the gettext/textdomain/bindtextdomain to point (either by macro, inlined function,
    // or assembler definition to the associated _libintl_ prefixed function, which symbol
    // is missing at linking time while the -lint is properly given on command-line.
    // calling gettext/textdomain/bindtext in strange() below do not define any symbol
    // ... so why does it makes linking succeed? ... why does it fails else? I admin my
    // limited understanding here. Anyway, this fix works and has no impact at execution time
    // just making the all_feature.cpp binary slightly larger, binary that is only used
    // when invoking 'make check'.
void strange()
{
    bindtextdomain("", "");
    textdomain("");
    string tmp = gettext("-");
}
    // end of the strange trick!
//////////////

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
	    cerr << "MISSING LZO COMPRESSION SUPPORT TO BE ABLE TO PERFORM ALL TESTS, ABORTING" << endl;
	    throw Efeature("lzo2");
	}

	if(!compile_time::libxz())
	{
	    cerr << "MISSING LZMA/XZ COMPRESSION SUPPORT TO BE ABLE TO PERFORM ALL TESTS, ABORTING" << endl;
	    throw Efeature("libxz");
	}

	if(!compile_time::libzstd())
	{
	    cerr << "MISSING ZSTD COMPRESSION SUPPORT TO BE ABLE TO PERFORM ALL TESTS, ABORTING" << endl;
	    throw Efeature("libzstd");
	}

	if(!compile_time::liblz4())
	{
	    cerr << "MISSING LZ4 COMPRESSION SUPPORT TO BE ABLE TO PERFORM ALL TESTS, ABORTING" << endl;
	    throw Efeature("liblz4");
	}

	if(!compile_time::libgcrypt())
	{
	    cerr << "MISSING STRONG ENCRYPTION SUPPORT TO BE ABLE TO PERFORM ALL TESTS, ABORTING" << endl;
	    throw Efeature("libz2");
	}

	if(!compile_time::public_key_cipher())
	{
	    cerr << "MISSING STRONG ENCRYPTION SUPPORT TO BE ABLE TO PERFORM ALL TESTS, ABORTING" << endl;
	    throw Efeature("public key encryption");
	}

	if(!compile_time::librsync())
	{
	    cerr << "MISSING LIBRSYNC SUPPORT TO BE ABLE TO PERFORM ALL TESTS, ABORTING" << endl;
	    throw Efeature("librsync");
	}

	if(!compile_time::remote_repository())
	{
	    cerr << "MISSING LIBCURL SUPPORT TO BE ABLE TO PERFORM ALL TESTS, ABORTING" << endl;
	    throw Efeature("libcurl");
	}

	if(!compile_time::ea())
	{
	    cerr << "MISSING EA SUPPORT TO BE ABLE TO PERFORM ALL TESTS, ABORTING" << endl;
	    throw Efeature("libcurl");
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


