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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: dar_cp.cpp,v 1.4.4.1 2003/12/20 23:05:34 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"

extern "C"
{
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRIN_H
#include <string.h>
#endif
}

#include "dar_suite.hpp"
#include "tools.hpp"

#define DAR_CP_VERSION "1.0.0"

using namespace libdar;

void show_usage(char *argv0);
void show_version(char *argv0);
int open_files(char *src, char *dst, int *fds, int *fdd);
void copy_max(int src, int dst);

int main(int argc, char *argv[])
{
    int fds, fdd;

    if(argc > 1 && strcmp(argv[1],"-V") == 0)
    {
        show_version(argv[0]);
        exit(0);
    }

    if(argc != 3 || argv[1] == "-h")
    {
        show_usage(argv[0]);
        exit(1);
    }

    if(open_files(argv[1], argv[2], &fds, &fdd))
    {
        copy_max(fds, fdd);
        close(fds);
        close(fdd);
        exit(0);
    }
    else
        exit(2);
}

void show_usage(char *argv0)
{
    printf("usage : %s <source> <destination>\n", argv0);
}

void show_version(char *argv0)
{
/* C++ syntax used*/
    try
    {
	char *cmd = tools_extract_basename(argv0);
	    // never return a NULL pointer but
	try
	{
	    printf("\n %s version %s Copyright (C) 2002-2052 Denis Corbin\n\n", cmd, DAR_CP_VERSION);
	    printf("\n");
	    printf(" %s with %s version %s\n", __DATE__, CC_NAT,  __VERSION__);
	    printf(" %s is part of the Disk ARchive suite (Release %s)\n", cmd, PACKAGE_VERSION);
	    printf(" %s comes with ABSOLUTELY NO WARRANTY; for details\n", cmd);
	    printf(" type `dar -W'.  This is free software, and you are welcome\n");
	    printf(" to redistribute it under certain conditions; type `dar -L | more'\n");
	    printf(" for details.\n\n");
	}
	catch(...)
	{
	    delete cmd;
	    throw;
	}
	delete cmd;
    }
    catch(...)
    {
	printf("unexpected exception from libdar");
	exit(3);
    }
/* END of C++ syntax used*/
}

int open_files(char *src, char *dst, int *fds, int *fdd)
{
    struct stat buf;
    int val = stat(dst, &buf);
    char *tmp = NULL;

    if(val == 0 && S_ISDIR(buf.st_mode))
    {
        tmp = (char *)malloc(strlen(src)+strlen(dst)+1+1);
        char *tmp2 = NULL;
        if(tmp == NULL)
        {
            perror("memory allocation failed");
            return 0;
        }
        try
        {
            tmp2 = tools_extract_basename(src);
        }
        catch(...)
        {
            fprintf(stderr,"memory allocation failed\n");
            return 0;
        }
        strcpy(tmp, dst);
        strcat(tmp, "/");
        strcat(tmp, tmp2);
        delete tmp2;
        dst = tmp; // dst is not dynamically allocated, thus we can
            // loose its reference.
    }

    *fds = open(src, O_RDONLY);

    if(*fds < 0)
    {
        perror("cannot open source file");
        if(tmp != NULL)
            free(tmp);
        return 0; // error
    }

    *fdd = open(dst, O_WRONLY|O_CREAT|O_EXCL, 0666);
    if(tmp != NULL)
        free(tmp);
    if(*fdd < 0)
    {
        perror("cannot open destination file");
        close(*fds);
        *fds = -1;
        return 0;
    }
    else
        return 1;
}

void xfer_before_error(int block, char *buffer, int src, int dst);
int skip_to_next_readable(int block, char *buffer, int src, int dst);
    /* return 0 if not found any more readable data, else return 1 */
int normal_copy(int block, char *buffer, int src, int dst);
    /* return the number of copied bytes (negative value upon error, zero at end of file) */

void copy_max(int src, int dst)
{
#define BUF_SIZE 10240
    char buffer[BUF_SIZE];
    int lu = 0;

    do
    {
        lu = normal_copy(BUF_SIZE, buffer, src, dst);
        if(lu < 0)
        {
            xfer_before_error(BUF_SIZE/2, buffer, src, dst);
            if(skip_to_next_readable(BUF_SIZE/2, buffer, src, dst))
                lu = 1;
            else
                lu = 0;
        }
    }
    while(lu > 0);
}

void xfer_before_error(int block, char *buffer, int src, int dst)
{
    while(block > 1)
    {
        int lu = read(src, buffer, block);
        if(lu > 0)
        {
            if(write(dst, buffer, lu) < 0)
            {
                perror("canot write to destination, aborting");
                exit(3);
            }
        }
        else
            block /= 2;
    }
}

int skip_to_next_readable(int block, char *buffer, int src, int dst)
{
    int lu;
    off_t shift = block;

        // we assume that we can't read at current position
        // so we start seeking ...
    lseek(src, shift, SEEK_CUR);

    do
    {
        lu = read(src, buffer, 1);
        if(lu < 0)
        {
            if(shift*2 > shift) // avoid integer overflow
                shift *= 2;
            lseek(src, shift, SEEK_CUR);
        }
        else
        {
            if(shift > 1)
            {
                shift /= 2;
                lseek(src, -shift, SEEK_CUR);
            }

        }
    }
    while(shift > 1 && lu != 0);
    if(lu > 0)
        lseek(dst, lseek(src, 0, SEEK_CUR), SEEK_SET);
        // we eventually make a hole in dst,
        // which will be full of zero
        // see lseek man page.

    return lu;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: dar_cp.cpp,v 1.4.4.1 2003/12/20 23:05:34 edrusb Rel $";
    dummy_call(id);
}

int normal_copy(int block, char *buffer, int src, int dst)
{
    int lu = read(src, buffer, block);
    if(lu > 0)
    {
        int ecrit = 0;
        while(ecrit < lu)
        {
            int tmp = write(dst, buffer+ecrit, lu-ecrit);
            if(tmp < 0)
            {
                perror("cannot write to destination, aborting");
                exit(3);
            }
            else
                if(tmp == 0)
                    printf("non fatal error during writing to destination file, retrying\n");
                else
                    ecrit += tmp;
        }
    }
    else
        if(lu < 0)
            perror("error reading source file, trying to read further");
    return lu;
}
