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

#include "slave_thread.hpp"

using namespace std;

namespace libdar
{


    slave_thread::slave_thread(generic_file *x_data, libthreadar::tampon<char> *x_input, libthreadar::tampon<char> *x_output)
    {
	data = x_data;
	input = x_input;
	output = x_output;
    }

    slave_thread::~slave_thread()
    {
	if(data != NULL)
	    delete data;
    }

    void slave_thread::inherited_run()
    {

	try
	{
	    num = 0;
	    ptr = NULL;
	    set_header_vars();
	    read_ahead = 0;
	    to_send_ahead = 0;
	    immediate_read = 0;
	    stop = false;

	    do
	    {
		if(to_send_ahead > 0)
		{
		    U_I tmp = 0;
		    to_send_ahead.unstack(tmp);
		    U_I wrote = send_data_block(tmp);
		    if(wrote > 0)
		    {
			tmp -= wrote;
			to_send_ahead += tmp;
			read_ahead += wrote;
		    }
		    else
			to_send_ahead = 0; // reached eof
		}

		if(pending_order() || to_send_ahead == 0)
		{
		    bool need_answer;

		    read_order();
		    try
		    {
			need_answer = treat_order();
		    }
		    catch(...)
		    {
			input->fetch_recycle(ptr);
			throw;
		    }
		    input->fetch_recycle(ptr);

		    if(need_answer)
			send_answer();

		    if(immediate_read > 0)
			go_read();
		}

	    }
	    while(!stop);

	}
	catch(...)
	{
		// send an exception message to master thread
	    answer.clear();
	    answer.set_type(msg_type::answr_exception);
	    send_answer();

		// and then die
	    throw;
	}
    }


    void slave_thread::set_header_vars()
    {
	unsigned int tmp;

	answer.clear();
	answer.set_type(msg_type::data);
	answer.reset_get_block();
	tmp = 1;
	if(!answer.get_block(&data_header, tmp))
	    throw SRC_BUG;
	if(tmp != 1)
	    throw SRC_BUG;

	answer.clear();
	answer.set_type(msg_type::answr_read_eof);
	answer.reset_get_block();
	tmp = 1;
	if(!answer.get_block(&data_eof_header, tmp))
	    throw SRC_BUG;
	if(tmp != 1)
	    throw SRC_BUG;
    }

    void slave_thread::read_order()
    {
	bool completed = false;

	order.clear();
	do
	{
	    input->fetch(ptr, num);
	    completed = order.add_block(ptr, num);
	    if(!completed)
		input->fetch_recycle(ptr);
	}
	while(!completed);
	    // note ptr and num are relative
	    // to the last block read which
	    // must be released/recycled calling release_answer()


    }

    void slave_thread::send_answer()
    {
	bool completed = false;

	answer.reset_get_block();
	do
	{
	    output->get_block_to_feed(ptr, num);
	    completed = answer.get_block(ptr, num);
	    output->feed(ptr, num);
	}
	while(!completed);
    }

    U_I slave_thread::send_data_block(U_I size)
    {
	U_I min;

	    // preparing the new answer

	output->get_block_to_feed(ptr, num);

	try
	{
	    if(num == 0)
		throw SRC_BUG;
	    ptr[0] = data_header;
	    --num;

	    if(size == 0)
		min = num; // filling the block with as much data as possible
	    else
		min = size > num ? num : size;

	    size = data->read(ptr + 1, min);
	    if(size < min) // reached eof
		ptr[0] = data_eof_header;

	}
	catch(...)
	{
	    output->feed_cancel_get_block(ptr);
	    throw;
	}

	output->feed(ptr, size + 1);

	return size; // note that size has been modified by the effective number of byte read from *data
    }

    bool slave_thread::treat_order()
    {
	bool need_answer = false;
	contextual *ct_data = dynamic_cast<contextual *>(data);

	answer.clear();
	switch(order.get_type())
	{
	case msg_type::data:
	    data->write(ptr + 1, num - 1);
	    if(to_send_ahead > 0)
		throw SRC_BUG; // read_ahead asked but no read done, received data to write instead
	    if(read_ahead > 0)
		throw SRC_BUG; // read_ahead started but data not read, received data to write instead
	    break;
	case msg_type::order_read_ahead:
	    to_send_ahead += order.get_infinint();
	    break;
	case msg_type::order_read:
	    immediate_read = order.get_U_I();
	    break;
	case msg_type::order_sync_write:
	    data->sync_write();
	    break;
	case msg_type::order_skip:
	    answer.set_type(msg_type::answr_skip_done);
	    answer.set_bool(data->skip(order.get_infinint()));
	    read_ahead = 0;
	    to_send_ahead = 0;
	    need_answer = true;
	    break;
	case msg_type::order_skip_to_eof:
	    answer.set_type(msg_type::answr_skip_done);
	    answer.set_bool(data->skip_to_eof());
	    read_ahead = 0;
	    to_send_ahead = 0;
	    need_answer = true;
	    break;
	case msg_type::order_skip_fwd:
	    answer.set_type(msg_type::answr_skip_done);
	    answer.set_bool(data->skip_relative(order.get_U_I()));
	    read_ahead = 0;
	    to_send_ahead = 0;
	    need_answer = true;
	    break;
	case msg_type::order_skip_bkd:
	    answer.set_type(msg_type::answr_skip_done);
	    answer.set_bool(data->skip_relative(-order.get_U_I()));
	    read_ahead = 0;
	    to_send_ahead = 0;
	    need_answer = true;
	    break;
	case msg_type::order_set_context:
	    if(ct_data == NULL)
	    {
		answer.set_type(msg_type::answr_not_contextual);
		need_answer = true;
	    }
	    else
		ct_data->set_info_status(order.get_string());
	    break;
	case msg_type::order_skippable_fwd:
	    answer.set_type(msg_type::answr_skippable);
	    answer.set_bool(data->skippable(generic_file::skip_forward, order.get_infinint()));
	    need_answer = true;
	    break;
	case msg_type::order_skippable_bkd:
	    answer.set_type(msg_type::answr_skippable);
	    answer.set_bool(data->skippable(generic_file::skip_backward, order.get_infinint()));
	    need_answer = true;
	    break;
	case msg_type::order_get_position:
	    answer.set_type(msg_type::answr_position);
	    if(read_ahead > 0)
	    {
		    // all read_ahead blocks are in the output pipe
		    // and will be dropped by the master to reach the
		    // answer to the get_position() order it just sent
		    // so we must ignore all read_ahead block, stop further
		    // read ahead and seek back the data to the position
		    // of the first block in pipe that will be ignored
		    // by the master thread.
		if(data->get_position() >= read_ahead)
		    data->skip(data->get_position() - read_ahead);
		else
		    throw SRC_BUG; // first byte of data read ahead is before offset zero !?!
		read_ahead = 0;
	    }
	    answer.set_infinint(data->get_position());
	    to_send_ahead = 0;
	    need_answer = true;
	    break;
	case msg_type::order_is_oldarchive:
	    if(ct_data == NULL)
	    {
		answer.set_type(msg_type::answr_not_contextual);
		need_answer = true;
	    }
	    else
	    {
		answer.set_type(msg_type::answr_oldarchive);
		answer.set_bool(ct_data->is_an_old_start_end_archive());
		need_answer = true;
	    }
	    break;
	case msg_type::order_get_dataname:
	    if(ct_data == NULL)
	    {
		answer.set_type(msg_type::answr_not_contextual);
		need_answer = true;
	    }
	    else
	    {
		answer.set_type(msg_type::answr_dataname);
		answer.set_label(ct_data->get_data_name());
		need_answer = true;
	    }
	    break;
	case msg_type::order_end_of_xmit:
	    stop = true;
	    break;
	default:
	    throw SRC_BUG;
	}

	return need_answer;
    }


    void slave_thread::go_read()
    {
	U_I read;
	U_I amount;
	U_I sent = 0;
	bool eof = false;

	if(read_ahead > 0)
	{
	    if(infinint(immediate_read) >= read_ahead)
	    {
		U_I tmp = 0;

		read_ahead.unstack(tmp);
		if(read_ahead != 0)
		    throw SRC_BUG;
		immediate_read -= tmp;
	    }
	    else
	    {
		read_ahead -= immediate_read;
		immediate_read = 0;
	    }
	}


	while(sent < immediate_read && !eof)
	{
	    output->get_block_to_feed(ptr, num);

	    try
	    {
		if(num == 0)
		    throw SRC_BUG;
		--num;

		read = immediate_read - sent;      // what amount remains to sent
		amount = read > num  ? num : read; // what amount we can sent in the next block
		read = data->read(ptr + 1, amount);// what amount could be get from the source
		if(read == amount)
		    ptr[0] = data_header;
		else
		{
		    ptr[0] = data_eof_header;
		    eof = true;
		}
	    }
	    catch(...)
	    {
		output->feed_cancel_get_block(ptr);
		throw;
	    }

	    output->feed(ptr, read + 1); // +1 for the data header
	    immediate_read -= read;
	    sent += read;

	    if(eof)
	    {
		immediate_read = 0; // stopping the read request
		to_send_ahead = 0;  // stopping a pending read_ahead
	    }
	}

	if(to_send_ahead > 0)
	{
	    if(infinint(sent) > to_send_ahead)
		to_send_ahead = 0;
	    else
		to_send_ahead -= sent;
	}
    }

} // end of generic_thread::namespace
