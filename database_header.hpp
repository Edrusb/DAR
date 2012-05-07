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
// $Id: database_header.hpp,v 1.1 2002/10/28 20:39:29 edrusb Rel $
//
/*********************************************************************/

#ifndef DATABASE_HEADER_HPP
#define DATABASE_HEADER_HPP

#include "generic_file.hpp"

struct database_header
{
    unsigned char version;
    unsigned char options;
    
    void read(generic_file & f) { f.read((char *)&version, 1); f.read((char *)&options, 1); };
    void write(generic_file & f) { f.write((char *)&version, 1); f.write((char *)&options, 1); };
};

extern generic_file *database_header_create(const string & filename, bool overwrite);
extern generic_file *database_header_open(const string & filename);

#endif
