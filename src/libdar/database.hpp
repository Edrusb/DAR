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

    /// \file database.hpp
    /// \brief this file holds the database class definition


#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "../my_config.h"

#include <list>

#include "archive.hpp"
#include "generic_file.hpp"
#include "data_tree.hpp"
#include "storage.hpp"

namespace libdar
{
	/// the database class defines the dar_manager database

	/// all operations for a dar_manager database are defines through the
	/// use of this class interface. This class also defines internally
	/// the data structure of the database.
	/// \ingroup API
    class database
    {
    public:
	    /// this constructor build an empty database
	database();

	    /// this constructor reads database from a file

	    /// \param[in] dialog for user interaction
	    /// \param[in] base database filename
	    /// \param[in] partial set to true to only load an manipulate database header
	    /// \note not all methods are available if the database is partially built from file (see bellow)
 	database(user_interaction & dialog, const std::string & base, bool partial);

	    /// database destructor (no implicit file saving)
	~database();

	    /// write the database to a file (see database_header first)

	    /// \param[in] dialog for user interaction
	    /// \param[in] filename name of file to save database to
	    /// \param[in] overwrite whether we can overwrite the file if it already exists
	void dump(user_interaction & dialog, const std::string & filename, bool overwrite) const;

	    // SETTINGS

	    /// add an archive to the database

	    /// \param[in] arch is the archive to add to the database (may be a partial archive)
	    /// \param[in] chemin is the path to this archive to record in the database
	    /// \param[in] basename is the archive's basename to record in the database
	    /// \note this method is not available with partially extracted databases.
	void add_archive(const archive & arch, const std::string & chemin, const std::string & basename);

	    /// remove an archive from a database

	    /// \param[in] min first archive index to remove
	    /// \param[in] max last archive index to remove
	    /// \note the archives which indexes are in the range [min-max] are
	    /// removed. If you want to remove only one archive choose min equal to max.
	    /// \note this method is not available with partially extracted databases.
	void remove_archive(archive_num min, archive_num max);

	    /// change order of archive within the database

	    /// \param[in] src archive index to move
	    /// \param[in] dst archive index to move to
	    /// \note this method is not available with partially extracted databases.
	void set_permutation(archive_num src, archive_num dst);

	    /// change one's archive basename recorded in the database

	    /// \param[in] num is the archive index to rename
	    /// \param[in] basename is the new basename to give to that archive
	void change_name(archive_num num, const std::string & basename);

	    /// change one's archive path recorded in the database

	    /// \param[in] num is the archive index who's path to change
	    /// \param[in] chemin is the new path to give to that archive
	void set_path(archive_num num, const std::string & chemin);

	    /// change the default options given to dar when performing restoration

	    /// \param[in] opt is a vector a arguments.
	    /// \note Each element of the vector must match a single argument of the command line
	    /// like for example "-R". Any leading or trailing space will make a different argument
	    /// than the one without spaces (" -R" is different than "-R" for example).
    	void set_options(const std::vector<std::string> &opt) { options_to_dar = opt; };

	    /// change the path to dar command

	    /// \param[in] chemin is the full path to dar (including dar filename) to use for restoration
	    /// \note if set to an empty string the dar command found from the PATH will be used (if any)
	void set_dar_path(const std::string & chemin) { dar_path = chemin; };


	    // "GETTINGS"

	    /// show the list of archive used to build the database

	    /// \param[in,out] dialog is the user_interaction to use to report the listing
	    ///
	void show_contents(user_interaction & dialog) const; // displays all archive information

	    /// return the options used with dar for restoration
	std::vector<std::string> get_options() const { return options_to_dar; }; // show option passed to dar

	    /// return the path for dar

	    /// \return the path to dar used when restoring files
	    /// \note empty string means that dar is taken from the PATH variable
	std::string get_dar_path() const { return dar_path; }; // show path to dar command

	    /// list files which are present in a given archive

	    /// \param[in,out] dialog where to display listing to
	    /// \param[in] num is the archive number to look at
	    /// \note if "num" is set to zero all archive contents is listed
	    /// \note this method is not available with partially extracted databases.
	void show_files(user_interaction & dialog, archive_num num) const;

	    /// list the archive where a give file is present

	    /// \param[in,out] dialog where to display the listing to
	    /// \param[in] chemin path to the file to look for
	    /// \note this method is not available with partially extracted databases.
	void show_version(user_interaction & dialog, path chemin) const;

	    /// compute some statistics about the location of most recent file versions

	    /// \param[in] dialog where to display the listing to
	    /// \note this method is not available with partially extracted databases.
	void show_most_recent_stats(user_interaction & dialog) const;

	    // "ACTIONS" (not available with partially extracted databases)

	    /// restore files calling dar on the appropriated archive

	    /// \param[in,out] dialog where to have user interaction
	    /// \param[in] filename list of filename to restore
	    /// \param[in] early_release if set to true release memory before calling dar, consequences is that many calls to the database are no more possible.
	    /// \param[in] extra_options_for_dar list of options to pass to dar
	    /// \param[in] date passed this date files are ignored. So you can restore files in the most recent state before a certain "date".
	    /// \note if "date" is set to zero, the most recent state available is looked for.
	    /// \note this method is not available with partially extracted databases.
	    /// \note if early_release is true, free almost all memory allocated by the database before calling dar.
	    /// drawback is that no more action is possible after this call (except destruction)
	    /// if date is zero the most recent version is looked for, else the last version before (including) the given date is looked for
	void restore(user_interaction & dialog,
		     const std::vector<std::string> & filename,
		     bool early_release,
		     const std::vector<std::string> & extra_options_for_dar,
		     const infinint & date = 0);

    private:

	    /// holds the archive used to create the database
	struct archive_data
	{
	    std::string chemin;      //< path to the archive
	    std::string basename;    //< basename of the archive
	};

	std::vector<struct archive_data> coordinate; //< list of archive used to build the database
	std::vector<std::string> options_to_dar;     //< options to use when calling dar for restoration
	std::string dar_path;                        //< path to dar
	data_dir *files;                             //< structure containing files and they status in the set of archive used for that database
	storage *data_files;                         //< when reading archive in partial mode, this is where is located the "not readed" part of the archive

	void build(generic_file & f, bool partial);  //< used by constructors
    };

} // end of namespace

#endif
