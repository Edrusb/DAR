//*********************************************************************/
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

#include "../my_config.h"

#include "line_tools.hpp"

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

#include <iostream>

#include "dar_suite.hpp"
#include "tools.hpp"
#include "cygwin_adapt.hpp"
#include "dar_suite.hpp"
#include "libdar.hpp"
#include "thread_cancellation.hpp"

#define DAR_CP_VERSION "1.4.0"

using namespace libdar;
using namespace std;

static void show_usage(user_interaction & dialog, char *argv0);
static void show_version(user_interaction & dialog, char *argv0);
static int open_files(user_interaction & dialog, char *src, char *dst, int *fds, int *fdd, bool force);
static int copy_max(user_interaction & dialog, int src, int dst);
static void xfer_before_error(int block, char *buffer, int src, int dst);
static int skip_to_next_readable(int block, char *buffer, int src, int dst, off_t & missed);
    /* return 0 if not found any more readable data, else return 1 */
static int normal_copy(int block, char *buffer, int src, int dst);
    /* return the number of copied bytes (negative value upon error, zero at end of file) */
static int little_main(shared_ptr<user_interaction> & dialog, int argc, char * const argv[], const char **env);

int main(int argc, char * const argv[], const char **env)
{
    return dar_suite_global(argc,
			    argv,
			    env,
			    "hf",
#if HAVE_GETOPT_LONG
			    nullptr,
#endif
			    '\0', // should never be met as option, thus early read the whole command-line for -j and -Q options
			    &little_main);
}

static int little_main(shared_ptr<user_interaction> & dialog, int argc, char * const argv[], const char **env)
{
    int fds, fdd;
    int ret = EXIT_OK;

    if(argc > 1 && strcmp(argv[1],"-V") == 0)
    {
        show_version(*dialog, argv[0]);
        ret = EXIT_OK;
    }
    else
    {
	bool force = (argc > 1 && string(argv[1]) == string("-f"));

	if((argc != 3 && !force) || (argc != 4 && force) || string(argv[1]) == string("-h"))
	{
	    show_usage(*dialog, argv[0]);
	    ret = EXIT_SYNTAX;
	}
	else
	{
	    char *src = force ? argv[2] : argv[1];
	    char *dst = force ? argv[3] : argv[2];

	    if(open_files(*dialog, src, dst, &fds, &fdd, force))
	    {
		try
		{
		    ret = copy_max(*dialog, fds, fdd);
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
	}
    }

    return ret;
}

static void show_usage(user_interaction & dialog, char *argv0)
{
    dialog.message(tools_printf(gettext("usage : %s [-f] <source> <destination>\n"), argv0));
}

static void show_version(user_interaction & dialog, char *argv0)
{
/* C++ syntax used*/
    try
    {
	string cmd;
	line_tools_extract_basename(argv0, cmd);
	    // never return a nullptr pointer but
	dialog.message(tools_printf("\n %s version %s Copyright (C) 2002-2025 Denis Corbin\n\n\n", cmd.c_str(), DAR_CP_VERSION));
	dialog.message(tools_printf(gettext(" compiled the %s with %s version %s\n"), __DATE__, CC_NAT,  __VERSION__));
	dialog.message(tools_printf(gettext(" %s is part of the Disk ARchive suite (Release %s)\n"), cmd.c_str(), PACKAGE_VERSION));
	dialog.message(tools_printf(gettext(" %s comes with ABSOLUTELY NO WARRANTY; for details type `dar -W'."), cmd.c_str()));
	dialog.message(tools_printf(gettext(" This is free software, and you are welcome to redistribute it under")));
	dialog.message(tools_printf(gettext(" certain conditions; type `dar -L | more' for details.\n\n")));
    }
    catch(...)
    {
	dialog.message(tools_printf(gettext("Unexpected exception from libdar")));
	throw SRC_BUG;
    }
/* END of C++ syntax used*/
}

static int open_files(user_interaction & dialog, char *src, char *dst, int *fds, int *fdd, bool force)
{
    struct stat buf;
    int val = stat(dst, &buf);
    char *tmp = nullptr;

    if(val == 0 && S_ISDIR(buf.st_mode))
    {
        tmp = (char *)malloc(strlen(src)+strlen(dst)+1+1);
        if(tmp == nullptr)
        {
            dialog.message(tools_printf(gettext("Memory allocation failed : %s"), strerror(errno)));
            return 0;
        }
        string tmp2;
        line_tools_extract_basename(src, tmp2);
        strcpy(tmp, dst);
        strcat(tmp, "/");
        strcat(tmp, tmp2.c_str());
        dst = tmp; // dst is not dynamically allocated, thus we can
            // loose its reference.
    }

    *fds = ::open(src, O_RDONLY|O_BINARY);

    if(*fds < 0)
    {
        dialog.message(tools_printf(gettext("Cannot open source file : %s"), strerror(errno)));
        if(tmp != nullptr)
            free(tmp);
        return 0; // error
    }

    *fdd = ::open(dst, O_WRONLY|O_CREAT|(force ? 0 : O_EXCL)|O_BINARY, 0666);
    if(tmp != nullptr)
        free(tmp);
    if(*fdd < 0)
    {
        dialog.message(tools_printf(gettext("Cannot open destination file : %s"), strerror(errno)));
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
	// too long makes I/O error very long to recover
	// too small makes copying under normal condition a bit slow
	// 1024 seems a balanced choice.
	//
    char buffer[BUF_SIZE];
    int lu = 0;
    off_t missed = 0;
    off_t taille = lseek(src, 0, SEEK_END);
    lseek(src, 0, SEEK_SET);

    dialog.message(tools_printf(gettext("Starting the copy of %u byte(s)"), taille));
    do
    {
	thr.check_self_cancellation();
        lu = normal_copy(BUF_SIZE, buffer, src, dst);
        if(lu < 0)
        {
	    off_t local_missed;
	    off_t current = lseek(src, 0, SEEK_CUR);

            printf(gettext("[%.2f %% read, %.2f %% missing] Error reading source file, trying to read further: %s\n"),
		   (float)(current)/(float)(taille)*100,
		   (float)(missed)/(float)(taille)*100,
		   strerror(errno));
            xfer_before_error(BUF_SIZE/2, buffer, src, dst);
            if(skip_to_next_readable(BUF_SIZE/2, buffer, src, dst, local_missed))
	    {
		missed += local_missed;
		printf(gettext("[%.2f %% read, %.2f %% missing] Skipping done (missing %.0f byte(s)), found correct data to read continuing the copy...\n"),
		       (float)(current)/(float)(taille)*100,
		       (float)(missed)/(float)(taille)*100,
		       (float)local_missed);
                lu = 1;
	    }
            else
	    {
		missed += (taille - current);
		dialog.message(tools_printf(gettext("[%.2f %% read, %.2f %% missing] Reached End of File, no correct data could be found after the last error\n"),
					    (float)(current)/(float)(taille)*100,
					    (float)(missed)/(float)(taille)*100));
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
