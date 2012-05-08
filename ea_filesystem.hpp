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
// $Id: ea_filesystem.hpp,v 1.5 2003/02/11 22:01:38 edrusb Rel $
//
/*********************************************************************/
//
#ifndef EA_FILESYSTEM_HPP
#define EA_FILESYSTEM_HPP

#include <vector>
#include <string>
#include "ea.hpp"

extern void ea_filesystem_read_ea(const string & chemin, ea_attributs & val, bool root, bool user);
extern bool ea_filesystem_write_ea(const string & chemin, const ea_attributs & val, bool root, bool user);
    // false if no EA could be set (true if at least one could be set)
extern void ea_filesystem_clear_ea(const string & name, ea_domain dom);
extern bool ea_filesystem_is_present(const string & name, ea_domain dom);

#endif
