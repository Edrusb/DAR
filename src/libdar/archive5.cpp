/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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
#include "shell_interaction_emulator.hpp"

#define ARCHIVE_NOT_EXPLOITABLE "Archive of reference given is not exploitable"

using namespace std;

using libdar::Erange;
using libdar::Ebug;

namespace libdar5
{

    void archive::op_listing(user_interaction & dialog,
			     const archive_options_listing & options)
    {
	libdar::shell_interaction_emulator emul(&dialog);

	emul.archive_show_contents(*this, options);
    }

    bool archive::get_children_of(user_interaction & dialog,
				  const std::string & dir)
    {
	if(!dialog.get_use_listing())
	    throw Erange("archive::get_childen_of", gettext("listing() method must be given"));

	return libdar::archive::get_children_of(listing_callback,
						&dialog,
						dir);
    }

    void archive::listing_callback(const string & the_path,
				   const libdar::list_entry & entry,
				   void *context)
    {
	user_interaction *dialog = (user_interaction *)(context);
	const std::string & flag =
	    entry.get_data_flag()
	    + entry.get_delta_flag()
	    + entry.get_ea_flag()
	    + entry.get_fsa_flag()
	    + entry.get_compression_ratio_flag()
	    + entry.get_sparse_flag();
	const std::string & perm = entry.get_perm();
	const std::string & uid = entry.get_uid(true);
	const std::string & gid = entry.get_gid(true);
	const std::string & size = entry.get_file_size(true); //<<<<< listing sizes in bytes
	const std::string & date = entry.get_last_modif();
	const std::string & filename = entry.get_name();
	bool is_dir = entry.is_dir();
	bool has_children = !entry.is_empty_dir();

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
