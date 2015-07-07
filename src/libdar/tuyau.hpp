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

    /// \file tuyau.hpp
    /// \brief defines the implementation of pipe under the generic_file interface.
    /// \ingroup Private
    ///
    /// mainly used between zapette and slave_zapette, this is a full implementation
    /// of the generic_file interface that takes care of dead lock when two pipes needs
    /// to be openned between the same two entities, each having one for reading and the
    /// other for writing.

#ifndef TUYAU_HPP
#define TUYAU_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "thread_cancellation.hpp"
#include "mem_ui.hpp"

namespace libdar
{

	/// pipe implementation under the generic_file interface.

	/// \ingroup Private

    class tuyau : public generic_file, public thread_cancellation, protected mem_ui
    {
    public:
        tuyau(const user_interaction & dialog, //< for user interaction
	      int fd);                         //< fd is the filedescriptor of a pipe extremity already openned
        tuyau(const user_interaction & dialog, //< for user interaction
	      int fd,                          //< fd is the filedescriptor of a pipe extremity already openned
	      gf_mode mode);                   //< forces the mode if possible
        tuyau(const user_interaction & dialog, //< for user interaction
	      const std::string &filename,     //< named pipe to open
	      gf_mode mode);                   //< forces the mode if possible
	tuyau(const user_interaction & dialog);//< creates a anonymous pipe and bind itself to the writing end. The reading end can be obtained by get_read_side() method
        ~tuyau();

	    // provides the reading end of the anonymous pipe when the current object has created it (no filedesc, no path given to constructor).
	    // it cannot be called more than once.
	int get_read_fd() const;

	    /// closes the read fd of the anonymous pipe (this is to be used by a writer)

	    /// \note to ensure a proper behavior of the 'eof', the writer must close the read fd
	    /// this call let this be done, assuming the read has already fetched the fd and forked
	    /// in a new process
	void close_read_fd();

	    /// ask to not close the read descriptor upon object destruction (the fd survives the object)
	void do_not_close_read_fd();

            // inherited from generic_file
	bool skippable(skippability direction, const infinint & amount);
        bool skip(const infinint & pos);
        bool skip_to_eof();
        bool skip_relative(signed int x);
        infinint get_position() { return position; };

	bool has_next_to_read();

    protected:
	void inherited_read_ahead(const infinint & amount) {}; // relying on the operating system
        virtual U_I inherited_read(char *a, U_I size);
        virtual void inherited_write(const char *a, U_I size);
	void inherited_sync_write() {};
	void inherited_flush_read() {};
	void inherited_terminate();

    private:
	enum
	{
	    pipe_fd,        //< holds a single file descriptor for the pipe
	    pipe_path,      //< holds a filename to be openned (named pipe)
	    pipe_both       //< holds a pair of file descriptors
	}
	    pipe_mode;          //< defines how the object's status (which possible values defined by the anonymous enum above)
	infinint position;  //< recorded position in the stream
	int filedesc;       //< file descriptors of the pipe
	int other_end_fd;   //< in pipe_both mode, this holds the reading side of the anonymous pipe
        std::string chemin; //< in pipe_path mode only, this holds the named pipe to be open
	bool has_one_to_read; //< if true, the next char to read is placed in "next_to_read"
	char next_to_read;  //< when has_one_to_read is true, contains the next to read byte

        void ouverture();

	    /// skip forward by reading data

	    /// \param[in] byte is the amount of byte to skip forward
	    /// \return true if the given amount of byte could be read, false otherwise (reached EOF).
	bool read_and_drop(infinint byte);

	    /// skip to eof by reading data
	bool read_to_eof();
    };

} // end of namespace

#endif
