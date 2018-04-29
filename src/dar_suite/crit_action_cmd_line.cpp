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

#include <new>

#include "crit_action_cmd_line.hpp"
#include "tools.hpp"
#include "line_tools.hpp"

using namespace std;
using namespace libdar;

static const criterium * criterium_create_from_string(user_interaction & dialog, const string & argument, const infinint & hourshift);

string crit_action_canonize_string(const std::string & argument)
{
    string ret = "";
    string::const_iterator it = argument.begin();

    while(it != argument.end())
    {
	if(*it != ' '
	   && *it != '\r'
	   && *it != '\n'
	   && *it != '\t')
	    ret += *it;
	++it;
    }

    return ret;
}


const crit_action * crit_action_create_from_string(user_interaction & dialog,
						   const string & argument,
						   const infinint & hourshift)
{
    string::const_iterator it;
    const crit_action *ret = nullptr;

    if(argument.begin() == argument.end())
	throw Erange("crit_action_create_from_string","Unexpected empty string in expression");

	// looking for ';' (chain actions)

    try
    {
	it = line_tools_find_first_char_out_of_parenth(argument, ';');
	if(it != argument.end())
	{
	    const crit_action *tmp = nullptr;
	    const crit_chain *tmp_chain = nullptr;
	    crit_chain *ret_chain = nullptr;

	    try
	    {
		if(*it != ';')
		    throw SRC_BUG;
		ret = ret_chain = new (nothrow) crit_chain();
		if(ret == nullptr)
		    throw Ememory("crit_action_create_from_string");
		tmp = crit_action_create_from_string(dialog, string(argument.begin(), it), hourshift);
		if(tmp == nullptr)
		    throw SRC_BUG;
		ret_chain->add(*tmp);
		delete tmp;
		tmp = nullptr;
		tmp = crit_action_create_from_string(dialog, string(it+1, argument.end()), hourshift);
		tmp_chain = dynamic_cast<const crit_chain *>(tmp);
		if(tmp_chain != nullptr)
		    ret_chain->gobe(*(const_cast<crit_chain *>(tmp_chain)));
		else
		    ret_chain->add(*tmp);
		delete tmp;
		tmp = nullptr;
	    }
	    catch(...)
	    {
		if(tmp != nullptr)
		    delete tmp;
		throw;
	    }

	    return ret;
	}

	if(*argument.begin() == '{')
	{
	    string::const_iterator fin;
	    const criterium *crit = nullptr;
	    const crit_action *go_true = nullptr, *go_false = nullptr;

	    it = line_tools_find_first_char_out_of_parenth(argument, '}');
	    if(it == argument.end())
		throw Erange("crit_action_create_from_string", string(gettext("Missing } in conditional statement: ")) + argument);
	    if(*it != '}')
		throw SRC_BUG;
	    if(it + 1 == argument.end() || *(it + 1) != '[' )
		throw Erange("crit_action_create_from_string", string(gettext("Missing [ after } in conditional statement: ")) + argument);
	    fin =  line_tools_find_first_char_out_of_parenth(argument, ']');
	    if(fin == argument.end())
		throw Erange("crit_action_create_from_string", string(gettext("Missing ] in conditional statement: ")) + argument);
	    if(*fin != ']')
		throw SRC_BUG;

	    try
	    {
		crit = criterium_create_from_string(dialog, string(argument.begin()+1, it), hourshift);
		if(crit == nullptr)
		    throw SRC_BUG;
		go_true = crit_action_create_from_string(dialog, string(it + 2, fin), hourshift);
		if(go_true == nullptr)
		    throw SRC_BUG;
		if(fin + 1 == argument.end())
		{
		    go_false = new (nothrow) crit_constant_action(data_undefined, EA_undefined);
		    if(go_false == nullptr)
			throw Ememory("crit_action_create_from_string");
		}
		else
		{
		    go_false = crit_action_create_from_string(dialog, string(fin + 1, argument.end()), hourshift);
		    if(go_false == nullptr)
			throw SRC_BUG;
		}

		ret = new (nothrow) testing(*crit, *go_true, *go_false);
		if(ret == nullptr)
		    throw Ememory("crit_action_create_from_string");
		delete crit;
		crit = nullptr;
		delete go_true;
		go_true = nullptr;
		delete go_false;
		go_false = nullptr;
	    }
	    catch(...)
	    {
		if(crit != nullptr)
		    delete crit;
		if(go_true != nullptr)
		    delete go_true;
		if(go_false != nullptr)
		    delete go_false;
		throw;
	    }

	    return ret;
	}

	if(argument.size() == 2)
	{
	    over_action_data data;
	    over_action_ea ea;

	    switch(*argument.begin())
	    {
	    case 'P':
		data = data_preserve;
		break;
	    case 'O':
		data = data_overwrite;
		break;
	    case 'S':
		data = data_preserve_mark_already_saved;
		break;
	    case 'T':
		data = data_overwrite_mark_already_saved;
		break;
	    case 'R':
		data = data_remove;
		break;
	    case '*':
		data = data_undefined;
		break;
	    case 'A':
		data = data_ask;
		break;
	    default:
		throw Erange("crit_action_create_from_string", tools_printf(gettext("Unknown policy for data '%c' in expression %S"), *argument.begin(), &argument));
	    }

	    switch(*(argument.begin() +1))
	    {
	    case 'p':
		ea = EA_preserve;
		break;
	    case 'o':
		ea = EA_overwrite;
		break;
	    case 's':
		ea = EA_preserve_mark_already_saved;
		break;
	    case 't':
		ea = EA_overwrite_mark_already_saved;
		break;
	    case 'm':
		ea = EA_merge_preserve;
		break;
	    case 'n':
		ea = EA_merge_overwrite;
		break;
	    case 'r':
		ea = EA_clear;
		break;
	    case '*':
		ea = EA_undefined;
		break;
	    case 'a':
		ea = EA_ask;
		break;
	    default:
		throw Erange("crit_action_create_from_string", tools_printf(gettext("Unknown policy for EA '%c' in expression %S"), *(argument.begin() +1), &argument));
	    }

	    ret = new (nothrow) crit_constant_action(data, ea);
	    if(ret == nullptr)
		throw Ememory("crit_action_create_from_string");
	    else
		return ret;
	}

	throw Erange("crit_action_create_from_string", string(gettext("Unknown expression in overwriting policy: ")) + argument);
    }
    catch(...)
    {
	if(ret != nullptr)
	    delete ret;
	throw;
    }

    throw SRC_BUG; // we should never reach this statement

}

static const criterium * criterium_create_from_string(user_interaction &dialog, const string & argument, const infinint & hourshift)
{
    string::const_iterator it;
    const criterium *ret = nullptr;

    if(argument.begin() == argument.end())
	throw Erange("criterium_create_from_string","Unexpected empty string in expression");

    try
    {

	    // looking for '|' operator first

	it = line_tools_find_first_char_out_of_parenth(argument, '|');
	if(it != argument.end())
	{
	    const criterium * tmp = nullptr;
	    const crit_or *tmp_or = nullptr;
	    crit_or *ret_or = nullptr;

	    try
	    {
		if(*it != '|')
		    throw SRC_BUG;
		ret = ret_or = new (nothrow) crit_or();
		if(ret == nullptr)
		    throw Ememory("criterium_create_from_string");
		tmp = criterium_create_from_string(dialog, string(argument.begin(), it), hourshift);
		if(tmp == nullptr)
		    throw SRC_BUG;
		ret_or->add_crit(*tmp);
		delete tmp;
		tmp = nullptr;
		tmp = criterium_create_from_string(dialog, string(it+1, argument.end()), hourshift);
		tmp_or = dynamic_cast<const crit_or *>(tmp);
		if(tmp_or != nullptr)
		    ret_or->gobe(*(const_cast<crit_or *>(tmp_or)));
		else
		    ret_or->add_crit(*tmp);
		delete tmp;
		tmp = nullptr;
	    }
	    catch(...)
	    {
		if(tmp != nullptr)
		    delete tmp;
		throw;
	    }

	    return ret;
	}

	    // if no '|' is found, looking for '&' operator

	it = line_tools_find_first_char_out_of_parenth(argument, '&');
	if(it != argument.end())
	{
	    const criterium *tmp = nullptr;
	    const crit_and *tmp_and = nullptr;
	    crit_and *ret_and = nullptr;

	    try
	    {
		if(*it != '&')
		    throw SRC_BUG;
		ret = ret_and = new (nothrow) crit_and();
		if(ret == nullptr)
		    throw Ememory("criterium_create_from_string");
		tmp = criterium_create_from_string(dialog, string(argument.begin(), it), hourshift);
		if(tmp == nullptr)
		    throw SRC_BUG;
		ret_and->add_crit(*tmp);
		delete tmp;
		tmp = nullptr;
		tmp = criterium_create_from_string(dialog, string(it+1, argument.end()), hourshift);
		tmp_and = dynamic_cast<const crit_and *>(tmp);
		if(tmp_and != nullptr)
		    ret_and->gobe(*(const_cast<crit_and *>(tmp_and)));
		else
		    ret_and->add_crit(*tmp);
		delete tmp;
		tmp = nullptr;
	    }
	    catch(...)
	    {
		if(tmp != nullptr)
		    delete tmp;
		throw;
	    }

	    return ret;
	}

	    // else looking whether we have global parenthesis around the expresion

	if(*argument.begin() == '(' && *(argument.end() - 1) == ')')
	    return criterium_create_from_string(dialog, string(argument.begin() + 1, argument.end() - 1), hourshift);

	else // well, this "else" statment is not necessary, I just found cleaner to add it to have a block in which to declare a temporary pointer
	{

		// else looking for unary operators

	    const criterium *tmp = nullptr;

	    try
	    {
		switch(*argument.begin())
		{
		case '!':
		    tmp = criterium_create_from_string(dialog, string(argument.begin() + 1, argument.end()), hourshift);
		    if(tmp == nullptr)
			throw SRC_BUG;
		    ret = new (nothrow) crit_not(*tmp);
		    delete tmp;
		    tmp = nullptr;
		    if(ret == nullptr)
			throw Ememory("criterium_create_from_string");
		    return ret;
		case '~':
		    tmp = criterium_create_from_string(dialog, string(argument.begin() + 1, argument.end()), hourshift);
		    if(tmp == nullptr)
			throw SRC_BUG;
		    ret = new (nothrow) crit_invert(*tmp);
		    delete tmp;
		    tmp = nullptr;
		    if(ret == nullptr)
			throw Ememory("criterium_create_from_string");
		    return ret;
		}
	    }
	    catch(...)
	    {
		if(tmp != nullptr)
		    delete tmp;
		throw;
	    }

	}


	    // else looking for atomic operator with argument

	if(argument.size() >= 4) // minimum size is "X(a)" thus 4 chars
	{
	    if(*(argument.begin() + 1) == '(' && *(argument.end() - 1) == ')')
	    {
		infinint date;
		string sub_arg = string(argument.begin() + 2, argument.end() - 1);

		    // parsing and converting the argument of the atomic operator

		switch(*argument.begin())
		{
		case 'R':
		case 'r':
		    try
		    {
			    // note that the namespace specification is necessary
			    // due to similar existing name in std namespace under
			    // certain OS (FreeBSD 10.0)
			libdar::deci tmp = sub_arg;
			date = tmp.computer();
		    }
		    catch(Edeci & e)
		    {
			date = line_tools_convert_date(sub_arg);
		    }
		    break;
		default:
		    throw Erange("criterium_create_from_string", string(gettext("Unknown atomic operator, or atomic not allowed with an argument: ") + argument));
		}

		    // creating the criterium with its parameter

		switch(*argument.begin())
		{
		case 'R':
		    ret = new (nothrow) crit_in_place_data_more_recent_or_equal_to(date, hourshift);
		    break;
		case 'r':
		    ret = new (nothrow) crit_in_place_EA_more_recent_or_equal_to(date, hourshift);
		    break;
		default:
		    throw SRC_BUG;
		}

		if(ret == nullptr)
		    throw Ememory("criterium_create_from_string");
		else
		    return ret;
	    }
	}

	    // else looking for atomic operator

	if(argument.size() == 1)
	{
	    switch(*argument.begin())
	    {
	    case 'I':
		ret = new (nothrow) crit_in_place_is_inode();
		break;
	    case 'D':
		ret = new (nothrow) crit_in_place_is_dir();
		break;
	    case 'F':
		ret = new (nothrow) crit_in_place_is_file();
		break;
	    case 'H':
		ret = new (nothrow) crit_in_place_is_hardlinked_inode();
		break;
	    case 'A':
		ret = new (nothrow) crit_in_place_is_new_hardlinked_inode();
		break;
	    case 'R':
		ret = new (nothrow) crit_in_place_data_more_recent(hourshift);
		break;
	    case 'B':
		ret = new (nothrow) crit_in_place_data_bigger();
		break;
	    case 'S':
		ret = new (nothrow) crit_in_place_data_saved();
		break;
	    case 'Y':
		ret = new (nothrow) crit_in_place_data_dirty();
		break;
	    case 'X':
		ret = new (nothrow) crit_in_place_data_sparse();
		break;
	    case 'L':
		ret = new (nothrow) crit_in_place_has_delta_sig();
		break;
	    case 'e':
		ret = new (nothrow) crit_in_place_EA_present();
		break;
	    case 'r':
		ret = new (nothrow) crit_in_place_EA_more_recent(hourshift);
		break;
	    case 'm':
		ret = new (nothrow) crit_in_place_more_EA();
		break;
	    case 'b':
		ret = new (nothrow) crit_in_place_EA_bigger();
		break;
	    case 's':
		ret = new (nothrow) crit_in_place_EA_saved();
		break;
	    case 'T':
		ret = new (nothrow) crit_same_type();
		break;
	    default:
		throw Erange("criterium_create_from_string", string(gettext("Unknown character found while parsing conditional string: ")) + argument);
	    }

	    if(ret == nullptr)
		throw Ememory("criterium_create_from_string");
	    else
		return ret;
	}

	throw Erange("criterium_create_from_string", string(gettext("Unknown expression found while parsing conditional string: ")) + argument);

    }
    catch(...)
    {
	if(ret != nullptr)
	    delete ret;
	throw;
    }

    throw SRC_BUG;  // we should never reach this statement.

}
