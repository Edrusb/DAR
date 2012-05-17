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
// $Id: dar_cp.cpp,v 1.4.4.3 2004/04/25 17:10:25 edrusb Rel $
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
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
}

#include "dar_suite.hpp"
#include "tools.hpp"

#define DAR_CP_VERSION "1.0.1"

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

    if(argc != 3 || strcmp(argv[1],"-h") == 0)
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

    *fds = ::open(src, O_RDONLY);

    if(*fds < 0)
    {
        perror("cannot open source file");
        if(tmp != NULL)
            free(tmp);
        return 0; // error
    }

    *fdd = ::open(dst, O_WRONLY|O_CREAT|O_EXCL, 0666);
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
int skip_to_next_readable(int block, char *buffer, int src, int dst, off_t & missed);
    /* return 0 if not found any more readable data, else return 1 */
int normal_copy(int block, char *buffer, int src, int dst);
    /* return the number of copied bytes (negative value upon error, zero at end of file) */

void copy_max(int src, int dst)
{
#define BUF_SIZE 1024
	// choosing the correct value for BUF_SIZE
	// too long make I/O error very long to recover
	// too small make copying under normal condition a bit slow
	// 1024 seems a balanced choice.
	//
    char buffer[BUF_SIZE];
    int lu = 0;
    off_t missed = 0;
    off_t taille = lseek(src, 0, SEEK_END);
    lseek(src, 0, SEEK_SET);

    printf("Starting the copy of %lu byte(s)\n", taille);
    do
    {
        lu = normal_copy(BUF_SIZE, buffer, src, dst);
        if(lu < 0)
        {
	    off_t local_missed;
	    off_t current = lseek(src, 0, SEEK_CUR);

            printf("Error reading source file (we are at %.2f %% of data copied), trying to read further: %s\n", (float)(current)/(float)(taille)*100, strerror(errno));
            xfer_before_error(BUF_SIZE/2, buffer, src, dst);
            if(skip_to_next_readable(BUF_SIZE/2, buffer, src, dst, local_missed))
	    {
		printf("Skipping done (missing %lu byte(s)), found correct data to read, continuing the copy\n", local_missed);
		missed += local_missed;
                lu = 1;
	    }
            else
	    {
		printf("Reached End of File, no correct data could be found after the last error\n");
                lu = 0;
	    }
        }
    }
    while(lu > 0);

    printf("Copy finished. Missing %lu byte(s) of data (%.2f %% of the total amount of data)\n", missed, (float)(missed)/((float)(taille))*100);
}

void xfer_before_error(int block, char *buffer, int src, int dst)
{
    if(read(src, buffer, 1) == 1)
    {
	if(lseek(src, -1, SEEK_CUR) < 0)
	{
	    perror("cannot seek back one char");
	    exit(3);
	}
    }
    else
	return; // the error is just next char to read

    while(block > 1)
    {
        int lu = read(src, buffer, block);
        if(lu > 0)
        {
            if(write(dst, buffer, lu) < 0)
            {
                perror("cannot write to destination, aborting");
                exit(3);
            }
        }
        else
            block /= 2;
    }
}

int skip_to_next_readable(int block, char *buffer, int src, int dst, off_t & missed)
{
    int lu = 0;
    off_t min = lseek(src, 0, SEEK_CUR), max;
    missed = min;

	//////////////////////////
	// first step: looking for a next readable (maybe not the very next readable)
	// we put its position in "max"
	//////////////////////////

    do
    {
	lu = read(src, buffer, 1);
	    // once we will read after the end of file, read() will return 0
	if(lu < 0)
	{
	    if(block*2 > block) // avoid integer overflow
		block *= 2;
	    lseek(src, block, SEEK_CUR);
	}
    }
    while(lu < 0);
    max = lseek(src, 0, SEEK_CUR); // the current position

	//////////////////////////
	// second step: looking for the very next readable char between offsets min and max
	//////////////////////////

    while(max - min > 1)
    {
	off_t mid = (max + min) / 2;
	if(lseek(src, mid, SEEK_SET) < 0)
	{
	    perror("cannot seek in file");
	    return 0;
	}
	lu = read(src, buffer, 1);
	switch(lu)
	{
	case 1: // valid character read
	case 0: // still after EOF
	    max = mid;
	    break;
	default: // no character could be read
	    min = mid;
	    break;
	}
    }

    lseek(src, max, SEEK_SET);
    lu = read(src, buffer, 1);
    lseek(src, max, SEEK_SET);
    lseek(dst, max, SEEK_SET);
        // we eventually make a hole in dst,
        // which will be full of zero
        // see lseek man page.
    missed = max - missed;
    if(lu != 1)
	return 0; // could not find valid char to read
    else
	return 1; // there is valid char to read
    return 1;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: dar_cp.cpp,v 1.4.4.3 2004/04/25 17:10:25 edrusb Rel $";
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
    return lu;
}
