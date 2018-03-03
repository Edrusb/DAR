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
#include "erreurs.hpp"

#define ARCHIVE_NOT_EXPLOITABLE "Archive of reference given is not exploitable"

using namespace std;

using libdar::Erange;
using libdar::Ebug;

namespace libdar5
{

    void archive::op_listing(user_interaction & dialog,
			     const archive_options_listing & options)
    {
	libdar::archive::op_listing(user_interaction5_clone_to_shared_ptr(dialog),
				    nullptr,
				    nullptr,
				    options);
    }

    bool archive::get_children_of(user_interaction & dialog,
				  const std::string & dir)
    {
	if(!dialog.get_use_listing())
	    throw Erange("archive::get_childen_of", gettext("listing() method must be given"));

	return libdar::archive::get_children_of(user_interaction5_clone_to_shared_ptr(dialog),
						listing_callback,
						&dialog,
						dir);
    }

    void archive::listing_callback(void *context,
				   const string & flag,
				   const string & perm,
				   const string & uid,
				   const string & gid,
				   const string & size,
				   const string & date,
				   const string & filename,
				   bool is_dir,
				   bool has_children)
    {
	user_interaction *dialog = (user_interaction *)(context);

	if(dialog == nullptr)
	    throw SRC_BUG;

	if(dialog->get_use_listing())
	    dialog->listing(flag,
			    perm,
			    uid,
			    gid,
			    size,
			    date,
			    filename,
			    is_dir,
			    has_children);
	else
	    throw SRC_BUG;

    }


} // end of namespace
