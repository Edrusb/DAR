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

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif
} // end extern "C"

#include "generic_thread.hpp"
#include "thread_cancellation.hpp"

using namespace std;

namespace libdar
{
    generic_thread::generic_thread(generic_file * x_ptr,
				   U_I data_block_size,
				   U_I data_num_block,
				   U_I ctrl_block_size,
				   U_I ctrl_num_block):
	generic_file(gf_read_only),
	toslave_data(data_num_block, data_block_size),
	tomaster_data(data_num_block, data_block_size),
	toslave_ctrl(ctrl_num_block, ctrl_block_size),
	tomaster_ctrl(ctrl_num_block, ctrl_block_size)
    {
	unsigned int tmp = sizeof(data_header);

	if(tmp < sizeof(char))
	    throw SRC_BUG;

	if(x_ptr == nullptr)
	    throw SRC_BUG;
	set_mode(x_ptr->get_mode());
	reached_eof = false;
	running = false;

	order.clear();
	order.set_type(msg_type::data_partial);
	order.reset_get_block();
	if(!order.get_block(&data_header, tmp))
	    throw SRC_BUG; // data header larger than one byte !

	order.set_type(msg_type::data_completed);
	order.reset_get_block();
	if(!order.get_block(&data_header_eof, tmp))
	    throw SRC_BUG;

	remote = new (nothrow) slave_thread(x_ptr,
					    &toslave_data,
					    &tomaster_data,
					    &toslave_ctrl,
					    &tomaster_ctrl);
	if(remote == nullptr)
	    throw Ememory("generic_thread::generic_thread");
	try
	{
	    my_run(); // launching the new thread
	}
	catch(...)
	{
	    if(remote != nullptr)
		delete remote;
	    throw;
	}
    }

    generic_thread::~generic_thread()
    {
	if(remote != nullptr)
	{
	    if(!is_terminated())
		terminate();
	    delete remote;
	    remote = nullptr;
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

	purge_data_pipe();

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

	purge_data_pipe();

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

	purge_data_pipe();

	return ret;
    }

    bool generic_thread::truncatable(const infinint & pos) const
    {
	bool ret;
	generic_thread *me = const_cast<generic_thread *>(this);

	if(me == nullptr)
	    throw SRC_BUG;

	    // rerun the thread if an exception has occured previously
	me->my_run();

	    // preparing the order message

	me->order.clear();
	me->order.set_type(msg_type::order_truncatable);
	me->order.set_infinint(pos);

	    // order completed

	me->send_order();
	me->read_answer();
	me->check_answer(msg_type::answer_truncatable);
	ret = answer.get_bool();
	me->release_block_answer();

	return ret;
    }

    infinint generic_thread::get_position() const
    {
	infinint ret;
	generic_thread *me = const_cast<generic_thread *>(this);

	if(me == nullptr)
	    throw SRC_BUG;

	    // rerun the thread if an exception has occured previously
	me->my_run();

	    // preparing the order message

	me->order.clear();
	me->order.set_type(msg_type::order_get_position);

	    // order completed

	me->send_order();
	me->read_answer();
	me->check_answer(msg_type::answr_position);
	ret = answer.get_infinint();
	me->release_block_answer();

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
	char *data_ptr = nullptr;
	unsigned int data_num;


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

	    // now retreiving the data from the data_pipe
	do
	{
	    tomaster_data.fetch(data_ptr, data_num);
	    --data_num; // the first byte contains the message type

	    min = size - read; // what's still need to be read
	    if(data_num > min) // we retreived more data than necessary
	    {
		U_I kept = data_num - min;

		(void)memcpy(a + read, data_ptr + 1, min);
		read += min;
		(void)memmove(data_ptr + 1, data_ptr + 1 + min, kept);
		tomaster_data.fetch_push_back(data_ptr, kept + 1);
	    }
	    else // the whole block will be read
	    {
		(void)memcpy(a + read, data_ptr + 1, data_num);
		read += data_num;
		if(data_ptr[0] == data_header_eof)
		    reached_eof = true;
		tomaster_data.fetch_recycle(data_ptr);
	    }
	}
	while(!reached_eof && read < size);

	return read;
    }

    void generic_thread::inherited_write(const char *a, U_I size)
    {
	U_I wrote = 0;
	unsigned int bksize;
	char *tmptr = nullptr;
	U_I min;

	    // rerun the thread if an exception has occured previously
	my_run();

	if(tomaster_data.is_not_empty())
	    throw SRC_BUG;

	do
	{
	    toslave_data.get_block_to_feed(tmptr, bksize);
	    if(bksize > 1)
		tmptr[0] = data_header;
	    else
	    {
		toslave_data.feed_cancel_get_block(tmptr);
		throw SRC_BUG;
	    }
	    min = bksize - 1 > size - wrote ? size - wrote : bksize - 1;
	    (void)memcpy(tmptr + 1, a + wrote, min);
	    wrote += min;
	    toslave_data.feed(tmptr, min + 1);
	    wake_up_slave_if_asked();
	}
	while(wrote < size);
    }

    void generic_thread::inherited_truncate(const infinint & pos)
    {
	    // rerun the thread if an exception has occured previously
	my_run();

	    // preparing the order message

	order.clear();
	order.set_type(msg_type::order_truncate);
	order.set_infinint(pos);

	    // order completed
	send_order();
	read_answer();
	check_answer(msg_type::answer_truncate_done);
	release_block_answer();

	purge_data_pipe();
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

	    // dropping all data currently in the pipe
	purge_data_pipe();

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

	purge_data_pipe();
	my_join();
    }

    void generic_thread::send_order()
    {
	bool completed = false;

	order.reset_get_block();
	do
	{
	    toslave_ctrl.get_block_to_feed(ptr, num);
	    completed = order.get_block(ptr, num);
	    toslave_ctrl.feed(ptr, num);
	}
	while(!completed);
    }

    void generic_thread::read_answer()
    {
	bool completed = false;

	answer.clear();
	do
	{
	    tomaster_ctrl.fetch(ptr, num);
	    completed = answer.add_block(ptr, num);
	    if(!completed)
		tomaster_ctrl.fetch_recycle(ptr);
	}
	while(!completed);
	    // note ptr and num are relative
	    // to the last block read which
	    // must be released/recycled calling release_answer()
    }

    void generic_thread::check_answer(msg_type expected)
    {
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


    void generic_thread::wake_up_slave_if_asked()
    {
	if(remote == nullptr)
	    throw SRC_BUG;
	if(remote->wake_me_up())
	{
	    order.clear();
	    order.set_type(msg_type::order_wakeup);
	    send_order();
	}
    }

    void generic_thread::purge_data_pipe()
    {
	char *tmp;
	unsigned int tmp_ui;

	while(tomaster_data.is_not_empty())
	{
	    tomaster_data.fetch(tmp, tmp_ui);
	    tomaster_data.fetch_recycle(tmp);
	}
    }

    void generic_thread::my_run()
    {
	if(!running)
	{
	    running = true;
	    if(remote == nullptr)
		throw SRC_BUG;
	    if(remote->is_running())
		throw SRC_BUG;
	    toslave_data.reset();
	    tomaster_data.reset();
	    toslave_ctrl.reset();
	    tomaster_ctrl.reset();
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
