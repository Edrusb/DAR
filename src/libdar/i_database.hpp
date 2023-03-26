/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

    /// \file i_database.hpp
    /// \brief this file holds the definition of class database implementation (pimpl idiom)
    /// \ingroup Private


#ifndef I_DATABASE_HPP
#define I_DATABASE_HPP

#include "../my_config.h"

#include <list>

#include "archive.hpp"
#include "generic_file.hpp"
#include "data_dir.hpp"
#include "storage.hpp"
#include "mem_ui.hpp"
#include "database_options.hpp"
#include "database_archives.hpp"
#include "database.hpp"
#include "tools.hpp"

namespace libdar
{

        /// \addtogroup Private
        /// @{

        /// the class i_database is the implementation of class database

    class database::i_database: public mem_ui
    {
    public:
            /// this constructor build an empty database
        i_database(const std::shared_ptr<user_interaction> & dialog);

            /// this constructor reads i_database from a file
        i_database(const std::shared_ptr<user_interaction> & dialog,
                 const std::string & base,
                 const database_open_options & opt);

            /// disabling copy constructor
        i_database(const i_database & ref) = delete;

            /// disabling move constructor
        i_database(i_database && ref) noexcept = delete;

            /// disabling assignement operator
        i_database & operator = (const i_database & ref) = delete;

            /// disabling move assignment operator
        i_database & operator = (i_database && ref) noexcept = delete;

            /// i_database destructor (no implicit file saving)
        ~i_database();

            /// write the database to a file (see database_header first)
        void dump(const std::string & filename, const database_dump_options & opt) const;

            // SETTINGS

            /// add an archive to the database
        void add_archive(const archive & arch, const std::string & chemin, const std::string & basename, const database_add_options & opt);

            /// remove an archive from a database
        void remove_archive(archive_num min, archive_num max, const database_remove_options & opt);

            /// change order of archive within the database
        void set_permutation(archive_num src, archive_num dst);

            /// change one's archive basename recorded in the database
        void change_name(archive_num num, const std::string & basename, const database_change_basename_options &opt);

            /// change one's archive path recorded in the database
        void set_path(archive_num num, const std::string & chemin, const database_change_path_options & opt);

            /// change the default options given to dar when performing restoration
        void set_options(const std::vector<std::string> &opt) { options_to_dar = opt; };

            /// change the path to dar command
        void set_dar_path(const std::string & chemin) { dar_path = chemin; };

            /// change compression to use when storing base on file
        void set_compression(compression algozip) const { algo = algozip; };

	    /// change the compression level to use when storing base in file
	void set_compression_level(U_I level) const { compr_level = level; };

	    ////////////////////////
	    //
            // "GETTINGS"
	    //

            /// provides the list of archive used to build the database
        database_archives_list get_contents() const;

            /// return the options used with dar for restoration
        std::vector<std::string> get_options() const { return options_to_dar; }; // show option passed to dar

            /// returns the path for dar
        std::string get_dar_path() const { return dar_path; }; // show path to dar command

            /// returns the compression algorithm used on filesystem
        compression get_compression() const { return algo; };

	    /// returns the compression level used on file
	U_I get_compression_level() const { return compr_level; };

            /// return the database format version
        std::string get_database_version() const { return tools_uint2str(cur_db_version); };

            /// list files which are present in a given archive
        void get_files(database_listing_show_files_callback callback,
                       void *context,
                       archive_num num,
                       const database_used_options & opt) const;

            /// list the archive where a give file is present
        void get_version(database_listing_get_version_callback callback,
                         void *context,
                         path chemin) const;

            /// compute some statistics about the location of most recent file versions
        void show_most_recent_stats(database_listing_statistics_callback callback,
                                    void *context) const;

            // "ACTIONS" (not available with partially extracted databases)

            /// restore files calling dar on the appropriated archive
        void restore(const std::vector<std::string> & filename,
                     const database_restore_options & opt);

            /// check that all files's Data and EA are more recent when archive number grows within the database, only warn the user
        bool check_order() const
        {
            bool initial_warn = true;

            if(files == nullptr)
                throw SRC_BUG;
            if(check_order_asked)
                return files->check_order(get_ui(), path("."), initial_warn) && initial_warn;
            else
                return true;
        }


    private:

            /// holds the archive used to create the database
        struct archive_data
        {
            std::string chemin;      ///< path to the archive
            std::string basename;    ///< basename of the archive
            datetime root_last_mod;  ///< last modification date of the root directory
        };

        std::deque<struct archive_data> coordinate;  ///< list of archive used to build the database
        std::vector<std::string> options_to_dar;     ///< options to use when calling dar for restoration
	std::string dar_path;                        ///< path to dar
	data_dir *files;                             ///< structure containing files and their status in the set of archive used for that database (is set to nullptr in partial mode)
	storage *data_files;                         ///< when reading archive in partial mode, this is where is located the "not readed" part of the archive (is set to nullptr in partial-read-only mode)
	bool check_order_asked;                      ///< whether order check has been asked
	unsigned char cur_db_version;                ///< current db version (for informational purposes)
	mutable compression algo;                    ///< compression used/to use when writing down the base to file
	mutable U_I compr_level;                     ///< the compression level to use

	void build(generic_file & f, bool partial, bool read_only, unsigned char db_version);  ///< used by constructors
	archive_num get_real_archive_num(archive_num num, bool revert) const;

	const datetime & get_root_last_mod(const archive_num & num) const;
    };

	/// @}

} // end of namespace

#endif
