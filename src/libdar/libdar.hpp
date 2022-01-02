/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
/*********************************************************************/

    // NOTE : The following comments are used by doxygen to generate the documentation of reference

    /// \mainpage
    /// You will find here the reference documentation for the dar and libdar source code, split in several "modules".
    /// - API module: contains all information for using libdar within your program
    /// - Private module: contains all libdar internal documentation, you do not have to read it in order use libdar
    /// - CMDLINE module: contains the documentation for command-line tools,
    /// you might want to have a look for illustration of libdar usage.
    /// - API5 module: contains API v5 symbols as found on release 2.4.x and 2.5.x
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

    /// \defgroup API5 API5
    /// \brief APplication Interface backward compatibility for API version 5
    ///
    /// backward compatible namespace with dar/libdar releases 2.5.x and 2.4.x

    /// \file libdar.hpp
    /// \brief the main file of the libdar API definitions
    /// \ingroup API


#ifndef LIBDAR_HPP
#define LIBDAR_HPP

#include "../my_config.h"

#include "archive.hpp"
#include "database.hpp"
#include "libdar_xform.hpp"
#include "libdar_slave.hpp"
#include "erreurs.hpp"
#include "compile_time_features.hpp"
#include "entrepot_libcurl.hpp"
#include "get_version.hpp"
#include "archive_options_listing_shell.hpp"
#include "shell_interaction.hpp"
#include "user_interaction_callback.hpp"
#include "user_interaction_blind.hpp"
#include "shell_interaction_emulator.hpp"

#endif
