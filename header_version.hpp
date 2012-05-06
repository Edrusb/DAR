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
// $Id: header_version.hpp,v 1.2 2002/05/15 21:56:01 denis Rel $
//
/*********************************************************************/

#ifndef HEADER_VERSION_HPP
#define HEADER_VERSION_HPP

#pragma interface

#include "generic_file.hpp"
#include "tools.hpp"

#define VERSION_FLAG_SAVED_EA_ROOT 0x80
#define VERSION_FLAG_SAVED_EA_USER 0x40

#define VERSION_SIZE 3
typedef char version[VERSION_SIZE];
extern void version_copy(version & left, const version & right);
extern bool version_greater(const version & left, const version & right);
    // operation left > right

struct header_version
{
    version edition;
    char algo_zip;
    string cmd_line;
    unsigned char flag; // added at edition 02
    
    void read(generic_file &f) 
	{ 
	    f.read(edition, sizeof(edition)); 
	    f.read(&algo_zip, sizeof(algo_zip)); 
	    tools_read_string(f, cmd_line); 
	    if(version_greater(edition, "01"))
		f.read((char *)&flag, (size_t)1);
	    else
		flag = 0;
	};
    void write(generic_file &f) 
	{ 
	    f.write(edition, sizeof(edition)); 
	    f.write(&algo_zip, sizeof(algo_zip)); 
	    tools_write_string(f, cmd_line);
	    f.write((char *)&flag, (size_t)1);
	};
};

#endif
