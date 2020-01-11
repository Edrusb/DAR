/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

#include "pile.hpp"

#include <algorithm>

using namespace std;

namespace libdar
{


    void pile::push(generic_file *f, const string & label, bool extend_mode)
    {
	face to_add;

	if(is_terminated())
	    throw SRC_BUG;

	if(f == nullptr)
	    throw SRC_BUG;
	if(look_for_label(label) != stack.end())
	    throw Erange("pile::push", "Label already used while pushing a generic_file on a stack");
	if(stack.empty())
	    set_mode(f->get_mode());
	if(f->get_mode() != get_mode()                         // not the exact same mode
	   && (!extend_mode || f->get_mode() != gf_read_write) // not extending the mode
	   && get_mode() != gf_read_write)                     // not reducing the mode
	    throw Erange("pile::push", "Adding to the stack of generic_file an object using an incompatible read/write mode");
	set_mode(f->get_mode());
	to_add.ptr = f;
	to_add.labels.clear();
	if(label != "")
	    to_add.labels.push_back(label);
	stack.push_back(to_add);
    }

    generic_file *pile::pop()
    {
        face ret;

	if(stack.size() > 0)
	{
	    ret = stack.back();
	    stack.pop_back();
	}
	else
	    ret.ptr = nullptr;

	return ret.ptr; // nullptr is returned if the stack is empty
    }

    generic_file *pile::get_below(const generic_file *ref)
    {
	deque<face>::reverse_iterator it = stack.rbegin();

	while(it != stack.rend() && it->ptr != ref)
	    ++it;

	if(it != stack.rend())
	{
	    ++it; // getting the next object, that's it the one below as this is a reverse iterator
	    if(it != stack.rend())
		return it->ptr;
	    else
		return nullptr;
	}
	else
	    return nullptr;
    }

    generic_file *pile::get_above(const generic_file *ref)
    {
	deque<face>::iterator it = stack.begin();

	while(it != stack.end() && it->ptr != ref)
	    ++it;

	if(it != stack.end())
	{
	    ++it; // getting the next object, that's it the one above
	    if(it != stack.end())
		return it->ptr;
	    else
		return nullptr;
	}
	else
	    return nullptr;
    }


    generic_file *pile::get_by_label(const std::string & label)
    {
	if(label == "")
	    throw SRC_BUG;
	else
	{
	    deque<face>::iterator it = look_for_label(label);

	    if(it == stack.end())
		throw Erange("pile::get_by_label", "Label requested in generic_file stack is unknown");

	    if(it->ptr == nullptr)
		throw SRC_BUG;

	    return it->ptr;
	}
    }

    void pile::clear_label(const string & label)
    {
	if(label == "")
	    throw Erange("pile::clear_label", "Empty string is an invalid label, cannot clear it");
	deque<face>::iterator it = look_for_label(label);
	if(it != stack.end())
	{
	    list<string>::iterator lab = find(it->labels.begin(), it->labels.end(), label);
	    if(lab == it->labels.end())
		throw SRC_BUG;
	    it->labels.erase(lab);
	}
    }


    void pile::add_label(const string & label)
    {
	if(stack.empty())
	    throw Erange("pile::add_label", "Cannot add a label to an empty stack");

	if(label == "")
	    throw Erange("pile::add_label", "An empty string is an invalid label, cannot add it");

	if(look_for_label(label) != stack.end())
	    throw Erange("pile::add_label", "Label already used in stack, cannot add it");

	stack.back().labels.push_back(label);
    }

    bool pile::skippable(skippability direction, const infinint & amount)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(stack.size() > 0)
	{
	    if(stack.back().ptr == nullptr)
		throw SRC_BUG;
	    return stack.back().ptr->skippable(direction, amount);
	}
	else
	    throw Erange("pile::skip", "Error: skippable() on empty stack");
    }

    bool pile::skip(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(stack.size() > 0)
	{
	    if(stack.back().ptr == nullptr)
		throw SRC_BUG;
	    return stack.back().ptr->skip(pos);
	}
	else
	    throw Erange("pile::skip", "Error: skip() on empty stack");
    }

    bool pile::skip_to_eof()
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(stack.size() > 0)
	{
	    if(stack.back().ptr == nullptr)
		throw SRC_BUG;
	    return stack.back().ptr->skip_to_eof();
	}
	else
	    throw Erange("pile::skip_to_eof", "Error: skip_to_eof() on empty stack");
    }

    bool pile::skip_relative(S_I x)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(stack.size() > 0)
	{
	    if(stack.back().ptr == nullptr)
		throw SRC_BUG;
	    return stack.back().ptr->skip_relative(x);
	}
	else
	    throw Erange("pile::skip_relative", "Error: skip_relative() on empty stack");
    }

    infinint pile::get_position() const
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(stack.size() > 0)
	{
	    if(stack.back().ptr == nullptr)
		throw SRC_BUG;
	    return stack.back().ptr->get_position();
	}
	else
	    throw Erange("pile::get_position", "Error: get_position() on empty stack");
    }

    void pile::copy_to(generic_file & ref)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(stack.size() > 0)
	{
	    if(stack.back().ptr == nullptr)
		throw SRC_BUG;
	    stack.back().ptr->copy_to(ref);
	}
	else
	    throw Erange("pile::copy_to", "Error: copy_to() from empty stack");
    }

    void pile::copy_to(generic_file & ref, const infinint & crc_size, crc * & value)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(stack.size() > 0)
	{
	    if(stack.back().ptr == nullptr)
		throw SRC_BUG;
	    stack.back().ptr->copy_to(ref, crc_size, value);
	}
	else
	    throw Erange("pile::copy_to(crc)", "Error: copy_to(crc) from empty stack");
    }

    void pile::inherited_read_ahead(const infinint & amount)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(stack.size() > 0)
	{
	    if(stack.back().ptr == nullptr)
		throw SRC_BUG;
	    return stack.back().ptr->read_ahead(amount);
	}
    }


    U_I pile::inherited_read(char *a, U_I size)
    {
	if(stack.size() > 0)
	{
	    if(stack.back().ptr == nullptr)
		throw SRC_BUG;
	    return stack.back().ptr->read(a, size);
	}
	else
	    throw Erange("pile::skip", "Error: inherited_read() on empty stack");
    }

    void pile::inherited_write(const char *a, U_I size)
    {
	if(stack.size() > 0)
	{
	    if(stack.back().ptr == nullptr)
		throw SRC_BUG;
	    stack.back().ptr->write(a, size);
	}
	else
	    throw Erange("pile::skip", "Error: inherited_write() on empty stack");
    }

    void pile::sync_write_above(generic_file *ptr)
    {
	deque<face>::reverse_iterator it = stack.rbegin();

	    //  we start from the top of the stack down to ptr
	while(it != stack.rend() && it->ptr != ptr)
	{
	    it->ptr->sync_write();
	    ++it;
	}
	if(it->ptr != ptr)
	    throw SRC_BUG;
    }

    void pile::flush_read_above(generic_file *ptr)
    {
	deque<face>::reverse_iterator it = stack.rbegin();

	    // we start from the top of the stack down to ptr
	while(it != stack.rend() && it->ptr != ptr)
	{
	    it->ptr->flush_read();
	    ++it;
	}
	if(it->ptr != ptr)
	    throw SRC_BUG;
    }


    void pile::inherited_sync_write()
    {
	for(deque<face>::reverse_iterator it = stack.rbegin() ; it != stack.rend() ; ++it)
	    if(it->ptr != nullptr)
		it->ptr->sync_write();
	    else
		throw SRC_BUG;
    }

    void pile::inherited_flush_read()
    {
	for(deque<face>::iterator it = stack.begin() ; it != stack.end() ; ++it)
	    if(it->ptr != nullptr)
		it->ptr->flush_read();
	    else
		throw SRC_BUG;
    }


    void pile::inherited_terminate()
    {
	for(deque<face>::reverse_iterator it = stack.rbegin() ; it != stack.rend() ; ++it)
	    if(it->ptr != nullptr)
		it->ptr->terminate();
	    else
		throw SRC_BUG;
    }


    void pile::detruit()
    {
	for(deque<face>::reverse_iterator it = stack.rbegin() ; it != stack.rend() ; ++it)
	{
	    if(it->ptr != nullptr)
	    {
		try
		{
		    delete it->ptr;
		}
		catch(...)
		{
			// ignore all exceptions
		}
		it->ptr = nullptr;
	    }
	}
	stack.clear();
    }

    deque<pile::face>::iterator pile::look_for_label(const std::string & label)
    {
	deque<face>::iterator it = stack.begin();

	while(it != stack.end() && find(it->labels.begin(), it->labels.end(), label) == it->labels.end())
	    ++it;

	return it;
    }

} // end of namespace
