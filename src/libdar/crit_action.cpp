/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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

#include "crit_action.hpp"
#include "nls_swap.hpp"
#include "cat_all_entrees.hpp"
#include "tools.hpp"
#include "cat_nomme.hpp"
#include "op_tools.hpp"

using namespace std;

namespace libdar
{

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

} // end of namespace
