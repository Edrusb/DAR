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
// $Id: dar_cp.cpp,v 1.16.2.2 2007/02/24 17:43:02 edrusb Rel $
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
}

#include <iostream>

#include "dar_suite.hpp"
#include "tools.hpp"
#include "cygwin_adapt.hpp"
#include "dar_suite.hpp"
#include "user_interaction.hpp"
#include "thread_cancellation.hpp"

#define DAR_CP_VERSION "1.2.0"

using namespace libdar;
using namespace std;

static void show_usage(user_interaction & dialog, char *argv0);
static void show_version(user_interaction & dialog, char *argv0);
static int open_files(user_interaction & dialog, char *src, char *dst, int *fds, int *fdd);
static int copy_max(user_interaction & dialog, int src, int dst);
static int little_main(user_interaction & dialog, int argc, char *argv[], const char **env);
static void xfer_before_error(int block, char *buffer, int src, int dst);
static int skip_to_next_readable(int block, char *buffer, int src, int dst, off_t & missed);
    /* return 0 if not found any more readable data, else return 1 */
static int normal_copy(int block, char *buffer, int src, int dst);
    /* return the number of copied bytes (negative value upon error, zero at end of file) */
static int little_main(user_interaction & dialog, int argc, char *argv[], const char **env);

int main(int argc, char *argv[], const char **env)
{
    return dar_suite_global(argc, argv, env, &little_main);
}

static int little_main(user_interaction & dialog, int argc, char *argv[], const char **env)
{
    int fds, fdd;
    int ret = EXIT_OK;

    if(argc > 1 && strcmp(argv[1],"-V") == 0)
    {
        show_version(dialog, argv[0]);
        ret = EXIT_OK;
    }
    else
	if(argc != 3 || argv[1] == "-h")
	{
	    show_usage(dialog, argv[0]);
	    ret = EXIT_SYNTAX;
	}
	else
	    if(open_files(dialog, argv[1], argv[2], &fds, &fdd))
	    {
		try
		{
		    ret = copy_max(dialog, fds, fdd);
		    close(fds);
		    close(fdd);
		}
		catch(Erange & e)
		{
		    ret = EXIT_BUG; // not the same meaning for dar_cp
		}
	    }
	    else
		ret = EXIT_ERROR;

    return ret;
}

static void show_usage(user_interaction & dialog, char *argv0)
{
    dialog.warning(tools_printf(gettext("usage : %s <source> <destination>\n"), argv0));
}

static void show_version(user_interaction & dialog, char *argv0)
{
/* C++ syntax used*/
    try
    {
	char *cmd = tools_extract_basename(argv0);
	    // never return a NULL pointer but
	try
	{
	    dialog.warning(tools_printf("\n %s version %s Copyright (C) 2002-2052 Denis Corbin\n\n\n", cmd, DAR_CP_VERSION));
	    dialog.warning(tools_printf(gettext(" compiled the %s with %s version %s\n"), __DATE__, CC_NAT,  __VERSION__));
	    dialog.warning(tools_printf(gettext(" %s is part of the Disk ARchive suite (Release %s)\n"), cmd, PACKAGE_VERSION));
	    dialog.warning(tools_printf(gettext(" %s comes with ABSOLUTELY NO WARRANTY; for details\n type `dar -W'."), cmd));
	    dialog.warning(tools_printf(gettext(" This is free software, and you are welcome\n to redistribute it under certain conditions;")));
	    dialog.warning(tools_printf(gettext(" type `dar -L | more'\n for details.\n\n")));
	}
	catch(...)
	{
	    delete [] cmd;
	    throw;
	}
	delete [] cmd;
    }
    catch(...)
    {
	dialog.warning(tools_printf(gettext("Unexpected exception from libdar")));
	throw SRC_BUG;
    }
/* END of C++ syntax used*/
}

static int open_files(user_interaction & dialog, char *src, char *dst, int *fds, int *fdd)
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
            dialog.warning(tools_printf(gettext("Memory allocation failed : %s"), strerror(errno)));
            return 0;
        }
        try
        {
            tmp2 = tools_extract_basename(src);
        }
        catch(...)
        {
            dialog.warning(tools_printf(gettext("Memory allocation failed")));
            return 0;
        }
        strcpy(tmp, dst);
        strcat(tmp, "/");
        strcat(tmp, tmp2);
        delete [] tmp2;
        dst = tmp; // dst is not dynamically allocated, thus we can
            // loose its reference.
    }

    *fds = ::open(src, O_RDONLY|O_BINARY);

    if(*fds < 0)
    {
        dialog.warning(tools_printf(gettext("Cannot open source file : %s"), strerror(errno)));
        if(tmp != NULL)
            free(tmp);
        return 0; // error
    }

    *fdd = ::open(dst, O_WRONLY|O_CREAT|O_EXCL|O_BINARY, 0666);
    if(tmp != NULL)
        free(tmp);
    if(*fdd < 0)
    {
        dialog.warning(tools_printf(gettext("Cannot open destination file : %s"), strerror(errno)));
        close(*fds);
        *fds = -1;
        return 0;
    }
    else
        return 1;
}

static int copy_max(user_interaction & dialog, int src, int dst)
{
    thread_cancellation thr;

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

    dialog.warning(tools_printf(gettext("Starting the copy of %d byte(s)"), taille));
    do
    {
	thr.check_self_cancellation();
        lu = normal_copy(BUF_SIZE, buffer, src, dst);
        if(lu < 0)
        {
	    off_t local_missed;
	    off_t current = lseek(src, 0, SEEK_CUR);

            printf(gettext("Error reading source file (we are at %.2f %% of data copied), trying to read further: %s\n"), (float)(current)/(float)(taille)*100, strerror(errno));
            xfer_before_error(BUF_SIZE/2, buffer, src, dst);
            if(skip_to_next_readable(BUF_SIZE/2, buffer, src, dst, local_missed))
	    {
		printf(gettext("Skipping done (missing %.0f byte(s)), found correct data to read, continuing the copy...\n"), (float)local_missed);
		missed += local_missed;
                lu = 1;
	    }
            else
	    {
		dialog.warning(tools_printf(gettext("Reached End of File, no correct data could be found after the last error\n")));
		missed += (taille - current);
                lu = 0;
	    }
        }
    }
    while(lu > 0);

    printf(gettext("Copy finished. Missing %.0f byte(s) of data\n"), (float)missed);
    printf(gettext("Which is  %.2f %% of the total amount of data\n"), (float)(missed)/((float)(taille))*100);
    return missed == 0 ? EXIT_OK : EXIT_DATA_ERROR;
}

static void xfer_before_error(int block, char *buffer, int src, int dst)
{
    if(read(src, buffer, 1) == 1)
    {
	if(lseek(src, -1, SEEK_CUR) < 0)
	    throw Erange("xfer_before_error", gettext("Cannot seek back one char"));
    }
    else
	return; // the error is just next char to read

    while(block > 1)
    {
        int lu = read(src, buffer, block);
        if(lu > 0)
        {
            if(write(dst, buffer, lu) < 0)
                throw Erange("xfer_before_error", gettext("Cannot write to destination, aborting"));
        }
        else
            block /= 2;
    }
}

static int skip_to_next_readable(int block, char *buffer, int src, int dst, off_t & missed)
{
    int lu = 0;
    off_t min = lseek(src, 0, SEEK_CUR), max;
    missed = min;
    thread_cancellation thr;

	//////////////////////////
	// first step: looking for a next readable (maybe not the very next readable)
	// we put its position in "max"
	//////////////////////////

    do
    {
	thr.check_self_cancellation();
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
	thr.check_self_cancellation();
	off_t mid = (max + min) / 2;
	if(lseek(src, mid, SEEK_SET) < 0)
	{
	    perror(gettext("Cannot seek in file"));
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
    static char id[]="$Id: dar_cp.cpp,v 1.16.2.2 2007/02/24 17:43:02 edrusb Rel $";
    dummy_call(id);
}

static int normal_copy(int block, char *buffer, int src, int dst)
{
    int lu = read(src, buffer, block);
    if(lu > 0)
    {
        int ecrit = 0;
        while(ecrit < lu)
        {
            int tmp = write(dst, buffer+ecrit, lu-ecrit);
            if(tmp < 0)
		throw Erange("normal_copy", gettext("Cannot write to destination, aborting"));
            else
                if(tmp == 0)
                    printf(gettext("Non fatal error while writing to destination file, retrying\n"));
                else
                    ecrit += tmp;
        }
    }
    return lu;
}
