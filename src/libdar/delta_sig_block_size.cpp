/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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
#if HAVE_LIBRSYNC_H
#include <stdio.h>
#include <librsync.h>
#endif
} // end extern "C"

#include "integers.hpp"
#include "delta_sig_block_size.hpp"
#include "erreurs.hpp"
#include "tools.hpp"

#ifndef RS_DEFAULT_BLOCK_LEN
#define RS_DEFAULT_BLOCK_LEN 2048
#endif

using namespace std;

namespace libdar
{
    void delta_sig_block_size::reset()
    {
	fs_function = root2;
	multiplier = 1;
	divisor = 1;
	min_block_len = RS_DEFAULT_BLOCK_LEN;
	max_block_len = 64*RS_DEFAULT_BLOCK_LEN;
    }

    bool delta_sig_block_size::operator == (const delta_sig_block_size & ref) const
    {
	return fs_function == ref.fs_function
	    && multiplier == ref.multiplier
	    && divisor == ref.divisor
	    && min_block_len == ref.min_block_len
	    && max_block_len == ref.max_block_len;
    }

    void delta_sig_block_size::check() const
    {
	if(divisor == 0)
	    throw Erange("delta_sig_block_size::check", gettext("Invalid divisor used for delta signature block len calculation"));
	if(max_block_len != 0 && min_block_len > max_block_len)
	    throw Erange("delta_sig_block_size::check", gettext("minimum size should be lesser or equal than maximum size when specifying delta signature block size formula"));
    }

    U_I delta_sig_block_size::calculate(const infinint & filesize) const
    {
	U_I ret = 0;
	infinint val(multiplier);

	switch(fs_function)
	{
	case fixed:
	    break;
	case linear:
	    val *= filesize;
	    break;
	case log2:
	    val *= tools_upper_rounded_log2(filesize);
	    break;
	case root2:
	    val *= tools_rounded_square_root(filesize);
	    break;
	case root3:
	    val *= tools_rounded_cube_root(filesize);
	    break;
	default:
	    throw SRC_BUG;
	}

	val /= divisor;

	    // ret is already set to zero we can unstack to it
	val.unstack(ret);

	    // if val is larger than the max value
	    // that can be carried by ret, ret will
	    // be this way set to the maximum value
	    // it can carry

	if(ret < min_block_len)
	    ret = min_block_len;

	if(max_block_len > 0 && ret > max_block_len)
	    ret = max_block_len;

	return ret;
    }

} // end of namespace
