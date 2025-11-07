/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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

    /// \file database.hpp
    /// \brief this file holds the database class definition
    /// \ingroup API


#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "../my_config.h"

#include <list>

#include "archive.hpp"
#include "database_options.hpp"
#include "database_archives.hpp"
#include "archive_num.hpp"
#include "database_listing_callback.hpp"

namespace libdar
{

	/// \addtogroup API
        /// @{

	/// the database class defines the dar_manager database

	/// all operations for a dar_manager database are defines through the
	/// use of this class interface. This class also defines internally
	/// the data structure of the database.
    class database
    {
    public:
	    /// this constructor build an empty database
	database(const std::shared_ptr<user_interaction> & dialog);

	    /// this constructor reads database from a file

	    /// \param[in] dialog for user interaction
	    /// \param[in] base database filename
	    /// \param[in] opt extendable list of options to use for this operation
 	database(const std::shared_ptr<user_interaction> & dialog,
		 const std::string & base,
		 const database_open_options & opt);

	    /// disabling copy constructor
	database(const database & ref) = delete;

	    /// disabling move constructor
	database(database && ref) noexcept = delete;

	    /// disabling assignement operator
	database & operator = (const database & ref) = delete;

	    /// disabling move assignment operator
	database & operator = (database && ref) noexcept = delete;

	    /// database destructor (no implicit file saving)
	~database();

	    /// write the database to a file (see database_header first)

	    /// \param[in] filename name of file to save database to
	    /// \param[in] opt extendable list of options to use for this operation
	    /// \note this method is not available with partially extracted databases.
	void dump(const std::string & filename,
		  const database_dump_options & opt) const;

	    // SETTINGS

	    /// add an archive to the database

	    /// \param[in] arch is the archive to add to the database (may be a partial archive)
	    /// \param[in] chemin is the path to this archive to record in the database
	    /// \param[in] basename is the archive's basename to record in the database
	    /// \param[in] opt extendable list of options to use for this operation
	    /// \note this method is not available with partially extracted databases.
	void add_archive(const archive & arch,
			 const std::string & chemin,
			 const std::string & basename,
			 const database_add_options & opt);

	    /// remove an archive from a database

	    /// \param[in] min first archive index to remove
	    /// \param[in] max last archive index to remove
	    /// \param[in] opt extendable list of options to use for this operation
	    /// \note the archives which indexes are in the range [min-max] are
	    /// removed. If you want to remove only one archive choose min equal to max.
	    /// \note this method is not available with partially extracted databases.
	void remove_archive(archive_num min,
			    archive_num max,
			    const database_remove_options & opt);

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
	void change_name(archive_num num,
			 const std::string & basename,
			 const database_change_basename_options &opt);

	    /// change one's archive path recorded in the database

	    /// \param[in] num is the archive index who's path to change
	    /// \param[in] chemin is the new path to give to that archive
	    /// \param[in] opt optional parameters for this operation
	    /// \note this method *is* available with partially extracted databases, but with partial_read_only ones
	void set_path(archive_num num,
		      const std::string & chemin,
		      const database_change_path_options & opt);


	    /// change one's archive crypto parameters recorded in the database

	    /// \param[in] num is the archive index who's parameters to change
	    /// \param[in] algo is the crypto algo to record in order to open this archive
	    /// \param[in] pass is the symmetrical key to use in order to open this archive
	    /// \param[in] opt optional parameters for this operation
	    /// \note this method *is* available with partially extracted databases, but with partial_read_only ones
	    /// \note: algo should stay equal to crypto_algo none for the algo to be fetched from the archive header
	    /// \note: an empty password will drive interactive request of the password, if you want to remove
	    /// crypto info associated to this archive, use clear_crypto_pass() method below
	void change_crypto_algo_pass(archive_num num,
				     crypto_algo algo,
				     const secu_string & pass,
				     const database_change_crypto_options & opt);

	    /// clear the crypto, algo, pass and optional crypto infos from the database for this archive
	void clear_crypto_algo_pass(archive_num num, const database_numbering & opt);

	    /// change the default options given to dar when performing restoration

	    /// \param[in] opt is a vector a arguments.
	    /// \note Each element of the vector must match a single argument of the command line
	    /// like for example "-R". Any leading or trailing space will make a different argument
	    /// than the one without spaces (" -R" is different than "-R" for example).
	    /// \note this method *is* available with partially extracted databases, but with partial_read_only ones
    	void set_options(const std::vector<std::string> &opt);

	    /// change the path to dar command

	    /// \param[in] chemin is the full path to dar (including dar filename) to use for restoration
	    /// \note if set to an empty string the dar command found from the PATH will be used (if any)
	    /// \note this method *is* available with partially extracted databases, but with partial_read_only ones
	void set_dar_path(const std::string & chemin);

	    /// change compression to use when storing base in file
	void set_compression(compression algozip) const;

	    /// change the compression level to use when storing base in file
	void set_compression_level(U_I compr_level) const;

	    /// change crypto algo to use
	void set_database_crypto_algo(crypto_algo algo);

	    /// change crypto key to use
	void set_database_crypto_pass(const secu_string & key);

	    /// change kdf hash
	void set_database_kdf_hash(hash_algo val);

	    /// change kdf iteration count
	void set_database_kdf_iteration_count(const infinint & val);

	    /// change encryption block size
	void set_database_crypto_block_size(U_32 val);


	    // "GETTINGS"

	    /// provides the list of archive used to build the database
	database_archives_list get_contents() const;

	    /// return the options used with dar for restoration
	std::vector<std::string> get_options() const;

	    /// returns the path for dar

	    /// \return the path to dar used when restoring files
	    /// \note empty string means that dar is taken from the PATH variable
	std::string get_dar_path() const;

	    /// returns the compression algorithm used on filesystem
	compression get_compression() const;

	    /// returns the compression level used on file
	U_I get_compression_level() const;

	    /// returns the database format version
        std::string get_database_version() const;

	    /// returns the crypto algo used
	crypto_algo get_database_crypto_algo() const;

	    /// returns the kdf hash
	hash_algo get_database_kdf_hash() const;

	    /// returns the kdf iteration count
	const infinint & get_database_kdf_iteration() const;

	    /// returns the crypto block size
	U_32 get_database_crypto_block_size() const;


	    /// list files which are present in a given archive

	    /// \param[in] callback is called to provide each entry in turn from the list
	    /// \param[in] context is given as first argument of the callback as is provided here
	    /// \param[in] num is the archive number to look at
	    /// \param[in] opt optional parameters for this operation
	    /// \note if "num" is set to zero all archive contents is listed
	    /// \note this method is not available with partially extracted databases.
	void get_files(database_listing_show_files_callback callback,
		       void *context,
		       archive_num num,
		       const database_used_options & opt) const;

	    /// list the archive where a give file is present

	    /// \param[in] callback is used to provide each entry in turn from the list
	    /// \param[in] context is given as first argument of the callback as is provided here
	    /// \param[in] chemin path to the file to look for
	    /// \note this method is not available with partially extracted databases.
	void get_version(database_listing_get_version_callback callback,
			 void *context,
			 path chemin) const;

	    /// compute some statistics about the location of most recent file versions

	    /// \param[in] callback is used to provide each entry in turn from the list
	    /// \param[in] context is given as first argument of the callback as is provided here
	    /// \note this method is not available with partially extracted databases.
	void show_most_recent_stats(database_listing_statistics_callback callback,
				    void *context) const;

	    // "ACTIONS" (not available with partially extracted databases)

	    /// restore files calling dar on the appropriated archive

	    /// \param[in] filename list of filename to restore
	    /// \param[in] opt extendable list of options to use for this operation
	    /// \note this method is not available with partially extracted databases.
	void restore(const std::vector<std::string> & filename,
		     const database_restore_options & opt);


	    /// restore files without calling dar as a seperated process

	    /// \param[in] read_options options to use to open archive for restoration
	    /// \param[in] fs_root where to restore data
	    /// \param[in] extract_options options to use to perform archive extraction
	    /// \param[in] opt dar_manager relative info for this operation
	    /// \param[in] progressive_report if not nullptr, will fill the progressive_report during the operation
	    /// \note opt fields that are taken into account here are:
	    ///  - set_date()
	    ///  - set_info_details() but only for database specific informations (see archive options else)
	    ///  - even_when_removed()
	    ///  The fiels/methods (set_early_release(), set_extra_options_for_dar(),
	    ///  set_ignore_dar_options_in_database() are ignored as they concern CLI
	    ///  format of options (not defined in libdar but in dar).
	void restore(const archive_options_read & read_options,
		     const path & fs_root,
		     const archive_options_extract & extract_options,
		     const database_restore_options & opt,
		     statistics* progressive_report);


	    /// check that all files's Data and EA are more recent when archive number grows within the database, only warn the user

	    /// \return true if check succeeded, false if warning have been issued
	    /// \note this method is not available with partially extracted databases.
	bool check_order() const;

    private:
	class i_database;
	std::unique_ptr<i_database> pimpl;

    };

	/// @}

} // end of namespace

#endif
