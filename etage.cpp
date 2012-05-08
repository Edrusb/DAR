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
// $Id: etage.cpp,v 1.4 2003/02/11 22:01:41 edrusb Rel $
//
/*********************************************************************/

#include <dirent.h>
#include <errno.h>
#include "etage.hpp"


etage::etage(char *dirname)
{
    struct dirent *ret;
    DIR *tmp = opendir(dirname);
    
    if(tmp == NULL)
	throw Erange("filesystem etage::etage" , strerror(errno));
    
    fichier.clear();
    while((ret = readdir(tmp)) != NULL)
	if(strcmp(ret->d_name, ".") != 0 && strcmp(ret->d_name, "..") != 0) 
	    fichier.push_back(string(ret->d_name));
    closedir(tmp);
}

static void dummy_call(char *x)
{
    static char id[]="$Id: etage.cpp,v 1.4 2003/02/11 22:01:41 edrusb Rel $";
    dummy_call(id);
}

    
bool etage::read(string & ref)
{
    if(fichier.size() > 0)
    {
	ref = fichier.front();
	fichier.pop_front();
	return true;
    }
    else 
	return false;
}
