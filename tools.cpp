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
// $Id: tools.cpp,v 1.14 2002/03/18 11:00:54 denis Rel $
//
/*********************************************************************/

#include <iostream>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "tools.hpp"
#include "erreurs.hpp"

char *tools_str2charptr(string x)
{
    unsigned int size = x.size();
    char *ret = new char[size+1];

    if(ret == NULL)
	throw Ememory("tools_str2charptr");
    memcpy(ret, x.c_str(), size);
    ret[size] = '\0';

    return ret;
}

void tools_write_string(generic_file & f, const string & s)
{
    char *tmp = tools_str2charptr(s);

    if(tmp == NULL)
	throw Ememory("tools_write_string");
    try
    {
	unsigned int len;
	unsigned int wrote = 0;

	if(tmp == NULL)
	    throw Ememory("tools_write_string");
	len = strlen(tmp) + 1;
	while(wrote < len)
	    wrote += f.write(tmp+wrote, len-wrote);
    }
    catch(...)
    {
	delete tmp;
    }
    delete tmp;
}

void tools_read_string(generic_file & f, string & s)
{
    char a[2] = { 0, 0 };
    int lu;

    s = "";
    do
    {
	lu = f.read(a, 1);
	if(lu == 1  && a[0] != '\0')
	    s += a;
    }
    while(lu == 1 && a[0] != '\0');

    if(lu != 1 || a[0] != '\0')
	throw Erange("tools_read_string", "not a zero terminated string in file");
}

infinint tools_get_filesize(const path &p)
{
    struct stat buf;
    char *name = tools_str2charptr(p.display());

    if(name == NULL)
	throw Ememory("tools_get_filesize");

    try
    {
	if(lstat(name, &buf) < 0)
	    throw Erange("tools_get_filesize", strerror(errno));
    }
    catch(...)
    {
	delete name;
    }

    delete name;
    return (unsigned long int)buf.st_size;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: tools.cpp,v 1.14 2002/03/18 11:00:54 denis Rel $";
    dummy_call(id);
}
