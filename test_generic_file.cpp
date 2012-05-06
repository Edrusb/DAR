/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: test_generic_file.cpp,v 1.4 2002/06/26 22:20:20 denis Rel $
//
/*********************************************************************/
//
#include <iostream.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "generic_file.hpp"
#include "null_file.hpp"

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
	cout << "usage " << argv[0] << " <filename> <filename>" << endl;
	return -1;
    }
    
    fichier f1 = fichier(argv[1], gf_read_only);
    int fd = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC);
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
    
    
