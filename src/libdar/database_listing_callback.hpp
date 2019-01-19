/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

    /// \file database_listing_callback.hpp
    /// \brief definition of the user defined callback function used for database listing
    /// \ingroup API

#ifndef DATABASE_LISTING_CALLBACK_HPP
#define DATABASE_LISTING_CALLBACK_HPP

#include "../my_config.h"

#include <string>
#include "database_aux.hpp"
#include "archive_num.hpp"
#include "datetime.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

	/// called by the database::get_files() routine

	/// \param[in] tag is passed as is from the callback caller (database::get_files)
	/// \param[in] filename the name of the new entry
	/// \param[in] available_data whether this entry has data saved in the requested archive
	/// \param[in] available_ea whether this entry has ea saved in the requested archive
    using database_listing_show_files_callback = void (*) (void *context,
							   const std::string & filename,
							   bool available_data,
							   bool available_ea);

	/// called with the information of presence for an entry in archive number num

	/// \param[in] tag is passed as is from the callback caller (database::get_version)
	/// \param[in] num number of the archive the information is taken from
	/// \param[in] data_presence status of the data for the requested file in that archive "num"
	/// \param[in] has_data_date when false the following argument (data) is meaningless
	/// \param[in] data modification date of the archive of the requested file in the archive "num"
	/// \param[in] ea_presence status of the EA for the requested file in the archive "num"
	/// \param[in] has_ea_date when false the following argument (ea) is meaningless
	/// \param[in] ea change date of the EA for the requested file in the archive "num"
    using database_listing_get_version_callback = void (*) (void *context,
							    archive_num num,
							    db_etat data_presence,
							    bool has_data_date,
							    datetime data,
							    db_etat ea_presence,
							    bool has_ea_date,
							    datetime ea);


	/// called with teh information of statistics for each archive in turn

	/// \param[in] context is passed as is from the callback caller (database::show_most_recent_stats)
	/// \param[in] number the archive number in the database
	/// \param[in] data_count amount of file which last version is located in this archive
	/// \param[in] total_data total number of file covered in this database
	/// \param[in] ea_count amount of EA which last version is located in this archive
	/// \param[in] total_ea total number of file that have EA covered by this database
    using database_listing_statistics_callback = void (*) (void *context,
							   U_I number,
							   const infinint & data_count,
							   const infinint & total_data,
							   const infinint & ea_count,
							   const infinint & total_ea);

	/// @}

} // end of namespace

#endif
