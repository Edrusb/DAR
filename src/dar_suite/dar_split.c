/*********************************************************************
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

#if HAVE_GETOPT_H
#include <getopt.h>
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
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
#define DAR_SPLIT_VERSION "1.2.0"

#define ERR_SYNTAX 1
#define ERR_TIMER 2
#define ERR_BUG 3

static void usage(char *a);
static void show_version(char *a);
static int init();
static void blocking_read(int fd, int mode);
static void purge_fd(int fd);
static void stop_and_wait();
static void pipe_handle_pause(int x);
static void pipe_handle_end(int x);
static void alarm_handle(int x);
static void normal_read_to_multiple_write(char *filename, int sync_mode, unsigned int bs, unsigned int rate);
static void multi_read_to_normal_write(char *filename, unsigned int bs, unsigned int rate, unsigned int count);
static int open_read(char *filemane);  /* returns the filedescriptor */
static int open_write(char *filename, int sync_mode); /* returns the filedescriptor */
static void init_timer(unsigned int bufsize, unsigned int rate);
static void rate_limit(unsigned int rate);

static int fd_inter = -1;
static int can_go = 0;

int main(int argc, char *argv[])
{
    signed int lu;

	/* variables used to carry arguments */

    int split_mode = 0; /* 0 : unset, 1 = normal_read , 2 = normal_write */
    char *splitted_file = NULL;
    int sync_mode = 0;
    unsigned int bs = 0;
    unsigned int rate = 0;
    unsigned int count = 0;

	/* command line parsing */

    while((lu = getopt(argc, argv, "-sb:hvr:c:")) != EOF)
    {
	switch(lu)
	{
	case 1: /* non option argument due to the initial '-' in getopt string */
	    if(split_mode == 0)
	    {
		if(strcmp(KEY_OUTPUT, optarg) == 0)
		    split_mode = 1;
		else if(strcmp(KEY_INPUT, optarg) == 0)
		    split_mode = 2;
		else
		{
		    fprintf(stderr, "Unknown mode %s. Please use either %s or %s\n",
			    optarg,
			    KEY_OUTPUT,
			    KEY_INPUT);
		    return ERR_SYNTAX;
		}
	    }
	    else /* now reading the filename */
		splitted_file = optarg;
	    break;
	case 's':
	    sync_mode = 1;
	    break;
	case 'b':
	    bs = atoi(optarg);
	    break;
	case 'r':
	    rate = atoi(optarg);
	    break;
	case 'c':
	    count = atoi(optarg);
	    break;
	case 'h':
	    usage(argv[0]);
	    return 0;
	case 'v':
	    show_version(argv[0]);
	    return 0;
	case ':':
	    fprintf(stderr, "Missing parameter to option -%c\n", (char)optopt);
	    return ERR_SYNTAX;
	case '?':
	    fprintf(stderr, "Ignoring unknown option -%c\n", (char)optopt);
	    return ERR_SYNTAX;
	}
    }

    if(splitted_file == NULL)
    {
	fprintf(stderr, "Missing filename argument\n");
	return ERR_SYNTAX;
    }

    if(count > 0 && split_mode != 2)
    {
	fprintf(stderr, "-c option is only available in split_input mode\n");
	return ERR_SYNTAX;
    }

    if(sync_mode == 1 && split_mode != 1)
    {
	fprintf(stderr, "-s option is only available in split_output mode\n");
	return ERR_SYNTAX;
    }

	/* initialization */

    if(!init())
	return 2;

	/* execution */

    switch(split_mode)
    {
    case 1:
	normal_read_to_multiple_write(splitted_file, sync_mode, bs, rate);
	break;
    case 2:
	multi_read_to_normal_write(splitted_file, bs, rate, count);
	break;
    default:
	fprintf(stderr, "missing read mode argument. Please user either %s or %s\n",
		KEY_OUTPUT,
		KEY_INPUT);
	return ERR_SYNTAX;
    }

	/* normal exit */

    return 0;
}

static void usage(char *a)
{
    fprintf(stderr, "\nusage: %s [-b <block size>] [-r <rate>] { [-c <count>] %s | [-s] %s } <filename>\n\n", a, KEY_INPUT, KEY_OUTPUT);
    fprintf(stderr, "- in %s mode, the data sent to %s's input is copied to the given filename\n  which may possibly be a non permanent output (retrying to write in case of failure)\n", KEY_OUTPUT, a);
    fprintf(stderr, "- in %s mode, the data is read from the given filename which may possibly\n  be non permanent input (retrying to read in case of failure) and copied to\n  %s's output\n", KEY_INPUT, a);
    fprintf(stderr, "\nThe -s option for %s mode leads %s to make SYNC writes, this avoid\n  operating system's caching to wrongly report a write as successful. This flag\n  reduces write performances but may be necessary when the end of tape is not\n  properly detected by %s\n", KEY_OUTPUT, a, a);
    fprintf(stderr, "With -b option the amount of bytes sent per read or write system call does\n  not exceed this amount in byte\n");
    fprintf(stderr, "With -r option the transfer is limited to the given byte/second rate\n");
    fprintf(stderr, "With -c option dar_split will assume there is at most <count> tape and not ask\n   for further \n");
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
    (void)read(fd_inter, tmp, 3);
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

static void alarm_handle(int x)
{
    can_go = 1;
}

static void normal_read_to_multiple_write(char *filename, int sync_mode, unsigned int bs, unsigned int rate)
{
    unsigned int bufsize = (bs == 0 ? BUFSIZE : bs);
    char* buffer = (char*) malloc(bufsize);
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
    init_timer(bufsize, rate);

    while(run)
    {
	rate_limit(rate);
	lu = read(0, buffer, bufsize);
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
#if HAVE_SYNCFS
			syncfs(fd);
#else
			sync();
#endif
			close(fd);
			fprintf(stderr, "No space left on destination after having written %lu bytes, please to something!\n", (unsigned long)tape_size);
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
		    exit(ERR_BUG); /* end or processing */
		}
		else
		{
			/* starting from here no error occurred and ecru >= 0 */

		    offset += ecru;
		    lu -= ecru;
		    if(step > lu)
			step = lu;
		    tape_size += ecru;
		}
	    }
	}
	while(lu > 0);

	if(lu > 0)
	    break; /* not all data could be written, aborting */
    }

    fprintf(stderr, "%ld bytes written since the last media change", (unsigned long)tape_size);

    if(fd >= 0)
    {
#if HAVE_SYNCFS
	syncfs(fd);
#else
	sync();
#endif
	close(fd);
    }
    if(buffer != NULL)
	free(buffer);
}


static void multi_read_to_normal_write(char *filename, unsigned int bs, unsigned int rate, unsigned int count)
{
    unsigned int bufsize = (bs == 0 ? BUFSIZE: bs);
    char* buffer = (char*)malloc(BUFSIZE);
    int lu;
    int ecru;
    int offset;
    int fd = buffer == NULL ? -1 : open_read(filename);
    int run = 1;
    int iter = 0;

    if(buffer == NULL)
    {
	fprintf(stderr, "Error allocating %u bytes memory block: %s\n", BUFSIZE, strerror(errno));
	run = 0;
    }

    signal(SIGPIPE, pipe_handle_end); /* handling case when writing to pipe that has no reader */
    init_timer(bufsize, rate);

    while(run)
    {
	rate_limit(rate);
	lu = read(fd, buffer, bufsize);
	if(lu == 0) // EOF
	{
	    close(fd);
	    ++iter;
	    if(count == 0 || iter < count)
	    {
		if(count == 0)
		    fprintf(stderr, "No more data available from source, please do something!\n");
		else
		    fprintf(stderr, "No more data available from source number %d, please do something!\n", iter);
		stop_and_wait();
		open_read(filename);
		continue; /* start over the while loop */
	    }
	    else
		break; // we have done all tapes, ending the process
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
		exit(ERR_BUG); /* end or processing */
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

static void init_timer(unsigned int bufsize, unsigned int rate)
{
    if(rate > 0)
    {
	struct timeval tm_int;
	struct itimerval timer;

		    /* program alarm() at correct rate */

	signal(SIGALRM, alarm_handle);

	tm_int.tv_sec = bufsize / rate ; /* integer division */
	tm_int.tv_usec =  (bufsize % rate) * 1000000 / rate; /* remaining of the integer division expressed in microseconds */
	timer.it_interval = tm_int;
	timer.it_value = tm_int;

	if(setitimer(ITIMER_REAL, &timer, NULL) != 0)
	{
	    fprintf(stderr, "Cannot set timer to control transfer rate: %s\n", strerror(errno));
	    exit(ERR_TIMER);
	}
	can_go = 1;
    }
}

static void rate_limit(unsigned int rate)
{
    if(rate > 0)
    {
	if(!can_go)
	    pause();
	    /* waiting for a signal (ALARM) some time fraction of time to
	       not exceed the expected transfer rate
	       if the incoming flow is faster, the input buffer will fill up to
	       a time the OS will suspend the writer, dar_split will have enough
	       data to write and continue at its pace. The OS may awake the writer
	       at a later time for before the dar_split input get empty
	    */

	can_go = 0; // will be set to 1 by the timer handle
    }
}
