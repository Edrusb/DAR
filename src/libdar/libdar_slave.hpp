/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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
// to contact the author, see the AUTHOR file
/*********************************************************************/

    /// \file libdar_slave.hpp
    /// \brief API for dar_slave functionnality
    /// \ingroup API

#ifndef LIBDAR_SLAVE_HPP
#define LIBDAR_SLAVE_HPP

#include "../my_config.h"

#include <memory>

#include "infinint.hpp"
#include "user_interaction.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{


	/// class implementing the dar_slave feature

    class libdar_slave
    {
    public:
	    /// libdar_slave constructor

	    /// \param[in] dialog for user interaction. Can be set to nullptr
	    /// \param[in] folder is the directory where resides the backup to read
	    /// \param[in] basename is the backup basename
	    /// \param[in] extension should be set to "dar"
	    /// \param[in] input_pipe_is_fd if true the input_pipe argument is expected to be an
	    /// integer (a file descriptor open for reading)
	    /// \param[in] input_pipe is the name of the pipe order will come from or a filedescriptor if input_pipe_is_fd is true
	    /// \param[in] output_pipe_is_fd if true the output_pipe argument is expected to be
	    /// an integer (a file descriptor open for writing)
	    /// \param[in] output_pipe is the name of the pipe to send data to dar or a filedescriptor depending
	    /// on output_pipe_is_fd value
 	    /// \param[in] execute is a command to execute before reading a new slice, same macro substition is available as
	    /// libdar::archive::set_execute()
	    /// \param[in] min_digits minimum digits used to create the archive. Set it to zero if this option was not used
	    /// at archive creation time
	    /// \note if input_pipe is an empty string stdin is used
	    /// \note if output_pipe is an empty string stdout is used
	libdar_slave(std::shared_ptr<user_interaction> & dialog,
		     const std::string & folder,
		     const std::string & basename,
		     const std::string & extension,
		     bool input_pipe_is_fd,
		     const std::string & input_pipe,
		     bool output_pipe_is_fd,
		     const std::string & output_pipe,
		     const std::string & execute,
		     const infinint & min_digits);
	libdar_slave(const libdar_slave & ref) = delete;
	libdar_slave(libdar_slave && ref) noexcept = default;
	libdar_slave & operator = (const libdar_slave & ref) = delete;
	libdar_slave & operator = (libdar_slave && ref) noexcept = default;
	~libdar_slave();

	    /// enslave this object to the dar process through the created pipes

	    /// \note run() will return when dar will no more need of this slave.
	    /// if you need to abort this run(), let dar abort properly this will do the
	    /// expected result properly.
	void run();

    private:
	class i_libdar_slave;
	std::unique_ptr<i_libdar_slave> pimpl;

    };

	/// @}

} // end of namespace

#endif
