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
// $Id: tuyau.cpp,v 1.5 2002/06/11 17:46:32 denis Rel $
//
/*********************************************************************/

#pragma implementation

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "tuyau.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "user_interaction.hpp"

tuyau::tuyau(int fd) : generic_file(generic_file_get_mode(fd))
{
    if(filedesc < 0)
	throw Erange("tuyau::tuyau", "bad file descriptor given");
    filedesc = fd;
    position = 0;
    chemin = "";
}

tuyau::tuyau(int fd, gf_mode mode) : generic_file(mode)
{
    gf_mode tmp;

    if(filedesc < 0)
	throw Erange("tuyau::tuyau", "bad file descriptor given");
    tmp = generic_file_get_mode(fd);
    if(tmp != gf_read_write && tmp != mode)
	throw Erange("tuyau::tuyau", generic_file_get_name(tmp)+" cannot be restricted to "+generic_file_get_name(mode));
    filedesc = fd;
    position = 0;
    chemin = "";
}

tuyau::tuyau(const string & filename, gf_mode mode) : generic_file(mode)
{
    chemin = filename;
    position = 0;
    filedesc = -1; // not yet openned
}

bool tuyau::skip(infinint pos)
{
    if(pos != position)
	throw Erange("tuyau::skip", "skipping is not possible on a tuyau (= pipe)");
    return true;
}

bool tuyau::skip_to_eof()
{
    throw Erange("tuyau::skip", "skipping is not possible on a tuyau (= pipe)");
}

bool tuyau::skip_relative(signed int x)
{
    if(x != 0)
	throw Erange("tuyau::skip", "skipping is not possible on a tuyau (= pipe)");
    return true;
}

int tuyau::inherited_read(char *a, size_t size)
{
    int ret;
    unsigned int lu = 0;

    if(filedesc < 0)
	ouverture();

    do
    {
	ret = ::read(filedesc, a+lu, size-lu);
	if(ret < 0)
	{
	    switch(errno)
	    {
	    case EINTR:
		break;
	    case EIO:
		throw Ehardware("tuyau::inherited_read", "");
	    default:
		throw Erange("tuyau::inherited_read", string("error while reading from pipe: ")+strerror(errno));
	    }
	}
	else
	    if(ret > 0)
		lu += ret;
    }
    while(lu < size && ret != 0);
    position += lu;

    return lu;
}

int tuyau::inherited_write(const char *a, size_t size)
{
    size_t total = 0;

    if(filedesc < 0)
	ouverture();

    while(total < size)
    {
	int ret = ::write(filedesc, a+total, size-total);
	if(ret < 0)
	{
	    switch(errno)
	    {
	    case EINTR:
		break;
	    case EIO:
		throw Ehardware("tuyau::inherited_write", strerror(errno));
	    case ENOSPC:
		user_interaction_pause("no space left on device, you have the oportunity to make room now. When ready : can we continue ?");
		break;
	    default :
		throw Erange("tuyau::inherited_write", string("error while writing to pipe: ") + strerror(errno));
	    }
	}
	else
	    total += (unsigned int)ret;
    }

    position += total;
    return total;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: tuyau.cpp,v 1.5 2002/06/11 17:46:32 denis Rel $";
    dummy_call(id);
}

void tuyau::ouverture()
{
    if(chemin != "")
    {
	char *ch = tools_str2charptr(chemin);
	try
	{
	    int flag;

	    switch(get_mode())
	    {
	    case gf_read_only:
		flag = O_RDONLY;
		break;
	    case gf_write_only:
		flag = O_WRONLY;
		break;
	    case gf_read_write:
		flag = O_RDWR;
		break;
	    default:
		throw SRC_BUG;
	    }
	    filedesc = ::open(ch, flag);
	    if(filedesc < 0)
		throw Erange("tuyau::ouverture", string("error openning pipe: ")+strerror(errno));
	}
	catch(...)
	{
	    delete ch;
	    throw;
	}
	delete ch;
    }
    else
	throw SRC_BUG; // no path nor file descriptor
}
