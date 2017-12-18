/*********************************************************************
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
******************************************************************** */

#include "../my_config.h"
#include "cygwin_adapt.h"

#if HAVE_STDIO_H
#include <stdio.h>
#endif

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#if HAVE_TERMIOS_H
#include <termios.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_SYS_TYPE_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

/// the compiler Nature MACRO
#ifdef __GNUC__
#define CC_NAT "GNUC"
#else
#define CC_NAT "unknown"
#endif

#define BUFSIZE 102400
#ifdef SSIZE_MAX
#if SSIZE_MAX < BUFSIZE
#undef BUFSIZE
#define BUFSIZE SSIZE_MAX
#endif
#endif

#define KEY_INPUT "split_input"
#define KEY_OUTPUT "split_output"
#define DAR_SPLIT_VERSION "1.1.0"

static void usage(char *a);
static void show_version(char *a);
static int init();
static void blocking_read(int fd, int mode);
static void purge_fd(int fd);
static void stop_and_wait();
static void pipe_handle_pause(int x);
static void pipe_handle_end(int x);
static void normal_read_to_multiple_write(char *filename, int sync_mode);
static void multi_read_to_normal_write(char *filename);
static int open_read(char *filemane);  /* returns the filedescriptor */
static int open_write(char *filename, int sync_mode); /* returns the filedescriptor */

static int fd_inter = -1;

int main(int argc, char *argv[])
{
    int key_index = 1; /* the expected position of the command */

	/* command line parsing */

    switch(argc)
    {
    case 2:
	if(strcmp("-v", argv[1]) == 0
	   || strcmp("--version", argv[1]) == 0
	   || strcmp("-V", argv[1]) == 0)
	{
	    show_version(argv[0]);
	    return 0;
	}
	else
	{
	    usage(argv[0]);
	    return 1;
	}
    case 3:
	break;
    case 4:
	if(strcmp(argv[1], "-s") != 0)
	{
	    usage(argv[0]);
	    return 1;
	}
	else /* -s option has been given */
	    key_index = 2;
	break;
    default:
	usage(argv[0]);
	return 1;
    }

	/* initialization */

    if(!init())
	return 2;

	/* execution */

    if(strcmp(KEY_OUTPUT, argv[key_index]) == 0)
	normal_read_to_multiple_write(argv[key_index + 1], key_index == 2);
    else
	if(strcmp(KEY_INPUT, argv[key_index]) == 0)
	    if(key_index == 1)
		multi_read_to_normal_write(argv[key_index + 1]);
	    else
		usage(argv[0]); /* -s option not available with KEY_INPUT */
	else
	    usage(argv[0]);

	/* normal exit */

    return 0;
}

static void usage(char *a)
{
    fprintf(stderr, "usage: %s { %s | [-s] %s } <filename>\n", a, KEY_INPUT, KEY_OUTPUT);
    fprintf(stderr, "- in %s mode, the data sent to %s's input is copied to the given filename\n  which may possibly be a non permanent output (retrying to write in case of failure)\n", KEY_OUTPUT, a);
    fprintf(stderr, "- in %s mode, the data is read from the given filename which may possibly\n  be non permanent input (retrying to read in case of failure) and copied to\n  %s's output\n", KEY_INPUT, a);
    fprintf(stderr, "\nThe -s option for %s mode leads %s to make SYNC writes, this avoid\n  operating system's caching to wrongly report a write as successful. This flag\n  reduces write performances but may be necessary when the end of tape is not\n  properly detected by %s\n", KEY_OUTPUT, a, a);
}

static void show_version(char *a)
{
    fprintf(stderr, "\n %s version %s, Copyright (C) 2015 Denis Corbin\n", a, DAR_SPLIT_VERSION);
    fprintf(stderr, " compiled the %s with %s version %s\n", __DATE__, CC_NAT, __VERSION__);
    fprintf(stderr, " %s is part of the Disk ARchive suite (Release %s)\n", a, PACKAGE_VERSION);
    fprintf(stderr, " %s comes with ABSOLUTELY NO WARRANTY; for details\n type `dar -W'.", a);
    fprintf(stderr, " This is free software, and you are welcome\n to redistribute it under certain conditions;");
    fprintf(stderr, " type `dar -L | more'\n for details.\n\n");
}

static int init()
{
    char tty[L_ctermid+1];

    (void)ctermid(tty);
    tty[L_ctermid] = '\0';

    fd_inter = open(tty, O_RDONLY);
    if(fd_inter < 0)
    {
	fprintf(stderr, "Cannot open filedscriptor to provide user interaction: %s\n", strerror(errno));
	return 0; /* false */
    }
    else
	return 1; /* true */
}

static void blocking_read(int fd, int mode)
{
    int flags = fcntl(fd_inter, F_GETFL, 0);

	/* code fetched from tools_blocking_read() but not usable as is because dar_split is written in basic C (not C++) */

    if(flags < 0)
	fprintf(stderr, "Cannot read \"fcbtl\" file's flags: %s\n", strerror(errno));
    else
    {
	if(!mode)
	    flags |= O_NONBLOCK;
	else
	    flags &= ~O_NONBLOCK;
	if(fcntl(fd, F_SETFL, flags) < 0)
	    fprintf(stderr, "Cannot modify the NONBLOCK fcntl flag: %s", strerror(errno));
    }
}

static void purge_fd(int fd)
{
    static const int bufsize = 10;
    char buf[bufsize];

    blocking_read(fd, 0 == 1);
    while(read(fd, buf, bufsize) >= 0)
	;
    blocking_read(fd, 0 == 0);
}

static void stop_and_wait()
{
    char tmp[10];

    purge_fd(fd_inter);
    fprintf(stderr, "Press return when ready to continue or hit CTRL-C to abort\n");
    read(fd_inter, tmp, 3);
}

static void pipe_handle_pause(int x)
{
    fprintf(stderr, "No reader to pipe we output data to, do something!\n");
    stop_and_wait();
}

static void pipe_handle_end(int x)
{
    exit(0);
}


static void normal_read_to_multiple_write(char *filename, int sync_mode)
{
    char* buffer = (char*) malloc(BUFSIZE);
    int lu;
    int ecru;
    int offset;
    int fd = buffer == NULL ? -1 : open_write(filename, sync_mode);
    int run = 1;
    size_t tape_size = 0;
    size_t step;

    if(buffer == NULL)
    {
	fprintf(stderr, "Error allocating %u bytes memory block: %s\n", BUFSIZE, strerror(errno));
	run = 0;
    }

    signal(SIGPIPE, pipe_handle_pause); /* handling case when writing to pipe that has no reader */

    while(run)
    {
	lu = read(0, buffer, BUFSIZE);
	if(lu == 0) /* reached EOF */
	    break; /* exiting the while loop, end of processing */

	if(lu < 0)
	{
	    if(errno == EAGAIN || errno == EINTR)
		continue; /* start over the while loop */
	    else
	    {
		fprintf(stderr, "Error reading data: %s\n", strerror(errno));
		break; /* exiting the while loop, end of processing */
	    }
	}

	    /* now we have data to write down */

	offset = 0;
	step = lu;
	do
	{
	    ecru = write(fd, buffer + offset, step);

	    if(ecru < 0) /* an error occured */
	    {
		switch(errno)
		{
		case EAGAIN:
		case EINTR:
		    break;
		case ENOSPC:
		    if(step > 1)
			step /= 2;
		    else
		    {
			syncfs(fd);
			close(fd);
			fprintf(stderr, "No space left on destination after having written %ld bytes, please to something!\n", tape_size);
			tape_size = 0;
			step = lu;
			stop_and_wait();
			fd = open_write(filename, sync_mode);
		    }
		    break;
		default:
		    fprintf(stderr, "Error writing data: %s\n", strerror(errno));
		    stop_and_wait();
		}
	    }
	    else
	    {
		if(ecru > lu)
		{
		    fprintf(stderr, "BUG MET at line %d\n", __LINE__);
		    break; /* end or processing */
		}
		else
		{
			/* starting from here no error occurred and ecru >= 0 */

		    offset += ecru;
		    lu -= ecru;
		    tape_size += ecru;
		}
	    }
	}
	while(lu > 0);

	if(lu > 0)
	    break; /* not all data could be written, aborting */
    }

    fprintf(stderr, "%ld bytes written since the last media change", tape_size);

    if(fd >= 0)
    {
	syncfs(fd);
	close(fd);
    }
    if(buffer != NULL)
	free(buffer);
}


static void multi_read_to_normal_write(char *filename)
{
    char* buffer = (char*)malloc(BUFSIZE);
    int lu;
    int ecru;
    int offset;
    int fd = buffer == NULL ? -1 : open_read(filename);
    int run = 1;

    if(buffer == NULL)
    {
	fprintf(stderr, "Error allocating %u bytes memory block: %s\n", BUFSIZE, strerror(errno));
	run = 0;
    }

    signal(SIGPIPE, pipe_handle_end); /* handling case when writing to pipe that has no reader */

    while(run)
    {
	lu = read(fd, buffer, BUFSIZE);
	if(lu == 0) // EOF
	{
	    close(fd);
	    fprintf(stderr, "No more data available from source, please do something!\n");
	    stop_and_wait();
	    open_read(filename);
	    continue; /* start over the while loop */
	}
	else
	{
	    if(lu < 0)
	    {
		switch(errno)
		{
		case EAGAIN:
		case EINTR:
		    break;
		default:
		    fprintf(stderr, "Error reading data: %s\n", strerror(errno));
		    stop_and_wait();
		    break;
		}
		continue; /* start over the while loop */
	    }
	}

	    /* now we have data to write down */

	offset = 0;
	do
	{
	    ecru = write(1, buffer + offset, lu);

	    if(ecru < 0) /* an error occured */
	    {
		int loop = 0;

		switch(errno)
		{
		case EAGAIN:
		case EINTR:
		    loop = 1;
		    break;
		default:
		    fprintf(stderr, "Error writing data: %s\n", strerror(errno));
		    break;
		}

		if(loop)
		    continue; /* start over the do - while loop */
		else
		    break; /* exiting the do - while loop, aborting the process */
	    }

	    if(ecru > lu)
	    {
		fprintf(stderr, "BUG MET at line %d\n", __LINE__);
		break; /* end or processing */
	    }

		/* starting from here no error occurred and lu >= ecru >= 0 */

	    offset += ecru;
	    lu -= ecru;
	}
	while(lu > 0);

	if(lu > 0)
	    break; /* not all data could be written, aborting */
    }

    if(fd >= 0)
	close(fd);
    if(buffer != NULL)
	free(buffer);
}


static int open_write(char *filename, int sync_mode)
{
    int fd;
    int flag = sync_mode ? O_SYNC : 0;

    do
    {
	fd = open(filename, O_WRONLY|O_BINARY|flag);
	if(fd < 0)
	{
	    fprintf(stderr,"Error opening output: %s\n", strerror(errno));
	    stop_and_wait();
	}
    }
    while(fd < 0);

    return fd;
}

static int open_read(char *filename)
{
    int fd;

    do
    {
	fd = open(filename, O_RDONLY|O_BINARY);
	if(fd < 0)
	{
	    fprintf(stderr,"Error opening input: %s\n", strerror(errno));
	    stop_and_wait();
	}
    }
    while(fd < 0);

    return fd;
}
