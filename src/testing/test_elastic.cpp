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
#include "elastic.hpp"
#include "erreurs.hpp"
#include "shell_interaction.hpp"
#include "deci.hpp"
#include "cygwin_adapt.hpp"
#include "macro_tools.hpp"
#include "fichier_local.hpp"

using namespace libdar;
using namespace std;

void f1();
void f2();
void f3();

int main()
{
    try
    {
	U_I maj, med, min;

	get_version(maj, med, min);
	f1();
	f2();
	f3();
    }
    catch(Egeneric & e)
    {
	cout << "exception caught : " + e.get_message() << endl;
    }
    catch(...)
    {
	cout << "unknown exception caught" << endl;
    }
}

void f1()
{
    elastic stic = 10;
    const unsigned int taille = 100;
    unsigned char buffer[taille];
    unsigned char biffir[taille];

    stic.dump(buffer, taille);
	 // check the resulting buffer thanks to debugger
     cout << stic.get_size() << endl;

     elastic stoc = elastic(buffer, taille, elastic_forward, macro_tools_supported_version);
     cout << stoc.get_size() << endl;

     stoc.dump(biffir, taille);

     elastic stuc = elastic(biffir, 10, elastic_backward, macro_tools_supported_version);
     cout << stuc.get_size() << endl;
}

void f2()
{
    const unsigned int taille = 500;
    unsigned char buffer[taille];
	 // testing the elastic of size 1 and 2

    elastic stic = 1;
    stic.dump(buffer, taille);
    cout << stic.get_size() << endl;

    elastic stuc = elastic(buffer, taille, elastic_forward, macro_tools_supported_version);
    cout << stuc.get_size() << endl;
    stuc = elastic(buffer, 1, elastic_backward, macro_tools_supported_version);
    cout << stuc.get_size() << endl;

    stic = 2;
    stic.dump(buffer, taille);
    cout << stic.get_size() << endl;

    stuc = elastic(buffer, taille, elastic_forward, macro_tools_supported_version);
    cout << stuc.get_size() << endl;
    stuc = elastic(buffer, 2, elastic_backward, macro_tools_supported_version);
    cout << stuc.get_size() << endl;


	 // testing the elastic buffers of size larger than 255

    stic = 256;
    stic.dump(buffer, taille);
    cout << stic.get_size() << endl;

    stuc = elastic(buffer, taille, elastic_forward, macro_tools_supported_version);
    cout << stuc.get_size() << endl;
    stuc = elastic(buffer, 256, elastic_backward, macro_tools_supported_version);
    cout << stuc.get_size() << endl;

}

void f3()
{
    user_interaction *dialog = shell_interaction_init(&cout, &cerr, false);
    try
    {
	int fd = open("toto", O_RDWR|O_TRUNC|O_CREAT|O_BINARY, 0666);
	fichier_local fic = fichier_local(*dialog, fd);
	const unsigned int taille = 500;
	unsigned char buffer[taille];
	const char *ttt =  "Bonjour les amis comment ça va ? ";
	elastic tic = 1, toc = 1;

	fic.write(ttt, strlen(ttt));

	tic = 1;
	fic.skip(3);
	tic.dump(buffer, taille);
	fic.write((char *)buffer, 1);
	fic.skip(3);
	toc = elastic(fic, elastic_forward, macro_tools_supported_version);
	cout << toc.get_size() << " " << fic.get_position() << endl;
	fic.skip(4);
	toc = elastic(fic, elastic_backward, macro_tools_supported_version);
	cout << toc.get_size() << " " <<  fic.get_position() << endl;

	tic = 2;
	fic.skip(3);
	tic.dump(buffer, taille);
	fic.write((char *)buffer, 2);
	fic.skip(3);
	toc = elastic(fic, elastic_forward, macro_tools_supported_version);
	cout << toc.get_size() <<  " " << fic.get_position() << endl;
	fic.skip(5);
	toc = elastic(fic, elastic_backward, macro_tools_supported_version);
	cout << toc.get_size() <<  " " << fic.get_position() << endl;

	tic = 3;
	fic.skip(3);
	tic.dump(buffer, taille);
	fic.write((char *)buffer, 3);
	fic.skip(3);
	toc = elastic(fic, elastic_forward, macro_tools_supported_version);
	cout << toc.get_size() <<  " " << fic.get_position() << endl;
	fic.skip(6);
	toc = elastic(fic, elastic_backward, macro_tools_supported_version);
	cout << toc.get_size() <<  " " << fic.get_position() << endl;

	tic = 3;
	fic.skip(0);
	tic.dump(buffer, taille);
	fic.write((char *)buffer, 3);
	fic.skip(0);
	toc = elastic(fic, elastic_forward, macro_tools_supported_version);
	cout << toc.get_size() <<  " " << fic.get_position() << endl;
	fic.skip(3);
	toc = elastic(fic, elastic_backward, macro_tools_supported_version);
	cout << toc.get_size() <<  " " << fic.get_position() << endl;

	tic = 257;
	fic.skip(3);
	tic.dump(buffer, taille);
	fic.write((char *)buffer, 257);
	fic.skip(3);
	toc = elastic(fic, elastic_forward, macro_tools_supported_version);
	cout << toc.get_size() <<  " " << fic.get_position() << endl;
	fic.skip(260);
	toc = elastic(fic, elastic_backward, macro_tools_supported_version);
	cout << toc.get_size() <<  " " << fic.get_position() << endl;
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
