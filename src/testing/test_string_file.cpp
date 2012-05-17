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
// $Id: test_string_file.cpp,v 1.4 2011/01/07 19:53:16 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

#include "string_file.hpp"
#include "shell_interaction.hpp"
#include "testtools.hpp"
#include "libdar.hpp"

int main()
{
    user_interaction *dialog = shell_interaction_init(&cout, &cerr, false);
    U_I min, med, maj;

	// initializing libdar
    get_version(maj, med, min);

    try
    {
	string_file strfile = string_file("Bonjour les amis, il fait beau, il fait chaud, les mouches pettent et les cailloux fleurissent");
	char buffer[100];
	S_I lu;
	infinint pos;

	pos = strfile.get_position();
	display(pos);
	lu = strfile.read(buffer, 99);
	buffer[lu] = '\0';
	dialog->warning(buffer);
	pos = strfile.get_position();
	display(pos);

	strfile.skip(0);
	pos = strfile.get_position();
	display(pos);
	lu = strfile.read(buffer, 100);
	buffer[99] = '\0';
	dialog->warning(buffer);

	strfile.skip(10);
	pos = strfile.get_position();
	display(pos);
	lu = strfile.read(buffer, 1);
	pos = strfile.get_position();
	display(pos);

	strfile.skip_relative(-1);
	pos = strfile.get_position();
	display(pos);
	lu = strfile.read(buffer, 1);

	pos = strfile.get_position();
	display(pos);
	strfile.skip_relative(1);
	pos = strfile.get_position();
	display(pos);
	lu = strfile.read(buffer, 1);

	strfile.skip_to_eof();
	pos = strfile.get_position();
	display(pos);
	lu = strfile.read(buffer, 100);
	cout << lu << endl;
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

