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

extern "C"
{
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
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
	case 'Z':
            return compression::gzip;
        case 'y':
	case 'Y':
            return compression::bzip2;
	case 'j':
	case 'J':
	    return compression::lzo1x_1_15;
	case 'k':
	case 'K':
	    return compression::lzo1x_1;
	case 'l':
	case 'L':
	    return compression::lzo;
	case 'x':
	case 'X':
	    return compression::xz;
	case 'd':
	case 'D':
	    return compression::zstd;
	case 'q':
	case 'Q':
	    return compression::lz4;
        default :
            throw Erange("char2compression", gettext("unknown compression"));
        }
    }

    bool char2compression_mode(char a)
    {
    	return isupper(a);
    }

    char compression2char(compression c, bool per_block)
    {
        switch(c)
        {
        case compression::none:
            return per_block ? 'N': 'n';
        case compression::gzip:
            return per_block ? 'Z': 'z';
        case compression::bzip2:
            return per_block ? 'Y': 'y';
	case compression::lzo:
	    return per_block ? 'L': 'l';
	case compression::xz:
	    return per_block ? 'X': 'x';
	case compression::lzo1x_1_15:
	    return per_block ? 'J': 'j';
	case compression::lzo1x_1:
	    return per_block ? 'K': 'k';
	case compression::zstd:
	    return per_block ? 'D': 'd';
	case compression::lz4:
	    return per_block ? 'Q': 'q';
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
        case compression::bzip2:
            return "bzip2";
	case compression::lzo:
	    return "lzo";
	case compression::xz:
	    return "xz";
	case compression::lzo1x_1_15:
	    return "lzop-1";
	case compression::lzo1x_1:
	    return "lzop-3";
	case compression::zstd:
	    return "zstd";
	case compression::lz4:
	    return "lz4";
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


} // end of namespace
