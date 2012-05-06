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
// $Id: header.hpp,v 1.15 2002/03/18 11:00:54 denis Rel $
//
/*********************************************************************/

#ifndef HEADER_H
#define HEADER_H

#include "infinint.hpp"
#include "generic_file.hpp"
#include "tools.hpp"

#define LABEL_SIZE 10

#define FLAG_NON_TERMINAL 'N'
#define FLAG_TERMINAL 'T'
#define EXTENSION_NO 'N'
#define EXTENSION_SIZE 'S'

#define SAUV_MAGIC_NUMBER 123

typedef unsigned long int magic_number;
typedef char label[LABEL_SIZE];

struct header
{
    magic_number magic;
    label internal_name;
    char flag;
    char extension; // extension rules the use of the following fields
    infinint size_ext; // if EXTENSION_SIZE

    void read(generic_file & f);
    void write(generic_file & f);
    void read(int fd);
    void write(int fd);

    static unsigned int size() { return sizeof(magic_number) + sizeof(label) + 2*sizeof(char); };
};

typedef char version[3];

struct header_version
{
    version edition;
    char algo_zip;
    string cmd_line;
    
    void read(generic_file &f) 
	{ 
	    f.read(edition, sizeof(edition)); 
	    f.read(&algo_zip, sizeof(algo_zip)); 
	    tools_read_string(f, cmd_line); 
	};
    void write(generic_file &f) 
	{ 
	    f.write(edition, sizeof(edition)); 
	    f.write(&algo_zip, sizeof(algo_zip)); 
	    tools_write_string(f, cmd_line);
	};
};


extern bool header_label_is_equal(const label &a, const label &b);
extern void header_generate_internal_filename(label & ret);

        
#endif






