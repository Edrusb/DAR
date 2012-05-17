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

    /// \file terminateur.hpp
    /// \brief the terminateur class which defines the position of the catalogue
    ///
    /// the terminateur is a byte sequence present as the last bytes of an archive
    /// which indicates how much byte backward libdar must skip back to find the
    /// beginning of the catalogue.

#ifndef TERMINATEUR_HPP
#define TERMINATEUR_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "header_version.hpp"

namespace libdar
{

	/// terminateur class indicates the location of the beginning of the catalogue

	/// it is the last bytes sequence of an archive.
	/// \ingroup Private

    class terminateur
    {
    public :
        void set_catalogue_start(infinint xpos) { pos = xpos; };
        void dump(generic_file &f);
        void read_catalogue(generic_file &f, bool with_elastic, const dar_version & reading_ver);
        infinint get_catalogue_start() { return pos; };

    private :
        infinint pos;
    };

} // end of namespace

#endif
