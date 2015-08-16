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
#include "tools.hpp"
#include "list_entry.hpp"

using namespace std;

namespace libdar
{
    list_entry::list_entry()
    {
	my_name = "";
	type = ' ';
	hard_link = false;
	uid = 0;
	gid = 0;
	perm = 0;
	data_status = s_saved;
	ea_status = cat_inode::ea_none;
	fsa_status = cat_inode::fsa_none;
	file_size = 0;
	storage_size = 0;
	sparse_file = 0;
	compression_algo = none;
	dirty = false;
	target = "";
	major = 0;
	minor = 0;
	delta_sig = false;
    }

    time_t list_entry::datetime2time_t(const datetime & val)
    {
	time_t ret = 0;
	time_t unused;

	(void) val.get_value(ret, unused, datetime::tu_second);

	return ret;
    }

} // end of namespace
