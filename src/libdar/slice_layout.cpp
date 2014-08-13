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
#include "slice_layout.hpp"

using namespace std;

namespace libdar
{
    void slice_layout::read(generic_file & f)
    {
	char tmp;

	first_size.read(f);
	other_size.read(f);
	first_slice_header.read(f);
	other_slice_header.read(f);
	if(f.read(&tmp , 1) == 1)
	{
	    switch(tmp)
	    {
	    case OLDER_THAN_V8:
		older_sar_than_v8 = true;
		break;
	    case V8:
		older_sar_than_v8 = false;
		break;
	    default:
		throw SRC_BUG;
	    }
	}
	else
	    throw Erange("slice_layout::read", gettext("Missing data while reading slice_layout object"));
    }

    void slice_layout::write(generic_file & f) const
    {
	char tmp = older_sar_than_v8 ? OLDER_THAN_V8 : V8;
	first_size.dump(f);
	other_size.dump(f);
	first_slice_header.dump(f);
	other_slice_header.dump(f);
	f.write(&tmp, 1);
    }


    void slice_layout::which_slice(const infinint & offset,
				   infinint & slice_num,
				   infinint & slice_offset)
    {

	    // considering particular case of a non-sliced archive

	if(first_size == 0 || other_size == 0)
	{
	    slice_num = 1;
	    if(offset < first_slice_header)
		slice_offset = first_slice_header;
	    else
		slice_offset = offset - first_slice_header;
	    return;
	}

	    // sanity checks

	if(first_size < first_slice_header)
	    throw SRC_BUG;
	if(other_size < other_slice_header)
	    throw SRC_BUG;
	if(first_slice_header == 0)
	    throw SRC_BUG;
	if(other_slice_header == 0)
	    throw SRC_BUG;

	    // end of sanity checks

        infinint byte_in_first_file = first_size - first_slice_header;
        infinint byte_per_file = other_size - other_slice_header;

	if(!older_sar_than_v8)
	{
	    --byte_in_first_file;
	    --byte_per_file;
		// this is due to the trailing flag (one byte length)
	}

        if(offset < byte_in_first_file)
        {
            slice_num = 1;
            slice_offset = offset + first_slice_header;
        }
        else
        {
	    euclide(offset - byte_in_first_file, byte_per_file, slice_num, slice_offset);
            slice_num += 2;
                // "+2" because file number starts to 1 and first file is already counted
            slice_offset += other_slice_header;
        }
    }

} // end of namespace
