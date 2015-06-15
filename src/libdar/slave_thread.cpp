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

#include "tronconneuse.hpp"
#include "slave_thread.hpp"

using namespace std;

namespace libdar
{


    slave_thread::slave_thread(generic_file *x_data,
			       libthreadar::fast_tampon<char> *x_input_data,
			       libthreadar::fast_tampon<char> *x_output_data,
			       libthreadar::fast_tampon<char> *x_input_ctrl,
			       libthreadar::fast_tampon<char> *x_output_ctrl)
    {
	if(x_data == NULL)
	    throw SRC_BUG;
	if(x_input_data == NULL)
	    throw SRC_BUG;
	if(x_output_data == NULL)
	    throw SRC_BUG;
	if(x_input_ctrl == NULL)
	    throw SRC_BUG;
	if(x_output_ctrl == NULL)
	    throw SRC_BUG;

	data = x_data;
	input_data = x_input_data;
	output_data = x_output_data;
	input_ctrl = x_input_ctrl;
	output_ctrl = x_output_ctrl;
	ptr = NULL;
	set_header_vars();
	init();
    }

    void slave_thread::init()
    {
	num = 0;
	ptr = NULL;
	read_ahead = 0;
	endless_read_ahead = false;
	to_send_ahead = 0;
	immediate_read = 0;
	wake_me = false;
	stop = false;
    }

    void slave_thread::inherited_run()
    {
	try
	{
	    init();

	    do
	    {
		if((!to_send_ahead.is_zero() || endless_read_ahead) && !output_data->is_full())
		{
		    U_I tmp = 0;
		    bool eof;

		    if(!endless_read_ahead)
			to_send_ahead.unstack(tmp);
			// else tmp == 0 which lead to send a maximum sized block of data in the tampon

		    U_I wrote = send_data_block(tmp, eof);
		    if(wrote > 0)
		    {
			if(!endless_read_ahead)
			{
			    tmp -= wrote;
			    to_send_ahead += tmp;
			}
			read_ahead += wrote;
		    }
		    if(eof)
		    {
			endless_read_ahead = false;
			to_send_ahead = 0;
		    }
		}

		if(pending_input_data())
		    treat_input_data();
		else
		{
		    if(pending_order() || output_data->is_full() || (to_send_ahead.is_zero() && !endless_read_ahead))
		    {
			bool need_answer;

			if(!pending_order())
			    wake_me = true;

			read_order();
			wake_me = false;
			try
			{
			    need_answer = treat_order();
			}
			catch(...)
			{
			    input_ctrl->fetch_recycle(ptr);
			    throw;
			}
			input_ctrl->fetch_recycle(ptr);

			if(need_answer)
			{
				// sending the answer
			    send_answer();
			}

			if(immediate_read > 0)
			    go_read();
		    }
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
	answer.set_type(msg_type::data_partial);
	answer.reset_get_block();
	tmp = 1;
	if(!answer.get_block(&data_header, tmp))
	    throw SRC_BUG;
	if(tmp != 1)
	    throw SRC_BUG;

	answer.clear();
	answer.set_type(msg_type::data_completed);
	answer.reset_get_block();
	tmp = 1;
	if(!answer.get_block(&data_header_completed, tmp))
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
	    input_ctrl->fetch(ptr, num);
	    completed = order.add_block(ptr, num);
	    if(!completed)
		input_ctrl->fetch_recycle(ptr);
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
	    output_ctrl->get_block_to_feed(ptr, num);
	    completed = answer.get_block(ptr, num);
	    output_ctrl->feed(ptr, num);
	}
	while(!completed);
    }

    U_I slave_thread::send_data_block(U_I size, bool & eof)
    {
	U_I min;
	char *local_ptr = NULL;
	unsigned int local_num;

	eof = false;

	    // preparing the new answer

	output_data->get_block_to_feed(local_ptr, local_num);

	try
	{
	    if(local_num == 0)
		throw SRC_BUG;
	    local_ptr[0] = data_header;
	    --local_num;

	    if(size == 0)
		min = local_num; // filling the block with as much data as possible
	    else
		min = size > local_num ? local_num : size; // returning the minimum of (size, num)

	    size = data->read(local_ptr + 1, min);
	    if(size < min) // reached eof
	    {
		local_ptr[0] = data_header_completed;
		eof = true;
	    }

	}
	catch(...)
	{
	    output_data->feed_cancel_get_block(local_ptr);
	    throw;
	}

	output_data->feed(local_ptr, size + 1);

	return size; // note that size has been modified by the effective number of byte read from *data
    }

    void slave_thread::treat_input_data()
    {
	char *local_ptr = NULL;
	unsigned int local_num;

	if(input_data->is_not_empty())
	{
	    if(!to_send_ahead.is_zero() || endless_read_ahead)
		throw SRC_BUG; // read_ahead asked but no read done, received data to write instead
	    if(!read_ahead.is_zero())
		throw SRC_BUG; // read_ahead started but data not read, received data to write instead
	}

	while(input_data->is_not_empty())
	{
	    input_data->fetch(local_ptr, local_num);
	    try
	    {
		data->write(local_ptr+1, local_num - 1);
	    }
	    catch(...)
	    {
		input_data->fetch_recycle(local_ptr);
		throw;
	    }
	    input_data->fetch_recycle(local_ptr);
	}
    }

    bool slave_thread::treat_order()
    {
	bool need_answer = false;
	infinint tmp;

	answer.clear();
	switch(order.get_type())
	{
	case msg_type::order_read_ahead:
	    tmp = order.get_infinint();
	    if(tmp.is_zero())
	    {
		endless_read_ahead = true;
		to_send_ahead = 0;
	    }
	    else
	    {
		endless_read_ahead = false;
		to_send_ahead += tmp;
	    }
	    data->read_ahead(tmp); // propagate the read_ahead request
	    break;
	case msg_type::order_read:
	    immediate_read = order.get_U_I();
	    break;
	case msg_type::order_sync_write:
	    treat_input_data();
		// we do not propagate to below data, just the inter-thread
		// exchanges have to be synced
	    answer.set_type(msg_type::answr_sync_write_done);
	    need_answer = true;
	    break;
	case msg_type::order_skip:
	    treat_input_data();
	    answer.set_type(msg_type::answr_skip_done);
	    answer.set_bool(data->skip(order.get_infinint()));
	    read_ahead = 0; // all in transit data will be dropped by the master
	    to_send_ahead = 0;
	    endless_read_ahead = false;
	    need_answer = true;
	    break;
	case msg_type::order_skip_to_eof:
	    treat_input_data();
	    answer.set_type(msg_type::answr_skip_done);
	    answer.set_bool(data->skip_to_eof());
	    read_ahead = 0; // all in transit data will be dropped by the master
	    to_send_ahead = 0;
	    endless_read_ahead = false;
	    need_answer = true;
	    break;
	case msg_type::order_skip_fwd:
	    treat_input_data();
	    answer.set_type(msg_type::answr_skip_done);
	    answer.set_bool(data->skip_relative(order.get_U_I()));
	    read_ahead = 0; // all in transit data will be dropped by the master
	    to_send_ahead = 0;
	    endless_read_ahead = false;
	    need_answer = true;
	    break;
	case msg_type::order_skip_bkd:
	    treat_input_data();
	    answer.set_type(msg_type::answr_skip_done);
	    answer.set_bool(data->skip_relative(-order.get_U_I()));
	    read_ahead = 0; // all in transit data will be dropped by the master
	    to_send_ahead = 0;
	    endless_read_ahead = false;
	    need_answer = true;
	    break;
	case msg_type::order_skippable_fwd:
	    treat_input_data();
	    if(!read_ahead.is_zero())
		throw SRC_BUG; // code is not adapted, it should take into consideration read_ahead data
	    answer.set_type(msg_type::answr_skippable);
	    answer.set_bool(data->skippable(generic_file::skip_forward, order.get_infinint()));
	    need_answer = true;
	    break;
	case msg_type::order_skippable_bkd:
	    treat_input_data();
	    if(!read_ahead.is_zero())
		throw SRC_BUG; // code is not adapted, it should take into consideration read_ahead data
	    answer.set_type(msg_type::answr_skippable);
	    answer.set_bool(data->skippable(generic_file::skip_backward, order.get_infinint()));
	    need_answer = true;
	    break;
	case msg_type::order_get_position:
	    treat_input_data();
	    answer.set_type(msg_type::answr_position);
	    tmp = data->get_position();
	    if(!read_ahead.is_zero())
	    {
		    // we need to compensate the position of "data"
		    // by the amount of bytes read ahead from it
		    // but not yet requested for reading by the master thread
		    // (still in transit in the tampon)
		if(tmp < read_ahead)
		    throw SRC_BUG; // we read ahead more data than available in the underlying generic_file!!!
		else
		    tmp -= read_ahead;
	    }
	    answer.set_infinint(tmp);
	    need_answer = true;
	    break;
	case msg_type::order_end_of_xmit:
	    treat_input_data();
	    stop = true;
	    break;
	case msg_type::order_stop_readahead:
	    answer.set_type(msg_type::answr_readahead_stopped);
	    read_ahead = 0; // all in transit data will be dropped by the master
	    to_send_ahead = 0;
	    endless_read_ahead = false;
	    need_answer = true;
	    break;
	case msg_type::order_wakeup:
	    wake_me = false;
	    break;
	default:
	    throw SRC_BUG;
	}

	return need_answer;
    }


    void slave_thread::go_read()
    {
	U_I sent = 0;
	bool eof = false;

	if(input_data->is_not_empty())
	    throw SRC_BUG;

	if(!read_ahead.is_zero())
	{
	    if(infinint(immediate_read) >= read_ahead)
	    {
		U_I tmp = 0;

		    // counting out the already sent data thanks to a previous read_ahead

		read_ahead.unstack(tmp);
		if(!read_ahead.is_zero())
		    throw SRC_BUG;
		immediate_read -= tmp;
	    }
	    else
	    {
		    // all data already sent thanks to a previous read_ahead

		read_ahead -= immediate_read;
		immediate_read = 0;
	    }
	}

	    // after read_ahead consideration, checking what remains to send

	while(sent < immediate_read && !eof)
	    sent += send_data_block(immediate_read - sent, eof);

	if(eof)
	{
		// stopping a pending read_ahead
	    to_send_ahead = 0;
	    endless_read_ahead = false;
	}
	immediate_read = 0; // requested read has completed (either eof or all data sent)


	    // counting down from what remains to send due to pending read_ahead what we have just sent

	if(!to_send_ahead.is_zero())
	{
	    if(infinint(sent) > to_send_ahead)
		to_send_ahead = 0;
	    else
		to_send_ahead -= sent;
	}
    }

} // end of generic_thread::namespace
