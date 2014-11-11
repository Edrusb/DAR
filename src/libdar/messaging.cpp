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
} // end extern "C"

#include "messaging.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    bool msg_equivalent(msg_type arg1, msg_type arg2)
    {
	bool ret = false;
	switch(arg1)
	{
	case msg_type::order_read_ahead:
	case msg_type::order_read_ahead_begin:
	    if(arg2 == msg_type::order_read_ahead
	       || arg2 == msg_type::order_read_ahead_begin)
		ret = true;
	    break;
	case msg_type::order_skip:
	case msg_type::order_skip_begin:
	    if(arg2 == msg_type::order_skip
	       || arg2 == msg_type::order_skip_begin)
		ret = true;
	    break;
	case msg_type::order_skippable_fwd:
	case msg_type::order_skippable_fwd_begin:
	    if(arg2 == msg_type::order_skippable_fwd
	       || arg2 == msg_type::order_skippable_fwd_begin)
		ret = true;
	    break;
	case msg_type::order_skippable_bkd:
	case msg_type::order_skippable_bkd_begin:
	    if(arg2 == msg_type::order_skippable_bkd
	       || arg2 == msg_type::order_skippable_bkd_begin)
		ret = true;
	    break;
	case msg_type::answr_position:
	case msg_type::answr_position_begin:
	    if(arg2 == msg_type::answr_position
	       || arg2 == msg_type::answr_position_begin)
		ret = true;
	    break;
	case msg_type::unset:
	case msg_type::data:
	case msg_type::order_read:
	case msg_type::answr_read_eof:
	case msg_type::order_sync_write:
	case msg_type::answr_sync_write_done:
	case msg_type::order_skip_to_eof:
	case msg_type::order_skip_fwd:
	case msg_type::order_skip_bkd:
	case msg_type::answr_skip_done:
	case msg_type::answr_skippable:
	case msg_type::order_get_position:
	case msg_type::answr_exception:
	case msg_type::order_end_of_xmit:
	case msg_type::order_stop_readahead:
	case msg_type::answr_readahead_stopped:
	    ret = (arg1 == arg2);
	    break;
	default:
	    throw SRC_BUG;
	}

	return ret;
    }

    bool msg_continues(msg_type msg)
    {
	switch(msg)
	{
	case msg_type::order_read_ahead_begin:
	case msg_type::order_skip_begin:
	case msg_type::order_skippable_fwd_begin:
	case msg_type::order_skippable_bkd_begin:
	case msg_type::answr_position_begin:
	    return true;
	case msg_type::unset:
	case msg_type::data:
	case msg_type::order_read_ahead:
	case msg_type::order_read:
	case msg_type::answr_read_eof:
	case msg_type::order_sync_write:
	case msg_type::answr_sync_write_done:
	case msg_type::order_skip:
	case msg_type::order_skip_to_eof:
	case msg_type::order_skip_fwd:
	case msg_type::order_skip_bkd:
	case msg_type::answr_skip_done:
	case msg_type::order_skippable_fwd:
	case msg_type::order_skippable_bkd:
	case msg_type::answr_skippable:
	case msg_type::order_get_position:
	case msg_type::answr_position:
	case msg_type::answr_exception:
	case msg_type::order_end_of_xmit:
	case msg_type::order_stop_readahead:
	case msg_type::answr_readahead_stopped:
	    return false;
	default:
	    throw SRC_BUG;
	}
    }

    char msg_type2char(msg_type x)
    {
	switch(x)
	{
	case msg_type::unset:
	    throw SRC_BUG;
	case msg_type::data:
	    return 'd';
	case msg_type::order_read_ahead:
	    return 'a';
	case msg_type::order_read_ahead_begin:
	    return 'A';
	case msg_type::order_read:
	    return 'r';
	case msg_type::answr_read_eof:
	    return 'e';
	case msg_type::order_sync_write:
	    return 'y';
	case msg_type::answr_sync_write_done:
	    return 'Y';
	case msg_type::order_skip:
	    return 's';
	case msg_type::order_skip_begin:
	    return 'S';
	case msg_type::order_skip_to_eof:
	    return 'z';
	case msg_type::order_skip_fwd:
	    return 'g';
	case msg_type::order_skip_bkd:
	    return 'c';
	case msg_type::answr_skip_done:
	    return 'o';
	case msg_type::order_skippable_fwd:
	    return 'f';
	case msg_type::order_skippable_fwd_begin:
	    return 'F';
	case msg_type::order_skippable_bkd:
	    return 'b';
	case msg_type::order_skippable_bkd_begin:
	    return 'B';
	case msg_type::answr_skippable:
	    return 'i';
	case msg_type::order_get_position:
	    return 'q';
	case msg_type::answr_position:
	    return 'p';
	case msg_type::answr_position_begin:
	    return 'P';
	case msg_type::answr_exception:
	    return 'X';
	case msg_type::order_end_of_xmit:
	    return 'Z';
	case msg_type::order_stop_readahead:
	    return 'W';
	case msg_type::answr_readahead_stopped:
	    return 'w';
	default:
	    throw SRC_BUG;
	}
    }

    msg_type char2msg_type(char x)
    {
	switch(x)
	{
	case 'd':
	    return msg_type::data;
	case 'a':
	    return msg_type::order_read_ahead;
	case 'A':
	    return msg_type::order_read_ahead_begin;
	case 'r':
	    return msg_type::order_read;
	case 'e':
	    return msg_type::answr_read_eof;
	case 'y':
	    return msg_type::order_sync_write;
	case 'Y':
	    return msg_type::answr_sync_write_done;
	case 's':
	    return msg_type::order_skip;
	case 'S':
	    return msg_type::order_skip_begin;
	case 'z':
	    return msg_type::order_skip_to_eof;
	case 'g':
	    return msg_type::order_skip_fwd;
	case 'c':
	    return msg_type::order_skip_bkd;
	case 'o':
	    return msg_type::answr_skip_done;
	case 'f':
	    return msg_type::order_skippable_fwd;
	case 'F':
	    return msg_type::order_skippable_fwd_begin;
	case 'b':
	    return msg_type::order_skippable_bkd;
	case 'B':
	    return msg_type::order_skippable_bkd_begin;
	case 'i':
	    return msg_type::answr_skippable;
	case 'q':
	    return msg_type::order_get_position;
	case 'p':
	    return msg_type::answr_position;
	case 'P':
	    return msg_type::answr_position_begin;
	case 'X':
	    return msg_type::answr_exception;
	case 'Z':
	    return msg_type::order_end_of_xmit;
	case 'W':
	    return msg_type::order_stop_readahead;
	case 'w':
	    return msg_type::answr_readahead_stopped;
	default:
	    throw SRC_BUG;
	}
    }

    msg_type msg_continuation_of(msg_type x)
    {
	switch(x)
	{
	case msg_type::order_read_ahead_begin:
	    throw SRC_BUG;
	case msg_type::order_skip_begin:
	    throw SRC_BUG;
	case msg_type::order_skippable_fwd_begin:
	    throw SRC_BUG;
	case msg_type::order_skippable_bkd_begin:
	    throw SRC_BUG;
	case msg_type::answr_position_begin:
	    throw SRC_BUG;
	case msg_type::unset:
	    throw SRC_BUG;
	case msg_type::data:
	    throw SRC_BUG;
	case msg_type::order_read_ahead:
	    return msg_type::order_read_ahead_begin;
	case msg_type::order_read:
	    throw SRC_BUG;
	case msg_type::answr_read_eof:
	    throw SRC_BUG;
	case msg_type::order_sync_write:
	    throw SRC_BUG;
	case msg_type::answr_sync_write_done:
	    throw SRC_BUG;
	case msg_type::order_skip:
	    return msg_type::order_skip_begin;
	case msg_type::order_skip_to_eof:
	    throw SRC_BUG;
	case msg_type::order_skip_fwd:
	    throw SRC_BUG;
	case msg_type::order_skip_bkd:
	    throw SRC_BUG;
	case msg_type::answr_skip_done:
	    throw SRC_BUG;
	case msg_type::order_skippable_fwd:
	    return msg_type::order_skippable_fwd_begin;
	case msg_type::order_skippable_bkd:
	    return msg_type::order_skippable_bkd_begin;
	case msg_type::answr_skippable:
	    throw SRC_BUG;
	case msg_type::order_get_position:
	    throw SRC_BUG;
	case msg_type::answr_position:
	    return msg_type::answr_position_begin;
	case msg_type::answr_exception:
	    throw SRC_BUG;
	case msg_type::order_end_of_xmit:
	    throw SRC_BUG;
	case msg_type::order_stop_readahead:
	    throw SRC_BUG;
	case msg_type::answr_readahead_stopped:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}
    }

	//////////////////////////////////////////////////////////////////////

    void messaging_decode::clear()
    {
	buffer.reset();
	msgt = msg_type::unset;
    }

    bool messaging_decode::add_block(const char *x_input, U_I size)
    {
	if(size < 1)
	    throw SRC_BUG;
	else
	{
	    msg_type tmp = char2msg_type(x_input[0]);
	    if(msgt == msg_type::unset
	       || msg_equivalent(msgt, tmp))
		msgt = tmp;
	}

	if(msgt != msg_type::data)
	{
	    buffer.skip_to_eof();
	    buffer.write(x_input + 1, size - 1);
	}

	return !msg_continues(msgt);
    }

    infinint messaging_decode::get_infinint() const
    {
	infinint ret;

	messaging_decode *me = const_cast<messaging_decode *>(this);
	if(me == NULL)
	    throw SRC_BUG;

	me->buffer.skip(0);
	ret.read(me->buffer);

	return ret;
    }

    U_I messaging_decode::get_U_I() const
    {
	U_I lu;
	U_I ret;

	messaging_decode *me = const_cast<messaging_decode *>(this);
	if(me == NULL)
	    throw SRC_BUG;

	    // no ending conversion, we stay in the same process on the same host

	me->buffer.skip(0);
	lu = me->buffer.read((char *)(&ret), sizeof(ret));

	if(lu < sizeof(ret))
	    throw SRC_BUG;

	return ret;
    }


    string messaging_decode::get_string() const
    {
	string ret;

	messaging_decode *me = const_cast<messaging_decode *>(this);
	if(me == NULL)
	    throw SRC_BUG;

	me->buffer.skip(0);
	tools_read_string(me->buffer, ret);

	return ret;
    }

    bool messaging_decode::get_bool() const
    {
	char tmp;
	U_I lu;

	messaging_decode *me = const_cast<messaging_decode *>(this);
	if(me == NULL)
	    throw SRC_BUG;

	    // no ending conversion, we stay in the same process on the same host

	me->buffer.skip(0);
	lu = me->buffer.read(&tmp, sizeof(tmp));
	if(lu < sizeof(tmp))
	    throw SRC_BUG;

	return tmp != 0;
    }


    label messaging_decode::get_label() const
    {
	label ret;

	messaging_decode *me = const_cast<messaging_decode *>(this);
	if(me == NULL)
	    throw SRC_BUG;

	me->buffer.skip(0);
	ret.read(me->buffer);

	return ret;
    }


	//////////////////////////////////////////////////////////////////////


    void messaging_encode::clear()
    {
	buffer.reset();
	msgt = msg_type::unset;
    }


    void messaging_encode::set_infinint(const infinint & val)
    {
	buffer.skip_to_eof();
	val.dump(buffer);
    }


    void messaging_encode::set_U_I(U_I val)
    {
	    // no ending conversion, we stay in the same process on the same host

	buffer.skip_to_eof();
	buffer.write((char *)(&val), sizeof(val));
    }

    void messaging_encode::set_string(const string & val)
    {
	buffer.skip_to_eof();
	tools_write_string(buffer, val);
    }

    void messaging_encode::set_bool(bool val)
    {
	char tmp = val ? 1 : 0;

	buffer.skip_to_eof();
	buffer.write(&tmp, sizeof(tmp));
    }

    void messaging_encode::set_label(const label & val)
    {
	buffer.skip_to_eof();
	val.dump(buffer);
    }

    void messaging_encode::reset_get_block()
    {
	buffer.skip(0);
    }

    bool messaging_encode::get_block(char *ptr, unsigned int & size)
    {
	U_I wrote;
	bool completed = true;

	if(size < 1)
	    throw SRC_BUG;
	ptr[0] = msg_type2char(msgt);
	wrote = buffer.read(ptr + 1, size - 1);

	if(wrote == size - 1) // filled the buffer
	{
	    if(buffer.get_position() < buffer.size())
	    {
		    // some data remains to be written

		ptr[0] = msg_type2char(msg_continuation_of(msgt));
		completed = false;
	    }
	}

	size = wrote + 1;

	return completed;
    }

} // end of generic_thread::namespace

