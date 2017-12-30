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

#include "crit_action.hpp"
#include "nls_swap.hpp"
#include "cat_all_entrees.hpp"

using namespace std;

namespace libdar
{
    static const char *signature2string(unsigned char sign);

    testing::testing(const criterium & input, const crit_action & go_true, const crit_action & go_false)
    {
	x_input = input.clone();
	x_go_true = go_true.clone();
	x_go_false = go_false.clone();
	if(!check())
	{
	    free();
	    throw Ememory("testing::testing");
	}
    }

    void testing::free() noexcept
    {
	if(x_input != nullptr)
	{
	    delete x_input;
	    x_input = nullptr;
	}
	if(x_go_true != nullptr)
	{
	    delete x_go_true;
	    x_go_true = nullptr;
	}
	if(x_go_false != nullptr)
	{
	    delete x_go_false;
	    x_go_false = nullptr;
	}
    }

    void testing::copy_from(const testing & ref)
    {
	x_input = ref.x_input->clone();
	x_go_true = ref.x_go_true->clone();
	x_go_false = ref.x_go_false->clone();
	if(!check())
	{
	    free();
	    throw Ememory("testing::copy_from");
	}
    }

    void testing::move_from(testing && ref) noexcept
    {
	swap(x_input, ref.x_input);
	swap(x_go_true, ref.x_go_true);
	swap(x_go_false, ref.x_go_false);
    }

    bool testing::check() const
    {
	return x_input != nullptr && x_go_true != nullptr && x_go_false != nullptr;
    }

    void crit_chain::add(const crit_action & act)
    {
	crit_action *tmp = act.clone();
	if(tmp == nullptr)
	    throw Ememory("crit_chain::add");
	sequence.push_back(tmp);
    }

    void crit_chain::gobe(crit_chain & to_be_voided)
    {
	deque<crit_action *>::iterator it = to_be_voided.sequence.begin();

	try
	{
	    while(it != to_be_voided.sequence.end())
	    {
		if(*it == nullptr)
		    throw SRC_BUG;
		sequence.push_back(*it);
		++it;
	    }
	    to_be_voided.sequence.clear();
	}
	catch(...)
	{
	    if(it != to_be_voided.sequence.end() && sequence.back() == *it)
		++it;
	    while(it != to_be_voided.sequence.end())
	    {
		if(*it != nullptr)
		{
		    delete *it;
		    *it = nullptr;
		}
		++it;
	    }
	    to_be_voided.sequence.clear();
	    throw;
	}

    }

    void crit_chain::get_action(const cat_nomme & first, const cat_nomme & second, over_action_data & data, over_action_ea & ea) const
    {
 	NLS_SWAP_IN;
	try
	{
	    deque<crit_action *>::const_iterator it = sequence.begin();
	    over_action_data tmp_data;
	    over_action_ea tmp_ea;

	    data = data_undefined;
	    ea = EA_undefined;
	    if(it == sequence.end())
		throw Erange("crit_chain::get_action", gettext("cannot evaluate an empty chain in an overwriting policy"));

	    while(it != sequence.end() && (data == data_undefined || ea == EA_undefined))
	    {
		if(*it == nullptr)
		    throw SRC_BUG;
		(*it)->get_action(first, second, tmp_data, tmp_ea);
		if(data == data_undefined || tmp_data != data_undefined)
		    data = tmp_data;
		if(ea == EA_undefined || tmp_ea != EA_undefined)
		    ea = tmp_ea;
		++it;
	    }
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void crit_chain::destroy()
    {
	deque<crit_action *>::iterator it = sequence.begin();

	while(it != sequence.end())
	{
	    if(*it != nullptr)
	    {
		delete *it;
		*it = nullptr;
	    }
	    ++it;
	}

	sequence.clear();
    }

    void crit_chain::copy_from(const crit_chain & ref)
    {
	deque<crit_action *>::const_iterator it = ref.sequence.begin();
	crit_action * tmp = nullptr;
	sequence.clear();

	try
	{
	    while(it != ref.sequence.end())
	    {
		if(*it == nullptr)
		    throw SRC_BUG;
		tmp = (*it)->clone();
		if(tmp == nullptr)
		    throw Ememory("crit_chain::copy_from");
		sequence.push_back(tmp);
		tmp = nullptr;
		++it;
	    }
	}
	catch(...)
	{
	    destroy();
	    if(tmp != nullptr)
		delete tmp;
	    throw;
	}
    }

    over_action_data crit_ask_user_for_data_action(user_interaction & dialog, const string & full_name, const cat_entree *already_here, const cat_entree *dolly)
    {
	over_action_data ret = data_undefined;

	NLS_SWAP_IN;
	try
	{

	    const string confirm = gettext("yes");
	    bool loop = true;

	    string resp;

	    while(loop)
	    {
		dialog.printf(gettext("Conflict found while selecting the file to retain in the resulting archive:"));
		dialog.printf(gettext("User Decision requested for data of file %S"), &full_name);
		crit_show_entry_info(dialog, full_name, already_here, dolly);

		resp = dialog.get_string(gettext("\nYour decision about file's data:\n[P]reserve\n[O]verwrite\nmark [S]aved and preserve\nmark saved and overwri[T]e\n[R]emove\n[*] keep undefined\n[A]bort\n Your Choice? "), true);
		if(resp.size() != 1)
		    dialog.warning(gettext("Please answer by the character between brackets ('[' and ']') and press return"));
		else
		{
		    switch(*resp.begin())
		    {
		    case 'P':
			ret = data_preserve;
			loop = false;
			break;
		    case 'O':
			ret = data_overwrite;
			loop = false;
			break;
		    case 'S':
			ret = data_preserve_mark_already_saved;
			loop = false;
			break;
		    case 'T':
			ret = data_overwrite_mark_already_saved;
			loop = false;
			break;
		    case 'R':
			ret = data_remove;
			loop = false;
			break;
		    case '*':
			ret = data_undefined;
			loop = false;
			break;
		    case 'A':
			resp = dialog.get_string(tools_printf(gettext("Warning, are you sure you want to abort (please answer \"%S\" to confirm)? "), &confirm), true);
			if(resp == confirm)
			    throw Ethread_cancel(false, 0);
			else
			    dialog.warning(gettext("Cancellation no confirmed"));
			break;
		    default:
			dialog.warning(string(gettext("Unknown choice: ")) + resp);
		    }
		}
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
    }

    over_action_ea crit_ask_user_for_EA_action(user_interaction & dialog, const string & full_name, const cat_entree *already_here, const cat_entree *dolly)
    {
	over_action_ea ret = EA_undefined;

	NLS_SWAP_IN;
	try
	{
	    const string confirm = gettext("yes");
	    bool loop = true;
	    string resp;

	    while(loop)
	    {
		dialog.printf(gettext("Conflict found while selecting the file to retain in the resulting archive:"));
		dialog.printf(gettext("User Decision requested for EA of file %S"), &full_name);
		crit_show_entry_info(dialog, full_name, already_here, dolly);

		resp = dialog.get_string(gettext("\nYour decision about file's EA:\n[p]reserve\n[o]verwrite\nmark [s]aved and preserve\nmark saved and overwri[t]e\n[m]erge EA and preserve\nmerge EA a[n]d overwrite\n[r]emove\n[*] keep undefined\n[a]bort\n  Your choice? "), true);
		if(resp.size() != 1)
		    dialog.warning(gettext("Please answer by the character between brackets ('[' and ']') and press return"));
		else
		{
		    switch(*resp.begin())
		    {
		    case 'p':
			ret = EA_preserve;
			loop = false;
			break;
		    case 'o':
			ret = EA_overwrite;
			loop = false;
			break;
		    case 's':
			ret = EA_preserve_mark_already_saved;
			loop = false;
			break;
		    case 't':
			ret = EA_overwrite_mark_already_saved;
			loop = false;
			break;
		    case 'm':
			ret = EA_merge_preserve;
			loop = false;
			break;
		    case 'n':
			ret = EA_merge_overwrite;
			loop = false;
			break;
		    case 'r':
			ret = EA_clear;
			loop = false;
			break;
		    case '*':
			ret = EA_undefined;
			loop = false;
			break;
		    case 'a':
			resp = dialog.get_string(tools_printf(gettext("Warning, are you sure you want to abort (please answer \"%S\" to confirm)? "), &confirm), true);
			if(resp == confirm)
			    throw Ethread_cancel(false, 0);
			else
			    dialog.warning(gettext("Cancellation no confirmed"));
			break;
		    default:
			dialog.warning(string(gettext("Unknown choice: ")) + resp);
		    }
		}
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
    }


    over_action_ea crit_ask_user_for_FSA_action(user_interaction & dialog, const string & full_name, const cat_entree *already_here, const cat_entree *dolly)
    {
	over_action_ea ret = EA_undefined;

	NLS_SWAP_IN;
	try
	{
	    const string confirm = gettext("yes");
	    bool loop = true;
	    string resp;

	    while(loop)
	    {
		dialog.printf(gettext("Conflict found while selecting the file to retain in the resulting archive:"));
		dialog.printf(gettext("User Decision requested for FSA of file %S"), &full_name);
		crit_show_entry_info(dialog, full_name, already_here, dolly);

		resp = dialog.get_string(gettext("\nYour decision about file's FSA:\n[p]reserve\n[o]verwrite\nmark [s]aved and preserve\nmark saved and overwri[t]e\n[*] keep undefined\n[a]bort\n  Your choice? "), true);
		if(resp.size() != 1)
		    dialog.warning(gettext("Please answer by the character between brackets ('[' and ']') and press return"));
		else
		{
		    switch(*resp.begin())
		    {
		    case 'p':
			ret = EA_preserve;
			loop = false;
			break;
		    case 'o':
			ret = EA_overwrite;
			loop = false;
			break;
		    case 's':
			ret = EA_preserve_mark_already_saved;
			loop = false;
			break;
		    case 't':
			ret = EA_overwrite_mark_already_saved;
			loop = false;
			break;
		    case '*':
			ret = EA_undefined;
			loop = false;
			break;
		    case 'a':
			resp = dialog.get_string(tools_printf(gettext("Warning, are you sure you want to abort (please answer \"%S\" to confirm)? "), &confirm), true);
			if(resp == confirm)
			    throw Ethread_cancel(false, 0);
			else
			    dialog.warning(gettext("Cancellation no confirmed"));
			break;
		    default:
			dialog.warning(string(gettext("Unknown choice: ")) + resp);
		    }
		}
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
    }


    void crit_show_entry_info(user_interaction & dialog, const string & full_name, const cat_entree *already_here, const cat_entree *dolly)
    {
	NLS_SWAP_IN;
	try
	{
	    const string yes = gettext("YES");
	    const string no = gettext("NO");

	    const cat_inode * al_inode = dynamic_cast<const cat_inode *>(already_here);
	    const cat_directory * al_directory = dynamic_cast<const cat_directory *>(already_here);
	    const cat_file * al_file = dynamic_cast<const cat_file *>(already_here);
	    const cat_mirage * al_mirage = dynamic_cast<const cat_mirage *>(already_here);

	    const cat_inode * do_inode = dynamic_cast<const cat_inode *>(dolly);
	    const cat_directory * do_directory = dynamic_cast<const cat_directory *>(dolly);
	    const cat_file * do_file = dynamic_cast<const cat_file *>(dolly);
	    const cat_mirage * do_mirage = dynamic_cast<const cat_mirage *>(dolly);

	    dialog.printf(gettext("Entry information:\t\"in place\"\t\"to be added\""));
	    dialog.printf(gettext("Is inode         :\t  %S  \t\t  %S"), al_inode == nullptr ? &no : &yes , do_inode == nullptr ? &no : &yes);
	    dialog.printf(gettext("Is directory     :\t  %S  \t\t  %S"), al_directory == nullptr ? &no : &yes , do_directory == nullptr ? &no : &yes);
	    dialog.printf(gettext("Is plain file    :\t  %S  \t\t  %S"), al_file == nullptr ? &no : &yes , do_file == nullptr ? &no : &yes);
	    dialog.printf(gettext("Is hard linked   :\t  %S  \t\t  %S"), al_mirage == nullptr ? &no : &yes , do_mirage == nullptr ? &no : &yes);
	    dialog.printf(gettext("Entry type       :\t  %s  \t  %s"), signature2string(already_here->signature()), signature2string(dolly->signature()));

	    if(al_inode != nullptr && do_inode != nullptr)
	    {
		const string me = gettext("me");
		const string notme = "";
		bool in_place_data_recent = al_inode->get_last_modif() >= do_inode->get_last_modif();
		bool in_place_ea_recent = al_inode->get_last_change() >= do_inode->get_last_change();
		bool al_ea_saved = al_inode->ea_get_saved_status() == cat_inode::ea_full;
		bool do_ea_saved = do_inode->ea_get_saved_status() == cat_inode::ea_full;
		bool al_fsa_saved = al_inode->fsa_get_saved_status() == cat_inode::fsa_full;
		bool do_fsa_saved = do_inode->fsa_get_saved_status() == cat_inode::fsa_full;

		dialog.printf(gettext("Data more recent :\t  %S  \t\t  %S"), in_place_data_recent ? &me : &notme , in_place_data_recent ? &notme : &me);
		if(al_file != nullptr && do_file != nullptr)
		{
		    infinint al_size = al_file->get_size();
		    infinint do_size = do_file->get_size();
		    bool al_dirty = al_file->is_dirty();
		    bool do_dirty = do_file->is_dirty();
		    bool al_sparse = al_file->get_sparse_file_detection_read();
		    bool do_sparse = do_file->get_sparse_file_detection_read();

		    dialog.printf(gettext("Data size        :\t  %i  \t\t  %i"), &al_size, &do_size);
		    dialog.printf(gettext("Sparse file      :\t  %S  \t\t  %S"), al_sparse ? &yes : &no, do_sparse ? &yes : &no);
		    dialog.printf(gettext("Dirty file       :\t  %S  \t\t  %S"), al_dirty ? &yes : &no, do_dirty ? &yes : &no);
		}
		dialog.printf(gettext("Data full saved  :\t  %S  \t\t  %S"),al_inode->get_saved_status() == s_saved ? &yes:&no , do_inode->get_saved_status() == s_saved ? &yes:&no);
		dialog.printf(gettext("EA full saved    :\t  %S  \t\t  %S"),al_ea_saved ? &yes:&no , do_ea_saved ? &yes:&no);
		if(al_ea_saved || do_ea_saved)
		    dialog.printf(gettext("EA more recent   :\t  %S  \t\t  %S"),in_place_ea_recent ? &me : &notme , in_place_data_recent ? &notme : &me);
		dialog.printf(gettext("FSA full saved   :\t  %S  \t\t  %S"), al_fsa_saved ? &yes:&no , do_fsa_saved ? &yes:&no);
		if(al_fsa_saved || do_fsa_saved)
		{
		    string al_fam = al_fsa_saved ? fsa_scope_to_string(al_fsa_saved, al_inode->fsa_get_families()) : "-";
		    string do_fam = do_fsa_saved ? fsa_scope_to_string(do_fsa_saved, do_inode->fsa_get_families()) : "-";
		    dialog.printf(gettext("FSA familly      :\t  %S  \t\t  %S"), &al_fam, &do_fam);
		}

		if(al_ea_saved && do_ea_saved)
		{
		    const ea_attributs *al_ea = al_inode->get_ea();
		    const ea_attributs *do_ea = do_inode->get_ea();
		    infinint al_tmp = al_ea->size();
		    infinint do_tmp = do_ea->size();
		    dialog.printf(gettext("EA number        :\t  %i  \t\t  %i"), &al_tmp, &do_tmp);
		    al_tmp = al_ea->space_used();
		    do_tmp = do_ea->space_used();
		    dialog.printf(gettext("EA size          :\t  %i  \t\t  %i"), &al_tmp, &do_tmp);
		}
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    static const char *signature2string(unsigned char sign)
    {
	unsigned char normalized_sig = toupper(sign);

	switch(normalized_sig)
	{
	case 'D':
	    return gettext("directory");
	case 'Z':
	    throw SRC_BUG; // EOD should never be considered by overwriting policy
	case 'M':
	    return gettext("hard linked inode");
	case 'F':
	    return gettext("plain file");
	case 'L':
	    return gettext("soft link");
	case 'C':
	    return gettext("char device");
	case 'B':
	    return gettext("block device");
	case 'P':
	    return gettext("named pipe");
	case 'S':
	    return gettext("unix socket");
	case 'X':
	    return gettext("deleted entry");
	case 'O':
	    return gettext("door inode");
	case 'I':
	    throw SRC_BUG; // ignored entry should never be found in an archive
	case 'J':
	    throw SRC_BUG; // ignored directory entry should never be found in an archive
	default:
	    throw SRC_BUG; // unknown entry type
	}
    }

} // end of namespace
