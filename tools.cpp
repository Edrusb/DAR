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
// $Id: tools.cpp,v 1.22 2002/06/11 17:46:32 denis Rel $
//
/*********************************************************************/

#include <iostream.h>
#include <strstream.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "tools.hpp"
#include "erreurs.hpp"
#include "deci.hpp"

char *tools_str2charptr(string x)
{
    char *ret;

    ostrstream buf;
    buf << x << '\0';
    ret = buf.str();

    return ret;
}

void tools_write_string(generic_file & f, const string & s)
{
    tools_write_string_all(f, s);
    f.write("", 1); // adding a '\0' at the end;
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

void tools_write_string_all(generic_file & f, const string & s)
{
    char *tmp = tools_str2charptr(s);

    if(tmp == NULL)
	throw Ememory("tools_write_string_all");
    try
    {
	unsigned int len = s.size();
	unsigned int wrote = 0;

	while(wrote < len)
	    wrote += f.write(tmp+wrote, len-wrote);
    }
    catch(...)
    {
	delete tmp;
    }
    delete tmp;
}

void tools_read_string_size(generic_file & f, string & s, infinint taille)
{
    unsigned short small_read = 0;
    size_t max_read = 0;
    int lu = 0;
    const unsigned int buf_size = 10240;
    char buffer[buf_size];
    
    s = "";
    do
    {
	if(small_read > 0)
	{
	    max_read = small_read > buf_size ? buf_size : small_read;
	    lu = f.read(buffer, max_read);
	    small_read -= lu;
	    s += string((char *)buffer, (char *)buffer+lu);
	}
	taille.unstack(small_read);
    }
    while(small_read > 0);
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

infinint tools_get_extended_size(string s)
{
    unsigned int len = s.size();
    infinint factor = 1;

    if(len < 1)
	return false;
    switch(s[len-1])
    {
    case 'K':
    case 'k': // kilobyte
	factor = 1024;
	break;
    case 'M': // megabyte
	factor = infinint(1024)*infinint(1024);
	break;
    case 'G': // gigabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case 'T': // terabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case 'P': // petabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case 'E': // exabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case 'Z': // zettabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case 'Y':  // yottabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
	break;
    default :
	throw Erange("command_line get_extended_size", string("unknown sufix in string : ")+s);
    }

    if(factor != 1)
	s = string(s.begin(), s.end()-1);

    deci tmp = s;
    factor *= tmp.computer();

    return factor;
}

char *tools_extract_basename(const char *command_name)
{
    path commande = command_name;
    string tmp = commande.basename();
    char *name = tools_str2charptr(tmp);
    
    return name;
}


static void dummy_call(char *x)
{
    static char id[]="$Id: tools.cpp,v 1.22 2002/06/11 17:46:32 denis Rel $";
    dummy_call(id);
}

void tools_split_path_basename(const char *all, path * &chemin, string & base)
{
    chemin = new path(all);
    if(chemin == NULL)
	throw Ememory("tools_split_path_basename");

    try
    {
	if(chemin->degre() > 1)
	{
	    if(!chemin->pop(base))
		throw SRC_BUG; // a path of degree > 1 should be able to pop()
	}
	else
	{
	    delete chemin;
	    base = all;
	    chemin = new path(".");
	    if(chemin == NULL)
		throw Ememory("tools_split_path_basename");
	}
    }
    catch(...)
    {
	if(chemin != NULL)
	    delete chemin;
	throw;
    }
}

void tools_open_pipes(const string &input, const string & output, tuyau *&in, tuyau *&out)
{
    in = out = NULL;
    try
    {
	if(input != "")
	    in = new tuyau(input, gf_read_only);
	else
	    in = new tuyau(0, gf_read_only); // stdin by default
	if(in == NULL)
	    throw Ememory("tools_open_pipes");
	
	if(output != "")
	    out = new tuyau(output, gf_write_only);
	else
	    out = new tuyau(1, gf_write_only); // stdout by default
	if(out == NULL)
	    throw Ememory("tools_open_pipes");

    }
    catch(...)
    {
	if(in != NULL)
	    delete in;
	if(out != NULL)
	    delete out;
	throw;
    }
}

void tools_blocking_read(int fd, bool mode)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if(!mode)
	flags |= O_NDELAY;
    else
	flags &= ~O_NDELAY;
    fcntl(fd, F_SETFL, flags);
}
