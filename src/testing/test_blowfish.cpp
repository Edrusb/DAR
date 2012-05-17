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
// $Id: test_blowfish.cpp,v 1.4.2.3 2008/02/09 17:41:30 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
}

#include <iostream>

#include "libdar.hpp"
#include "erreurs.hpp"
#include "crypto.hpp"
#include "shell_interaction.hpp"
#include "deci.hpp"
#include "cygwin_adapt.hpp"
#include "macro_tools.hpp"

using namespace libdar;
using namespace std;

void f1(user_interaction *dialog);
void f2(user_interaction *dialog);

int main()
{
    user_interaction *dialog = NULL;
    U_I maj, med, min;

    get_version(maj, med, min);
    dialog = shell_interaction_init(&cout, &cerr, false);
    try
    {
	f1(dialog);
	f2(dialog);
    }
    catch(Egeneric & e)
    {
	cout << "exception caught : " + e.get_message() << endl;
    }
    catch(...)
    {
	cout << "unknown exception caught" << endl;
    }
    shell_interaction_close();
    if(dialog != NULL)
	delete dialog;
 }

void f1(user_interaction *dialog)
{
    int fd = open("toto", O_WRONLY|O_TRUNC|O_CREAT|O_BINARY, 0644);
    fichier fic = fichier(*dialog, fd);
    blowfish bf = blowfish(*dialog, 10, string("bonjour"), fic, macro_tools_supported_version, false);
    char buffer[100] = "bonjour les amis il fait chaud il fait beau ! ";

    bf.write(buffer, strlen(buffer));
    bf.write("toto", 4);
    bf.write(" a !", 4);
    bf.write_end_of_file();
}

void f2(user_interaction *dialog)
{
    fichier fic = fichier(*dialog, "toto", gf_read_only);
    blowfish bf = blowfish(*dialog, 10, string("bonjour"), fic, macro_tools_supported_version, false);
    char buffer[100];
    S_I lu;
    bool ret;

    cout << bf.get_position() << endl;
    lu = bf.read(buffer, 100);
    cout << bf.get_position() << endl;
    ret = bf.skip(0);
    cout << bf.get_position() << endl;
    lu = bf.read(buffer, 100);
    cout << bf.get_position() << endl;
    ret = bf.skip_to_eof();
    cout << bf.get_position() << endl;
    lu = bf.read(buffer, 100);
    cout << bf.get_position() << endl;
}
