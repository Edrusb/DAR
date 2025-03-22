/*********************************************************************/
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

extern "C"
{
#if STDC_HEADERS
#include <stdlib.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <signal.h>
#if HAVE_SIGNAL_H
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
} // end extern "C"

#include <iostream>
#include <memory>

#include "tuyau.hpp"
#include "user_interaction.hpp"
#include "dar_suite.hpp"
#include "shell_interaction.hpp"
#include "macro_tools.hpp"

using namespace libdar;
using namespace std;

static const unsigned int buffer_size = 10000;
static bool xmit = true;

static int little_main(shared_ptr<user_interaction> & dialog, int argc, char * const argv[], const char **env);
static void action_xmit(user_interaction & dialog, tuyau *in, tuyau *out, U_32 duration);
static void action_loop(tuyau *in, tuyau *out);
static void stop_xmit(int l);

int main(int argc, char * const argv[])
{
    return dar_suite_global(argc,
			    argv,
			    nullptr,
			    "",
			    nullptr,
			    '\0',
			    &little_main);
}

static int little_main(shared_ptr<user_interaction> & dialog, int argc, char * const argv[], const char **env)
{
    tuyau *in = nullptr, *out = nullptr;
    U_32 duration;

    shell_interaction *shelli = dynamic_cast<shell_interaction *>(dialog.get());
    if(shelli == nullptr)
	throw SRC_BUG;

    shelli->change_non_interactive_output(cout);
    if(argc != 4)
    {
        dialog->printf("usage : %s <input> <output> <seconds>\n", argv[0]);
        dialog->printf("usage : %s <input> <output> loop\n", argv[0]);
        return 0;
    }

    macro_tools_open_pipes(dialog, argv[1], argv[2], in, out);
    if(strcmp(argv[3],"loop") == 0)
        action_loop(in, out);
    else
    {
        duration = atol(argv[3]);
        action_xmit(*dialog, in, out, duration);
    }
    return 0;
}

static void action_xmit(user_interaction & dialog, tuyau *in, tuyau *out, U_32 duration)
{
    char out_buffer[buffer_size];
    char in_buffer[buffer_size];
    unsigned int lu;
    bool xmit_error = false;

    signal(SIGALRM, &stop_xmit);
    alarm(duration);
    srand((unsigned int)getpid());

    while(xmit)
    {
            // generate data to send;
        for(unsigned int i = 0; i < buffer_size; i++)
            out_buffer[i] = rand() % 256;

            // sending data
        out->write(out_buffer, buffer_size);

            // reading it through pipes
        lu = 0;
        while(lu < buffer_size)
            lu += in->read(in_buffer+lu, buffer_size-lu);

            // compairing received data with sent one

        lu = 0;
        for(unsigned int i = 0; i < buffer_size; i++)
            if(out_buffer[i] != in_buffer[i])
                lu++;
        if(lu > 0)
        {
            dialog.printf("ERROR: on %d bytes transfered %d byte(s) had error\n", buffer_size, lu);
            xmit_error = true;
        }
    }

    if(xmit_error)
        dialog.printf("TEST FAILED: some transmission error occured\n");
    else
        dialog.printf("TEST PASSED SUCCESSFULLY\n");

    dialog.printf("you can stop the loop instance with Control-C\n");
}

static void stop_xmit(int l)
{
    xmit = false;
}

static void action_loop(tuyau *in, tuyau *out)
{
    char buffer[buffer_size];
    U_32 lu;

    while(1)
    {
        lu = 0;
        while(lu < buffer_size)
            lu += in->read(buffer+lu, buffer_size-lu);

        out->write(buffer, buffer_size);
    }
}
