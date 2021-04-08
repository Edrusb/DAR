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
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_SYS_TYPE_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
} // end extern "C"

#include <string>
#include <iostream>

#include "infinint.hpp"
#include "deci.hpp"
#include "erreurs.hpp"
#include "generic_file.hpp"
#include "integers.hpp"
#include "cygwin_adapt.hpp"
#include "fichier_local.hpp"
#include "../dar_suite/dar_suite.hpp"

using namespace libdar;
using namespace std;

static int little_main(shared_ptr<user_interaction> & ui, int argc, char * const argv[], const char **env);

int main(S_I argc, char * const argv[], const char **env)
{
    return dar_suite_global(argc,
			    argv,
			    env,
			    "",
			    nullptr,
			    '\0',
			    &little_main);
}

static int little_main(shared_ptr<user_interaction> & ui, int argc, char * const argv[], const char **env)
{
    if(argc != 2 && argc != 3)
	exit(1);

    string s = argv[1];
    libdar::deci f = s;
    infinint max = f.computer();
    infinint i = 2;
    infinint p = 1;

    while(i <= max)
    {
	p *= i;
	++i;
    }

    ui->message("calcul finished, now computing the decimal representation ... ");
    f = libdar::deci(p);
    ui->message(f.human());

    if(argc == 3)
    {
	fichier_local fic = fichier_local(ui,
					  argv[2],
					  gf_read_write,
					  0666,
					  false,
					  true,
					  false);
	infinint cp;

	p.dump(fic);
	fic.skip(0);
	cp = infinint(fic);
	ui->message(string("read from file: ") + libdar::deci(cp).human());
    }

    return EXIT_OK;
}
