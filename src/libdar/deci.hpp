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

    /// \file deci.hpp
    /// \brief manages the decimal representation of infinint
    /// \ingroup Tools

#ifndef DECI_HPP
#define DECI_HPP

#include "../my_config.h"
#include <iostream>
#include "storage.hpp"
#include "infinint.hpp"
#include "on_pool.hpp"

namespace libdar
{

	/// \ingroup Tools
	/// @}


	/// decimal class, convert infinint from and to decimal represention

	/// the class contains the decimal representation of an integer
	/// and can produce a readable string to display the corresponding
	/// integer it can also produce a computer value corresponding to
	/// the decimal value. In the other side, objects of this class can
	/// be built from a integer as well as from a string representing
	/// the decimals of an integer.
    class deci : public on_pool
    {
    public :
	    /// constructor to build a "deci" object from a string representing decimals
	    /// \note may throw Edeci exception if the given string does not correspond to a
	    /// positive integer in decimal notation
        deci(std::string s);

	    /// constructor to build a "deci" from an infinint
        deci(const infinint & x);

	    /// copy constructor
        deci(const deci & ref)
	{ copy_from(ref); };

	    /// destructor
        ~deci()
	{ detruit(); };


	    /// copy operator
        deci & operator = (const deci & ref)
	{ detruit(); copy_from(ref); return *this; };

	    /// this produce a infinint from the decimal stored in the current object
        infinint computer() const;
	    /// this produce a string from the decimal stored in the current object
        std::string human() const;

    private :
        storage *decimales;

        void detruit();
        void copy_from(const deci & ref);
        void reduce();
    };

	/// specific << operator to use infinint in std::ostream

	/// including "deci.hpp" let this operator available so you can
	/// display infinint with the << std::ostream operator as you can
	/// do for standard types.
	/// \ingroup Tools
    extern std::ostream & operator << (std::ostream & ref, const infinint & arg);

	/// @}

} // end of namespace

#endif
