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

#include "cat_tools.hpp"
#include "cat_all_entrees.hpp"
#include "deci.hpp"

using namespace std;

namespace libdar
{

	// local sub routines

    static string local_fsa_fam_to_string(const inode & ref);

	// exported routine implementation

    string local_perm(const inode &ref, bool hard)
    {
	saved_status st;
	char type;

	U_32 perm = ref.get_perm();
	if(!extract_base_and_status(ref.signature(), (unsigned char &)type, st))
	    throw SRC_BUG;

	return tools_get_permission_string(type, perm, hard);
    }

    string local_uid(const inode & ref)
    {
	return tools_name_of_uid(ref.get_uid());
    }

    string local_gid(const inode & ref)
    {
	return tools_name_of_gid(ref.get_gid());
    }

    string local_size(const inode & ref)
    {
	string ret;

	const file *fic = dynamic_cast<const file *>(&ref);
	const directory *dir = dynamic_cast<const directory *>(&ref);
	if(fic != NULL)
	    ret = tools_display_integer_in_metric_system(fic->get_size(), "o", true);
	else if(dir != NULL)
	    ret = tools_display_integer_in_metric_system(dir->get_size(), "o", true);
	else
	    ret = "0";

	return ret;
    }

    string local_storage_size(const inode & ref)
    {
	string ret;

	const file *fic = dynamic_cast<const file*>(&ref);
	if(fic != NULL)
	{
	    deci d = fic->get_storage_size();
	    ret = d.human();
	}
	else
	    ret = "0";

	return ret;
    }

    string local_date(const inode & ref)
    {
	return tools_display_date(ref.get_last_modif());
    }

    string local_flag(const inode & ref, bool isolated, bool dirty_seq)
    {
	string ret;
	const file *ref_f = dynamic_cast<const file *>(&ref);
	bool dirty = dirty_seq || (ref_f != NULL ? ref_f->is_dirty() : false);
	saved_status st = ref.get_saved_status();
	inode::ea_status ea_st = ref.ea_get_saved_status();

	if(isolated && st == s_saved && !dirty)
	    st = s_fake;

	if(isolated && ea_st == inode::ea_full)
	    ea_st = inode::ea_fake;

	switch(st)
	{
	case s_saved:
	    if(dirty)
		ret = gettext("[DIRTY]");
	    else
		ret = gettext("[Saved]");
	    break;
	case s_fake:
	    ret = gettext("[InRef]");
	    break;
	case s_not_saved:
	    ret = "[     ]";
	    break;
	default:
	    throw SRC_BUG;
	}


	switch(ea_st)
	{
	case inode::ea_full:
	    ret += gettext("[Saved]");
	    break;
	case inode::ea_fake:
	    ret += gettext("[InRef]");
	    break;
	case inode::ea_partial:
	    ret += "[     ]";
	    break;
	case inode::ea_none:
	    ret += "       ";
	    break;
	case inode::ea_removed:
	    ret += "[Suppr]";
	    break;
	default:
	    throw SRC_BUG;
	}

	ret += "[" + local_fsa_fam_to_string(ref) + "]";
	const file *fic = dynamic_cast<const file *>(&ref);
	const directory *dir = dynamic_cast<const directory *>(&ref);
	if(fic != NULL && fic->get_saved_status() == s_saved)
	    ret += string("[")
		+ tools_get_compression_ratio(fic->get_storage_size(),
					      fic->get_size(),
					      fic->get_compression_algo_read() != none || fic->get_sparse_file_detection_read())
		+ "]";

	else if(dir != NULL)
	    ret += string("[")
		+ tools_get_compression_ratio(dir->get_storage_size(),
					      dir->get_size(),
					      true)
		+ "]";
	else
	    ret += "[-----]";

	if(fic != NULL && fic->get_sparse_file_detection_read())
	    ret += "[X]";
	else
	    ret += "[ ]";

	return ret;
    }

    void xml_listing_attributes(user_interaction & dialog,
				const string & beginning,
				const string & data,
				const string & metadata,
				const cat_entree * obj,
				bool list_ea)
    {
	string user;
	string group;
	string permissions;
	string atime;
	string mtime;
	string ctime;
	const inode *e_ino = dynamic_cast<const inode *>(obj);
	const mirage *e_hard = dynamic_cast<const mirage *>(obj);

	if(e_hard != NULL)
	    e_ino = e_hard->get_inode();

	if(e_ino != NULL)
	{
	    user = local_uid(*e_ino);
	    group = local_gid(*e_ino);
	    permissions = local_perm(*e_ino, e_hard != NULL);
	    atime = deci(e_ino->get_last_access().get_second_value()).human();
	    mtime = deci(e_ino->get_last_modif().get_second_value()).human();
	    if(e_ino->has_last_change())
	    {
		ctime = deci(e_ino->get_last_change().get_second_value()).human();
		if(ctime == "0")
		    ctime = "";
	    }
	    else
		ctime = "";
	}
	else
	{
	    user = "";
	    group = "";
	    permissions = "";
	    atime = "";
	    mtime = "";
	    ctime = "";
	}

	dialog.printf("%S<Attributes data=\"%S\" metadata=\"%S\" user=\"%S\" group=\"%S\" permissions=\"%S\" atime=\"%S\" mtime=\"%S\" ctime=\"%S\" />\n",
		      &beginning, &data, &metadata, &user, &group, &permissions, &atime, &mtime, &ctime);
	if(list_ea && e_ino != NULL && e_ino->ea_get_saved_status() == inode::ea_full)
	{
	    string new_begin = beginning + "\t";
	    local_display_ea(dialog, e_ino, new_begin + "<EA_entry> ea_name=\"", "\">", true);
	    dialog.printf("%S</Attributes>", &beginning);
	}
    }


    bool extract_base_and_status(unsigned char signature, unsigned char & base, saved_status & saved)
    {
	bool fake = (signature & SAVED_FAKE_BIT) != 0;

	signature &= ~SAVED_FAKE_BIT;
	if(!isalpha(signature))
	    return false;
	base = tolower(signature);

	if(fake)
	    if(base == signature)
		saved = s_fake;
	    else
		return false;
	else
	    if(signature == base)
		saved = s_saved;
	    else
		saved = s_not_saved;
	return true;
    }


    void local_display_ea(user_interaction & dialog,
			  const inode * ino,
			  const string &prefix,
			  const string &suffix,
			  bool xml_output)
    {
	if(ino == NULL)
	    return;

	if(ino->ea_get_saved_status() == inode::ea_full)
	{
	    const ea_attributs *owned = ino->get_ea();
	    string key, val;

	    if(owned == NULL)
		throw SRC_BUG;

	    owned->reset_read();
	    while(owned->read(key, val))
	    {
		if(xml_output)
		    key = tools_output2xml(key);
		dialog.warning(prefix + key + suffix);
	    }
	}
    }

	// local routine implementation

    static string local_fsa_fam_to_string(const inode & ref)
    {
	string ret = "";

	if(ref.fsa_get_saved_status() != inode::fsa_none)
	{
	    fsa_scope sc = ref.fsa_get_families();
	    bool upper = ref.fsa_get_saved_status() == inode::fsa_full;
	    ret = fsa_scope_to_string(upper, sc);
	    if(ret.size() < 3)
		ret += "-";
	}
	else
	    ret = "---";

	return ret;
    }



} // end of namespace

