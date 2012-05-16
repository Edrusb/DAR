//*********************************************************************/
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
// $Id: test_hide_file.cpp,v 1.6.4.1 2003/12/20 23:05:35 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"

extern "C"
{
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
} // end extern "C"

#include "no_comment.hpp"
#include "config_file.hpp"
#include "cygwin_adapt.hpp"

using namespace libdar;

void f1();
void f2();

int main()
{
    f1();
    f2();
}

void f1()
{
    fichier src = fichier("toto", gf_read_only);
    no_comment strip = src;
    int fd = open("titi", O_WRONLY|O_TRUNC|O_CREAT|O_BINARY, 0644);
    fichier dst = fd;

    strip.copy_to(dst);
}

void f2()
{
    vector<string> cibles;

    cibles.push_back("coucou");
    cibles.push_back("all");
    cibles.push_back("default");

    fichier src = fichier("toto", gf_read_only);
    config_file strip = config_file(cibles, src);

    int fd = open("tutu", O_WRONLY|O_TRUNC|O_CREAT|O_BINARY, 0644);
    fichier dst = fd;

    strip.copy_to(dst);
}
