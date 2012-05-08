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
// $Id: user_interaction.cpp,v 1.31 2002/06/26 22:04:13 denis Rel $
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
#include <stdarg.h>
#include <string.h>
#include "user_interaction.hpp"
#include "erreurs.hpp"
#include "tools.hpp"

using namespace std;

static int input = -1;
static ostream *output = NULL;
static ostream *inter = NULL;
static bool beep = false;
static termio initial;
static termio interaction;
static bool has_terminal = false;

static void set_term_mod(const struct termio & etat);

void user_interaction_init(int input_filedesc, ostream *out, ostream *interact)
{
    struct termio term;
    has_terminal = false;

    if(input_filedesc < 0)
	throw SRC_BUG;

    if(out != NULL)
	output = out;
    else
	throw SRC_BUG;
    if(interact != NULL)
	inter = interact;
    else
	throw SRC_BUG;

	// preparing input for swaping between char mode and line mode (terminal settings)
    if(ioctl(input_filedesc, TCGETA, &term) >= 0)
    {
	initial = term;
	term.c_lflag &= ~ICANON;
	term.c_lflag &= ~ECHO;
	term.c_lflag &= ~ECHOE;
	interaction = term;
	if(input < 0)
	    close(input);
	input = input_filedesc;

	    // checking now that we can change to character mode
	set_term_mod(interaction);
	set_term_mod(initial);
	    // but we don't need it right now, so swapping back to line mode
	has_terminal = true;
    }
    else
        user_interaction_warning("No terminal found for user interaction. All questions will abort the program.");
}

void user_interaction_change_non_interactive_output(ostream *out)
{
    if(out != NULL)
	output = out;
    else
	throw SRC_BUG;
}

static void set_term_mod(const struct termio & etat)
{
    if(ioctl(input, TCSETA, &etat) < 0)
	throw Erange("user_interaction : set_term_mod", string("Error while changing user terminal properties: ") + strerror(errno));
}

void user_interaction_close()
{
    if(has_terminal)
	ioctl(input, TCSETA, &initial);
}

void user_interaction_pause(string message)
{
    const int bufsize = 1024;
    char buffer[bufsize];
    char & a = buffer[0];

    if(!has_terminal)
    {
	user_interaction_warning("No terminal found and user interaction needed, aborting.");
	throw Euser_abort(message);
    }

    if(input < 0)
	throw SRC_BUG;

    set_term_mod(interaction);
    try
    {
	    // flushing any character remaining in the input stream
	tools_blocking_read(input, false);
	while(read(input, buffer, bufsize) >= 0)
	    ;
	tools_blocking_read(input, true);

	    // now asking the user
	do
	{
	    *inter << message << " [return = OK | esc = cancel]" << (beep ? "\007\007\007" : "") << endl;
	    if(read(input, &a, 1) < 0)
		throw Erange("user_interaction_pause", string("Error while reading user answer from terminal: ") + strerror(errno));
	}
	while(a != 27 && a != '\n');

	if(a == 27) // escape key
	    throw Euser_abort(message);
	else
	    *inter << "Continuing..." << endl;
    }
    catch(...)
    {
	set_term_mod(initial);
	throw;
    }
}

void user_interaction_warning(string message)
{
    if(output == NULL)
	throw SRC_BUG; // user_interaction has not been properly initialized
    *output << message << endl;
}

ostream &user_interaction_stream()
{
    return *output;
}

void user_interaction_set_beep(bool mode)
{
    beep = mode;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: user_interaction.cpp,v 1.31 2002/06/26 22:04:13 denis Rel $";
    dummy_call(id);
}

void ui_printf(const char *format, ...)
{
    va_list ap;
    bool end;
    unsigned long taille = strlen(format)+1;
    char *copie;

    if(output == NULL)
	throw Erange("ui_printf", "user_interaction  module is not initialized");

    copie = new char[taille];
    if(copie == NULL)
	throw Ememory("ui_printf");

    va_start(ap, format);
    try
    {
	char *ptr = copie, *start = copie;

	strcpy(copie, format);
	copie[taille-1] = '\0';

	do
	{
	    while(*ptr != '%' && *ptr != '\0')
		ptr++;
	    if(*ptr == '%')
	    {
		*ptr = '\0';
		end = false;
	    }
	    else
		end = true;
	    *output << start;
	    if(!end)
	    {
		ptr++;
		switch(*ptr)
		{
		case '%':
		    *output << "%";
		    break;
		case 'd':
		    *output << va_arg(ap, int);
		    break;
		case 's':
		    *output << va_arg(ap, char *);
		    break;
		case 'c':
		    *output << static_cast<char>(va_arg(ap, int));
		    break;
		default:
		    throw Efeature(string("%") + (*ptr) + " is not implemented in ui_printf format argument");
		}
		ptr++;
		start = ptr;
	    }
	}
	while(!end);
    }
    catch(...)
    {
	va_end(ap);
	delete copie;
	throw;
    }
    delete copie;
    va_end(ap);
}
