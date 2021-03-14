/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
} // end extern "C"

#include <iostream>

#include "libdar.hpp"
#include "terminateur.hpp"
#include "generic_file.hpp"
#include "deci.hpp"
#include "integers.hpp"
#include "cygwin_adapt.hpp"
#include "shell_interaction.hpp"
#include "user_interaction.hpp"
#include "macro_tools.hpp"
#include "fichier_local.hpp"

using namespace libdar;
using namespace std;

static void f1();

static shared_ptr<user_interaction> ui;

int main()
{
    U_I maj, med, min;

    get_version(maj, med, min);
    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(!ui)
	cout << "ERREUR !" << endl;
    f1();
    ui.reset();
}

static void f1()
{
    fichier_local toto = fichier_local(ui, "toto", gf_read_write, 0666, false, true, false);
    terminateur term;

    infinint grand = 1;

    for(S_I i=2;i<30;i++)
        grand *= i;

    libdar::deci conv = grand;
    cout << conv.human() << endl;
    term.set_catalogue_start(grand);
    term.dump(toto);
    toto.skip(0);
    term.read_catalogue(toto, false, macro_tools_supported_version);
    conv = term.get_catalogue_start();
    cout << conv.human() << endl;
}
