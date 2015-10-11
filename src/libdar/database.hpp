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

    /// \file database.hpp
    /// \brief this file holds the database class definition
    /// \ingroup API


#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "../my_config.h"

#include <list>

#include "archive.hpp"
#include "generic_file.hpp"
#include "data_tree.hpp"
#include "storage.hpp"
#include "database_options.hpp"

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
	    /// \param[in] opt extendable list of options to use for this operation
 	database(user_interaction & dialog, const std::string & base, const database_open_options & opt);

	    /// database destructor (no implicit file saving)
	~database();

	    /// write the database to a file (see database_header first)

	    /// \param[in] dialog for user interaction
	    /// \param[in] filename name of file to save database to
	    /// \param[in] opt extendable list of options to use for this operation
	    /// \note this method is not available with partially extracted databases.
	void dump(user_interaction & dialog, const std::string & filename, const database_dump_options & opt) const;

	    // SETTINGS

	    /// add an archive to the database

	    /// \param[in] arch is the archive to add to the database (may be a partial archive)
	    /// \param[in] chemin is the path to this archive to record in the database
	    /// \param[in] basename is the archive's basename to record in the database
	    /// \param[in] opt extendable list of options to use for this operation
	    /// \note this method is not available with partially extracted databases.
	void add_archive(const archive & arch, const std::string & chemin, const std::string & basename, const database_add_options & opt);

	    /// remove an archive from a database

	    /// \param[in] min first archive index to remove
	    /// \param[in] max last archive index to remove
	    /// \param[in] opt extendable list of options to use for this operation
	    /// \note the archives which indexes are in the range [min-max] are
	    /// removed. If you want to remove only one archive choose min equal to max.
	    /// \note this method is not available with partially extracted databases.
	void remove_archive(archive_num min, archive_num max, const database_remove_options & opt);

	    /// change order of archive within the database

	    /// \param[in] src archive index to move
	    /// \param[in] dst archive index to move to
	    /// \note this method is not available with partially extracted databases.
	void set_permutation(archive_num src, archive_num dst);

	    /// change one's archive basename recorded in the database

	    /// \param[in] num is the archive index to rename
	    /// \param[in] basename is the new basename to give to that archive
	    /// \param[in] opt optional parameters for this operation
	    /// \note this method *is* available with partially extracted databases, but with partial_read_only ones
	void change_name(archive_num num, const std::string & basename, const database_change_basename_options &opt);

	    /// change one's archive path recorded in the database

	    /// \param[in] num is the archive index who's path to change
	    /// \param[in] chemin is the new path to give to that archive
	    /// \param[in] opt optional parameters for this operation
	    /// \note this method *is* available with partially extracted databases, but with partial_read_only ones
	void set_path(archive_num num, const std::string & chemin, const database_change_path_options & opt);

	    /// change the default options given to dar when performing restoration

	    /// \param[in] opt is a vector a arguments.
	    /// \note Each element of the vector must match a single argument of the command line
	    /// like for example "-R". Any leading or trailing space will make a different argument
	    /// than the one without spaces (" -R" is different than "-R" for example).
	    /// \note this method *is* available with partially extracted databases, but with partial_read_only ones
    	void set_options(const std::vector<std::string> &opt) { options_to_dar = opt; };

	    /// change the path to dar command

	    /// \param[in] chemin is the full path to dar (including dar filename) to use for restoration
	    /// \note if set to an empty string the dar command found from the PATH will be used (if any)
	    /// \note this method *is* available with partially extracted databases, but with partial_read_only ones
	void set_dar_path(const std::string & chemin) { dar_path = chemin; };


	    // "GETTINGS"

	    /// show the list of archive used to build the database

	    /// \param[in,out] dialog is the user_interaction to use to report the listing
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

	    // "ACTIONS" (not available with partially extracted databases)

	    /// restore files calling dar on the appropriated archive

	    /// \param[in,out] dialog where to have user interaction
	    /// \param[in] filename list of filename to restore
	    /// \param[in] opt extendable list of options to use for this operation
	    /// \note this method is not available with partially extracted databases.
	void restore(user_interaction & dialog,
		     const std::vector<std::string> & filename,
		     const database_restore_options & opt);

	    /// check that all files's Data and EA are more recent when archive number grows within the database, only warn the user

	    /// \param[in,out] dialog for user interaction
	    /// \return true if check succeeded, false if warning have been issued
	    /// \note this method is not available with partially extracted databases.
	bool check_order(user_interaction & dialog) const
	{
	    bool initial_warn = true;

	    if(files == NULL)
		throw SRC_BUG;
	    if(check_order_asked)
		return files->check_order(dialog, ".", initial_warn) && initial_warn;
	    else
		return true;
	}


    private:

	    /// holds the archive used to create the database
	struct archive_data
	{
	    std::string chemin;      //< path to the archive
	    std::string basename;    //< basename of the archive
	    infinint root_last_mod;  //< last modification date of the root directory
	};

	std::vector<struct archive_data> coordinate; //< list of archive used to build the database
	std::vector<std::string> options_to_dar;     //< options to use when calling dar for restoration
	std::string dar_path;                        //< path to dar
	data_dir *files;                             //< structure containing files and their status in the set of archive used for that database (is set to NULL in partial mode)
	storage *data_files;                         //< when reading archive in partial mode, this is where is located the "not readed" part of the archive (is set to NULL in partial-read-only mode)
	bool check_order_asked;                      //< whether order check has been asked

	void build(user_interaction & dialog, generic_file & f, bool partial, bool read_only, unsigned char db_version);  //< used by constructors
	archive_num get_real_archive_num(archive_num num, bool revert) const;

	const infinint & get_root_last_mod(const archive_num & num) const;
    };

} // end of namespace

#endif
