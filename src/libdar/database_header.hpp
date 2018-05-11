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

    /// \file database_header.hpp
    /// \brief defines the database structure in file
    /// \ingroup Private

#ifndef DATABASE_HEADER_HPP
#define DATABASE_HEADER_HPP

#include "../my_config.h"
#include "generic_file.hpp"
#include "user_interaction.hpp"
#include "compression.hpp"

#include <memory>

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// create the header for a dar_manager database

	/// \param[in] dialog is used for user interaction
	/// \param[in] filename is the file's name to create/overwrite
	/// \param[in] overwrite set to true to allow file overwriting (else generates an error if file exists)
	/// \param[in] algozip compression algorithm used for the database
	/// \return the database header has been read and checked the database can now be read from the returned generic_file pointed by the returned value
	/// then it must be destroyed with the delete operator.
    extern generic_file *database_header_create(const std::shared_ptr<user_interaction> & dialog,
						const std::string & filename,
						bool overwrite,
						compression algozip);

	/// read the header of a dar_manager database

	/// \param[in] dialog for user interaction
	/// \param[in] filename is the filename to read from
	/// \param[out] db_version version of the database
	/// \param[out] algozip compression algorithm used in the database
	/// \return the generic_file where the database header has been put
    extern generic_file *database_header_open(const std::shared_ptr<user_interaction> & dialog,
					      const std::string & filename,
					      unsigned char & db_version,
					      compression & algozip);

    extern const unsigned char database_header_get_supported_version();

	///@}

} // end of namespace

#endif
