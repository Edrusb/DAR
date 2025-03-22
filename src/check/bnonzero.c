/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void usage(char* argvO, char* reason);
void routine(char* argv0, char* filename, char byte, unsigned int size);

int main(int argc, char* argv[])
{
    if(argc != 4)
	usage(argv[0], NULL);
    else
    {
	char *filename = argv[1];
	char *byte = argv[2];
	int size = atoi(argv[3]);

	if(strlen(byte) != 1)
	    usage(argv[0], "second argument must be a single character");

	if(size < 0)
	    usage(argv[0], "size must be positive");

	routine(argv[0], filename, byte[0], size);
    }
}


void usage(char* argv0, char *reason)
{
    if(reason != NULL)
	printf("\n\tError: %s\n\n", reason);
    printf("usage: %s <file> <char> <size>\n", argv0);
    printf("   creates a new <file> full of <char> characters with a size if <size> bytes\n");
    exit(1);
}

void routine(char* argv0, char* filename, char byte, unsigned int size)
{
    unsigned int wrote = 0;
    int fd = open(filename, O_WRONLY|O_CREAT|O_EXCL, 0644);

    if(fd < 0)
	usage(argv0, strerror(errno));

    while(wrote < size)
    {
	if(write(fd, &byte, 1) < 0)
	    usage(argv0, strerror(errno));
	++wrote;
    }

    close(fd);
}
