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
#if HAVE_LOCALE_H
#include <locale.h>
#endif

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if MUTEX_WORKS
#if HAVE_PTHREAD_H
#include <pthread.h>
#else
#define pthread_t U_I
#endif
#endif

}

#include <iostream>
#include <new>

#include "integers.hpp"
#include "dar_suite.hpp"
#include "shell_interaction.hpp"
#include "erreurs.hpp"
#include "libdar.hpp"
#include "thread_cancellation.hpp"
#include "memory_check.hpp"
#include "special_alloc.hpp"
#include "line_tools.hpp"

#define GENERAL_REPORT(msg) 	if(ui != NULL)\
                                {\
                                    shell_interaction_change_non_interactive_output(&cerr);\
	                            ui->warning(msg);\
				}\
	                        else\
                                    cerr << msg << endl;



using namespace libdar;

static void jogger();

static user_interaction *ui = NULL;
static void signals_abort(int l, bool now);
static void signal_abort_delayed(int l);
static void signal_abort_now(int l);

void dar_suite_reset_signal_handler()
{
#if HAVE_SIGNAL_H
    signal(SIGTERM, &signal_abort_delayed);
    signal(SIGINT, &signal_abort_delayed);
    signal(SIGQUIT, &signal_abort_delayed);
    signal(SIGHUP, &signal_abort_delayed);
    signal(SIGUSR1, &signal_abort_delayed);
    signal(SIGUSR2, &signal_abort_now);
#endif
}

int dar_suite_global(int argc,
		     char * const argv[],
		     const char **env,
		     const char *getopt_string,
#if HAVE_GETOPT_LONG
		     const struct option *long_options,
#endif
		     int (*call)(user_interaction & dialog, int, char * const [], const char **env))
{
    int ret = EXIT_OK;

    memory_check_snapshot();
    dar_suite_reset_signal_handler();

#ifdef ENABLE_NLS
	// gettext settings
    try
    {
	if(string(DAR_LOCALEDIR) != string(""))
	    if(bindtextdomain(PACKAGE, DAR_LOCALEDIR) == NULL)
		throw Erange("", "Cannot open the translated messages directory, native language support will not work");
	if(setlocale(LC_MESSAGES, "") == NULL || setlocale(LC_CTYPE, "") == NULL)
	    throw Erange("", "Cannot set locale category, native language support will not work");
	if(textdomain(PACKAGE) == NULL)
	    throw Erange("", "Cannot find dar's catalogue, native language support will not work");
    }
    catch(Erange & e)
    {
	cerr << e.get_message() << endl;
    }
#endif

    try
    {
	U_I min, med, maj;
	bool silent;
	bool jog;
	line_tools_look_for_jQ(argc,
			      argv,
			      getopt_string,
#if HAVE_GETOPT_LONG
			      long_options,
#endif
			      jog,
			      silent);
	ui = shell_interaction_init(&cerr, &cerr, silent);

	if(jog)
	    std::set_new_handler(&jogger);

	get_version(maj, med, min);
	if(maj != LIBDAR_COMPILE_TIME_MAJOR || med < LIBDAR_COMPILE_TIME_MEDIUM)
	{
	    GENERAL_REPORT(tools_printf(gettext("We have linked with an incompatible version of libdar. Expecting version %d.%d.x but having linked with version %d.%d.%d"), LIBDAR_COMPILE_TIME_MAJOR, LIBDAR_COMPILE_TIME_MEDIUM, maj, med, min));
	    ret = EXIT_ERROR;
	}
	else
	    ret = (*call)(*ui, argc, argv, env);

	// closing libdar

	close_and_clean();
    }
    catch(Efeature &e)
    {
	GENERAL_REPORT(string(gettext("NOT YET IMPLEMENTED FEATURE has been used: ")) + e.get_message());
	GENERAL_REPORT(string(gettext("Please check documentation or upgrade your software if available")));
	ret = EXIT_SYNTAX;
    }
    catch(Ehardware & e)
    {
	GENERAL_REPORT(string(gettext("SEEMS TO BE A HARDWARE PROBLEM: "))+e.get_message());
	GENERAL_REPORT(string(gettext("Please check your hardware")));
	ret = EXIT_ERROR;
    }
    catch(Esecu_memory & e)
    {
	GENERAL_REPORT(string(gettext("Lack of SECURED memory to achieve the operation, aborting operation")));
	ret = EXIT_ERROR;
    }
    catch(Ememory & e)
    {
	GENERAL_REPORT(string(gettext("Lack of memory to achieve the operation, aborting operation")));
	ret = EXIT_ERROR;
    }
    catch(std::bad_alloc & e)
    {
        GENERAL_REPORT(string(gettext("Lack of memory to achieve the operation, aborting operation")));
        ret = EXIT_ERROR;
    }
    catch(Erange & e)
    {
	GENERAL_REPORT(string(gettext("FATAL error, aborting operation")));
	GENERAL_REPORT(e.get_message());
	ret = EXIT_ERROR;
    }
    catch(Euser_abort & e)
    {
	GENERAL_REPORT(string(gettext("Aborting program. User refused to continue while asking: ")) + e.get_message());
	ret = EXIT_USER_ABORT;
    }
    catch(Ethread_cancel & e)
    {
	GENERAL_REPORT(string(gettext("Program has been aborted for the following reason: ")) + e.get_message());
	ret = EXIT_USER_ABORT;
    }
    catch(Edata & e)
    {
	GENERAL_REPORT(e.get_message());
	ret = EXIT_DATA_ERROR;
    }
    catch(Escript & e)
    {
	GENERAL_REPORT(string(gettext("Aborting program. An error occurred concerning user command execution: ")) + e.get_message());
	ret = EXIT_SCRIPT_ERROR;
    }
    catch(Elibcall & e)
    {
	GENERAL_REPORT(string(gettext("Aborting program. An error occurred while calling libdar: ")) + e.get_message());
	ret = EXIT_LIBDAR;
    }
    catch(Einfinint & e)
    {
	GENERAL_REPORT(string(gettext("Aborting program. ")) + e.get_message());
	ret = EXIT_BUG;
    }
    catch(Elimitint & e)
    {
	GENERAL_REPORT(string(gettext("Aborting program. ")) + e.get_message());
	ret = EXIT_LIMITINT;
    }
    catch(Ecompilation & e)
    {
	GENERAL_REPORT(string(gettext("Aborting program. The requested operation needs a feature that has been disabled at compilation time: ")) + e.get_message());
	ret = EXIT_COMPILATION;
    }
    catch(Egeneric & e)
    {
	cerr << e.dump_str();
	GENERAL_REPORT(string(gettext("INTERNAL ERROR, PLEASE REPORT THE PREVIOUS OUTPUT TO MAINTAINER")));
	ret = EXIT_BUG;
    }
    catch(...)
    {
	Ebug x = SRC_BUG;
	cerr << x.dump_str();
	GENERAL_REPORT(string(gettext("CAUGHT A NON (LIB)DAR EXCEPTION")));
	GENERAL_REPORT(string(gettext("INTERNAL ERROR, PLEASE REPORT THE PREVIOUS OUTPUT TO MAINTAINER")));
	ret = EXIT_BUG;
    }


    if(thread_cancellation::count() != 0)
    {
	GENERAL_REPORT(string(gettext("SANITY CHECK: AT LEAST ONE THREAD_CANCELLATION OBJECT HAS NOT BEEN DESTROYED AND REMAINS IN MEMORY WHILE THE PROGRAM REACHED ITS END")));
    }

	// restoring terminal settings
    try
    {
	shell_interaction_close();
	if(ui != NULL)
	    delete ui;
    }
    catch(...)
    {
	ret = EXIT_UNKNOWN_ERROR;
    }
#ifdef LIBDAR_SPECIAL_ALLOC
    special_alloc_garbage_collect(cerr);
#endif
    memory_check_snapshot();
    return ret;
}

static void jogger()
{
    if(ui == NULL)
	throw Ememory("jogger");
    ui->pause(gettext("No more (virtual) memory available, you have the opportunity to stop un-necessary applications to free up some memory. Can we continue now ?"));
}

string dar_suite_command_line_features()
{
#if HAVE_GETOPT_LONG
    const char *long_opt = gettext("YES");
#else
    const char *long_opt = gettext("NO");
#endif

    return tools_printf(gettext("Long options support       : %s\n"), long_opt);
}

static void signal_abort_delayed(int l)
{
    signals_abort(l, false);
}

static void signal_abort_now(int l)
{
    signals_abort(l, true);
}

static void signals_abort(int l, bool now)
{
#if HAVE_DECL_SYS_SIGLIST
    GENERAL_REPORT(tools_printf(gettext("Received signal: %s"), sys_siglist[l]));
#else
    GENERAL_REPORT(tools_printf(gettext("Received signal: %d"), l));
#endif

#if MUTEX_WORKS
    if(now)
    {
	GENERAL_REPORT(string(gettext("Archive fast termination engaged")));
    }
    else
    {
	GENERAL_REPORT(string(gettext("Archive delayed termination engaged")));
    }
#if HAVE_SIGNAL_H
    signal(l, SIG_DFL);
    GENERAL_REPORT(string(gettext("Disabling signal handler, the next time this signal is received the program will abort immediately")));
#endif
    cancel_thread(pthread_self(), now);
#else
    GENERAL_REPORT(string(gettext("Cannot cleanly abort the operation, thread-safe support is missing, will thus abruptly stop the program, generated archive may be unusable")));
    exit(EXIT_USER_ABORT);
#endif
}
