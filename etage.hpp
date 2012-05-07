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
// $Id: etage.hpp,v 1.3 2002/10/28 20:39:31 edrusb Rel $
//
/*********************************************************************/

#ifndef ETAGE_HPP
#define ETAGE_HPP

#include <list>
#include <string>
#include "infinint.hpp"

struct etage
{
    etage() { fichier.clear(); };
    etage(char *dirname);
    bool read(string & ref);

    list<string> fichier;
    infinint last_mod, last_acc;
};

#endif
