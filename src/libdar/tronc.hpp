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
// $Id: tronc.hpp,v 1.9 2004/11/07 18:21:39 edrusb Rel $
//
/*********************************************************************/

    /// \file tronc.hpp
    /// \brief defines a limited segment over another generic_file.
    ///
    /// This is used to read a part of a file as if it was a real file generating
    /// end of file behavior when reaching the given length.

#ifndef TRONC_HPP
#define TRONC_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"

namespace libdar
{

	/// make a segment of a generic_file appear like a real generic_file

    class tronc : public generic_file
    {
    public :
	    /// constructor

	    //! \param dialog for user interaction
	    //! \param f is the file to take out the segment
	    //! \param offset is the position of the beginning of the segment
	    //! \param size is the size of the segment
        tronc(user_interaction & dialog, generic_file *f, const infinint &offset, const infinint &size);
        tronc(user_interaction & dialog, generic_file *f, const infinint &offset, const infinint &size, gf_mode mode);

	    /// inherited from generic_file
        bool skip(const infinint & pos);
	    /// inherited from generic_file
        bool skip_to_eof();
	    /// inherited from generic_file
        bool skip_relative(S_I x);
	    /// inherited from generic_file
        infinint get_position() { return current; };

    protected :
	    /// inherited from generic_file
        S_I inherited_read(char *a, size_t size);
	    /// inherited from generic_file
        S_I inherited_write(const char *a, size_t size);

    private :
        infinint start, sz;
        generic_file *ref;
        infinint current;
    };

} // end of namespace

#endif
