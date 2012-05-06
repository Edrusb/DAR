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
// $Id: tools.hpp,v 1.18 2002/05/15 21:56:01 denis Rel $
//
/*********************************************************************/

#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <string>
#include "path.hpp"
#include "generic_file.hpp"
#include "tuyau.hpp"

using namespace std;

extern char *tools_str2charptr(string x);
extern void tools_write_string(generic_file & f, const string & s); 
    // add a '\0' at the end
extern void tools_read_string(generic_file & f, string & s); 
    // read up to '\0' char
extern void tools_write_string_all(generic_file & f, const string & s); 
    // '\0' has no special meaning no '\0' at the end
extern void tools_read_string_size(generic_file & f, string & s, infinint taille); 
    // '\0' has no special meaning
extern infinint tools_get_filesize(const path &p);
extern infinint tools_get_extended_size(string s);
extern char *tools_extract_basename(const char *command_name); 
extern void tools_split_path_basename(const char *all, path * &chemin, string & base);
extern void tools_open_pipes(const string &input, const string & output, tuyau *&in, tuyau *&out);
extern void tools_blocking_read(int fd, bool mode);

#endif



