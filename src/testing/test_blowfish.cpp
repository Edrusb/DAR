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
// to contact the author : http://dar.linux.free.fr/email.html
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
#include "crypto_sym.hpp"
#include "shell_interaction.hpp"
#include "deci.hpp"
#include "cygwin_adapt.hpp"
#include "macro_tools.hpp"
#include "fichier_local.hpp"

using namespace libdar;
using namespace std;

void f1(user_interaction *dialog);
void f2(user_interaction *dialog);

int main()
{
    user_interaction *dialog = new (nothrow) shell_interaction(&cout, &cerr, false);
    U_I maj, med, min;
    get_version(maj, med, min);

    if(dialog == NULL)
	cout << "ERREUR !" << endl;

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

    if(dialog != NULL)
	delete dialog;
 }

void f1(user_interaction *dialog)
{
    fichier_local fic = fichier_local(*dialog, "toto", gf_write_only, 0666, false, true, false);
    string pass = "bonjour";
    crypto_sym bf = crypto_sym(10, secu_string(pass.c_str(), pass.size()), fic, false, macro_tools_supported_version, crypto_blowfish);
    char buffer[100] = "bonjour les amis il fait chaud il fait beau ! ";

    bf.write(buffer, strlen(buffer));
    bf.write("toto", 4);
    bf.write(" a !", 4);
    bf.write_end_of_file();
}

void f2(user_interaction *dialog)
{
    fichier_local fic = fichier_local(*dialog, "toto", gf_read_only, 0666, false, false, false);
    string pass = "bonjour";
    crypto_sym bf = crypto_sym(10, secu_string(pass.c_str(), pass.size()), fic, false, macro_tools_supported_version, crypto_blowfish);
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

    lu = lu+1; // avoid warning of unused variable
    ret = !ret; // avoid warning of unused variable
}
