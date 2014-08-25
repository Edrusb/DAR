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

    class generic_thread : public generic_file, public contextual
    {
    public:
	    /// constructor
	    ///
	    /// \param[in] ptr is the generic_file that will be managed by a separated thread and becomes property of this new object
	    /// \param[in] block_size is the size of block used to pass information to and from the remote thread
	    /// \param[in] num_block maximum number of blocks that can be sent without being retrieved by the other threads
	    /// \note that the pointed to generic_file becomes property of the generic_thread object
	    /// and will be deleted by it
	generic_thread(generic_file *ptr, U_I block_size, U_I num_block);
	generic_thread(const generic_thread & ref); //< copy constructor is disabled (throws exception)
	const generic_thread & operator = (const generic_thread & ref) { throw SRC_BUG; };
	virtual ~generic_thread();

	    // inherited methods from generic_file
	virtual bool skippable(skippability direction, const infinint & amount);
	virtual bool skip(const infinint & pos);
	virtual bool skip_to_eof();
	virtual bool skip_relative(S_I x);
	virtual infinint get_position();

	    /// check whether the inside generic_file owned by the remote thread is a contextual or not
	bool is_contextual() const;

	    // inherited methods from contextual. The following methods do throw exception is_contextual() is false
	virtual void set_info_status(const std::string & s);
	virtual bool is_an_old_start_end_archive() const;
	virtual const label & get_data_name() const;

    protected:
	    // inherited from generic_file
	virtual void inherited_read_ahead(const infinint & amount);
	virtual U_I inherited_read(char *a, U_I size);
	virtual void inherited_write(const char *a, U_I size);
	virtual void inherited_sync_write();
	virtual void inherited_terminate();

    private:
	libthreadar::tampon<char> toslave;
	libthreadar::tampon<char> tomaster;
	slave_thread *remote;
	bool reached_eof;

	    // the following variables are locally in quite all methods
	    // they do not contain valuable information outside each method call
	messaging_encode order;
	messaging_decode answer;
	unsigned int num;
	char *ptr;
	label dataname;

	void send_order();
	void read_answer();   //< \note ptr must be released/recycled after this call
	void check_answer(msg_type expected);
	void release_block_answer() { tomaster.fetch_recycle(ptr); ptr = NULL; };
    };

} // end of namespace

#endif
