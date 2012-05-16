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
// $Id: test_generic_file.cpp,v 1.7.4.2 2004/04/20 09:27:02 edrusb Rel $
//
/*********************************************************************/
//

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

#include "generic_file.hpp"
#include "null_file.hpp"
#include "integers.hpp"
#include "cygwin_adapt.hpp"

using namespace libdar;
using namespace std;

int main(S_I argc, char *argv[])
{
    if(argc < 3)
    {
        cout << "usage " << argv[0] << " <filename> <filename>" << endl;
        return -1;
    }

    fichier f1 = fichier(argv[1], gf_read_only);
    S_I fd = ::open(argv[2], O_WRONLY|O_CREAT|O_TRUNC|O_BINARY);
    if(fd < 0)
    {
        cout << "cannot open "<< argv[2] << endl;
        return -1;
    }
    fichier f2 = fd;

    f1.reset_crc();
    f2.reset_crc();
    f1.copy_to(f2);
    crc crc1;
    crc crc2;
    f1.get_crc(crc1);
    f2.get_crc(crc2);
    if(same_crc(crc1, crc2))
        cout << "CRC OK" << endl;
    else
        cout << "CRC PROBLEM" << endl;
    f1.skip(0);
    null_file f3 = gf_write_only;
    crc crc3;
    f1.copy_to(f3, crc3);
    if(same_crc(crc1, crc3))
        cout << "CRC OK" << endl;
    else
        cout << "CRC PROBLEM" << endl;
}
