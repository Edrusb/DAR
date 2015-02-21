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
    generic_thread::generic_thread(generic_file * ptr, U_I block_size, U_I num_block):
	generic_file(gf_read_only),
	toslave(num_block, block_size),
	tomaster(num_block, block_size)
    {
	unsigned int tmp = sizeof(data_header);

	if(ptr == NULL)
	    throw SRC_BUG;
	set_mode(ptr->get_mode());
	reached_eof = false;
	running = false;

	order.clear();
	order.set_type(msg_type::data);
	order.reset_get_block();
	if(!order.get_block(&data_header, tmp))
	    throw SRC_BUG; // data header larger than one byte !

	remote = new (get_pool()) slave_thread(ptr, &toslave, &tomaster);
	if(remote == NULL)
	    throw Ememory("generic_thread::generic_thread");
	try
	{
	    my_run(); // launching the new thread
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
	toslave(2, 10),
	tomaster(2, 10)
    {
	throw SRC_BUG;
    }


    generic_thread::~generic_thread()
    {
	if(remote != NULL)
	{
	    if(!is_terminated())
		terminate();
	    delete remote;
	    remote = NULL;
	}
    }

    bool generic_thread::skippable(skippability direction, const infinint & amount)
    {
	bool ret;

	    // rerun the thread if an exception has occured previously
	my_run();

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
	reached_eof = false;

	    // rerun the thread if an exception has occured previously
	my_run();

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

	    // rerun the thread if an exception has occured previously
	my_run();

	    // preparing the order message

	order.clear();
	order.set_type(msg_type::order_skip_to_eof);

	    // order completed

	send_order();
	read_answer();
	check_answer(msg_type::answr_skip_done);
	ret = answer.get_bool();
	release_block_answer();
	reached_eof = ret;

	return ret;
    }

    bool generic_thread::skip_relative(S_I x)
    {
	U_I val;
	bool ret;

	    // rerun the thread if an exception has occured previously
	my_run();

	    // preparing the order message

	order.clear();
	if(x >= 0)
	{
	    val = x;
	    order.set_type(msg_type::order_skip_fwd);
	    order.set_U_I(val);
	}
	else // x < 0
	{
	    reached_eof = false;
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

	    // rerun the thread if an exception has occured previously
	my_run();

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

    void generic_thread::inherited_read_ahead(const infinint & amount)
    {

	    // rerun the thread if an exception has occured previously
	my_run();

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
	bool refilled = false;

	    // rerun the thread if an exception has occured previously
	my_run();

	if(reached_eof)
	    return 0;

	    // preparing the order message

	order.clear();
	order.set_type(msg_type::order_read);
	order.set_U_I(size);

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
		    refilled = true; // some data remain in the pipe
		}
		else // the whole block will be read
		{
		    (void)memcpy(a + read, ptr + 1, num);
		    read += num;
		    release_block_answer();
		    refilled = false;
		}
		break;
	    case msg_type::answr_exception:
		my_join();
		throw SRC_BUG;
	    default:
		throw SRC_BUG;
	    }
	}
	while(answer.get_type() != msg_type::answr_read_eof && read < size);

	if(answer.get_type() == msg_type::answr_read_eof && !refilled)
	    reached_eof = true;

	return read;
    }

    void generic_thread::inherited_write(const char *a, U_I size)
    {
	U_I wrote = 0;
	unsigned int bksize;
	char *tmptr = NULL;
	U_I min;

	    // rerun the thread if an exception has occured previously
	my_run();

	do
	{
	    toslave.get_block_to_feed(tmptr, bksize);
	    if(bksize > 1)
		tmptr[0] = data_header;
	    else
	    {
		toslave.feed_cancel_get_block(tmptr);
		throw SRC_BUG;
	    }
	    min = bksize - 1 > size - wrote ? size - wrote : bksize - 1;
	    (void)memcpy(tmptr + 1, a + wrote, min);
	    wrote += min;
	    toslave.feed(tmptr, min + 1);
	}
	while(wrote < size);
    }

    void generic_thread::inherited_sync_write()
    {
	    // rerun the thread if an exception has occured previously
	my_run();

	    // preparing the order message

	order.clear();
	order.set_type(msg_type::order_sync_write);

	    // order completed

	send_order();
	read_answer();
	check_answer(msg_type::answr_sync_write_done);
	release_block_answer();
    }

    void generic_thread::inherited_flush_read()
    {
	    // rerun the thread if an exception has occured previously
	my_run();

	    // preparing the order message to stop a possible read_ahead
	    // running in the slave_thread

	order.clear();
	order.set_type(msg_type::order_stop_readahead);

	    // order completed

	send_order();
	read_answer();
	check_answer(msg_type::answr_readahead_stopped);
	release_block_answer();

	    // resetting the current object
	reached_eof = false;
    }

    void generic_thread::inherited_terminate()
    {
	    // rerun the thread if an exception has occured previously
	my_run();

	    // preparing the order message

	order.clear();
	order.set_type(msg_type::order_end_of_xmit);

	    // order completed

	send_order();
	my_join();
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
	while(answer.get_type() == msg_type::data
	      || answer.get_type() == msg_type::answr_read_eof)
	{
	    if(expected == msg_type::data)
		break; // exiting the while loop to avoid releasing the block

	    switch(expected)
	    {
	    case msg_type::unset:
		throw SRC_BUG;
	    case msg_type::data:
		throw SRC_BUG; // should have been treated above
	    case msg_type::answr_read_eof:
		throw SRC_BUG; // should have been treated above
	    case msg_type::answr_sync_write_done:
	    case msg_type::answr_skippable:
	    case msg_type::answr_position:
	    case msg_type::answr_position_begin:
		skip_block_answer(); // skipping to next block to read the answer
		read_answer();
		break;
	    case msg_type::answr_exception:
	    case msg_type::answr_skip_done:
	    case msg_type::answr_readahead_stopped:
		release_block_answer(); // destroying read ahead data block from the pipe
		read_answer();
		break;
	    default:
		throw SRC_BUG;
	    }
	}

	switch(answer.get_type())
	{
	case msg_type::answr_exception:
	    release_block_answer();
	    my_join();
	    throw SRC_BUG; // join should rethrow the exception that has been raised in the thread "remote"
	default:
	    if(answer.get_type() == expected)
		break;
	    else
	    {
		release_block_answer();
		throw SRC_BUG;
	    }
	}
    }

    void generic_thread::my_run()
    {
	if(!running)
	{
	    running = true;
	    if(remote == NULL)
		throw SRC_BUG;
	    if(remote->is_running())
		throw SRC_BUG;
	    toslave.reset();
	    tomaster.reset();
	    remote->run(); // launching the new thread
	    if(remote->is_running(tid))
	    {
		thread_cancellation::associate_tid_to_tid(pthread_self(), tid);
		thread_cancellation::associate_tid_to_tid(tid, pthread_self());
	    }
	    else
		running = false;
	}
    }

    void generic_thread::my_join()
    {
	try
	{
	    remote->join(); // may throw exceptions
	}
	catch(...)
	{
	    if(running) // running may be false if the thread lived to shortly to obtain its tid
		thread_cancellation::dead_thread(tid);
	    running = false;
	    throw;
	}
	if(running)
	    thread_cancellation::dead_thread(tid);
	running = false;
    }

} // end of generic_thread::namespace
