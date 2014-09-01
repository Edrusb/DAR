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

extern "C"
{

} // end extern "C"

#include "cat_ignored_dir.hpp"

using namespace std;

namespace libdar
{

    void cat_ignored_dir::inherited_dump(generic_file & f, bool small) const
    {
	cat_directory tmp = cat_directory(get_uid(), get_gid(), get_perm(), get_last_access(), get_last_modif(), get_last_change(), get_name(), 0);
	tmp.set_saved_status(get_saved_status());
	tmp.specific_dump(f, small); // dump an empty directory
    }

} // end of namespace

