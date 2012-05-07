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
// $Id: user_interaction.cpp,v 1.18 2002/03/25 22:02:38 denis Rel $
//
/*********************************************************************/

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "user_interaction.hpp"
#include "erreurs.hpp"

using namespace std;

static int input = -1;
static ostream *output = NULL;
static bool beep = false;
static termio initial;

static void blocking_read(int fd, bool mode);

void user_interaction_init(int input_filedesc, ostream &out)
{
    struct termio term;

    if(input_filedesc < 0)
	throw SRC_BUG;

    output = &out;
    if(ioctl(input_filedesc, TCGETA, &term) >= 0)
    {
	initial = term;
	term.c_lflag &= ~ICANON;
	term.c_lflag &= ~ECHO;
	term.c_lflag &= ~ECHOE;
	if(ioctl(input_filedesc, TCSETA, &term) >= 0)
	{
	    if(input < 0)
		close(input);
	    input = input_filedesc;
	}
	else
	    throw Erange("user_interaction_init", string("error while setting user terminal to character mode : ") + strerror(errno));
    }
    else
	throw Erange("user_interaction_init", string("can't get informations from user terminal : ") + strerror(errno));
}

void user_interaction_close()
{
    ioctl(input, TCSETA, &initial);
}

void user_interaction_pause(string message)
{
    const int bufsize = 1024;
    char buffer[bufsize];
    char & a = buffer[0];

    if(input < 0)
	throw SRC_BUG;

	// flushing any character remaining in the input stream
    blocking_read(input, true);
    while(read(input, buffer, bufsize) >= 0)
	;
    blocking_read(input, false);

	// now asking the user
    do
    {
	*output << message << " [return = OK | esc = cancel]" << (beep ? "\007\007\007" : "") << endl;
	if(read(input, &a, 1) < 0)
	    throw Erange("user_interaction_pause", string("error while reading user answer from terminal : ") + strerror(errno));
    }
    while(a != 27 && a != '\n');

    if(a == 27) // escape key
	throw Euser_abort(message);
    else
	*output << "continuing ..." << endl;
}

void user_interaction_warning(string message)
{
    *output << message << endl;
}

void user_interaction_set_beep(bool mode)
{
    beep = mode;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: user_interaction.cpp,v 1.18 2002/03/25 22:02:38 denis Rel $";
    dummy_call(id);
}

static void blocking_read(int fd, bool mode)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if(mode)
	flags |= O_NDELAY;
    else
	flags &= ~O_NDELAY;
    fcntl(fd, F_SETFL, flags);
}
