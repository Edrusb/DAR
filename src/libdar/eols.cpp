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

extern "C"
{
} // end extern "C"

#include "eols.hpp"
#include "erreurs.hpp"

using namespace std;

namespace libdar
{

    eols::eols(const deque<string> & end_sequences)
    {
	eols_curs.clear();
	ref_progression = 0;

	    // checking against empty sequences

	deque<string>::const_iterator it = end_sequences.begin();
	while(it != end_sequences.end())
	{
	    add_sequence(*it);
	    ++it;
	}
    }

    void eols::add_sequence(const std::string & seq)
    {
	if(seq.empty())
	    throw Erange("eols::add_sequence", gettext("Empty string cannot be provided as sequence defining an end of line"));
	eols_curs.push_back(in_progress(seq));
    }

    void eols::reset_detection() const
    {
	deque<in_progress>::const_iterator it = eols_curs.begin();
	while(it != eols_curs.end())
	{
	    it->reset();
	    ++it;
	}

	ref_progression = 0;
    }

    bool eols::eol_reached(char next_read_byte,
			   U_I & eol_sequence_length,
			   U_I & after_eol_read_bytes) const
    {
	bool new_match = false;
	bool all_done = false;

	deque<in_progress>::const_iterator it = eols_curs.begin();
	while(it != eols_curs.end())
	{
	    if(it->match(next_read_byte))
	    {
		if(ref_progression < it->progression())
		    ref_progression = it->progression();
		new_match = true;
	    }
	    ++it;
	}

	if(!new_match)
	    all_done = all_bypassed_or_matched();
	else
	    all_done = false;

	if(all_done || new_match)
	{
	    if(all_done || bypass_or_larger(ref_progression))
	    {
		if(find_larger_match(eol_sequence_length, after_eol_read_bytes))
		{
		    reset_detection();
		    return true;
		}
		else
		    throw SRC_BUG;
	    }
		//else some sequences remain possible so we continue
	}
	    // else no new match but some remain to be evaluated

	return false;
    }

    bool eols::in_progress::match(char next_read_byte) const
    {
	if(bypass)
	{
	    if(has_matched())
		++passed;
	    return false;
	}

	if(next_to_match == ref.end())
	    throw SRC_BUG;

	if(*next_to_match == next_read_byte)
	{
	    ++next_to_match;
	    if(next_to_match == ref.end())
		return true;
	}
	else
	{
	    if(!larger)
		reset();
	    else
		set_bypass(progression()+1);
	}

	return false;
    }

    U_I eols::in_progress::progression() const
    {
	return next_to_match - ref.begin();
    }

    bool eols::in_progress::set_bypass(U_I prog) const
    {
	if(bypass)
	    return true;

	if(has_matched() || progression() < prog)
	{
	    bypass = true;
	    passed = 0;
	}

	return bypass;
    }

    bool eols::bypass_or_larger(U_I prog) const
    {
	bool ret = true;

	deque<in_progress>::const_iterator it = eols_curs.begin();
	while(it != eols_curs.end())
	{
	    if(! it->set_bypass(prog))
	    {
		it->set_larger();
		ret = false;
	    }
	    ++it;
	}

	return ret;
    }

    bool eols::all_bypassed_or_matched() const
    {
	deque<in_progress>::const_iterator it = eols_curs.begin();

	while(it != eols_curs.end() && (it->bypass || it->has_matched()))
	    ++it;

	return it == eols_curs.end();
    }

    bool eols::find_larger_match(U_I & seq_length, U_I & read_after_eol) const
    {
	bool ret = false;

	deque<in_progress>::const_iterator it = eols_curs.begin();

	while(it != eols_curs.end() && (! it->has_matched() || it->progression() < ref_progression))
	    ++it;

	if(it != eols_curs.end())
	{
	    if(it->progression() > ref_progression)
		throw SRC_BUG;

	    ret = true;
	    seq_length = it->ref.size();
	    read_after_eol = it->passed;
	}

	return ret;
    }

    void eols::copy_from(const eols & ref)
    {
	deque<in_progress>::iterator it;

	eols_curs = ref.eols_curs;
	it = eols_curs.begin();
	while(it != eols_curs.end())
	{
	    it->reset();
	    ++it;
	}
    }


} // end of namespace
