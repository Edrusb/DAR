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

    /// \file generic_thread.hpp
    /// \brief class generic_thread provides a way to interact with a generic_file ran in an other thread
    /// \ingroup Private
    ///


#ifndef GENERIC_THREAD_HPP
#define GENERIC_THREAD_HPP

#include "../my_config.h"

#include "generic_file.hpp"
#include "messaging.hpp"
#include "slave_thread.hpp"
#include "erreurs.hpp"

namespace libdar
{


    class generic_thread : public generic_file
    {
    public:
	    // default values for constuctor
	static const unsigned int tampon_block_size = 102401;
	static const unsigned int tampon_num_block = 1000;
	static const unsigned int tampon_block_size_ctrl = 1024;
	static const unsigned int tampon_num_block_ctrl = 10;

	    /// constructor
	    ///
	    /// \param[in] ptr is the generic_file that will be read from/written to by a separated thread
	    /// \param[in] block_size is the size of block used to pass information to and from the remote thread
	    /// \param[in] num_block maximum number of blocks that can be sent without being retrieved by the other threads
	    /// \note that the pointed to generic_file must exist during the whole life of the generic_thread. Its memory
	    /// management after the generic_thread has died is under the responsibility of the caller of the generic_thread
	generic_thread(generic_file *ptr,
		       U_I data_block_size = tampon_block_size,
		       U_I data_num_block = tampon_num_block,
		       U_I ctrl_block_size = tampon_block_size_ctrl,
		       U_I ctrl_num_block = tampon_num_block_ctrl);
	generic_thread(const generic_thread & ref); //< copy constructor is disabled (throws exception)
	const generic_thread & operator = (const generic_thread & ref) { throw SRC_BUG; };
	virtual ~generic_thread();

	    // inherited methods from generic_file

	virtual bool skippable(skippability direction, const infinint & amount);
	virtual bool skip(const infinint & pos);
	virtual bool skip_to_eof();
	virtual bool skip_relative(S_I x);
	virtual infinint get_position() const;

    protected:
	    // inherited from generic_file
	virtual void inherited_read_ahead(const infinint & amount);
	virtual U_I inherited_read(char *a, U_I size);
	virtual void inherited_write(const char *a, U_I size);

	    /// generic_file inherited method to sync all pending writes
	    ///
	    /// \note no data in transit in this object so we should not do anything,
	    /// however for performance propagating the order to slave_thread
	virtual void inherited_sync_write();
	virtual void inherited_flush_read();
	virtual void inherited_terminate();


    private:
	libthreadar::fast_tampon<char> toslave_data;
	libthreadar::fast_tampon<char> tomaster_data;
	libthreadar::fast_tampon<char> toslave_ctrl;
	libthreadar::fast_tampon<char> tomaster_ctrl;
	slave_thread *remote;
	bool reached_eof;     //< whether we reached end of file
	char data_header;     //< contains 1 byte header for data
	char data_header_eof; //< contains 1 byte header for data + eof
	bool running;         //< whether a remote is expected to run, tid is then set
	pthread_t tid;        //< thread id of remote

	    // the following variables are locally used in quite all methods
	    // they do not contain valuable information outside each method call
	messaging_encode order;
	messaging_decode answer;
	unsigned int num;
	char *ptr;
	label dataname;

	void send_order();
	void read_answer();   //< \note ptr must be released/recycled after this call
	void check_answer(msg_type expected);
	void wake_up_slave_if_asked(); //< check whether an order to wakeup the slave has been received, and send wake up the slave
	void release_block_answer() { tomaster_ctrl.fetch_recycle(ptr); ptr = nullptr; };
	void release_data_ptr();
	void purge_data_pipe(); //< drops all data in the toslave_data pipe
	void my_run();
	void my_join();
    };

} // end of namespace

#endif
