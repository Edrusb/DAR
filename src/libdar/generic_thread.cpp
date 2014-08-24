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
#if HAVE_STRING_H
#include <string.h>
#endif
} // end extern "C"

#include "generic_thread.hpp"

using namespace std;

namespace libdar
{
    generic_thread::generic_thread(generic_file * ptr):
	generic_file(gf_read_only),
	toslave(libthreadar::tampon<char>(block_num, block_size)),
	tomaster(libthreadar::tampon<char>(block_num, block_size))
    {
	if(ptr == NULL)
	    throw SRC_BUG;
	set_mode(ptr->get_mode());

	remote = new (get_pool()) slave_thread(ptr, &toslave, &tomaster);
	if(remote == NULL)
	    throw Ememory("generic_thread::generic_thread");
	try
	{
	    remote->run(); // launching the new thread
	}
	catch(...)
	{
	    if(remote != NULL)
		delete remote;
	    throw;
	}
    }

    generic_thread::generic_thread(const generic_thread & ref):
	generic_file (gf_read_only),
	toslave(libthreadar::tampon<char>(block_num, block_size)),
	tomaster(libthreadar::tampon<char>(block_num, block_size))
    {
	throw SRC_BUG;
    }


    generic_thread::~generic_thread()
    {
	if(remote != NULL)
	{
	    delete remote;
	    remote = NULL;
	}
    }

    bool generic_thread::is_contextual() const
    {
	bool tmp;

	try
	{
	    (void)is_an_old_start_end_archive();
	    tmp = true; // successful call, this is a contextual
	}
	catch(Erange & e)
	{
	    tmp = false; // failure, this is not a contextual
	}

	return tmp;
    }

    bool generic_thread::skippable(skippability direction, const infinint & amount)
    {
	bool ret;

	    // preparing the order message

	order.clear();
	switch(direction)
	{
	case generic_file::skip_backward:
	    order.set_type(msg_type::order_skippable_fwd);
	    break;
	case generic_file::skip_forward:
	    order.set_type(msg_type::order_skippable_bkd);
	    break;
	default:
	    throw SRC_BUG;
	}
	order.set_infinint(amount);

	    // order completed

	send_order();
	read_answer();
	check_answer(msg_type::answr_skippable);
	ret = answer.get_bool();
	release_block_answer();

	return ret;
    }

    bool generic_thread::skip(const infinint & pos)
    {
	bool ret;

	    // preparing the order message

	order.clear();
	order.set_type(msg_type::order_skip);
	order.set_infinint(pos);

	    // order completed

	send_order();
	read_answer();
	check_answer(msg_type::answr_skip_done); // this flushes all read ahead data up before skip occured
	ret = answer.get_bool();
	release_block_answer();

	return ret;
    }

    bool generic_thread::skip_to_eof()
    {
	bool ret;

	    // preparing the order message

	order.clear();
	order.set_type(msg_type::order_skip_to_eof);

	    // order completed

	send_order();
	read_answer();
	check_answer(msg_type::answr_skip_done);
	ret = answer.get_bool();
	release_block_answer();

	return ret;
    }

    bool generic_thread::skip_relative(S_I x)
    {
	U_I val;
	bool ret;

	    // preparing the order message

	order.clear();
	if(x >= 0)
	{
	    val = x;
	    order.set_type(msg_type::order_skip_fwd);
	    order.set_U_I(val);
	}
	else
	{
	    val = -x;
	    order.set_type(msg_type::order_skip_bkd);
	    order.set_U_I(val);
	}

	    // order completed

	send_order();
	read_answer();
	check_answer(msg_type::answr_skip_done);
	ret = answer.get_bool();
	release_block_answer();

	return ret;
    }

    infinint generic_thread::get_position()
    {
	infinint ret;

	    // preparing the order message

	order.clear();
	order.set_type(msg_type::order_get_position);

	    // order completed

	send_order();
	read_answer();
	check_answer(msg_type::answr_position);
	ret = answer.get_infinint();
	release_block_answer();

	return ret;
    }


    void generic_thread::set_info_status(const std::string & s)
    {

	    // preparing the order message

	order.clear();
	order.set_type(msg_type::order_set_context);
	order.set_string(s);

	    // order completed

	send_order();
    }

    bool generic_thread::is_an_old_start_end_archive() const
    {
	bool ret;

	generic_thread *me = const_cast<generic_thread *>(this);
	if(me == NULL)
	    throw SRC_BUG;

	    // preparing the order message

	me->order.clear();
	me->order.set_type(msg_type::order_is_oldarchive);

	    // order completed

	me->send_order();
	me->read_answer();
	if(answer.get_type() == msg_type::answr_not_contextual)
	    throw Erange("generic_thread", gettext("Remote threaded generic_file does not inherit from contextual class"));
	me->check_answer(msg_type::answr_oldarchive);
	ret = answer.get_bool();
	me->release_block_answer();

	return ret;
    }

    const label & generic_thread::get_data_name() const
    {
	generic_thread *me = const_cast<generic_thread *>(this);
	if(me == NULL)
	    throw SRC_BUG;

	    // preparing the order message

	me->order.clear();
	me->order.set_type(msg_type::order_get_dataname);

	    // order completed

	me->send_order();
	me->read_answer();
	if(answer.get_type() == msg_type::answr_not_contextual)
	    throw Erange("generic_thread::get_data_name", gettext("Remote threaded generic_file does not inherit from contextual class"));
	me->check_answer(msg_type::answr_dataname);
	me->dataname = answer.get_label();
	me-> release_block_answer();

	return dataname;
    }

    void generic_thread::inherited_read_ahead(const infinint & amount)
    {

	    // preparing the order message

	order.clear();
	order.set_type(msg_type::order_read_ahead);
	order.set_infinint(amount);

	    // order completed

	send_order();
    }

    U_I generic_thread::inherited_read(char *a, U_I size)
    {
	U_I read = 0;
	U_I min;

	    // preparing the order message

	order.clear();
	order.set_type(msg_type::order_read);
	order.set_U_I(size);
	send_order();

	    // order completed

	send_order();
	do
	{
	    read_answer();

		// the following replaces check_answer() as more complicated things have to be done here

	    switch(answer.get_type())
	    {
	    case msg_type::data:
	    case msg_type::answr_read_eof:
		--num; // the first byte contains the message type
		min = size - read; // what's still need to be read
		if(num > min)
		{
		    U_I kept = num - min;

		    (void)memcpy(a + read, ptr + 1, min);
		    read += min;
		    (void)memmove(ptr + 1, ptr + 1 + min, kept);
		    tomaster.fetch_push_back(ptr, kept + 1); // counting the first message type in addition (one byte)
		    ptr = NULL;
		}
		else // the whole block will be read
		{
		    (void)memcpy(a + read, ptr + 1, num);
		    read += num;
		    release_block_answer();
		}
		break;
	    case msg_type::answr_exception:
		remote->join();
		throw SRC_BUG;
	    default:
		throw SRC_BUG;
	    }
	}
	while(answer.get_type() != msg_type::answr_read_eof && read < size);

	return read;
    }

    void generic_thread::inherited_write(const char *a, U_I size)
    {
	U_I wrote = 0;
	unsigned int bksize;
	char *tmptr = NULL;
	U_I min;
	char header;

	    // preparing the order message

	order.clear();
	order.set_type(msg_type::data);
	order.reset_get_block();
	bksize = 1;
	if(!order.get_block(&header, bksize))
	    throw SRC_BUG; // data header larger than one byte !

	do
	{
	    toslave.get_block_to_feed(tmptr, bksize);
	    if(bksize > 1)
		tmptr[0] = header;
	    else
		throw SRC_BUG;
	    min = bksize - 1 > size - wrote ? size - wrote : bksize - 1;
	    (void)memcpy(tmptr + 1, a + wrote, min);
	    wrote += min;
	    toslave.feed(tmptr, wrote);
	}
	while(wrote < size);
    }

    void generic_thread::inherited_sync_write()
    {
	    // preparing the order message

	order.clear();
	order.set_type(msg_type::order_sync_write);

	    // order completed

	send_order();

    }

    void generic_thread::inherited_terminate()
    {
	    // preparing the order message

	order.clear();
	order.set_type(msg_type::order_end_of_xmit);

	    // order completed

	send_order();
	remote->join();
    }

    void generic_thread::send_order()
    {
	bool completed = false;

	order.reset_get_block();
	do
	{
	    toslave.get_block_to_feed(ptr, num);
	    completed = order.get_block(ptr, num);
	    toslave.feed(ptr, num);
	}
	while(!completed);
    }

    void generic_thread::read_answer()
    {
	bool completed = false;

	answer.clear();
	do
	{
	    tomaster.fetch(ptr, num);
	    completed = answer.add_block(ptr, num);
	    if(!completed)
		tomaster.fetch_recycle(ptr);
	}
	while(!completed);
	    // note ptr and num are relative
	    // to the last block read which
	    // must be released/recycled calling release_answer()
    }

    void generic_thread::check_answer(msg_type expected)
    {
	while(answer.get_type() == msg_type::data)
	{
	    if(expected == msg_type::data)
		break; // exiting the while loop to avoid releasing the block

	    release_block_answer(); // dropping that block up to the next non data block
	    read_answer();
	}

	switch(answer.get_type())
	{
	case msg_type::answr_exception:
	    remote->join();
	    throw SRC_BUG; // join should rethrow the exception that has been raised in the thread "remote"
	default:
	    if(answer.get_type() == expected)
		break;
	    else
		throw SRC_BUG;
	}
    }


} // end of generic_thread::namespace
