/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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

#include "gf_mode.hpp"
#include "erreurs.hpp"

using namespace std;

namespace libdar
{

    const char* generic_file_get_name(gf_mode mode)
    {
	const char *ret = nullptr;

        switch(mode)
	{
	case gf_read_only:
	    ret = gettext("read only");
	    break;
        case gf_write_only:
            ret = gettext("write only");
            break;
        case gf_read_write:
            ret = gettext("read and write");
            break;
        default:
            throw SRC_BUG;
        }
        return ret;
    }


} // end of namespace
