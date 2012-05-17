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
// $Id: string_file.hpp,v 1.1 2005/09/18 20:49:33 edrusb Rel $
//
/*********************************************************************/
//

    /// \file string_file.hpp
    /// \brief emulate a generic_file from a string of characters
    ///

#ifndef STRING_FILE_HPP
#define STRING_FILE_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "integers.hpp"
#include "erreurs.hpp"

namespace libdar
{

	/// class string_file emulates a generic_file frome a std::string

	/// \ingroup Private
    class string_file : public generic_file
    {
    public:

	    /// constructor
	string_file(user_interaction & dialog, const std::string & contents): generic_file(dialog, gf_read_only) { data = contents; cur = 0; len = data.size(); };

	    // inherited from generic_file
	bool skip(const infinint & pos);
	bool skip_to_eof() { cur = len; return true; };
	bool skip_relative(S_I x);
	infinint get_position() { return cur; };

    protected:
	    // inherited from generic_file
	S_I inherited_read(char *a, size_t size);
	S_I inherited_write(char *a, size_t size) { throw Efeature("Writing on a string_file is not allowed"); };

    private:
	std::string data;
	infinint cur;
	infinint len;
    };

} // end of namespace

#endif
