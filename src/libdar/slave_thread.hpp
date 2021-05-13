/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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

    /// \file slave_thread.hpp
    /// \brief class slave_thread is runs a I/O operations on a given generic_file in a separated thread
    /// \ingroup Private


#ifndef SLAVE_THREAD_HPP
#define SLAVE_THREAD_HPP

#include "../my_config.h"

#if HAVE_LIBTHREADAR_LIBTHREADAR_HPP
#include <libthreadar/libthreadar.hpp>
#endif

#include "generic_file.hpp"
#include "messaging.hpp"

namespace libdar
{

	/// \addtogroup Private
        /// @{

    class slave_thread : public libthreadar::thread
    {
    public:
	    /// constructor

	    /// \note none of the given pointer will be deleted by slave_thread nor a copy
	    /// of them will be done (which for some is forbidden anyway), these pointed to
	    /// objects must thus exist during the whole live of the slave_thread
	slave_thread(generic_file *x_data,
		     libthreadar::fast_tampon<char> *x_input_data,
		     libthreadar::fast_tampon<char> *x_output_data,
		     libthreadar::fast_tampon<char> *x_input_ctrl,
		     libthreadar::fast_tampon<char> *x_output_ctrl);
	slave_thread(const slave_thread & ref) = delete;
	slave_thread(slave_thread && ref) noexcept = delete;
	slave_thread & operator = (const slave_thread & ref) = delete;
	slave_thread & operator = (slave_thread && ref) noexcept = delete;
	~slave_thread() noexcept {};

	    /// true if the thread has suspended waiting for a new order (no data to write, no read_ahead to perform)
	bool wake_me_up() const { if(wake_me) { const_cast<slave_thread *>(this)->wake_me = false; return true; } else return false; };

    protected:
	virtual void inherited_run() override;

    private:
	generic_file *data;
	libthreadar::fast_tampon<char> *input_data;
	libthreadar::fast_tampon<char> *output_data;
	libthreadar::fast_tampon<char> *input_ctrl;
	libthreadar::fast_tampon<char> *output_ctrl;

	messaging_encode answer;    ///< used to communicate with master thread
	messaging_decode order;     ///< used to communicate with master thread
	unsigned int num;           ///< size of the read block or to be written block
	char *ptr;                  ///< address of the block to be written or to be read
	char data_header;           ///< the one byte message to prepend data with, when more data block follow to answer a given order
	char data_header_completed; ///< the one byte message to prepend data with, when end of order is reached
	infinint read_ahead;        ///< amount of data sent for reading and not yet asked for reading
	bool endless_read_ahead;    ///< whether an endeless read ahead request has been asked
	infinint to_send_ahead;     ///< remaining amount of data to send for the requested read_ahead
	U_I immediate_read;         ///< next action is to read this amount of data
	bool stop;                  ///< whether thread end has been asked
	bool wake_me;               ///< whether we ask the master awake us

	void init();
	void set_header_vars(); ///< set the value of data_header and data_header_completed fields
	void read_order();      ///< \note ptr must be released/recycled after this call
	void send_answer();     ///< send the answer to the last order received
	bool pending_order() { return input_ctrl->is_not_empty(); };
	bool pending_input_data() { return input_data->is_not_empty(); };
	void treat_input_data(); ///< empty input_data and write down the data
	void ask_to_wake_me_up(); ///< ask the master to wake the slave upon possible action

	    /// send a block of data to the master thread

	    /// \param[in] size is the number of byte to send, no more than
	    /// one block will be sent even if size is larger. If size is zero
	    /// as much as possible data will be sent in the block
	    /// \param[out] eof is set back to true of the operation reached end of file
	    /// \return the effective number of byte sent in the block
	U_I send_data_block(U_I size, bool & eof);

	bool treat_order(); ///< \return true if answer is prepared and must be sent back to the master thread

	    /// send the amount of byte equal to "immediate_read" to the master thread (counting read_ahead data)
	void go_read();
    };

	///@}

} // end of namespace

#endif
