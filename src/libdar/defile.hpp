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
// $Id: defile.hpp,v 1.5.4.1 2004/07/25 20:38:03 edrusb Exp $
//
/*********************************************************************/

#ifndef DEFILE_HPP
#define DEFILE_HPP

#include "../my_config.h"
#include "catalogue.hpp"
#include "path.hpp"

namespace libdar
{

    class defile
    {
    public :
        defile(const path &racine) : chemin(racine) { init = true; };

        void enfile(const entree *e);
        path get_path() const { return chemin; };
        std::string get_string() const { return chemin.display(); };

    private :
        path chemin;
        bool init;
    };

} // end of namespace

#endif
