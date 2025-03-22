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
} // end extern "C"

#include "compress_block_header.hpp"


using namespace std;

namespace libdar
{

    void compress_block_header::dump(generic_file & f)
    {
	f.write(&type, 1);
	size.dump(f);
    }

    bool compress_block_header::set_from(generic_file & f)
    {
	bool ret = (f.read(&type, 1) == 1);

	if(ret)
	    size.read(f);
	    // if read() fails for size while
	    // it succeeded for type, an exception
	    // is thrown calling size.read(f)

	return ret;
    }

} // end of namespace
