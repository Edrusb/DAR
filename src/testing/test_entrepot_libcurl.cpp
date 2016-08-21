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
// to contact the author : http://dar.linux.free.fr/email.html
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
#include "entrepot_libcurl.hpp"

using namespace libdar;

void f1();
void f2();

int main()
{
    U_I maj, med, min;

    get_version(maj, med, min);
    try
    {
	f1();
    }
    catch(Egeneric & e)
    {
	cout << "Execption caught: " << e.get_message() << endl;
    }
}

void f1()
{
    secu_string pass("themenac", 8);
    entrepot_libcurl reposito(entrepot_libcurl::proto_ftp,
			      "denis",
			      pass,
			      "localhost",
			      "",
			      "",
			      "");
    string tmp;
    shell_interaction ui(&cout, &cerr, true);

    reposito.set_location("/tmp");
    cout << "Listing:" << reposito.get_url() << endl;
    reposito.read_dir_reset();
    while(reposito.read_dir_next(tmp))
	cout << " -> " << tmp << endl;
    cout << endl;

    try
    {
	tmp = "cuicui";
	cout << "removing file " << tmp << endl;
	reposito.unlink(tmp);
	cout << endl;
    }
    catch(Erange & e)
    {
	ui.warning(e.get_message());
    }

    cout << "Listing:" << reposito.get_url() << endl;
    reposito.read_dir_reset();
    while(reposito.read_dir_next(tmp))
	cout << " -> " << tmp << endl;
    cout << endl;

    fichier_global *fic = reposito.open(ui,
					"coucou",
					gf_read_only,
					false,
					0,
					false,
					false,
					hash_none);

    infinint file_size = fic->get_size();
    ui.printf("size = %i", &file_size);

    const U_I bufsize = 10000;
    char buffer[bufsize+1];
    U_I read;
    do
    {
	read = fic->read(buffer, bufsize);
	buffer[read] = '\0';
	ui.printf(buffer);
    }
    while(read == bufsize);

}

