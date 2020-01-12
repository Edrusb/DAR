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
#include "infinint.hpp"
#include "generic_file.hpp"
#include "null_file.hpp"
#include "integers.hpp"
#include "cygwin_adapt.hpp"
#include "shell_interaction.hpp"
#include "crc.hpp"
#include "fichier_local.hpp"

using namespace libdar;
using namespace std;

static shared_ptr<user_interaction> ui;

int main(S_I argc, char *argv[])
{
    U_I maj, med, min;

    get_version(maj, med, min);
    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(!ui)
	cout << "ERREUR !" << endl;

    if(argc < 3)
    {
        cout << "usage " << argv[0] << " <filename> <filename>" << endl;
        return -1;
    }

    fichier_local f1 = fichier_local(ui, argv[1], gf_read_only, 0, false, false, false);
    S_I fd = ::open(argv[2], O_WRONLY|O_CREAT|O_TRUNC|O_BINARY);
    if(fd < 0)
    {
        cout << "cannot open "<< argv[2] << endl;
        return -1;
    }
    fichier_local f2 = fichier_local(ui, argv[2], gf_write_only, 0666, false, true, false);

    f1.reset_crc(crc::OLD_CRC_SIZE);
    f2.reset_crc(crc::OLD_CRC_SIZE);
    f1.copy_to(f2);
    crc *crc1 = f1.get_crc();
    crc *crc2 = f2.get_crc();
    crc *crc3 = nullptr;

    try
    {
	if(crc1 == nullptr || crc2 == nullptr)
	    throw SRC_BUG;
	if(*crc1 == *crc2)
	    cout << "CRC OK" << endl;
	else
	    cout << "CRC PROBLEM" << endl;
	f1.skip(0);
	null_file f3 = null_file(gf_write_only);
	f1.copy_to(f3, crc::OLD_CRC_SIZE, crc3);
	if(crc3 == nullptr)
	    throw SRC_BUG;
	if(*crc1 == *crc3)
	    cout << "CRC OK" << endl;
	else
	    cout << "CRC PROBLEM" << endl;

	ui.reset();
    }
    catch(...)
    {
	if(crc1 != nullptr)
	    delete crc1;
	if(crc2 != nullptr)
	    delete crc2;
	if(crc3 != nullptr)
	    delete crc3;
	throw;
    }
    if(crc1 != nullptr)
	delete crc1;
    if(crc2 != nullptr)
	delete crc2;
    if(crc3 != nullptr)
	delete crc3;
}
