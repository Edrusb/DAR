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

#include "criterium.hpp"
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


    void testing::free()
    {
	if(x_input != NULL)
	{
	    delete x_input;
	    x_input = NULL;
	}
	if(x_go_true != NULL)
	{
	    delete x_go_true;
	    x_go_true = NULL;
	}
	if(x_go_false != NULL)
	{
	    delete x_go_false;
	    x_go_false = NULL;
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

    bool testing::check() const
    {
	return x_input != NULL && x_go_true != NULL && x_go_false != NULL;
    }

    void crit_chain::add(const crit_action & act)
    {
	crit_action *tmp = act.clone();
	if(tmp == NULL)
	    throw Ememory("crit_chain::add");
	sequence.push_back(tmp);
    }

    void crit_chain::gobe(crit_chain & to_be_voided)
    {
	vector<crit_action *>::iterator it = to_be_voided.sequence.begin();

	try
	{
	    while(it != to_be_voided.sequence.end())
	    {
		if(*it == NULL)
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
		if(*it != NULL)
		{
		    delete *it;
		    *it = NULL;
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
	    vector<crit_action *>::const_iterator it = sequence.begin();
	    over_action_data tmp_data;
	    over_action_ea tmp_ea;

	    data = data_undefined;
	    ea = EA_undefined;
	    if(it == sequence.end())
		throw Erange("crit_chain::get_action", gettext("cannot evaluate an empty chain in an overwriting policy"));

	    while(it != sequence.end() && (data == data_undefined || ea == EA_undefined))
	    {
		if(*it == NULL)
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
	vector<crit_action *>::iterator it = sequence.begin();

	while(it != sequence.end())
	{
	    if(*it != NULL)
	    {
		delete *it;
		*it = NULL;
	    }
	    ++it;
	}

	sequence.clear();
    }

    void crit_chain::copy_from(const crit_chain & ref)
    {
	vector<crit_action *>::const_iterator it = ref.sequence.begin();
	crit_action * tmp = NULL;
	sequence.clear();

	try
	{
	    while(it != ref.sequence.end())
	    {
		if(*it == NULL)
		    throw SRC_BUG;
		tmp = (*it)->clone();
		if(tmp == NULL)
		    throw Ememory("crit_chain::copy_from");
		sequence.push_back(tmp);
		tmp = NULL;
		++it;
	    }
	}
	catch(...)
	{
	    destroy();
	    if(tmp != NULL)
		delete tmp;
	    throw;
	}
    }


	/////////////////////////////////////////////////////////////////////
	//////////// implementation of criterium classes follows ////////////
        /////////////////////////////////////////////////////////////////////

    const cat_inode *criterium::get_inode(const cat_nomme *arg)
    {
	const cat_inode *ret;
	const cat_mirage *arg_m = dynamic_cast<const cat_mirage *>(arg);

	if(arg_m != NULL)
	    ret = const_cast<const cat_inode *>(arg_m->get_inode());
	else
	    ret = dynamic_cast<const cat_inode *>(arg);

	return ret;
    }

    bool crit_in_place_is_inode::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	return dynamic_cast<const cat_inode *>(&first) != NULL
	    || dynamic_cast<const cat_mirage *>(&first) != NULL;
    }

    bool crit_in_place_is_file::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);

	return dynamic_cast<const file *>(first_i) != NULL && dynamic_cast<const door *>(first_i) == NULL;
    }

    bool crit_in_place_is_hardlinked_inode::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	return dynamic_cast<const cat_mirage *>(&first) != NULL;
    }

    bool crit_in_place_is_new_hardlinked_inode::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_mirage * tmp = dynamic_cast<const cat_mirage *>(&first);
	return tmp != NULL && tmp->is_first_mirage();
    }

    bool crit_in_place_data_more_recent::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const cat_inode *second_i = get_inode(&second);
	datetime first_date = first_i != NULL ? first_i->get_last_modif() : datetime(0);
	datetime second_date = second_i != NULL ? second_i->get_last_modif() : datetime(0);

	return first_i == NULL || first_date >= second_date || tools_is_equal_with_hourshift(x_hourshift, first_date, second_date);
    }

    bool crit_in_place_data_more_recent_or_equal_to::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	datetime first_date = first_i != NULL ? first_i->get_last_modif() : datetime(0);

	return first_i == NULL || first_date >= x_date || tools_is_equal_with_hourshift(x_hourshift, first_date, x_date);
    }

    bool crit_in_place_data_bigger::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const cat_inode *second_i = get_inode(&second);
	const file *first_f = dynamic_cast<const file *>(first_i);
	const file *second_f = dynamic_cast<const file *>(second_i);

	if(first_f != NULL && second_f != NULL)
	    return first_f->get_size() >= second_f->get_size();
	else
	    return true;
    }

    bool crit_in_place_data_saved::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);

	if(first_i != NULL)
	    return first_i->get_saved_status() == s_saved;
	else
	    return true;
    }

    bool crit_in_place_data_dirty::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const file *first_f = dynamic_cast<const file *>(first_i);

	if(first_f != NULL)
	    return first_f->is_dirty();
	else
	    return false;
    }

    bool crit_in_place_data_sparse::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const file *first_f = dynamic_cast<const file *>(first_i);

	if(first_f != NULL)
	    return first_f->get_sparse_file_detection_read();
	else
	    return false;
    }

    bool crit_in_place_EA_more_recent::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const cat_inode *second_i = get_inode(&second);

	datetime ctime_f, ctime_s;

	if(first_i != NULL)
	{
	    switch(first_i->ea_get_saved_status())
	    {
	    case cat_inode::ea_none:
	    case cat_inode::ea_removed:
		ctime_f = datetime(0);
		break;
	    default:
		ctime_f = first_i->get_last_change();
	    }
	}
	else
	    ctime_f = datetime(0);


	if(second_i != NULL)
	{
	    switch(second_i->ea_get_saved_status())
	    {
	    case cat_inode::ea_none:
	    case cat_inode::ea_removed:
		return true;
		break;
	    default:
		ctime_s = second_i->get_last_change();
	    }
	}
	else
	    return true;

	return ctime_f >= ctime_s || tools_is_equal_with_hourshift(x_hourshift, ctime_f, ctime_s);
    }


    bool crit_in_place_EA_more_recent_or_equal_to::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);

	datetime ctime_f;

	if(first_i != NULL)
	{
	    switch(first_i->ea_get_saved_status())
	    {
	    case cat_inode::ea_none:
	    case cat_inode::ea_removed:
		ctime_f = datetime(0);
		break;
	    default:
		ctime_f = first_i->get_last_change();
	    }
	}
	else
	    ctime_f = datetime(0);


	return ctime_f >= x_date || tools_is_equal_with_hourshift(x_hourshift, ctime_f, x_date);
    }


    bool crit_in_place_more_EA::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const cat_inode *second_i = get_inode(&second);
	infinint first_nEA, second_nEA;

	if(first_i != NULL)
	{
	    switch(first_i->ea_get_saved_status())
	    {
	    case cat_inode::ea_full:
		first_nEA = first_i->get_ea()->size();
		break;
	    default:
		first_nEA = 0;
	    }
	}
	else
	    first_nEA = 0;

	if(second_i != NULL)
	{
	    switch(second_i->ea_get_saved_status())
	    {
	    case cat_inode::ea_full:
		second_nEA = second_i->get_ea()->size();
		break;
	    default:
		second_nEA = 0;
	    }
	}
	else
	    second_nEA = 0;

	return first_nEA >= second_nEA;
    }

    bool crit_in_place_EA_bigger::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const cat_inode *second_i = get_inode(&second);
	infinint first_EA_size, second_EA_size;

	if(first_i != NULL)
	{
	    switch(first_i->ea_get_saved_status())
	    {
	    case cat_inode::ea_full:
		first_EA_size = first_i->get_ea()->space_used();
		break;
	    default:
		first_EA_size = 0;
	    }
	}
	else
	    first_EA_size = 0;

	if(second_i != NULL)
	{
	    switch(second_i->ea_get_saved_status())
	    {
	    case cat_inode::ea_full:
		second_EA_size = second_i->get_ea()->space_used();
		break;
	    default:
		second_EA_size = 0;
	    }
	}
	else
	    second_EA_size = 0;

	return first_EA_size >= second_EA_size;
    }

    bool crit_in_place_EA_saved::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);

	return first_i != NULL && first_i->ea_get_saved_status() == cat_inode::ea_full;
    }

    bool crit_same_type::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const cat_inode *second_i = get_inode(&second);

	const file * first_file = dynamic_cast<const file *>(first_i);
	const cat_lien * first_lien = dynamic_cast<const cat_lien *>(first_i);
	const cat_directory * first_dir = dynamic_cast<const cat_directory *>(first_i);
	const cat_chardev * first_char = dynamic_cast<const cat_chardev *>(first_i);
	const cat_blockdev * first_block = dynamic_cast<const cat_blockdev *>(first_i);
	const cat_tube * first_tube = dynamic_cast<const cat_tube *>(first_i);
	const cat_prise * first_prise = dynamic_cast<const cat_prise *>(first_i);
	const detruit *first_detruit = dynamic_cast<const detruit *>(&first); // first not first_i here !

	const file * second_file = dynamic_cast<const file *>(second_i);
	const cat_lien * second_lien = dynamic_cast<const cat_lien *>(second_i);
	const cat_directory * second_dir = dynamic_cast<const cat_directory *>(second_i);
	const cat_chardev * second_char = dynamic_cast<const cat_chardev *>(second_i);
	const cat_blockdev * second_block = dynamic_cast<const cat_blockdev *>(second_i);
	const cat_tube * second_tube = dynamic_cast<const cat_tube *>(second_i);
	const cat_prise * second_prise = dynamic_cast<const cat_prise *>(second_i);
	const detruit *second_detruit = dynamic_cast<const detruit *>(&second); // second not second_i here !

	return (first_file != NULL && second_file != NULL)
	    || (first_lien != NULL && second_lien != NULL)
	    || (first_dir != NULL && second_dir != NULL)
	    || (first_char != NULL && second_char != NULL)
	    || (first_block != NULL && second_block != NULL)
	    || (first_tube != NULL && second_tube != NULL)
	    || (first_prise != NULL && second_prise != NULL)
	    || (first_detruit != NULL && second_detruit != NULL);
    }

    void crit_not::copy_from(const crit_not & ref)
    {
	if(ref.x_crit == NULL)
	    throw SRC_BUG;

	if(ref.x_crit == NULL)
	    throw SRC_BUG;
	x_crit = ref.x_crit->clone();
	if(x_crit == NULL)
	    throw Ememory("crit_not::copy_from");
    }

    void crit_and::add_crit(const criterium & ref)
    {
	criterium *cloned = ref.clone();

	if(cloned == NULL)
	    throw Ememory("crit_and::add_crit");

	try
	{
	    operand.push_back(cloned);
	}
	catch(...)
	{
	    if(operand.back() == cloned)
		operand.pop_back();
	    delete cloned;
	    throw;
	}
    }

    void crit_and::gobe(crit_and & to_be_voided)
    {
	vector<criterium *>::iterator it = to_be_voided.operand.begin();

	try
	{
	    while(it != to_be_voided.operand.end())
	    {
		if(*it == NULL)
		    throw SRC_BUG;
		operand.push_back(*it);
		++it;
	    }
	    to_be_voided.operand.clear();
	}
	catch(...)
	{
	    if(it != to_be_voided.operand.end() && operand.back() == *it)
		++it;
	    while(it != to_be_voided.operand.end())
	    {
		if(*it != NULL)
		{
		    delete *it;
		    *it = NULL;
		}
		++it;
	    }
	    to_be_voided.operand.clear();
	    throw;
	}
    }

    bool crit_and::evaluate(const cat_nomme & first, const cat_nomme & second) const
    {
	bool ret = true;

	NLS_SWAP_IN;
	try
	{
	    vector<criterium *>::const_iterator it = operand.begin();

	    if(it == operand.end())
		throw Erange("crit_and::evaluate", gettext("Cannot evaluate this crit_and criterium as no criterium has been added to it"));

	    while(ret && it != operand.end())
	    {
		ret &= (*it)->evaluate(first, second);
		++it;
	    };
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

	return ret;
    }

    void crit_and::copy_from(const crit_and & ref)
    {
	vector<criterium *>::const_iterator it = ref.operand.begin();

	operand.clear();
	try
	{
	    criterium *cloned;

	    while(it != ref.operand.end())
	    {
		cloned = (*it)->clone();
		if(cloned == NULL)
		    throw Ememory("crit_add::copy_from");
		operand.push_back(cloned);
		++it;
	    }
	}
	catch(...)
	{
	    detruit();
	    throw;
	}
    }

    void crit_and::detruit()
    {
	vector<criterium *>::iterator it = operand.begin();

	while(it != operand.end())
	{
	    if(*it != NULL)
	    {
		delete *it;
		*it = NULL;
	    }
	    ++it;
	}
	operand.clear();
    }

    bool crit_or::evaluate(const cat_nomme & first, const cat_nomme & second) const
    {
	bool ret = false;

	NLS_SWAP_IN;
	try
	{
	    vector<criterium *>::const_iterator it = operand.begin();

	    if(it == operand.end())
		throw Erange("crit_or::evaluate", gettext("Cannot evaluate this crit_or criterium as no criterium has been added to it"));

	    while(!ret && it != operand.end())
	    {
		ret |= (*it)->evaluate(first, second);
		++it;
	    };
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
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
	    const file * al_file = dynamic_cast<const file *>(already_here);
	    const cat_mirage * al_mirage = dynamic_cast<const cat_mirage *>(already_here);

	    const cat_inode * do_inode = dynamic_cast<const cat_inode *>(dolly);
	    const cat_directory * do_directory = dynamic_cast<const cat_directory *>(dolly);
	    const file * do_file = dynamic_cast<const file *>(dolly);
	    const cat_mirage * do_mirage = dynamic_cast<const cat_mirage *>(dolly);

	    dialog.printf(gettext("Entry information:\t\"in place\"\t\"to be added\""));
	    dialog.printf(gettext("Is inode         :\t  %S  \t\t  %S"), al_inode == NULL ? &no : &yes , do_inode == NULL ? &no : &yes);
	    dialog.printf(gettext("Is directory     :\t  %S  \t\t  %S"), al_directory == NULL ? &no : &yes , do_directory == NULL ? &no : &yes);
	    dialog.printf(gettext("Is plain file    :\t  %S  \t\t  %S"), al_file == NULL ? &no : &yes , do_file == NULL ? &no : &yes);
	    dialog.printf(gettext("Is hard linked   :\t  %S  \t\t  %S"), al_mirage == NULL ? &no : &yes , do_mirage == NULL ? &no : &yes);
	    dialog.printf(gettext("Entry type       :\t  %s  \t  %s"), signature2string(already_here->signature()), signature2string(dolly->signature()));

	    if(al_inode != NULL && do_inode != NULL)
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
		if(al_file != NULL && do_file != NULL)
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
