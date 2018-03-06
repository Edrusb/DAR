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

    /// \file database5.hpp
    /// \brief this file holds the database class definition as defined in API version 5
    /// \ingroup API


#ifndef DATABASE5_HPP
#define DATABASE5_HPP

#include "../my_config.h"

#include <list>

#include "archive.hpp"
#include "generic_file.hpp"
#include "data_tree.hpp"
#include "storage.hpp"
#include "database_options.hpp"
#include "database.hpp"
#include "user_interaction5.hpp"
#include "data_tree.hpp"
#include "path.hpp"
#include "database_listing_callback.hpp"
#include "database_aux.hpp"
#include "datetime.hpp"
#include "user_interaction_blind.hpp"


    // from path.hpp
using libdar::path;

    // from database_options.hpp
using libdar::database_open_options;
using libdar::database_dump_options;
using libdar::database_add_options;
using libdar::database_remove_options;
using libdar::database_change_basename_options;
using libdar::database_change_path_options;
using libdar::database_restore_options;
using libdar::database_used_options;

    //  from database_aux.hpp
using libdar::archive_num;
using libdar::db_lookup;
using libdar::db_etat;

    // from datetime.hpp
using libdar::datetime;

namespace libdar5
{
	/// the database class defines the dar_manager database

	/// all operations for a dar_manager database are defines through the
	/// use of this class interface. This class also defines internally
	/// the data structure of the database.
	/// \ingroup API
    class database: public libdar::database
    {
    public:
	database(): libdar::database(std::shared_ptr<libdar::user_interaction>(new libdar::user_interaction_blind())) {};

	database(user_interaction & dialog,
		 const std::string & base,
		 const database_open_options & opt):
	    libdar::database(user_interaction5_clone_to_shared_ptr(dialog),
			     base,
			     opt)
	{}

	    /// disabling copy constructor
	database(const database & ref) = delete;

	    /// disabling move constructor
	database(database && ref) noexcept = delete;

	    /// disabling assignement operator
	database & operator = (const database & ref) = delete;

	    /// disabling move assignment operator
	database & operator = (database && ref) noexcept = delete;

	    /// database destructor (no implicit file saving)
	~database() = default;


	void dump(user_interaction & dialog, const std::string & filename, const database_dump_options & opt) const
	{
	    libdar::database::dump(filename,
				   opt);
	}



	    // "GETTINGS"

	    /// show the list of archive used to build the database

	    /// \param[in,out] dialog is the user_interaction to use to report the listing
	void show_contents(user_interaction & dialog) const; // displays all archive information

	    /// list files which are present in a given archive

	    /// \param[in,out] dialog where to display listing to
	    /// \param[in] num is the archive number to look at
	    /// \param[in] opt optional parameters for this operation
	    /// \note if "num" is set to zero all archive contents is listed
	    /// \note this method is not available with partially extracted databases.
	void show_files(user_interaction & dialog, archive_num num, const database_used_options & opt) const;

	    /// list the archive where a give file is present

	    /// \param[in,out] dialog where to display the listing to
	    /// \param[in] chemin path to the file to look for
	    /// \note this method is not available with partially extracted databases.
	void show_version(user_interaction & dialog, path chemin) const;

	    /// compute some statistics about the location of most recent file versions

	    /// \param[in] dialog where to display the listing to
	    /// \note this method is not available with partially extracted databases.
	void show_most_recent_stats(user_interaction & dialog) const;

	void restore(user_interaction & dialog,
		     const std::vector<std::string> & filename,
		     const database_restore_options & opt)
	{
	    libdar::database::restore(filename,
				      opt);
	}

	bool check_order(user_interaction & dialog) const
	{
	    return libdar::database::check_order();
	}

    private:

	static void show_files_callback(void *tag,
					const std::string & filename,
					bool available_data,
					bool available_ea);


	static void get_version_callback(void *tag,
					 archive_num num,
					 db_etat data_presence,
					 bool has_data_date,
					 datetime data,
					 db_etat ea_presence,
					 bool has_ea_date,
					 datetime ea);

	static void statistics_callback(void *tag,
					U_I number,
					const infinint & data_count,
					const infinint & total_data,
					const infinint & ea_count,
					const infinint & total_ea);
    };

} // end of namespace

#endif
