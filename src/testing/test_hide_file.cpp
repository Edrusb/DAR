//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
} // end extern "C"

#include "libdar.hpp"
#include "no_comment.hpp"
#include "config_file.hpp"
#include "cygwin_adapt.hpp"
#include "shell_interaction.hpp"
#include "user_interaction.hpp"
#include "fichier_local.hpp"

using namespace libdar;

void f1();
void f2();

static shared_ptr<user_interaction> ui;

int main()
{
    U_I maj, med, min;

    get_version(maj, med, min);
    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(!ui)
	cout << "ERREUR !" << endl;
    f1();
    f2();
    ui.reset();
}

void f1()
{
    fichier_local src = fichier_local(ui, "toto", gf_read_only, 0666, false, false, false);
    no_comment strip = no_comment(src);
    fichier_local dst = fichier_local(ui, "titi", gf_write_only, 0666, false, true, false);

    strip.copy_to(dst);
}

void f2()
{
    deque<string> cibles;

    cibles.push_back("coucou");
    cibles.push_back("all");
    cibles.push_back("default");

    fichier_local src = fichier_local(ui, "toto", gf_read_only, 0666, false, false, false);
    config_file strip = config_file(cibles, src);
    fichier_local dst = fichier_local(ui, "tutu", gf_write_only, 0666, false, true, false);

    strip.copy_to(dst);
}
