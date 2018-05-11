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

    // NOTE : The following comments are used by doxygen to generate the documentation of reference

    /// \mainpage
    /// You will find here the reference documentation for the dar and libdar source code, split in several "modules".
    /// - API module: contains all information for using libdar within your program
    /// - Private module: contains all libdar internal documentation, it is not necessary to read it to be able to use libdar
    /// - CMDLINE module: contains the documentation for command-line tools,
    /// you might want to have a look for illustration of library usage.
    /// - API5 module: contains API v5 symbols as found on release 2.5.x and 2.4.x
    /// .
    ///
    /// Please not that an API tutorial is also available and should be a starting point for using libdar


    /// \defgroup API API
    /// \brief APlication Interface
    ///
    /// This gathers all symbols that may be accessed from an external
    /// program. Other symbols are not as much documented, and
    /// may change or be removed without any warning or backward
    /// compatibility support. So only use the function, macro, types,
    /// classes... defined as member of the API module in you external programs.


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

#include "compressor.hpp"
#include "path.hpp"
#include "mask.hpp"
#include "mask_list.hpp"
#include "integers.hpp"
#include "infinint.hpp"
#include "statistics.hpp"
#include "user_interaction.hpp"
#include "user_interaction_callback.hpp"
#include "user_interaction_blind.hpp"
#include "deci.hpp"
#include "archive.hpp"
#include "crypto.hpp"
#include "thread_cancellation.hpp"
#include "compile_time_features.hpp"
#include "capabilities.hpp"
#include "entrepot_libcurl.hpp"
#include "fichier_local.hpp"
#include "memory_check.hpp"
#include "entrepot_local.hpp"
#include "data_tree.hpp"
#include "database.hpp"
#include "get_version.hpp"
#include "shell_interaction.hpp"
#include "libdar_slave.hpp"
#include "libdar_xform.hpp"

#endif
