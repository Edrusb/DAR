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

    /// \file database_options.hpp
    /// \brief this file holds the options for database operations
    /// \ingroup API

    ///. class database_open_options
    ///. class database_dump_options
    ///. class database_add_options
    ///. class database_remove_options
    ///. class database_change_basename_options
    ///. class database_change_path_options
    ///. class database_restore_options
    ///. class database_used_options


#ifndef DATABASE_OPTIONS_HPP
#define DATABASE_OPTIONS_HPP

#include "../my_config.h"

#include <string>
#include <vector>

namespace libdar
{

	/// \addtogroup API
	/// @{


	/// options to open a database

    class database_open_options
    {
    public:
	database_open_options() { clear(); };
	database_open_options(const database_open_options & ref) = default;
	database_open_options(database_open_options && ref) noexcept = default;
	database_open_options & operator = (const database_open_options & ref) = default;
	database_open_options & operator = (database_open_options && ref) noexcept = default;
	~database_open_options() = default;


	void clear() { x_partial = false; x_partial_read_only = false; x_warn_order = true; };

	    // setings

	    /// partial option

	    /// \param[in] value set to true to only load an manipulate database header
	    /// \note if value is set to true, the database loading is quick but only some database methods are available (see the database class documentation)
	void set_partial(bool value) { x_partial = value; };


	    /// partial and read only option

	    /// \param[in] value when set, the database is in partial mode *and* in read-only. It cannot be dumped or modified.
	    /// \note if value is set to true, all restriction found for partial mode apply, and in addition, the database cannot be dumped (written back to file)
	    /// \note partial_read_only implies partial, but partial does not imply partial_readonly (it can be dumped but modification
	    /// can only take place in the archive header)
	void set_partial_read_only(bool value) { x_partial_read_only = value; if(value) x_partial = value; };


	    /// warning about file ordering in database

	    /// \param[in] value whether to warn when file chronological ordering does not respect the order of archives
	void set_warn_order(bool value) { x_warn_order = value; };

	    // gettings
	bool get_partial() const { return x_partial; };
	bool get_partial_read_only() const { return x_partial_read_only; };
	bool get_warn_order() const { return x_warn_order; };

    private:
	bool x_partial;
	bool x_partial_read_only;
	bool x_warn_order;
    };

	/// options to write a database to file

    class database_dump_options
    {
    public:
	database_dump_options() { clear(); };
	database_dump_options(const database_dump_options & ref) = default;
	database_dump_options(database_dump_options && ref) noexcept = default;
	database_dump_options & operator = (const database_dump_options & ref) = default;
	database_dump_options & operator = (database_dump_options && ref) noexcept = default;
	~database_dump_options() = default;

	void clear() { x_overwrite = false; };

	    // settings

	    /// overwrite option

	    /// \param[in] value whether we can overwrite the file if it already exists
	    ///
	void set_overwrite(bool value) { x_overwrite = value; };

	    // gettings
	bool get_overwrite() const { return x_overwrite; };

    private:
	bool x_overwrite;
    };

	/// options to add an archive to base

    	/// for now this is a placeholder class
    class database_add_options
    {
    public:
	database_add_options() { clear(); };
	database_add_options(const database_add_options & ref) = default;
	database_add_options(database_add_options && ref) noexcept = default;
	database_add_options & operator = (const database_add_options & ref) = default;
	database_add_options & operator = (database_add_options && ref) noexcept = default;
	~database_add_options() = default;

	void set_crypto_algo(crypto_algo val) { algo = val; };
	void set_crypto_pass(const secu_string & val) { pass = val; };

	crypto_algo get_crypto_algo() const { return algo; };
	const secu_string & get_crypto_pass() const { return pass; };

	void clear() { algo = crypto_algo::none; pass.clear(); };

    private:
	crypto_algo algo;
	secu_string pass;
    };

	/// options to remove an archive from the base

    class database_remove_options
    {
    public:
	database_remove_options() { clear(); };
	database_remove_options(const database_remove_options & ref) = default;
	database_remove_options(database_remove_options && ref) noexcept = default;
	database_remove_options & operator = (const database_remove_options & ref) = default;
	database_remove_options & operator = (database_remove_options && ref) noexcept = default;
	~database_remove_options() = default;

	void clear() { x_revert_archive_numbering = false; };

	    /// defines whether the archive number is counted from the beginning or from the end of the database
	void set_revert_archive_numbering(bool revert) { x_revert_archive_numbering = revert; };

	bool get_revert_archive_numbering() const { return x_revert_archive_numbering; };

    private:
	bool x_revert_archive_numbering;

    };

	/// options for changing a given archive's basename

    class database_change_basename_options
    {
    public:
	database_change_basename_options() { clear(); };
	database_change_basename_options(const database_change_basename_options & ref) = default;
	database_change_basename_options(database_change_basename_options && ref) noexcept = default;
	database_change_basename_options & operator = (const database_change_basename_options & ref) = default;
	database_change_basename_options & operator = (database_change_basename_options && ref) noexcept = default;
	~database_change_basename_options() = default;

	void clear() { x_revert_archive_numbering = false; };

	    /// defines whether the archive number is counted from the beginning or from the end of the database
	void set_revert_archive_numbering(bool revert) { x_revert_archive_numbering = revert; };

	bool get_revert_archive_numbering() const { return x_revert_archive_numbering; };

    private:
	bool x_revert_archive_numbering;

    };


	/// options for changing a given archive's path

    class database_change_path_options
    {
    public:
	database_change_path_options() { clear(); };
	database_change_path_options(const database_change_path_options & ref) = default;
	database_change_path_options(database_change_path_options && ref) noexcept = default;
	database_change_path_options & operator = (const database_change_path_options & ref) = default;
	database_change_path_options & operator = (database_change_path_options && ref) noexcept = default;
	~database_change_path_options() = default;

	void clear() { x_revert_archive_numbering = false; };

	    /// defines whether the archive number is counted from the beginning or from the end of the database
	void set_revert_archive_numbering(bool revert) { x_revert_archive_numbering = revert; };

	bool get_revert_archive_numbering() const { return x_revert_archive_numbering; };

    private:
	bool x_revert_archive_numbering;

    };

	/// options for changing a given archive's encryption params

    class database_change_crypto_options
    {
    public:
	database_change_crypto_options() { clear(); };
	database_change_crypto_options(const database_change_crypto_options & ref) = default;
	database_change_crypto_options(database_change_crypto_options && ref) noexcept = default;
	database_change_crypto_options & operator = (const database_change_crypto_options & ref) = default;
	database_change_crypto_options & operator = (database_change_crypto_options && ref) noexcept = default;
	~database_change_crypto_options() = default;

	void clear() { x_revert_archive_numbering = false; };

	    /// defines whether the archive number is counted from the beginning or from the end of the database
	void set_revert_archive_numbering(bool revert) { x_revert_archive_numbering = revert; };

	bool get_revert_archive_numbering() const { return x_revert_archive_numbering; };

    private:
	bool x_revert_archive_numbering;

    };

	/// options for restoration from database

    class database_restore_options
    {
    public:
	database_restore_options() { clear(); };
	database_restore_options(const database_restore_options & ref) = default;
	database_restore_options(database_restore_options && ref) noexcept = default;
	database_restore_options & operator = (const database_restore_options & ref) = default;
	database_restore_options & operator = (database_restore_options && ref) noexcept = default;
	~database_restore_options() = default;

	void clear() { x_early_release = x_info_details = x_ignore_dar_options_in_database = x_even_when_removed = false; x_date = datetime(0); x_extra_options_for_dar.clear(); };

	    // settings


	    /// early_release option

	    /// if early_release is set to true, some memory is released before calling dar
	    /// \note if early_release is true, this will free almost all memory allocated by the database before calling dar.
	    /// drawback is that no more action is possible after this call (except destruction)

	void set_early_release(bool value) { x_early_release = value; };

	    /// info_details option

	    /// if set to true, more detailed informations is displayed
	void set_info_details(bool value) { x_info_details = value; };

	    /// extra options to dar

	    /// this option is a mean to provide some extra options to dar from dar_manager
	void set_extra_options_for_dar(const std::vector<std::string> & value) { x_extra_options_for_dar = value; };

	    /// ignore options to dar embedded in the database

	void set_ignore_dar_options_in_database(bool mode) { x_ignore_dar_options_in_database = mode; };

	    /// date option

	    /// informations about files more recent than the given date are ignored. So you can restore file in the most recent state before a certain "date".
	    /// \note if set to zero, the most recent state available is looked for (this is the default value).
	void set_date(const infinint & value) { x_date = datetime(value); };

	    /// date option in the new form of a datetime (eventually providing sub second precision)
	void set_date(const datetime & value) { x_date = value; };

	    /// find data or EA if they have been removed at the requested date

	    /// in the case a file was removed at the request date, the data or EA
	    /// that will be restored will be the one it had just before being removed
	void set_even_when_removed(bool value) { x_even_when_removed = value; };


	    // gettings
	bool get_early_release() const { return x_early_release; };
	bool get_info_details() const { return x_info_details; };
	const std::vector<std::string> & get_extra_options_for_dar() const { return x_extra_options_for_dar; };
	const datetime & get_date() const { return x_date; };
	bool get_ignore_dar_options_in_database() const { return x_ignore_dar_options_in_database; };
	bool get_even_when_removed() const { return x_even_when_removed; };

    private:
	bool x_early_release;
	bool x_info_details;
	std::vector<std::string> x_extra_options_for_dar;
	datetime x_date;
	bool x_ignore_dar_options_in_database;
	bool x_even_when_removed;
    };


	/// options for file "used" in archive

    class database_used_options
    {
    public:
	database_used_options() { clear(); };
	database_used_options(const database_used_options & ref) = default;
	database_used_options(database_used_options && ref) noexcept = default;
	database_used_options & operator = (const database_used_options & ref) = default;
	database_used_options & operator = (database_used_options && ref) noexcept = default;
	~database_used_options() = default;

	void clear() { x_revert_archive_numbering = false; };

	    /// defines whether the archive number is counted from the beginning or from the end of the database
	void set_revert_archive_numbering(bool revert) { x_revert_archive_numbering = revert; };

	bool get_revert_archive_numbering() const { return x_revert_archive_numbering; };

    private:
	bool x_revert_archive_numbering;

    };


	/// @}


} // end of namespace
#endif
