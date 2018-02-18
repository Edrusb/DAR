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
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // extern "C"

#include "archive5.hpp"

#define ARCHIVE_NOT_EXPLOITABLE "Archive of reference given is not exploitable"

using namespace std;

namespace libdar5
{

    void archive::op_listing(user_interaction & dialog,
			     const archive_options_listing & options)
    {
	if(dialog.get_use_listing())
	    libdar::archive::op_listing(dialog,
					dialog.listing,
					options);
	else
	    libdar::archive::op_listing(dialog,
					nullptr,
					options);
    }

    bool archive::get_children_of(user_interaction & dialog,
				  const std::string & dir)
    {
	if(dialog.get_use_listing())
	    return libdar::archive::get_children_of(dialog,
						    dialog.listing,
						    options);
	else
	    return libdar::archive::get_children_of(dialog,
						    nullptr,
						    options);
    }

} // end of namespace
