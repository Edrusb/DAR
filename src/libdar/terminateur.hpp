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
// $Id: terminateur.hpp,v 1.5.4.1 2004/07/25 20:38:04 edrusb Exp $
//
/*********************************************************************/

#ifndef TERMINATEUR_HPP
#define TERMINATEUR_HPP

#include "../my_config.h"
#include "generic_file.hpp"

namespace libdar
{

    class terminateur
    {
    public :
        void set_catalogue_start(infinint xpos) { pos = xpos; };
        void dump(generic_file &f);
        void read_catalogue(generic_file &f);
        infinint get_catalogue_start() { return pos; };

    private :
        infinint pos;
    };

} // end of namespace

#endif
