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

    /// \file macro_tools.hpp
    /// \brief macroscopic tools for libdar internals
    /// \ingroup Private

#ifndef MACRO_TOOLS_HPP
#define MACRO_TOOLS_HPP

#include "../my_config.h"
#include <string>
#include "catalogue.hpp"
#include "compressor.hpp"
#include "infinint.hpp"
#include "header_version.hpp"
#include "generic_file.hpp"
#include "scrambler.hpp"
#include "crypto.hpp"

namespace libdar
{

    extern const dar_version macro_tools_supported_version;

    extern void macro_tools_open_archive(user_interaction & dialog,
					 const path &sauv_path,  // path to slices
                                         const std::string &basename,  // slice basename
                                         const std::string &extension,  // slice extensions
					 crypto_algo crypto, // encryption algorithm
                                         const std::string &pass, // pass key for crypto/scrambling
					 U_32 crypto_size,    // crypto block size
                                         generic_file *&ret1, // level 1 file (raw data) sar or zapette
                                         generic_file *&scram, // NULL if pass is given an empty string else a scrambler/other crypto class (over raw data)
                                         compressor *&ret2, // compressor over scrambler or raw data (if no scrambler)
                                         header_version &ver, // header read from raw data
                                         const std::string &input_pipe, // named pipe for input when basename is "-" (dar_slave)
                                         const std::string &output_pipe, // named pipe for output when basename is "-" (dar_slave)
                                         const std::string & execute); // command to execute between slices
        // all allocated objects (ret1, ret2, scram), must be deleted when no more needed

    extern catalogue *macro_tools_get_catalogue_from(user_interaction & dialog,
						     generic_file & f,  // raw data access object
						     const header_version & ver, // version format as defined in the header of the archive to read
                                                     compressor & zip,  // compressor object over raw data
                                                     bool info_details, // verbose display (throught user_interaction)
                                                     infinint &cat_size, // return size of archive in file (not in memory !)
						     generic_file *zip_base);

    extern catalogue *macro_tools_get_catalogue_from(user_interaction & dialog,
						     const std::string &basename,
						     const std::string & extension,
						     crypto_algo crypto,
						     const std::string & pass,
						     U_32 crypto_size);


} // end of namespace

#endif
