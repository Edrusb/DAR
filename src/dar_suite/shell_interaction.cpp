/*********************************************************************/
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
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_TERMIOS_H
#include <termios.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if HAVE_STDIO_H
#include <stdio.h>
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

} // end extern "C"

#include "integers.hpp"
#include "shell_interaction.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "integers.hpp"
#include "cygwin_adapt.hpp"

using namespace std;
using namespace libdar;

static S_I input = -1;
static ostream *output = NULL;
static ostream *inter = NULL;
static bool beep = false;
static termios initial;
static termios interaction;
static termios initial_noecho;
static bool has_terminal = false;

static void set_term_mod(const struct termios & etat);
static bool interaction_pause(const string &message, void *context);
static void interaction_warning(const string & message, void *context);
static string interaction_string(const string & message, bool echo, void *context);
static secu_string interaction_secu_string(const string & message, bool echo, void *context);

user_interaction *shell_interaction_init(ostream *out, ostream *interact, bool silent)
{
    has_terminal = false;
    user_interaction *ret = new (nothrow) user_interaction_callback(interaction_warning,
								    interaction_pause,
								    interaction_string,
								    interaction_secu_string,
								    NULL);
    if(ret == NULL)
	throw Ememory("shell_interaction_init");

        // checking and recording parameters to static variable
    if(out != NULL)
        output = out;
    else
        throw SRC_BUG;
    if(interact != NULL)
        inter = interact;
    else
        throw SRC_BUG;

        // looking for an input terminal
        //
        // we do not use anymore standart input but open a new descriptor
        // from the controlling terminal. This allow in some case to keep use
        // standart input for piping data while still having user interaction
        // possible.

    if(input >= 0)
        close(input);

        // terminal settings
    try
    {
	char tty[L_ctermid+1];
	struct termios term;

	(void)ctermid(tty);
	tty[L_ctermid] = '\0';

	input = ::open(tty, O_RDONLY|O_TEXT);
	if(input < 0)
	    throw Erange("",""); // used locally
	else
	{
	    if(silent)
		has_terminal = false; // force non interactive mode
	    else
		    // preparing input for swaping between char mode and line mode (terminal settings)
		if(tcgetattr(input, &term) >= 0)
		{
		    initial = term;
		    initial_noecho = term;
		    initial_noecho.c_lflag &= ~ECHO;
		    term.c_lflag &= ~ICANON;
  		    term.c_lflag &= ~ECHO;
		    term.c_cc[VTIME] = 0;
		    term.c_cc[VMIN] = 1;
		    interaction = term;

			// checking now that we can change to character mode
		    set_term_mod(interaction);
		    set_term_mod(initial);
			// but we don't need it right now, so swapping back to line mode
		    has_terminal = true;
		}
		else // failed to retrieve parameters from tty
		    throw Erange("",""); // used locally
	}
    }
    catch(Erange & e)
    {
	if(e.get_message() == "")
	{
	    if(!silent)
		ret->warning(gettext("No terminal found for user interaction. All questions will be assumed a negative answer (less destructive choice), which most of the time will abort the program."));
	}
	else
	    throw;
    }
    return ret;
}

void shell_interaction_change_non_interactive_output(ostream *out)
{
    if(out != NULL)
        output = out;
    else
        throw SRC_BUG;
}

void shell_interaction_set_beep(bool mode)
{
    beep = mode;
}

void shell_interaction_read_char(char & a)
{
    sigset_t old_mask;

    if(input < 0)
	throw SRC_BUG;

    tools_block_all_signals(old_mask);
    set_term_mod(interaction);
    if(read(input, &a, 1) < 0)
	throw Erange("shell_interaction_read_char", string(gettext("Error reading character: ")) + strerror(errno));
    tools_blocking_read(input, true);
    set_term_mod(initial);
    tools_set_back_blocked_signals(old_mask);
}

void shell_interaction_close()
{
    if(has_terminal)
	set_term_mod(initial);

    if(input >= 0)
    {
	close(input);
	input = -1;
    }
}

static void set_term_mod(const struct termios & etat)
{
    if(tcsetattr(input, TCSANOW, const_cast<struct termios *>(&etat)) < 0)
        throw Erange("shell_interaction : set_term_mod", string(gettext("Error while changing user terminal properties: ")) + strerror(errno));
}

static bool interaction_pause(const string &message, void *context)
{
    const U_I expected_buf_size = 1024;
#ifdef SSIZE_MAX
    const U_I bufsize = expected_buf_size > SSIZE_MAX ? SSIZE_MAX : expected_buf_size;
#else
    const U_I bufsize = expected_buf_size;
#endif
    char buffer[bufsize];
    char & a = buffer[0];
    char & b = buffer[1];
    bool ret;

    if(!has_terminal)
        return false;

    if(input < 0)
        throw SRC_BUG;

    set_term_mod(interaction);
    try
    {
	sigset_t old_mask;
	S_I tmp_ret, errno_bk, tmp_sup, errno_sup;


        do
        {
		// flushing any character remaining in the input stream

	    tools_blocking_read(input, false);
	    while(read(input, buffer, bufsize) >= 0)
		;
	    tools_blocking_read(input, true);

		// now asking the user

            *inter << message << gettext(" [return = YES | Esc = NO]") << (beep ? "\007\007\007" : "") << endl;
	    tools_block_all_signals(old_mask);
	    tmp_ret = read(input, &a, 1);
	    errno_bk = errno;

		// checking if another character is available in the pipe

	    tools_blocking_read(input, false);
	    errno_sup = EAGAIN+1; // = something different from EAGAIN, whatever it is...
	    usleep(10000); // let a change for any other typed character to reach the input device
	    tmp_sup = read(input, &b, 1);
	    errno_sup = errno;
	    tools_blocking_read(input, true);

		// checking error conditions

	    tools_set_back_blocked_signals(old_mask);
            if(tmp_ret < 0)
		if(errno_bk != EINTR)
		    throw Erange("shell_interaction:interaction_pause", string(gettext("Error while reading user answer from terminal: ")) + strerror(errno_bk));
        }
        while((a != 27 && a != '\n') || tmp_sup != -1 || errno_sup != EAGAIN);

        if(a != 27)
            *inter << gettext("Continuing...") << endl;
        else
            *inter << gettext("Escaping...") << endl;

        ret = a != 27; // 27 is escape key
    }
    catch(...)
    {
        set_term_mod(initial);
        throw;
    }
    set_term_mod(initial);

    return ret;
}

static void interaction_warning(const string & message, void *context)
{
    if(output == NULL)
        throw SRC_BUG; // shell_interaction has not been properly initialized
    *output << message;
}

static string interaction_string(const string & message, bool echo, void *context)
{
    string ret;
    const U_I expected_taille = 100;
#ifdef SSIZE_MAX
    const U_I taille = expected_taille > SSIZE_MAX ? SSIZE_MAX : expected_taille;
#else
    const U_I taille = expected_taille;
#endif
    U_I lu, i;
    char buffer[taille+1];
    bool fin = false;

    if(!echo)
	set_term_mod(initial_noecho);

    if(output == NULL || input < 0)
	throw SRC_BUG;  // shell_interaction has not been properly initialized
    *inter << message;
    do
    {
	lu = ::read(input, buffer, taille);
	i = 0;
	while(i < lu && buffer[i] != '\n')
	    ++i;
	if(i < lu)
	    fin = true;
	buffer[i] = '\0';
	ret += string(buffer);
    }
    while(!fin);
    if(!echo)
	*inter << endl;
    set_term_mod(initial);

    return ret;
}

static secu_string interaction_secu_string(const string & message, bool echo, void *context)
{
    const U_I expected_taille = 1000;
#ifdef SSIZE_MAX
    const U_I taille = expected_taille > SSIZE_MAX ? SSIZE_MAX : expected_taille;
#else
    const U_I taille = expected_taille;
#endif
    secu_string ret = taille;
    bool fin = false;
    U_I last = 0, i = 0;

    if(!echo)
	set_term_mod(initial_noecho);

    try
    {
	if(output == NULL || input < 0)
	    throw SRC_BUG;  // shell_interaction has not been properly initialized
	*inter << message;
	do
	{
	    ret.append(input, taille - ret.size());
	    i = last;
	    while(i < ret.size() && ret.c_str()[i] != '\n')
		++i;
	    if(i < ret.size()) // '\n' found so we stop here but remove this last char
	    {
		fin = true;
		ret.reduce_string_size_to(i);
	    }
	    else
		last = i;

	    if(ret.size() == taille && !fin)
		throw Erange("interaction_secu_string", gettext("provided password is too long for the allocated memory"));
	}
	while(!fin);

	if(!echo)
	    *inter << endl;
    }
    catch(...)
    {
	set_term_mod(initial);
	throw;
    }
    set_term_mod(initial);

    return ret;
}

