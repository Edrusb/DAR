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
// $Id: header.cpp,v 1.15 2002/04/24 21:07:33 denis Rel $
//
/*********************************************************************/

#include <netinet/in.h>
#include <sstream>
#include <string.h>
#include "header.hpp"

void header::read(generic_file & f)
{
    magic_number tmp;

    f.read((char *)&tmp, sizeof(magic_number));
    magic = ntohl(tmp);
    f.read(internal_name, LABEL_SIZE);
    f.read(&flag, 1);
    f.read(&extension, 1);
    switch(extension)
    {
    case EXTENSION_NO:
	break;
    case EXTENSION_SIZE:
	size_ext.read_from_file(f);
	break;
    default :
	throw Erange("header::read", "badly formated SAR header");
    }
}

void header::write(generic_file & f)
{
    magic_number tmp;

    tmp = htonl(magic);
    f.write((char *)&tmp, sizeof(magic));
    f.write(internal_name, LABEL_SIZE);
    f.write(&flag, 1);
    f.write(&extension, 1);
    switch(extension)
    {
    case EXTENSION_NO:
	break;
    case EXTENSION_SIZE:
	size_ext.dump(f);
	break;
    default:
	throw SRC_BUG;
    }
}

void header::read(int fd)
{
    fichier fic = dup(fd);
    read(fic);
}

void header::write(int fd)
{
    fichier fic = dup(fd);
    write(fic);
}

static void dummy_call(char *x)
{
    static char id[]="$Id: header.cpp,v 1.15 2002/04/24 21:07:33 denis Rel $";
    dummy_call(id);
}

bool header_label_is_equal(const label &a, const label &b)
{
    register int i = 0;
    while(i < LABEL_SIZE && a[i] == b[i])
	i++;
    return i >= LABEL_SIZE;
}

void header_generate_internal_filename(label &ret)
{
    ostringstream tmp;
    const char *p;

    tmp << getpid() << time(NULL);
    p = tmp.str().c_str();
    strncpy(ret, p, LABEL_SIZE);
}

header::header()
{
    magic = 0;
    for(int i = 0; i < LABEL_SIZE; i++)
	internal_name[i] = '\0';
    extension = flag = '\0';
    size_ext = 0;
}

void header::copy_from(const header & ref)
{
    magic = ref.magic;
    label_copy(internal_name,ref.internal_name);
    flag = ref.flag;
    extension = ref.extension;
    size_ext = ref.size_ext;
}

void label_copy(label & left, const label & right)
{
    for(int i = 0; i < LABEL_SIZE; i++)
	left[i] = right[i];

}

void version_copy(version & left, const version & right)
{
    for(int i = 0; i < VERSION_SIZE; i++)
	left[i] = right[i];
}

