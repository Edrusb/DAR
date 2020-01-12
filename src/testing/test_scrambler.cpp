/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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
#if HAVE_STDIO_H
#include <stdio.h>
#endif
} // end extern "C"

#include <string>
#include <memory>

#include "scrambler.hpp"
#include "dar_suite.hpp"
#include "generic_file.hpp"
#include "integers.hpp"
#include "fichier_local.hpp"
#include "tools.hpp"

using namespace libdar;
using namespace std;

S_I little_main(shared_ptr<user_interaction> & dialog, S_I argc, char * const argv[], const char **env);

int main(S_I argc, char * const argv[])
{
    return dar_suite_global(argc,
			    argv,
			    nullptr,
			    "",
			    nullptr,
			    '\0',
			    &little_main);
}

S_I little_main(shared_ptr<user_interaction> & dialog, S_I argc, char * const argv[], const char **env)
{
    if(argc != 4)
    {
        printf("usage: %s <source> <destination_scrambled> <destination_clear>\n", argv[0]);
        return EXIT_SYNTAX;
    }

    fichier_local *src = new fichier_local(dialog, argv[1], gf_read_only, 0, false, false, false);
    fichier_local *dst = new fichier_local(dialog, argv[2], gf_write_only, 0666, false, true, false);
    std::string pass = "bonjour";
    scrambler *scr = new scrambler(secu_string(pass.c_str(), pass.size()), *dst);

    src->copy_to(*scr);

    delete scr; scr = nullptr;
    delete dst; dst = nullptr;
    delete src; src = nullptr;

    src = new fichier_local(dialog, argv[2], gf_read_only, 0, false, false, false);
    scr = new scrambler(secu_string(pass.c_str(), pass.size()), *src);
    dst = new fichier_local(dialog, argv[3], gf_write_only, 0666, false, true, false);

    scr->copy_to(*dst);

    delete scr;
    delete src;
    delete dst;

    return EXIT_OK;
}
