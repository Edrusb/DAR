/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: tronc.hpp,v 1.8 2002/05/15 21:56:01 denis Rel $
//
/*********************************************************************/

#ifndef TRONC_HPP
#define TRONC_HPP

#pragma interface

#include "generic_file.hpp"

class tronc : public generic_file
{
public :
    tronc(generic_file *f, const infinint &offset, const infinint &size);
    tronc(generic_file *f, const infinint &offset, const infinint &size, gf_mode mode);

    bool skip(infinint pos);
    bool skip_to_eof();
    bool skip_relative(signed int x);
    infinint get_position() { return current; };

protected :
    int inherited_read(char *a, size_t size);
    int inherited_write(const char *a, size_t size);

private :
    infinint start, sz;
    generic_file *ref;
    infinint current;
};

#endif
