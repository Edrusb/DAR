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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: shell_interaction.cpp,v 1.9.4.2 2003/12/26 14:09:50 edrusb Rel $
//
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
} // end extern "C"

#include "integers.hpp"
#include "shell_interaction.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "integers.hpp"
#include "user_interaction.hpp"
#include "cygwin_adapt.hpp"

using namespace std;
using namespace libdar;

static S_I input = -1;
static ostream *output = NULL;
static ostream *inter = NULL;
static bool beep = false;
static termios initial;
static termios interaction;
static bool has_terminal = false;

static void set_term_mod(const struct termios & etat);
static bool interaction_pause(const string &message);
static void interaction_warning(const string & message);

void shell_interaction_init(ostream *out, ostream *interact)
{
    has_terminal = false;

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

    char *tty = ctermid(NULL);
    if(tty == NULL)
        interaction_warning("No terminal found for user interaction. All questions will abort the program.\n");
    else // no filesystem path to tty
    {
        struct termios term;

        input = open(tty, O_RDONLY|O_TEXT);
        if(input < 0)
            interaction_warning("No terminal found for user interaction. All questions will abort the program.\n");
        else
        {
                // preparing input for swaping between char mode and line mode (terminal settings)
            if(tcgetattr(input, &term) >= 0)
            {
                initial = term;
                term.c_lflag &= ~ICANON;
                term.c_lflag &= ~ECHO;
                term.c_lflag &= ~ECHOE;
                interaction = term;

                    // checking now that we can change to character mode
                set_term_mod(interaction);
                set_term_mod(initial);
                    // but we don't need it right now, so swapping back to line mode
                has_terminal = true;
            }
            else // failed to retrieve parameters from tty
                interaction_warning("No terminal found for user interaction. All questions will abort the program.\n");
        }
    }

        // setting darlib callback :

    set_warning_callback(&interaction_warning);
    set_answer_callback(&interaction_pause);
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

void shell_interaction_close()
{
    if(has_terminal)
        tcsetattr(input, TCSADRAIN, &initial);
}

static void set_term_mod(const struct termios & etat)
{
    if(tcsetattr(input, TCSANOW, &etat) < 0)
        throw Erange("user_interaction : set_term_mod", string("Error while changing user terminal properties: ") + strerror(errno));
}

static bool interaction_pause(const string &message)
{
    const S_I bufsize = 1024;
    char buffer[bufsize];
    char & a = buffer[0];
    bool ret;

    if(!has_terminal)
        return false;

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
                throw Erange("dar_suite_user_interaction_pause", string("Error while reading user answer from terminal: ") + strerror(errno));
        }
        while(a != 27 && a != '\n');

        if(a != 27)
            *inter << "Continuing..." << endl;
        else
            *inter << "Escaping..." << endl;

        ret = a != 27; // 27 is escape key
    }
    catch(...)
    {
        set_term_mod(initial);
        throw;
    }

    return ret;
}

static void interaction_warning(const string & message)
{
    if(output == NULL)
        throw SRC_BUG; // user_interaction has not been properly initialized
    *output << message;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: shell_interaction.cpp,v 1.9.4.2 2003/12/26 14:09:50 edrusb Rel $";
    dummy_call(id);
}
