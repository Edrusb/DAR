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

    /// \file slave_thread.hpp
    /// \brief class slave_thread is runs a I/O operations on a given genercif_file in a separated thread
    /// \ingroup Private
    ///


#ifndef SLAVE_THREAD_HPP
#define SLAVE_THREAD_HPP

#include "../my_config.h"

#if HAVE_LIBTHREADAR_LIBTHREADAR_HPP
#include <libthreadar/libthreadar.hpp>
#endif

#include "generic_file.hpp"
#include "messaging.hpp"
#include "erreurs.hpp"

namespace libdar
{


    class slave_thread : public libthreadar::thread, public on_pool
    {
    public:
	    /// constructor
	    ///
	    /// \note none of the given pointer will be deleted by slave_thread nor a copy
	    /// of them will be done (which for some is forbidden anyway), these pointed to
	    /// objects must thus exist during the whole live of the slave_thread
	slave_thread(generic_file *x_data, libthreadar::tampon<char> *x_input, libthreadar::tampon<char> *x_output);
	slave_thread(const slave_thread & ref) { throw SRC_BUG; };
	const slave_thread & operator = (const slave_thread & ref) { throw SRC_BUG; };

    protected:
	virtual void inherited_run();

    private:
	generic_file *data;
	libthreadar::tampon<char> *input;
	libthreadar::tampon<char> *output;

	messaging_encode answer;    //< used to communicate with master thread
	messaging_decode order;     //< used to communicate with master thread
	unsigned int num;           //< size of the read block or to be written block
	char *ptr;                  //< address of the block to be written or to be read
	char data_header;           //< the one byte message to prepend data with in all data blocks
	char data_eof_header;       //< the one byte message to prepend data with when EOF is reached
	infinint read_ahead = 0;    //< amount of data sent for reading and not yet asked for reading
	infinint to_send_ahead = 0; //< remaining amount of data to send for the requested read_ahead
	U_I immediate_read = 0; //< next action is to read this amount of data
	bool stop;


	void set_header_vars();
	void read_order();      //< \note ptr must be released/recycled after this call
	void send_answer();
	bool pending_order() { return input->is_not_empty(); };

	    /// send a block of data to the master thread
	    ///
	    /// \param[in] size is the number of byte to send, no more than
	    /// one block will be sent even if size is larger. If size is zero
	    /// as much as possible data will be sent in the block
	    /// \return the effective number of byte sent in the block
	U_I send_data_block(U_I size);

	bool treat_order(); //< \return true if answer is prepared and must be sent back to the master thread

	    /// send the amount of byte equal to "immediate_read" to the master thread (counting read_ahead data)
	void go_read();
    };

} // end of namespace

#endif
