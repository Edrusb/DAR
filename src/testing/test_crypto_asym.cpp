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

#include "libdar.hpp"
#include "shell_interaction.hpp"
#include "fichier_local.hpp"
#include "crypto_asym.hpp"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif
}

#include <string>
#include <vector>
using namespace std;
using namespace libdar;

static shared_ptr<shell_interaction> ui(new shell_interaction(cout, cerr, false));

void encrypt(vector<string> recipients, const string & src, const string & dst);
void decrypt(const string & src, const string & dst);

int main(int argc, char *argv[])
{
    U_I maj, med, min;

    if(argc < 3)
    {
	cout << "usage: " << argv[0] << " crypt <source file> <dest file> <recipient's email> [... <recipient's email>]" << endl;
	cout << "       " << argv[0] << " decrypt <source file> <dest file>" << endl;
	return 1;
    }

    get_version(maj, med, min);

    try
    {
	if(strcmp(argv[1], "crypt") == 0)
	{
	    vector<string> recip;

	    for(signed int i = 4; i < argc; ++i)
		recip.push_back(argv[i]);
	    if(recip.empty())
		cout << "ERROR: need at least one email in the recipient list" << endl;
	    else
		encrypt(recip, argv[2], argv[3]);
	}
	else if(strcmp(argv[1], "decrypt") == 0)
	{
	    decrypt(argv[2], argv[3]);
	}
	else
	    cout << "ERROR: first argument must either be 'crypt' or 'decrypt'" << endl;
    }
    catch(Egeneric & e)
    {
	ui->printf("Exception caught: %S", &(e.get_message()));
	cout << e.dump_str() << endl;
    }
    catch(...)
    {
	ui->printf("unknown exception caught");
    }

    return 0;
}

void encrypt(vector<string> recipients, const string & src, const string & dst)
{
    fichier_local fsrc = fichier_local(ui, src, gf_read_only, 0, false, false, false);
    fichier_local fdst = fichier_local(ui, dst, gf_write_only, 0644, false, true, false);
    crypto_asym engine(ui);

    engine.encrypt(recipients, fsrc, fdst);
}

void decrypt(const string & src, const string & dst)
{
    fichier_local fsrc = fichier_local(ui, src, gf_read_only, 0, false, false, false);
    fichier_local fdst = fichier_local(ui, dst, gf_write_only, 0644, false, true, false);
    crypto_asym engine(ui);

    engine.decrypt(fsrc, fdst);
}
