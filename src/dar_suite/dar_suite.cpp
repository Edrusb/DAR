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
#if HAVE_LOCALE_H
#include <locale.h>
#endif

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#if HAVE_STRING_H
#include <string.h>
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

#include "dar_suite.hpp"
#include "libdar.hpp"
#include "line_tools.hpp"
#include "shell_interaction.hpp"
#if HAVE_LIBTHREADAR_LIBTHREADAR_HPP
#include <libthreadar/libthreadar.hpp>
#endif

using namespace libdar;
using namespace std;

static shared_ptr<user_interaction> ui;
static void signals_abort(int l, bool now);
static void signal_abort_delayed(int l);
static void signal_abort_now(int l);
static void general_report(const std::string & msg);

void dar_suite_reset_signal_handler()
{
#if HAVE_SIGNAL_H
    struct sigaction sigact;

    sigact.sa_handler = &signal_abort_delayed;
    sigact.sa_flags = SA_RESTART;

    sigaction(SIGTERM, &sigact, nullptr);
    sigaction(SIGINT, &sigact, nullptr);
    sigaction(SIGQUIT, &sigact, nullptr);
    sigaction(SIGHUP, &sigact, nullptr);
    sigaction(SIGUSR1, &sigact, nullptr);

    sigact.sa_handler = &signal_abort_now;
    sigaction(SIGUSR2, &sigact, nullptr);

#if GPGME_SUPPORT
	// for GPGME:
    sigact.sa_handler = SIG_IGN;
    sigact.sa_flags = 0;
    sigaction(SIGPIPE, &sigact, nullptr);
#endif
#endif
}

int dar_suite_global(int argc,
		     char * const argv[],
		     const char **env,
		     const char *getopt_string,
#if HAVE_GETOPT_LONG
		     const struct option *long_options,
#endif
		     char stop_scan,
		     cli_callback call)
{
    int ret = EXIT_OK;

    dar_suite_reset_signal_handler();

#ifdef ENABLE_NLS
	// gettext settings
    try
    {
	if(string(DAR_LOCALEDIR) != string(""))
	    if(bindtextdomain(PACKAGE, DAR_LOCALEDIR) == nullptr)
		throw Erange("", "Cannot open the translated messages directory, native language support will not work");
	if(setlocale(LC_MESSAGES, "") == nullptr || setlocale(LC_CTYPE, "") == nullptr)
	    throw Erange("", "Cannot set locale category, native language support will not work");
	if(textdomain(PACKAGE) == nullptr)
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
	line_tools_look_for_Q(argc,
			      argv,
			      getopt_string,
#if HAVE_GETOPT_LONG
			       long_options,
#endif
			      stop_scan,
			      silent);
	ui.reset(new (nothrow) shell_interaction(cerr, cerr, silent));
	if(!ui)
	    throw Ememory("dar_suite_global");

	try
	{
	    get_version(maj, med, min);
	}
	catch(Erange & e)
	{
	    if(e.get_source() == "libdar_init_gpgme")
		ui->pause("INITIALIZATION FAILED FOR GPGME, missing gpg binary? Retry initializing without gpgme support, which may lead libdar to silently fail reading or writing gpg ciphered archive)?");
	    close_and_clean();
	    get_version(maj, med, min, true, false);
	}

	if(maj != LIBDAR_COMPILE_TIME_MAJOR || med < LIBDAR_COMPILE_TIME_MEDIUM)
	{
	    general_report(tools_printf(gettext("We have linked with an incompatible version of libdar. Expecting version %d.%d.x but having linked with version %d.%d.%d"), LIBDAR_COMPILE_TIME_MAJOR, LIBDAR_COMPILE_TIME_MEDIUM, maj, med, min));
	    ret = EXIT_ERROR;
	}
	else
	    ret = (*call)(ui, argc, argv, env);

	    // closing libdar

	close_and_clean();
    }
    catch(Efeature & e)
    {
	general_report(string(gettext("NOT YET IMPLEMENTED FEATURE has been asked: ")) + e.get_message());
	general_report(string(gettext("Please check documentation or upgrade your software if available")));
	ret = EXIT_SYNTAX;
    }
    catch(Ehardware & e)
    {
	general_report(string(gettext("SEEMS TO BE A HARDWARE PROBLEM: "))+e.get_message());
	general_report(string(gettext("Please check your hardware")));
	ret = EXIT_ERROR;
    }
    catch(Esecu_memory & e)
    {
	general_report(string(gettext("Lack of SECURED memory to achieve the operation, aborting operation")));
	ret = EXIT_ERROR;
    }
    catch(Ememory & e)
    {
	general_report(string(gettext("Lack of memory to achieve the operation, aborting operation")));
	ret = EXIT_ERROR;
    }
    catch(std::bad_alloc & e)
    {
        general_report(string(gettext("Lack of memory to achieve the operation, aborting operation")));
        ret = EXIT_ERROR;
    }
    catch(Erange & e)
    {
	general_report(string(gettext("FATAL error, aborting operation: ")) + e.get_message());
	ret = EXIT_ERROR;
    }
    catch(Euser_abort & e)
    {
	general_report(string(gettext("Aborting program. User refused to continue while asking: ")) + e.get_message());
	ret = EXIT_USER_ABORT;
    }
    catch(Ethread_cancel & e)
    {
	general_report(string(gettext("Program has been aborted for the following reason: ")) + e.get_message());
	ret = EXIT_USER_ABORT;
    }
    catch(Edata & e)
    {
	general_report(e.get_message());
	ret = EXIT_DATA_ERROR;
    }
    catch(Escript & e)
    {
	general_report(string(gettext("Aborting program. An error occurred concerning user command execution: ")) + e.get_message());
	ret = EXIT_SCRIPT_ERROR;
    }
    catch(Elibcall & e)
    {
	general_report(string(gettext("Aborting program. An error occurred while calling libdar: ")) + e.get_message());
	ret = EXIT_LIBDAR;
    }
    catch(Einfinint & e)
    {
	general_report(string(gettext("Aborting program. ")) + e.get_message());
	ret = EXIT_BUG;
    }
    catch(Elimitint & e)
    {
	general_report(string(gettext("Aborting program. ")) + e.get_message());
	ret = EXIT_LIMITINT;
    }
    catch(Ecompilation & e)
    {
	general_report(string(gettext("Aborting program. The requested operation needs a feature that has been disabled at compilation time: ")) + e.get_message());
	ret = EXIT_COMPILATION;
    }
    catch(Esystem & e)
    {
	general_report(string(gettext("FATAL error, aborting operation: ")) + e.get_message());
	ret = EXIT_ERROR;
    }
    catch(Enet_auth & e)
    {
	general_report(string(gettext("FATAL error during network communication, aborting operation: ")) + e.get_message());
	ret = EXIT_ERROR;
    }
    catch(Egeneric & e)
    {
	cerr << e.dump_str();
	general_report(string(gettext("INTERNAL ERROR, PLEASE REPORT THE PREVIOUS OUTPUT TO MAINTAINER")));
	ret = EXIT_BUG;
    }
#ifdef LIBTHREADAR_AVAILABLE
    catch(libthreadar::exception_base & e)
    {
	string msg = "";

	for(unsigned int i = 0; i < e.size() ; ++i)
	    msg += e[i];

	cerr << msg << endl;
	ret = EXIT_BUG;
    }
#endif
    catch(...)
    {
	Ebug x = SRC_BUG;
	cerr << x.dump_str();
	general_report(string(gettext("CAUGHT A NON (LIB)DAR EXCEPTION")));
	general_report(string(gettext("INTERNAL ERROR, PLEASE REPORT THE PREVIOUS OUTPUT TO MAINTAINER")));
	ret = EXIT_BUG;
    }

#if MUTEX_WORKS
    if(get_thread_count() != 0)
    {
	general_report(string(gettext("SANITY CHECK: AT LEAST ONE THREAD_CANCELLATION OBJECT HAS NOT BEEN DESTROYED AND REMAINS IN MEMORY WHILE THE PROGRAM REACHED ITS END")));
    }
#endif

    ui.reset();
    return ret;
}

string dar_suite_command_line_features()
{
#if HAVE_GETOPT_LONG
    const char *long_opt = gettext("YES");
#else
    const char *long_opt = gettext("NO");
#endif

    return tools_printf(gettext("Long options support         : %s\n"), long_opt);
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
#if HAVE_SIGNAL_H
    struct sigaction sigact;
    sigact.sa_handler = SIG_DFL;
    sigact.sa_flags = 0;
#endif

    general_report(tools_printf(gettext("Received signal: %s"), strsignal(l)));

#if MUTEX_WORKS
    if(now)
    {
	general_report(string(gettext("Archive fast termination engaged")));
    }
    else
    {
	general_report(string(gettext("Archive delayed termination engaged")));
    }
#if HAVE_SIGNAL_H
    sigaction(l, &sigact, nullptr);
    general_report(string(gettext("Disabling signal handler, the next time this signal is received the program will abort immediately")));
#endif
    cancel_thread(pthread_self(), now);
#else
    general_report(string(gettext("Cannot cleanly abort the operation, thread-safe support is missing, will thus abruptly stop the program, generated archive may be unusable")));
    exit(EXIT_USER_ABORT);
#endif
}

static void general_report(const std::string & msg)
{
    if(ui)
    {
        shell_interaction *ptr = dynamic_cast<shell_interaction *>(ui.get());
        if(ptr != nullptr)
	    ptr->change_non_interactive_output(cerr);
	ui->message(msg);
    }
    else
	cerr << msg << endl;
}
