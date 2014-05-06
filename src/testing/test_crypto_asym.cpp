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

#include "libdar.hpp"
#include "shell_interaction.hpp"
#include "fichier_local.hpp"
#include "crypto_asym.hpp"


#include <string>
#include <vector>
using namespace std;
using namespace libdar;

static user_interaction *ui = NULL;

void encrypt(const string & src, const string & dst);
void decrypt(const string & src, const string & dst);

int main(int argc, char *argv[])
{
    U_I maj, med, min;

    if(argc < 4)
    {
	cout << "usage: " << argv[0] << "<source file> <filename 2> <filename 3>" << endl;
	return 1;
    }

    get_version(maj, med, min);
    if(ui == NULL)
	cout << "ERREUR !" << endl;

    try
    {
	encrypt(argv[1], argv[2]);
	decrypt(argv[2], argv[3]);
    }
    catch(Egeneric & e)
    {
	ui->printf("Exception caught: %S", &(e.get_message()));
    }
    catch(...)
    {
	ui->printf("unknown exception caught");
    }

    return 0;
}

void encrypt(const string & src, const string & dst)
{
    fichier_local fsrc = fichier_local(*ui, src, gf_read_only, 0, false, false, false);
    fichier_local fdst = fichier_local(*ui, dst, gf_write_only, 0644, false, true, false);
    vector<string> recipients;

    recipients.push_back("dar.linux@free.fr");
    recipients.push_back("denis.corbin@free.fr");

	// must use a pointer to be sure crypto_asym
	// object is destroyed before fdst
    crypto_asym *cr = new crypto_asym(*ui, &fdst, recipients);
    if(cr == NULL)
	ui->printf("Allocation failure");
    else
    {
	try
	{
	    fsrc.copy_to(*cr);
	}
	catch(...)
	{
	    delete cr;
	    throw;
	}
	delete cr;
    }
}

void decrypt(const string & src, const string & dst)
{
    fichier_local fsrc = fichier_local(*ui, src, gf_read_only, 0, false, false, false);
    fichier_local fdst = fichier_local(*ui, dst, gf_write_only, 0644, false, true, false);

	// same thing here, using pointer to be sure the crypto_asym objet
	// is destroyed before fsrc
    crypto_asym *decr = new crypto_asym(*ui, &fsrc);
    if(decr == NULL)
	ui->printf("Allocation failure");
    else
    {
	try
	{
	    decr->copy_to(fdst);
	}
	catch(...)
	{
	    delete decr;
	    throw;
	}
	delete decr;
    }
}
