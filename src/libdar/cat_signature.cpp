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

#include "cat_signature.hpp"

using namespace std;


namespace libdar
{

    cat_signature::cat_signature(unsigned char original, saved_status status)
    {
        if(!islower(original))
            throw SRC_BUG;

        switch(status)
        {
        case saved_status::saved:
	    field = 3;
	    break;
	case saved_status::inode_only:
	    field = 4;
	    break;
        case saved_status::fake:
	    field = 7;
	    break;
        case saved_status::not_saved:
	    field = 2;
	    break;
	case saved_status::delta:
	    field = 1;
	    break;
        default:
            throw SRC_BUG;
        }

	field <<= 5;
	field |= (original & 0x1F); // only adding the 5 lower bits of "original" field
    }

    cat_signature::cat_signature(generic_file & f, const archive_version & reading_ver)
    {
	unsigned char tmp_base;
	saved_status tmp_status;

	if(!read(f, reading_ver) || !get_base_and_status(tmp_base, tmp_status))
	    throw Erange("cat_signature::cat_signature(generic_file)", gettext("incoherent catalogue structure"));
    }

    bool cat_signature::read(generic_file & f, const archive_version & reading_ver)
    {
	return f.read((char *)&field, 1) == 1;
    }

    void cat_signature::write(generic_file &f)
    {
	f.write((const char *)&field, 1);
    }

    bool cat_signature::get_base_and_status(unsigned char & base, saved_status & saved) const
    {
	    // building the lowercase letter by forcing the bits 6, 7 and 8 to the value 011:
	    // 0x1F is 0001 1111 in binary
	    // 0x60 is 0110 0000 in binary
	base = ((field & 0x1F) | 0x60);
	if(!islower(base))
	    return false; // must be a letter

	U_I val = field >> 5;
	switch(val)
	{
	case 0:
	    return false;
	case 1:
	    saved = saved_status::delta;
	    break;
	case 2:
	    saved = saved_status::not_saved;
	    break;
	case 3:
	    saved = saved_status::saved;
	    break;
	case 4:
	    saved = saved_status::inode_only;
	    break;
	case 5:
	case 6:
	    return false;
	case 7:
	    saved = saved_status::fake;
	    break;
	default:
	    throw SRC_BUG;
	}

	return true;
    }

    bool cat_signature::compatible_signature(unsigned char a, unsigned char b)
    {
        switch(a)
        {
        case 'e':
        case 'f':
            return b == 'e' || b == 'f';
        default:
            return b == a;
        }
    }

} // end of namespace
