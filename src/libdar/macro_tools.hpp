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

    /// \file macro_tools.hpp
    /// \brief macroscopic tools for libdar internals
    /// \ingroup Private

#ifndef MACRO_TOOLS_HPP
#define MACRO_TOOLS_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_LIMITS_H
#include <limits.h>
#endif
}
#include <string>

#include "catalogue.hpp"
#include "compressor.hpp"
#include "infinint.hpp"
#include "header_version.hpp"
#include "generic_file.hpp"
#include "scrambler.hpp"
#include "crypto.hpp"
#include "escape.hpp"
#include "pile.hpp"

    /// \addtogroup Private
    /// @{


#define BUFFER_SIZE 102400
#ifdef SSIZE_MAX
#if SSIZE_MAX < BUFFER_SIZE
#undef BUFFER_SIZE
#define BUFFER_SIZE SSIZE_MAX
#endif
#endif

namespace libdar
{

    extern const archive_version macro_tools_supported_version;
    extern const std::string LIBDAR_STACK_LABEL_UNCOMPRESSED;
    extern const std::string LIBDAR_STACK_LABEL_CLEAR;
    extern const std::string LIBDAR_STACK_LABEL_UNCYPHERED;
    extern const std::string LIBDAR_STACK_LABEL_LEVEL1;

    extern void macro_tools_open_archive(user_interaction & dialog,
					 const path &sauv_path,  // path to slices
                                         const std::string &basename,  // slice basename
					 const infinint & min_digits,  // minimum digits for the slice number
                                         const std::string &extension,  // slice extensions
					 crypto_algo crypto, // encryption algorithm
                                         const secu_string &pass, // pass key for crypto/scrambling
					 U_32 crypto_size,    // crypto block size
					 pile & stack, // the stack of generic_file resulting of the archive openning
                                         header_version &ver, // header read from raw data
                                         const std::string &input_pipe, // named pipe for input when basename is "-" (dar_slave)
                                         const std::string &output_pipe, // named pipe for output when basename is "-" (dar_slave)
                                         const std::string & execute, // command to execute between slices
					 infinint & second_terminateur_offset, // where to start looking for the second terminateur (set to zero if there is only one terminateur).
					 bool lax,  // whether we skip&warn the usual verifications
					 bool sequential_read, // whether to use the escape sequence (if present) to get archive contents and proceed to sequential reading
					 bool info_details); // be or not verbose about the archive openning
        // all allocated objects (ret1, ret2, scram), must be deleted when no more needed by the caller of this routine

    extern catalogue *macro_tools_get_derivated_catalogue_from(user_interaction & dialog,
							       pile & data_stack,  // where to get the files and EA from
							       pile & cata_stack,  // where to get the catalogue from
							       const header_version & ver, // version format as defined in the header of the archive to read
							       bool info_details, // verbose display (throught user_interaction)
							       infinint &cat_size, // return size of archive in file (not in memory !)
							       const infinint & second_terminateur_offset, // location of the second terminateur (zero if none exist)
							        bool lax_mode);         // whether to do relaxed checkings

    extern catalogue *macro_tools_get_catalogue_from(user_interaction & dialog,
						     pile & stack,  // raw data access object
						     const header_version & ver, // version format as defined in the header of the archive to read
                                                     bool info_details, // verbose display (throught user_interaction)
                                                     infinint &cat_size, // return size of archive in file (not in memory !)
						     const infinint & second_terminateur_offset,
						     bool lax_mode);

    extern catalogue *macro_tools_lax_search_catalogue(user_interaction & dialog,
						       pile & stack,
						       const archive_version & edition,
						       compression compr_algo,
						       bool info_details,
						       bool even_partial_catalogues,
						       const label & layer1_data_name);

	// return the offset of the beginning of the catalogue.
    extern infinint macro_tools_get_terminator_start(generic_file & f, const archive_version & reading_ver);

} // end of namespace

	/// @}

#endif
