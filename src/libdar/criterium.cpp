/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

#include "criterium.hpp"
#include "nls_swap.hpp"
#include "cat_all_entrees.hpp"
#include "tools.hpp"
#include "cat_nomme.hpp"
#include "cat_inode.hpp"
#include "cat_directory.hpp"

using namespace std;

namespace libdar
{

    static const cat_inode *get_inode(const cat_nomme * arg);


	/////////////////////////////////////////////////////////////////////
	//////////// implementation of criterium classes follows ////////////
        /////////////////////////////////////////////////////////////////////

    bool crit_in_place_is_inode::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	return dynamic_cast<const cat_inode *>(&first) != nullptr
	    || dynamic_cast<const cat_mirage *>(&first) != nullptr;
    }

    bool crit_in_place_is_dir::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	return dynamic_cast<const cat_directory *>(&first) != nullptr;
    }

    bool crit_in_place_is_file::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);

	return dynamic_cast<const cat_file *>(first_i) != nullptr && dynamic_cast<const cat_door *>(first_i) == nullptr;
    }

    bool crit_in_place_is_hardlinked_inode::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	return dynamic_cast<const cat_mirage *>(&first) != nullptr;
    }

    bool crit_in_place_is_new_hardlinked_inode::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_mirage * tmp = dynamic_cast<const cat_mirage *>(&first);
	return tmp != nullptr && tmp->is_first_mirage();
    }

    bool crit_in_place_data_more_recent::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const cat_inode *second_i = get_inode(&second);
	datetime first_date = first_i != nullptr ? first_i->get_last_modif() : datetime(0);
	datetime second_date = second_i != nullptr ? second_i->get_last_modif() : datetime(0);

	return first_i == nullptr || first_date >= second_date || tools_is_equal_with_hourshift(x_hourshift, first_date, second_date);
    }

    bool crit_in_place_data_more_recent_or_equal_to::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	datetime first_date = first_i != nullptr ? first_i->get_last_modif() : datetime(0);

	return first_i == nullptr || first_date >= x_date || tools_is_equal_with_hourshift(x_hourshift, first_date, x_date);
    }

    bool crit_in_place_data_bigger::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const cat_inode *second_i = get_inode(&second);
	const cat_file *first_f = dynamic_cast<const cat_file *>(first_i);
	const cat_file *second_f = dynamic_cast<const cat_file *>(second_i);

	if(first_f != nullptr && second_f != nullptr)
	    return first_f->get_size() >= second_f->get_size();
	else
	    return true;
    }

    bool crit_in_place_data_saved::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);

	if(first_i != nullptr)
	    return first_i->get_saved_status() == saved_status::saved;
	else
	    return true;
    }

    bool crit_in_place_data_dirty::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const cat_file *first_f = dynamic_cast<const cat_file *>(first_i);

	if(first_f != nullptr)
	    return first_f->is_dirty();
	else
	    return false;
    }

    bool crit_in_place_data_sparse::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const cat_file *first_f = dynamic_cast<const cat_file *>(first_i);

	if(first_f != nullptr)
	    return first_f->get_sparse_file_detection_read();
	else
	    return false;
    }

    bool crit_in_place_has_delta_sig::evaluate(const cat_nomme & first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const cat_file *first_f = dynamic_cast<const cat_file *>(first_i);

	if(first_f != nullptr)
	    return first_f->has_delta_signature_available();
	else
	    return false;
    }

    bool crit_same_inode_data::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	crit_same_type same_type;
	crit_in_place_is_inode is_inode;


	if(! same_type.evaluate(first, second))
	    return false;

	if(! is_inode.evaluate(first, second))
	    return false;
	else
	{
		/////
		// inode level consideration

	    const cat_inode* first_i = get_inode(&first);
	    const cat_inode* second_i = get_inode(&second);

	    if(first_i == nullptr || second_i == nullptr)
		throw SRC_BUG; // show be both inodes

	    if(first_i->get_uid() != second_i->get_uid())
		return false;
	    if(first_i->get_gid() != second_i->get_gid())
		return false;
	    if(first_i->get_perm() != second_i->get_perm())
		return false;
	    if(first_i->get_last_modif() != second_i->get_last_modif())
		return false;

		/////
		// plain file specific considerations

	    const cat_file* first_f = dynamic_cast<const cat_file*>(first_i);
	    const cat_file* second_f = dynamic_cast<const cat_file*>(second_i);

	    if(first_f != nullptr)
	    {
		if(second_f == nullptr)
		    throw SRC_BUG; // they should be of the same type
		if(first_f->get_size() != second_f->get_size())
		    return false;
	    }

		/////
		// devices specific considerations

	    const cat_device* first_d = dynamic_cast<const cat_device*>(first_i);
	    const cat_device* second_d = dynamic_cast<const cat_device*>(second_i);

	    if(first_d != nullptr && first_d->get_saved_status() == saved_status::saved)
	    {
		if(second_d == nullptr)
		    throw SRC_BUG; // they should be of the same type
		if(second_d->get_saved_status() != saved_status::saved)
		    return true;   // cannot compare any further
		if(first_d->get_major() != second_d->get_major())
		    return false;
		if(first_d->get_minor() != second_d->get_minor())
		    return false;
	    }

		/////
		// symlink specific considerations

	    const cat_lien* first_l = dynamic_cast<const cat_lien*>(first_i);
	    const cat_lien* second_l = dynamic_cast<const cat_lien*>(second_i);

	    if(first_l != nullptr && first_l->get_saved_status() == saved_status::saved)
	    {
		if(second_l == nullptr)
		    throw SRC_BUG;
		if(second_l->get_saved_status() != saved_status::saved)
		    return true; // cannot comparer any further
		if(first_l->get_target() != second_l->get_target())
		    return false;
	    }

	    return true; // no difference was met so far
	}
    }


    bool crit_in_place_EA_present::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *tmp = dynamic_cast<const cat_inode *>(&first);
	return tmp != nullptr
	    && tmp->ea_get_saved_status() != ea_saved_status::none
	    && tmp->ea_get_saved_status() != ea_saved_status::removed;
    }

    bool crit_in_place_EA_more_recent::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const cat_inode *second_i = get_inode(&second);

	datetime ctime_f, ctime_s;

	if(first_i != nullptr)
	{
	    switch(first_i->ea_get_saved_status())
	    {
	    case ea_saved_status::none:
	    case ea_saved_status::removed:
		ctime_f = datetime(0);
		break;
	    default:
		ctime_f = first_i->get_last_change();
	    }
	}
	else
	    ctime_f = datetime(0);


	if(second_i != nullptr)
	{
	    switch(second_i->ea_get_saved_status())
	    {
	    case ea_saved_status::none:
	    case ea_saved_status::removed:
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

	if(first_i != nullptr)
	{
	    switch(first_i->ea_get_saved_status())
	    {
	    case ea_saved_status::none:
	    case ea_saved_status::removed:
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

	if(first_i != nullptr)
	{
	    switch(first_i->ea_get_saved_status())
	    {
	    case ea_saved_status::full:
		first_nEA = first_i->get_ea()->size();
		break;
	    default:
		first_nEA = 0;
	    }
	}
	else
	    first_nEA = 0;

	if(second_i != nullptr)
	{
	    switch(second_i->ea_get_saved_status())
	    {
	    case ea_saved_status::full:
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

	if(first_i != nullptr)
	{
	    switch(first_i->ea_get_saved_status())
	    {
	    case ea_saved_status::full:
		first_EA_size = first_i->get_ea()->space_used();
		break;
	    default:
		first_EA_size = 0;
	    }
	}
	else
	    first_EA_size = 0;

	if(second_i != nullptr)
	{
	    switch(second_i->ea_get_saved_status())
	    {
	    case ea_saved_status::full:
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

	return first_i != nullptr && first_i->ea_get_saved_status() == ea_saved_status::full;
    }

    bool crit_same_type::evaluate(const cat_nomme &first, const cat_nomme &second) const
    {
	const cat_inode *first_i = get_inode(&first);
	const cat_inode *second_i = get_inode(&second);

	const cat_file * first_file = dynamic_cast<const cat_file *>(first_i);
	const cat_lien * first_lien = dynamic_cast<const cat_lien *>(first_i);
	const cat_directory * first_dir = dynamic_cast<const cat_directory *>(first_i);
	const cat_chardev * first_char = dynamic_cast<const cat_chardev *>(first_i);
	const cat_blockdev * first_block = dynamic_cast<const cat_blockdev *>(first_i);
	const cat_tube * first_tube = dynamic_cast<const cat_tube *>(first_i);
	const cat_prise * first_prise = dynamic_cast<const cat_prise *>(first_i);
	const cat_detruit *first_detruit = dynamic_cast<const cat_detruit *>(&first); // first not first_i here !

	const cat_file * second_file = dynamic_cast<const cat_file *>(second_i);
	const cat_lien * second_lien = dynamic_cast<const cat_lien *>(second_i);
	const cat_directory * second_dir = dynamic_cast<const cat_directory *>(second_i);
	const cat_chardev * second_char = dynamic_cast<const cat_chardev *>(second_i);
	const cat_blockdev * second_block = dynamic_cast<const cat_blockdev *>(second_i);
	const cat_tube * second_tube = dynamic_cast<const cat_tube *>(second_i);
	const cat_prise * second_prise = dynamic_cast<const cat_prise *>(second_i);
	const cat_detruit *second_detruit = dynamic_cast<const cat_detruit *>(&second); // second not second_i here !

	return (first_file != nullptr && second_file != nullptr)
	    || (first_lien != nullptr && second_lien != nullptr)
	    || (first_dir != nullptr && second_dir != nullptr)
	    || (first_char != nullptr && second_char != nullptr)
	    || (first_block != nullptr && second_block != nullptr)
	    || (first_tube != nullptr && second_tube != nullptr)
	    || (first_prise != nullptr && second_prise != nullptr)
	    || (first_detruit != nullptr && second_detruit != nullptr);
    }

    void crit_not::copy_from(const crit_not & ref)
    {
	if(ref.x_crit == nullptr)
	    throw SRC_BUG;

	if(ref.x_crit == nullptr)
	    throw SRC_BUG;
	x_crit = ref.x_crit->clone();
	if(x_crit == nullptr)
	    throw Ememory("crit_not::copy_from");
    }

    void crit_and::add_crit(const criterium & ref)
    {
	criterium *cloned = ref.clone();

	if(cloned == nullptr)
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
	deque<criterium *>::iterator it = to_be_voided.operand.begin();

	try
	{
	    while(it != to_be_voided.operand.end())
	    {
		if(*it == nullptr)
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
		if(*it != nullptr)
		{
		    delete *it;
		    *it = nullptr;
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
	    deque<criterium *>::const_iterator it = operand.begin();

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
	deque<criterium *>::const_iterator it = ref.operand.begin();

	operand.clear();
	try
	{
	    criterium *cloned;

	    while(it != ref.operand.end())
	    {
		cloned = (*it)->clone();
		if(cloned == nullptr)
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
	deque<criterium *>::iterator it = operand.begin();

	while(it != operand.end())
	{
	    if(*it != nullptr)
	    {
		delete *it;
		*it = nullptr;
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
	    deque<criterium *>::const_iterator it = operand.begin();

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


	/////////////////////////////////////////////////////////////////////
	//////////// implementation of private function /////////////////////
        /////////////////////////////////////////////////////////////////////


    static const cat_inode *get_inode(const cat_nomme *arg)
    {
	const cat_inode *ret;
	const cat_mirage *arg_m = dynamic_cast<const cat_mirage *>(arg);

	if(arg_m != nullptr)
	    ret = const_cast<const cat_inode *>(arg_m->get_inode());
	else
	    ret = dynamic_cast<const cat_inode *>(arg);

	return ret;
    }

} // end of namespace
