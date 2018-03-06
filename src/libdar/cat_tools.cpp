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
    const U_8 SAVED_FAKE_BIT = 0x80;
    const U_8 SAVED_NON_DELTA_BIT = 0x40;
    // in ASCII lower case letter have binary values 11xxxxx and higher case
    // have binary values 10xxxxx The the 7th byte is always set to 1. We can
    // thus play with it to store the delta diff flag:
    // - when set to 1, this is a non delta binary file (backward compatibility)
    //   and the signature byte is a letter (lower or higher case to tell whether
    //   the inode is saved or not
    // - when set to 0, this is a delta saved file, setting it back to 1 should
    //   give a lower case letter corresponding to the inode type.


	// local sub routines

    static string local_fsa_fam_to_string(const cat_inode & ref);

	// exported routine implementation

    string local_perm(const cat_inode &ref, bool hard)
    {
	saved_status st;
	char type;

	U_32 perm = ref.get_perm();
	if(!extract_base_and_status(ref.signature(), (unsigned char &)type, st))
	    throw SRC_BUG;

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
	cat_inode::ea_status ea_st = ref.ea_get_saved_status();

	if(isolated && st == s_saved && !dirty)
	    st = s_fake;

	if(isolated && ea_st == cat_inode::ea_full)
	    ea_st = cat_inode::ea_fake;

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
	case s_delta:
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
	case cat_inode::ea_full:
	    ret += gettext("[Saved]");
	    break;
	case cat_inode::ea_fake:
	    ret += gettext("[InRef]");
	    break;
	case cat_inode::ea_partial:
	    ret += "[     ]";
	    break;
	case cat_inode::ea_none:
	    ret += "       ";
	    break;
	case cat_inode::ea_removed:
	    ret += "[Suppr]";
	    break;
	default:
	    throw SRC_BUG;
	}

	ret += "[" + local_fsa_fam_to_string(ref) + "]";
	const cat_file *fic = dynamic_cast<const cat_file *>(&ref);
	const cat_directory *dir = dynamic_cast<const cat_directory *>(&ref);
	if(fic != nullptr &&
	   (fic->get_saved_status() == s_saved ||fic->get_saved_status() == s_delta))
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

	bool go_ea = list_ea && e_ino != nullptr && e_ino->ea_get_saved_status() == cat_inode::ea_full;
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

    bool extract_base_and_status(unsigned char signature, unsigned char & base, saved_status & saved)
    {
        bool fake = (signature & SAVED_FAKE_BIT) != 0;
	bool non_delta = (signature & SAVED_NON_DELTA_BIT) != 0;

        signature &= ~SAVED_FAKE_BIT;
	signature |= SAVED_NON_DELTA_BIT;
        if(!isalpha(signature))
            return false;
        base = tolower(signature);

        if(fake)
            if(base == signature)
		if(non_delta)
		    saved = s_fake;
		else
		    return false; // cannot be s_fake and s_delta at the same time
	    else
		return false;
        else
            if(signature == base)
		if(non_delta)
		    saved = s_saved;
		else
		    saved = s_delta;
            else
		if(non_delta)
		    saved = s_not_saved;
		else
		    return false; // cannot be s_not_saved and s_delta at the same time
        return true;
    }

    bool extract_base_and_status_isolated(unsigned char sig, unsigned char & base, saved_status & state, bool isolated)
    {
	bool ret = extract_base_and_status(sig, base, state);

	if(isolated)
	    state = s_fake;

	return ret;
    }

    void local_display_ea(user_interaction & dialog,
			  const cat_inode * ino,
			  const string &prefix,
			  const string &suffix,
			  bool xml_output)
    {
	if(ino == nullptr)
	    return;

	if(ino->ea_get_saved_status() == cat_inode::ea_full)
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

    unsigned char mk_signature(unsigned char base, saved_status state)
    {
        if(! islower(base))
            throw SRC_BUG;
        switch(state)
        {
        case s_saved:
            return base;
        case s_fake:
            return base | SAVED_FAKE_BIT;
        case s_not_saved:
            return toupper(base);
	case s_delta:
	    return base & ~SAVED_NON_DELTA_BIT;
        default:
            throw SRC_BUG;
        }
    }

    bool compatible_signature(unsigned char a, unsigned char b)
    {
        a = get_base_signature(a);
        b = get_base_signature(b);

        switch(a)
        {
        case 'e':
        case 'f':
            return b == 'e' || b == 'f';
        default:
            return b == a;
        }
    }

    unsigned char get_base_signature(unsigned char a)
    {
	unsigned char ret;
	saved_status st;
	if(!extract_base_and_status_isolated(a, ret, st, false))
	    throw SRC_BUG;
	if(ret == 'e')
	    ret = 'f';

	return ret;
    }

    string entree_to_string(const cat_entree *obj)
    {
	string ret;
	if(obj == nullptr)
	    throw SRC_BUG;

	switch(get_base_signature(obj->signature()))
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

	if(ref.fsa_get_saved_status() != cat_inode::fsa_none)
	{
	    fsa_scope sc = ref.fsa_get_families();
	    bool upper = ref.fsa_get_saved_status() == cat_inode::fsa_full;
	    ret = fsa_scope_to_string(upper, sc);
	    if(ret.size() < 3)
		ret += "-";
	}
	else
	    ret = "---";

	return ret;
    }

} // end of namespace

