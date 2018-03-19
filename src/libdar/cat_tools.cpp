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

    static string local_fsa_fam_to_string(const cat_inode & ref);

	// exported routine implementation

    string local_perm(const cat_inode &ref, bool hard)
    {
	unsigned char type = ref.signature();
	U_32 perm = ref.get_perm();

	return tools_get_permission_string(type, perm, hard);
    }

    string local_uid(const cat_inode & ref)
    {
	return tools_name_of_uid(ref.get_uid());
    }

    string local_gid(const cat_inode & ref)
    {
	return tools_name_of_gid(ref.get_gid());
    }

    string local_size(const cat_inode & ref, bool sizes_in_bytes)
    {
	string ret;

	const cat_file *fic = dynamic_cast<const cat_file *>(&ref);
	const cat_directory *dir = dynamic_cast<const cat_directory *>(&ref);
	if(fic != nullptr)
	    if(sizes_in_bytes)
		ret = deci(fic->get_size()).human();
	    else
		ret = tools_display_integer_in_metric_system(fic->get_size(), "o", true);
	else
	    if(dir != nullptr)
		if(sizes_in_bytes)
		    ret = deci(dir->get_size()).human();
		else
		    ret = tools_display_integer_in_metric_system(dir->get_size(), "o", true);
	    else
		ret = "0";

	return ret;
    }

    string local_storage_size(const cat_inode & ref)
    {
	string ret;

	const cat_file *fic = dynamic_cast<const cat_file*>(&ref);
	if(fic != nullptr)
	{
	    deci d = fic->get_storage_size();
	    ret = d.human();
	}
	else
	    ret = "0";

	return ret;
    }

    string local_date(const cat_inode & ref)
    {
	return tools_display_date(ref.get_last_modif());
    }

    string local_flag(const cat_inode & ref, bool isolated, bool dirty_seq)
    {
	string ret;
	const cat_file *ref_f = dynamic_cast<const cat_file *>(&ref);
	bool dirty = dirty_seq || (ref_f != nullptr ? ref_f->is_dirty() : false);
	saved_status st = ref.get_saved_status();
	ea_saved_status ea_st = ref.ea_get_saved_status();

	if(isolated && st == saved_status::saved && !dirty)
	    st = saved_status::fake;

	if(isolated && ea_st == ea_saved_status::full)
	    ea_st = ea_saved_status::fake;

	switch(st)
	{
	case saved_status::saved:
	    if(dirty)
		ret = gettext("[DIRTY]");
	    else
		ret = gettext("[Saved]");
	    break;
	case saved_status::inode_only:
	    ret = gettext("[Inode]");
	    break;
	case saved_status::fake:
	    ret = gettext("[InRef]");
	    break;
	case saved_status::not_saved:
	    ret = "[     ]";
	    break;
	case saved_status::delta:
	    ret = "[Delta]";
	    break;
	default:
	    throw SRC_BUG;
	}

	if(ref_f == nullptr)
	    ret += "[-]";
	else
	{
	    if(ref_f->has_delta_signature_available())
		ret +="[D]";
	    else
		ret +="[ ]";
	}

	switch(ea_st)
	{
	case ea_saved_status::full:
	    ret += gettext("[Saved]");
	    break;
	case ea_saved_status::fake:
	    ret += gettext("[InRef]");
	    break;
	case ea_saved_status::partial:
	    ret += "[     ]";
	    break;
	case ea_saved_status::none:
	    ret += "       ";
	    break;
	case ea_saved_status::removed:
	    ret += "[Suppr]";
	    break;
	default:
	    throw SRC_BUG;
	}

	ret += "[" + local_fsa_fam_to_string(ref) + "]";
	const cat_file *fic = dynamic_cast<const cat_file *>(&ref);
	const cat_directory *dir = dynamic_cast<const cat_directory *>(&ref);
	if(fic != nullptr &&
	   (fic->get_saved_status() == saved_status::saved ||fic->get_saved_status() == saved_status::delta))
	    ret += string("[")
		+ tools_get_compression_ratio(fic->get_storage_size(),
					      fic->get_size(),
					      fic->get_compression_algo_read() != compression::none || fic->get_sparse_file_detection_read())
		+ "]";

	else if(dir != nullptr)
	    ret += string("[")
		+ tools_get_compression_ratio(dir->get_storage_size(),
					      dir->get_size(),
					      true)
		+ "]";
	else
	    ret += "[-----]";

	if(fic != nullptr && fic->get_sparse_file_detection_read())
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
	const cat_inode *e_ino = dynamic_cast<const cat_inode *>(obj);
	const cat_mirage *e_hard = dynamic_cast<const cat_mirage *>(obj);

	if(e_hard != nullptr)
	    e_ino = e_hard->get_inode();

	if(e_ino != nullptr)
	{
	    user = local_uid(*e_ino);
	    group = local_gid(*e_ino);
	    permissions = local_perm(*e_ino, e_hard != nullptr);
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

	bool go_ea = list_ea && e_ino != nullptr && e_ino->ea_get_saved_status() == ea_saved_status::full;
	string end_tag = go_ea ? ">" : " />";

	dialog.printf("%S<Attributes data=\"%S\" metadata=\"%S\" user=\"%S\" group=\"%S\" permissions=\"%S\" atime=\"%S\" mtime=\"%S\" ctime=\"%S\"%S",
		      &beginning, &data, &metadata, &user, &group, &permissions, &atime, &mtime, &ctime, &end_tag);
	if(go_ea)
	{
	    string new_begin = beginning + "\t";
	    local_display_ea(dialog, e_ino, new_begin + "<EA_entry ea_name=\"", "\" />", true);
	    dialog.printf("%S</Attributes>", &beginning);
	}
    }

    void local_display_ea(user_interaction & dialog,
			  const cat_inode * ino,
			  const string &prefix,
			  const string &suffix,
			  bool xml_output)
    {
	if(ino == nullptr)
	    return;

	if(ino->ea_get_saved_status() == ea_saved_status::full)
	{
	    const ea_attributs *owned = ino->get_ea();
	    string key, val;

	    if(owned == nullptr)
		throw SRC_BUG;

	    owned->reset_read();
	    while(owned->read(key, val))
	    {
		if(xml_output)
		    key = tools_output2xml(key);
		dialog.message(prefix + key + suffix);
	    }
	}
    }

    string entree_to_string(const cat_entree *obj)
    {
	string ret;
	if(obj == nullptr)
	    throw SRC_BUG;

	switch(obj->signature())
	{
	case 'j':
	    ret = gettext("ignored directory");
	    break;
	case 'd':
	    ret = gettext("folder");
	    break;
	case 'x':
	    ret = gettext("deleted file");
	    break;
	case 'o':
	    ret = gettext("door");
	    break;
	case 'f':
	    ret = gettext("file");
	    break;
	case 'l':
	    ret = gettext("symlink");
	    break;
	case 'c':
	    ret = gettext("char device");
	    break;
	case 'b':
	    ret = gettext("block device");
	    break;
	case 'p':
	    ret = gettext("pipe");
	    break;
	case 's':
	    ret = gettext("socket");
	    break;
	case 'i':
	    ret = gettext("ignored entry");
	    break;
	case 'm':
	    ret = gettext("hard linked inode");
	    break;
	case 'z':
	    ret = gettext("end of directory");
	    break;
	default:
	    throw SRC_BUG; // missing inode type
	}

	return ret;
    }

	// local routine implementation

    static string local_fsa_fam_to_string(const cat_inode & ref)
    {
	string ret = "";

	if(ref.fsa_get_saved_status() != fsa_saved_status::none)
	{
	    fsa_scope sc = ref.fsa_get_families();
	    bool upper = ref.fsa_get_saved_status() == fsa_saved_status::full;
	    ret = fsa_scope_to_string(upper, sc);
	    if(ret.size() < 3)
		ret += "-";
	}
	else
	    ret = "---";

	return ret;
    }

} // end of namespace

