//*********************************************************************/
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

#include "erreurs.hpp"
#include "tools.hpp"
#include "deci.hpp"
#include "cat_all_entrees.hpp"

#include "semaphore.hpp"

using namespace std;

namespace libdar
{

    semaphore::semaphore(user_interaction & dialog,
			 const string & backup_hook_file_execute,
			 const mask & backup_hook_file_mask) : mem_ui(dialog)
    {
	count = 0;
	chem = "";
	filename = "";
	uid = 0;
	gid = 0;
	execute = backup_hook_file_execute;
	match = backup_hook_file_mask.clone();
	if(match == NULL)
	    throw Ememory("semaphore::semaphore");
    }

    void semaphore::raise(const string & x_chem,
			  const cat_entree *object,
			  bool data_to_save)
    {
	if(count == 1)
	    throw SRC_BUG;
	if(count > 1) // outer directory inder controlled backup (execute ran in "start" context)
	{
	    if(dynamic_cast<const cat_eod *>(object) != NULL)
		if(data_to_save)
		    --count;
		else
		    throw SRC_BUG; // CAT_EOD should always have its data to be saved
	    else
	    {
		if(dynamic_cast<const cat_directory *>(object) != NULL)
		    ++count;
	    }
	}
	else // semaphore is ready for a new entry
	{
	    const cat_nomme *o_nom = dynamic_cast<const cat_nomme *>(object);

	    if(o_nom == NULL)
		return; // CAT_EOD associated to an unsaved directory

	    if(data_to_save && match->is_covered(x_chem))
	    {
		const cat_directory *o_dir = dynamic_cast<const cat_directory *>(object);
		const cat_inode *o_ino = dynamic_cast<const cat_inode *>(object);

		if(o_dir != NULL)
		    count = 2;
		else
		    count = 1;

		chem = x_chem;
		filename = o_nom->get_name();
		uid = o_ino != NULL ? o_ino->get_uid() : 0;
		gid = o_ino != NULL ? o_ino->get_gid() : 0;
		tools_hook_execute(get_ui(), build_string("start"));
	    }
	}
    }

    void semaphore::lower()
    {
	if(count == 1)
	{
	    count = 0;
	    tools_hook_execute(get_ui(), build_string("end"));
	}
    }

    string semaphore::build_string(const string & context)
    {
	map<char, string> corres;

	corres['%'] = "%";
	corres['p'] = chem;
	corres['f'] = filename;
	corres['c'] = context;

	try
	{
	    deci gid_string = gid;
	    deci uid_string = uid;

	    corres['u'] = uid_string.human();
	    corres['g'] = gid_string.human();
	}
	catch(Edeci & e)
	{
	    throw Edeci("semaphore::build_string", string(gettext("Error while converting UID/GID to string for backup hook file: ")) + e.get_message());
	}

	return tools_substitute(execute, corres);
    }

    void semaphore::copy_from(const semaphore & ref)
    {
	count = ref.count;
	chem = ref.chem;
	filename = ref.filename;
	uid = ref.uid;
	gid = ref.gid;
	execute = ref.execute;
	if(ref.match == NULL)
	    throw SRC_BUG;
	match = ref.match->clone();
	if(match == NULL)
	    throw Ememory("semaphore::copy_from");
    }

    void semaphore::detruit()
    {
	if(match != NULL)
	{
	    delete match;
	    match = NULL;
	}
    }

} // end of namespace

