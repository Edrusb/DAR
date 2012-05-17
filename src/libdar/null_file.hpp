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
// $Id: null_file.hpp,v 1.4.4.1 2004/07/25 20:38:03 edrusb Exp $
//
/*********************************************************************/

#ifndef NULL_FILE_HPP
#define NULL_FILE_HPP

#include "../my_config.h"
#include "generic_file.hpp"

namespace libdar
{

    class null_file : public generic_file
    {
    public :
        null_file(gf_mode m) : generic_file(m) {};
        bool skip(const infinint &pos) { return pos == 0; };
        bool skip_to_eof() { return true; };
        bool skip_relative(signed int x) { return false; };
        infinint get_position() { return 0; };

    protected :
        int inherited_read(char *a, size_t size) { return 0; };
        int inherited_write(const char *a, size_t size) { return size; };
    };

} // end of namespace

#endif
