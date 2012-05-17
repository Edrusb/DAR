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
// to contact the author : dar.linux@free.fr
/*********************************************************************/

    /// \file null_file.hpp
    /// \brief /dev/null type file implementation under the generic_file interface
    ///
    /// this class is used when doing dry-run execution

#ifndef NULL_FILE_HPP
#define NULL_FILE_HPP

#include "../my_config.h"
#include "generic_file.hpp"
#include "thread_cancellation.hpp"

namespace libdar
{

	/// the null_file class implements the /dev/null behavior

	/// this is a generic_file implementation that emulate the
	/// comportment of the /dev/null special file.
	/// all that is writen to is lost, and nothing can be read from
	/// it (empty file). This is a completed implementation all
	/// call are consistent.
	/// \ingroup Private

    class null_file : public generic_file, public thread_cancellation
    {
    public :
        null_file(user_interaction & dialog, gf_mode m) : generic_file(dialog, m) {};
        bool skip(const infinint &pos) { return pos == 0; };
        bool skip_to_eof() { return true; };
        bool skip_relative(signed int x) { return false; };
        infinint get_position() { return 0; };

    protected :
        int inherited_read(char *a, size_t size)
	{
#ifdef MUTEX_WORKS
	    check_self_cancellation();
#endif
	    return 0;
	};

        int inherited_write(const char *a, size_t size)
	{
#ifdef MUTEX_WORKS
	    check_self_cancellation();
#endif
	    return size;
	};
    };

} // end of namespace

#endif
