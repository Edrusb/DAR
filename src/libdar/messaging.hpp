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

    /// \file messaging.hpp
    /// \brief messaging_decode and messaging_encode are used to insert messages in a flow if data blocks
    /// \ingroup Private
    ///


#ifndef MESSAGING_HPP
#define MESSAGING_HPP

#include "../my_config.h"

#include <string>

#include "label.hpp"
#include "infinint.hpp"
#include "memory_file.hpp"
#include "infinint.hpp"

namespace libdar
{

    enum class msg_type
    {
	unset,                    //< no argument: message type is not set (error)
	data,                     //< + data     : block is pure data (no message in it)
	order_read_ahead,         //< + infinint : messge is an info that the given amount of data is about to be read
	order_read_ahead_begin,   //< + infinint : message continues with the next block
	order_read,               //< + U_I      : message is a read order (with expected size to be read ahead)
        answr_read_eof,           //< + data     : message from slave meaning slave has finished reading and expects new order, does not mean eof
	order_sync_write,         //< no argument: order to flush all pending writes
	order_skip,               //< + infinint : message is an order to seek at given position
	order_skip_begin,         //< + infinint : message is an order to seek but the seek info continues in the next message
	order_skip_to_eof,        //< no argument: message requesting slave to skip to end of file
	order_skip_fwd,           //< + U_I      : order to skip foward
	order_skip_bkd,           //< + U_I      : order to skip backward
	answr_skip_done,          //< + bool     : answer for all kind of skip orders
	order_set_context,        //< + string   : message contains the new contextual value to be set
        order_skippable_fwd,      //< + infinint : message from master containing a skippable forward request info
        order_skippable_fwd_begin,//< + infinint : message continues on the next block
        order_skippable_bkd,      //< + infinint : message from master containing a skippable backward request info
	order_skippable_bkd_begin,//< + infinint : message continues on the next block
	answr_skippable,          //< + bool     : answer from slace to a skippable forward/backward request
	order_get_position,       //< no argument: order to get the current position in file
	answr_position,           //< + infinint : answer with the current position
	answr_position_begin,     //< + infinint : message continues with the next block
	order_is_oldarchive,      //< no argument: master request to know whethe archive is old or not
	answr_oldarchive,         //< + bool     : slave answer containing boolean answer
	order_get_dataname,       //< no argument: master request of the dataname
	answr_dataname,           //< + label    : slave answer containing the dataname
	answr_not_contextual,     //< no argument: slave answer if the managed generic file is not a contextual object
	answr_exception,          //< no argument: last operation generated an exception for at slave side, slave has probably died after that
	order_end_of_xmit         //< no argument: message is the last message and implies freedom of the slave
    };

    extern bool msg_equivalent(msg_type arg1, msg_type arg2);
    extern bool msg_continues(msg_type msg);
    extern char msg_type2char(msg_type x);
    extern msg_type char2msg_type(char x);
    extern msg_type msg_continuation_of(msg_type x);


    class messaging_decode : public on_pool
    {
    public:
	messaging_decode() {msgt = msg_type::unset; };

	    /// reset the object to its initial state
	void clear();

	    /// add a block of data to be decoded
	    ///
	    /// \param[in] x_input is the address of the message to add
	    /// \param[in] x_size is the size of the message to add
	    /// \return true if the message is complet, false if a new block
	    /// need to be added with this call in order to decode the message
	bool add_block(const char *x_input, U_I x_size);

	    /// get the type of message pointed to at construction time
	msg_type get_type() const { return msgt; };

	    /// for messages of type order_skip, answr_filesize, order_read_ahead, answr_filesize,
	infinint get_infinint() const;

	    /// for messages of type order_read
	U_I get_U_I() const;

	    /// for messages of type order_set_context
	std::string get_string() const;

	    /// for messages of type anwsr_oldarchive
	bool get_bool() const;

	    /// for messages of type answr_get_dataname,
	label get_label() const;

    private:
	msg_type msgt;
	memory_file buffer;

    };


    class messaging_encode : public on_pool
    {
    public:
	    /// constructor
	messaging_encode() { msgt = msg_type::unset; };

	    /// reset the object to its initial state
	void clear();

	    /// define the type of the message to generate
	void set_type(msg_type val) { msgt = val; };

	    /// add infininit attribute
	void set_infinint(const infinint & val);

	    /// add U_I attribute
	void set_U_I(U_I val);

	    /// add string attribute
	void set_string(const std::string & val);

	    /// set boolean attribute
	void set_bool(bool val);

	    /// set label attribute
	void set_label(const label & val);

	    /// set the read block pointer to the first block
	void reset_get_block();

	    /// read the next block
	    ///
	    /// \param[in] ptr is the address where to write the next block of the message
	    /// \param[in,out] size is the maximum amount of byte that can be written to ptr
	    /// and is modified by this call to the effective number of byte written to ptr
	    /// \return true if the whole message could be written to block, else false
	    /// is returned an a new call to get_block is necessary to write down the rest of
	    /// the message up to the time get_block() returns true
	    /// \note for data encoding, the message is a single byte length.
	    /// this byte has been placed before the data before in the same block
	bool get_block(char * ptr, unsigned int & size);

    private:
	msg_type msgt;
	memory_file buffer;

    };


} // end of namespace

#endif
