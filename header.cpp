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
// $Id: header.cpp,v 1.12.2.1 2003/04/16 19:21:20 edrusb Rel $
//
/*********************************************************************/

#pragma implementation

#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
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

void header::read(S_I fd)
{
    fichier fic = dup(fd);
    read(fic);
}

void header::write(S_I fd)
{
    fichier fic = dup(fd);
    write(fic);
}

static void dummy_call(char *x)
{
    static char id[]="$Id: header.cpp,v 1.12.2.1 2003/04/16 19:21:20 edrusb Rel $";
    dummy_call(id);
}

bool header_label_is_equal(const label &a, const label &b)
{
    register S_I i = 0;
    while(i < LABEL_SIZE && a[i] == b[i])
	i++;
    return i >= LABEL_SIZE;
}

void header_generate_internal_filename(label &ret)
{
    time_t src1 = time(NULL);
    pid_t src2 = getpid();
    U_I wrote = 0;
    U_I read = 0;

    while(read < sizeof(src1) && wrote < LABEL_SIZE)
	ret[wrote++] = ((char *)(&src1))[read++];

    read = 0;
    while(read < sizeof(src2) && wrote < LABEL_SIZE)
	ret[wrote++] = ((char *)(&src2))[read++];
}

header::header()
{
    magic = 0;
    for(S_I i = 0; i < LABEL_SIZE; i++)
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
    for(S_I i = 0; i < LABEL_SIZE; i++)
	left[i] = right[i];

}
