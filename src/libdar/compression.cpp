/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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

extern "C"
{
} // end extern "C"

#include "compression.hpp"
#include "erreurs.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    compression char2compression(char a)
    {
        switch(a)
        {
        case 'n':
            return compression::none;
        case 'z':
            return compression::gzip;
	case 'Z':
	    return compression::b_gzip;
        case 'y':
            return compression::bzip2;
	case 'Y':
	    return compression::b_bzip2;
	case 'j':
	    return compression::lzo1x_1_15;
	case 'J':
	    return compression::b_lzo1x_1_15;
	case 'k':
	    return compression::lzo1x_1;
	case 'K':
	    return compression::b_lzo1x_1;
	case 'l':
	    return compression::lzo;
	case 'L':
	    return compression::b_lzo;
	case 'x':
	    return compression::xz;
	case 'X':
	    return compression::b_xz;
	case 'd':
	    return compression::zstd;
	case 'D':
	    return compression::b_zstd;
	case 'q':
	    return compression::lz4;
	case 'Q':
	    return compression::b_lz4;
        default :
            throw Erange("char2compression", gettext("unknown compression"));
        }
    }

    char compression2char(compression c)
    {
        switch(c)
        {
        case compression::none:
            return 'n';
        case compression::gzip:
            return 'z';
	case compression::b_gzip:
	    return 'Z';
        case compression::bzip2:
            return 'y';
	case compression::b_bzip2:
	    return 'Y';
	case compression::lzo:
	    return 'l';
	case compression::b_lzo:
	    return 'L';
	case compression::xz:
	    return 'x';
	case compression::b_xz:
	    return 'X';
	case compression::lzo1x_1_15:
	    return 'j';
	case compression::b_lzo1x_1_15:
	    return 'J';
	case compression::lzo1x_1:
	    return 'k';
	case compression::b_lzo1x_1:
	    return 'K';
	case compression::zstd:
	    return 'd';
	case compression::b_zstd:
	    return 'D';
	case compression::lz4:
	    return 'q';
	case compression::b_lz4:
	    return 'Q';
        default:
            throw Erange("compression2char", gettext("unknown compression"));
        }
    }

    string compression2string(compression c)
    {
        switch(c)
        {
        case compression::none:
            return "none";
        case compression::gzip:
            return "gzip";
	case compression::b_gzip:
	    return "gzip per block";
        case compression::bzip2:
            return "bzip2";
	case compression::b_bzip2:
	    return "bzip2 per block";
	case compression::lzo:
	    return "lzo";
	case compression::b_lzo:
	    return "lzo per block";
	case compression::xz:
	    return "xz";
	case compression::b_xz:
	    return "xz per block";
	case compression::lzo1x_1_15:
	    return "lzop-1";
	case compression::b_lzo1x_1_15:
	    return "lzop-1 per block";
	case compression::lzo1x_1:
	    return "lzop-3";
	case compression::b_lzo1x_1:
	    return "lzop-3 per block";
	case compression::zstd:
	    return "zstd";
	case compression::b_zstd:
	    return "zstd per block";
	case compression::lz4:
	    return "lz4";
	case compression::b_lz4:
	    return "lz4 per block";
        default:
            throw Erange("compresion2string", gettext("unknown compression"));
        }
    }

    compression string2compression(const std::string & a)
    {
	if(a == "gzip" || a == "gz")
	    return compression::gzip;

	if(a == "bzip2" || a == "bzip" || a == "bz")
	    return compression::bzip2;

	if(a == "lzo" || a == "lz" || a == "l")
	    return compression::lzo;

	if(a == "lzop-1" || a == "lzop1")
	    return compression::lzo1x_1_15;

	if(a == "lzop-3" || a == "lzop3")
	    return compression::lzo1x_1;

	if(a == "xz" || a == "lzma")
	    return compression::xz;

	if(a == "zstd")
	    return compression::zstd;

	if(a == "lz4")
	    return compression::lz4;

	if(a == "none")
	    return compression::none;

	throw Erange("string2compression", tools_printf(gettext("unknown compression algorithm: %S"), &a));
    }

    extern compression equivalent_with_block(compression c)
    {
	switch(c)
	{
	case compression::none:
	    return compression::none;
	case compression::gzip:
	    return compression::b_gzip;
	case compression::bzip2:
	    return compression::b_bzip2;
	case compression::lzo:
	    return compression::b_lzo;
	case compression::xz:
	    return compression::b_xz;
	case compression::lzo1x_1_15:
	    return compression::b_lzo1x_1_15;
	case compression::lzo1x_1:
	    return compression::b_lzo1x_1;
	case compression::zstd:
	    return compression::b_zstd;
	case compression::lz4:
	    return compression::b_lz4;
	case compression::b_gzip:
	case compression::b_bzip2:
	case compression::b_lzo:
	case compression::b_xz:
	case compression::b_lzo1x_1_15:
	case compression::b_lzo1x_1:
	case compression::b_zstd:
	case compression::b_lz4:
	    throw Erange("equivalent_with_block", gettext("the given algorithm is not a streamed compression algorithm"));
        default:
            throw Erange("equivalent_with_block", gettext("unknown compression"));
	}
    }

    compression equivalent_streamed(compression c)
    {
	switch(c)
	{
	case compression::none:
	    return compression::none;
	case compression::b_gzip:
	    return compression::gzip;
	case compression::b_bzip2:
	    return compression::bzip2;
	case compression::b_lzo:
	    return compression::lzo;
	case compression::b_xz:
	    return compression::xz;
	case compression::b_lzo1x_1_15:
	    return compression::lzo1x_1_15;
	case compression::b_lzo1x_1:
	    return compression::lzo1x_1;
	case compression::b_zstd:
	    return compression::zstd;
	case compression::b_lz4:
	    return compression::lz4;
	case compression::gzip:
	case compression::bzip2:
	case compression::lzo:
	case compression::xz:
	case compression::lzo1x_1_15:
	case compression::lzo1x_1:
	case compression::zstd:
	case compression::lz4:
	    throw Erange("equivalent_streamed", gettext("the given algorithm is not a per-block compression algorithm"));
        default:
            throw Erange("equivalent_streamed", gettext("unknown compression"));
	}
    }


} // end of namespace
