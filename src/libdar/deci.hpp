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
// $Id: deci.hpp,v 1.6 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/

#ifndef DECI_HPP
#define DECI_HPP

#pragma interface

#include "../my_config.h"
#include "storage.hpp"
#include "infinint.hpp"
#include <iostream>

namespace libdar
{

    class deci
    {
    public :
        deci(std::string s) throw(Edeci, Ememory, Erange, Ebug);
        deci(const infinint & x) throw(Ememory, Erange, Ebug);
        deci(const deci & ref) throw(Ememory, Erange, Ebug)
            { E_BEGIN; copy_from(ref); E_END("deci::deci", "deci"); };
        ~deci() throw(Ebug)
            { E_BEGIN; detruit(); E_END("deci::~deci", ""); };
    

        deci & operator = (const deci & ref) throw(Ememory, Erange, Ebug)
            { E_BEGIN; detruit(); copy_from(ref); return *this; E_END("deci::operator = ", ""); };

        infinint computer() const throw(Ememory, Erange, Ebug);
        std::string human() const throw(Ememory, Erange, Ebug);

    private :
        storage *decimales;

        void detruit() throw(Ebug);
        void copy_from(const deci & ref) throw(Ememory, Erange, Ebug);
        void reduce() throw(Ememory, Erange, Ebug);
    };

    extern std::ostream & operator << (std::ostream & ref, const infinint & arg);
} // end of namespace

#endif
