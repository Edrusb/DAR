/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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

    // NOTE : The following comments are used by doxygen to generate the documentation of reference

    /// \mainpage
    /// You will find here the reference documentation for the dar and libdar source code, split in several "modules".
    /// - API module: contains all information for using libdar within your program
    /// - Private module: contains all libdar internal documentation, you do not have to read it in order use libdar
    /// - CMDLINE module: contains the documentation for command-line tools,
    ///   you might want to have a look for illustration of libdar usage.
    /// .
    ///

    /// \defgroup API API
    /// \brief APlication Interface
    ///
    /// This namespace gathers all symbols that may be accessed from an external
    /// program. Other symbols are not as much documented, and
    /// may change or be removed without any warning or backward
    /// compatibility support. So only use the function, macro, types,
    /// classes... defined as member of the API module in you external programs.
    ///
    /// Please not that an API tutorial is also available and should be a starting point for using libdar,
    /// nevertheless, if you want to start from here, there is four main classes consider and several
    /// datastructures aside that they depend on:
    /// - class libdar::archive you can create and manipulate dar archive like the dar command-line tool does
    /// - class libdar::database you can create and manupulate dar_manager database, like the dar_manager CLI tool does
    /// - class libdar::libdar_xform you have guessed, this is the API for dar_xform
    /// - class libdar::libdar_slave still logicial, this is the API for dar_slave
    /// all the CLI command mentionned above do rely on these classes so it might help having a look at the
    /// CMDLINE namespace for illustration on how to use this classes.

    /// \defgroup Private Private
    /// \brief Libdar internal symbols
    ///
    /// Understanding this is not necessary to use libdar through the API.
    /// This is libdar internal code documentation

    /// \file libdar.hpp
    /// \brief the main file of the libdar API definitions
    /// \ingroup API


#ifndef LIBDAR_HPP
#define LIBDAR_HPP

#include "../my_config.h"

    // the mandatory libdar initialization routine
#include "get_version.hpp"

    // archive class abstraction, this is a good starting point to create and read dar archives
#include "archive.hpp"

    // dar_manager API
#include "database.hpp"

    //  dar_xform API
#include "libdar_xform.hpp"

    // dar_slave API
#include "libdar_slave.hpp"

    // common set of exception used within libdar
#include "erreurs.hpp"

    // if you want to know which feature has been activated at compilation time
#include "compile_time_features.hpp"

    // for remote reposity you will need to create such object and pass it where needed
#include "entrepot_libcurl.hpp"

    // for local filesystem, you should not need to create such object to call libdar as it is the default repo used
#include "entrepot_local.hpp"

    // the options class to give non default parameter to the archive class
#include "archive_options_listing_shell.hpp"

    // if you want to bind user input/output to shell command line
#include "shell_interaction.hpp"

    // if you want to bind user intput/output to your own provided callback functions
#include "user_interaction_callback.hpp"

    // this is a trivial way to ignore user input/output
#include "user_interaction_blind.hpp"

    // to redirect to a shell_interaction user I/O object any type of user interaction
#include "shell_interaction_emulator.hpp"

    // for even more flexibility you can create your own class inherited from class user_interaction
    // (see user_interaction.hpp include file)


#endif
