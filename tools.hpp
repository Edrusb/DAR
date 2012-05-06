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
// $Id: tools.hpp,v 1.11 2002/03/18 11:00:54 denis Rel $
//
/*********************************************************************/

#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <string>
#include "path.hpp"
#include "catalogue.hpp"
#include "generic_file.hpp"

using namespace std;

extern char *tools_str2charptr(string x);
extern void tools_write_string(generic_file & f, const string & s);
extern void tools_read_string(generic_file & f, string & s);
extern infinint tools_get_filesize(const path &p);

#endif
