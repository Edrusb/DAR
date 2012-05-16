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
// $Id: etage.cpp,v 1.9.2.1 2003/12/20 23:05:34 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
// this was necessary to compile under Mac OS-X (boggus dirent.h)
#if HAVE_STDINT_H
#include <stdint.h>
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif
} // end extern "C"

#include "etage.hpp"

using namespace std;

namespace libdar
{

    etage::etage(char *dirname)
    {
        struct dirent *ret;
        DIR *tmp = opendir(dirname);

        if(tmp == NULL)
            throw Erange("filesystem etage::etage" , string("Error openning directory: ") + dirname + " : " + strerror(errno));

        fichier.clear();
        while((ret = readdir(tmp)) != NULL)
            if(strcmp(ret->d_name, ".") != 0 && strcmp(ret->d_name, "..") != 0)
                fichier.push_back(string(ret->d_name));
        closedir(tmp);
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: etage.cpp,v 1.9.2.1 2003/12/20 23:05:34 edrusb Rel $";
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

} // end of namespace
