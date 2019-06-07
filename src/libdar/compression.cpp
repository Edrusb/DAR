/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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
        case 'y':
            return compression::bzip2;
	case 'j':
	    return compression::lzo1x_1_15;
	case 'k':
	    return compression::lzo1x_1;
	case 'l':
	    return compression::lzo;
	case 'x':
	    return compression::xz;
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
        case compression::bzip2:
            return 'y';
	case compression::lzo:
	    return 'l';
	case compression::xz:
	    return 'x';
	case compression::lzo1x_1_15:
	    return 'j';
	case compression::lzo1x_1:
	    return 'k';
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

	if(a == "none")
	    return compression::none;

	throw Erange("string2compression", tools_printf(gettext("unknown compression algorithm: %S"), &a));
    }

} // end of namespace
