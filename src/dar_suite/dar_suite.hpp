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

    /// \file dar_suite.hpp
    /// \brief contains routine to manage CLI's common initialization and ultimate exception catching
    /// \ingroup CMDLINE

#ifndef DAR_SUITE_HPP
#define DAR_SUITE_HPP

#include "../my_config.h"
#include "shell_interaction.hpp"

#define EXIT_OK 0           // all that was asked is done
#define EXIT_SYNTAX 1       // syntax error on command line
#define EXIT_ERROR 2        // error not related to the data treated
    // (lack of memory, hardware problem, etc.)
#define EXIT_BUG 3          // detected a condition that should never happen
#define EXIT_USER_ABORT 4   // user asked to abort (or question in non
    // interactive mode)
#define EXIT_DATA_ERROR 5   // error in data treated (could not save/restore/
    // compare all data due for example to bad access permission.  Comparison
    // mismatch of some files, archive testing failed etc...)
#define EXIT_SCRIPT_ERROR 6 // error around the execution of a user command
    // using -E or -F options
#define EXIT_LIBDAR  7     // error calling libdar. Arguments given to libdar
    // do not match those expected (sanity checks warning).
#define EXIT_LIMITINT 8    // limitinit overflow
    // fixed using full infinint version of the program
#define EXIT_UNKNOWN_ERROR 9
    // error not possible to report by other mean no access to stdout/stderr)
#define EXIT_COMPILATION 10 // feature not activated at compilation time
#define EXIT_SAVED_MODIFIED 11 // some files have been modified at the time they were saved

#define EXTENSION "dar"

/// the compiler version MACRO
#ifndef __VERSION__
#define __VERSION__ "unknown"
#endif

/// the compiler Nature MACRO
#ifdef __GNUC__
#define CC_NAT "GNUC"
#else
#define CC_NAT "unknown"
#endif

using namespace libdar;

    /// \addtogroup CMDLINE
    /// @{

extern void dar_suite_reset_signal_handler();

    /// common routine for all dar command-line tools to initialize environment and convert uncaught exceptions to exit status code
    /// \param[in] argc is the number of argument on the command line
    /// \param[in] argv is the list of arguments on the command line
    /// \param[in] env is the environment variables table obtained from main()
    /// \param[in] getopt_string is the parsing string to pass to getopt
#if HAVE_GETOPT_LONG
    /// \param[in] long_options is the optional list of long options (an nullptr pointer is acceptable for no long option)
#endif
    /// \param[in] stop_scan while looking early for -j and -Q option will ignore all that follows stop_scan option if met
    /// \param[in] call is a callback function to run once user interaction is initialized and to catch from the exceptions
    /// \return the application exist status to use
extern int dar_suite_global(int argc,
			    char * const argv[],
			    const char **env,
			    const char *getopt_string,
#if HAVE_GETOPT_LONG
			    const struct option *long_options,
#endif
			    char stop_scan,
			    int (*call)(shell_interaction & dialog, int, char *const [], const char **env));

extern std::string dar_suite_command_line_features();

    /// @}

#endif
