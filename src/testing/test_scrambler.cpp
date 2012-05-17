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

#include "../my_config.h"

extern "C"
{
#if HAVE_STDIO_H
#include <stdio.h>
#endif
} // end extern "C"

#include "scrambler.hpp"
#include "dar_suite.hpp"
#include "generic_file.hpp"
#include "test_memory.hpp"
#include "integers.hpp"

using namespace libdar;

S_I little_main(user_interaction & dialog, S_I argc, char *argv[], const char **env);

int main(S_I argc, char *argv[])
{
    return dar_suite_global(argc, argv, NULL, &little_main);
}

S_I little_main(user_interaction & dialog, S_I argc, char *argv[], const char **env)
{
    MEM_IN;
    if(argc != 4)
    {
        printf("usage: %s <source> <destination_scrambled> <destination_clear>\n", argv[0]);
        return EXIT_SYNTAX;
    }

    fichier *src = new fichier(dialog, argv[1], gf_read_only);
    fichier *dst = new fichier(dialog, argv[2], gf_write_only);
    scrambler *scr = new scrambler(dialog, "bonjour", *dst);

    src->copy_to(*scr);

    delete scr; scr = NULL;
    delete dst; dst = NULL;
    delete src; src = NULL;

    src = new fichier(dialog, argv[2], gf_read_only);
    scr = new scrambler(dialog, "bonjour", *src);
    dst = new fichier(dialog, argv[3], gf_write_only);

    scr->copy_to(*dst);

    delete scr;
    delete src;
    delete dst;

    MEM_OUT;
    return EXIT_OK;
}
