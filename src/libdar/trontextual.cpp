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
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"
#include "trontextual.hpp"

namespace libdar
{

    trontextual::trontextual(generic_file *f,
			     const infinint & offset,
			     const infinint & size,
			     bool own_f) : tronc(f, offset, size, own_f)
    {
	init(f);
    }

    trontextual::trontextual(generic_file *f,
			     const infinint & offset,
			     const infinint & size,
			     gf_mode mode,
			     bool own_f) : tronc(f, offset, size, mode, own_f)
    {
	init(f);
    }

    void trontextual::init(generic_file *f)
    {
	ref = dynamic_cast<contextual *>(f);
	if(ref == nullptr)
	    throw Erange("trontextual::trontextual", "Trontextual must receive a class contextual aware generic file as argument");
    }

} // end of namespace

