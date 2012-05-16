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
// $Id: deci.hpp,v 1.6.4.2 2004/07/25 20:38:03 edrusb Exp $
//
/*********************************************************************/

#ifndef DECI_HPP
#define DECI_HPP

#include "../my_config.h"
#include <iostream>
#include "storage.hpp"
#include "infinint.hpp"

namespace libdar
{

    class deci
    {
    public :
        deci(std::string s);
        deci(const infinint & x);
        deci(const deci & ref)
            { E_BEGIN; copy_from(ref); E_END("deci::deci", "deci"); };
        ~deci()
            { E_BEGIN; detruit(); E_END("deci::~deci", ""); };


        deci & operator = (const deci & ref)
            { E_BEGIN; detruit(); copy_from(ref); return *this; E_END("deci::operator = ", ""); };

        infinint computer() const;
        std::string human() const;

    private :
        storage *decimales;

        void detruit();
        void copy_from(const deci & ref);
        void reduce();
    };

    extern std::ostream & operator << (std::ostream & ref, const infinint & arg);
} // end of namespace

#endif
