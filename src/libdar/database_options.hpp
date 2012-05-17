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
// $Id: database_options.hpp,v 1.6.2.1 2011/06/20 14:02:03 edrusb Exp $
//
/*********************************************************************/

    /// \file database_options.hpp
    /// \brief this file holds the options for database operations
    /// \ingroup API

#ifndef DATABASE_OPTIONS_HPP
#define DATABASE_OPTIONS_HPP

#include "../my_config.h"

#include <string>
#include <vector>

namespace libdar
{

	/// \ingroup API
	/// @}


	/// options to open a database

    class database_open_options
    {
    public:
	database_open_options() { x_partial = false; };

	    // setings

	    /// partial option

	    /// \param[in] value set to true to only load an manipulate database header
	    /// \note if value is set to true, the database loading is quick but only some database methods are available (see the database class documentation)
	void set_partial(bool value) { x_partial = value; };


	    // gettings
	bool get_partial() const { return x_partial; };

    private:
	bool x_partial;

    };

	/// options to write a database to file

    class database_dump_options
    {
    public:
	database_dump_options() { x_overwrite = false; };

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

    class database_add_options
    {
    public:
	database_add_options() {};
    };

	/// options to remove an archive from the base

    class database_remove_options
    {
    public:
	database_remove_options() { x_revert_archive_numbering = false; };

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
	database_change_basename_options() { x_revert_archive_numbering = false; };

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
	database_change_path_options() { x_revert_archive_numbering = false; };

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
	database_restore_options() { x_early_release = x_info_details = x_ignore_dar_options_in_database = x_even_when_removed = false; x_date = 0; x_extra_options_for_dar.clear(); };

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
	void set_date(const infinint & value) { x_date = value; };

	    /// find data or EA if they have been removed at the requested data

	    /// in the case a file has was removed at the request date, the data or EA
	    /// that will be restored will be the one of it had just before being removed
	void set_even_when_removed(bool value) { x_even_when_removed = value; };


	    // gettings
	bool get_early_release() const { return x_early_release; };
	bool get_info_details() const { return x_info_details; };
	const std::vector<std::string> & get_extra_options_for_dar() const { return x_extra_options_for_dar; };
	const infinint & get_date() const { return x_date; };
	bool get_ignore_dar_options_in_database() const { return x_ignore_dar_options_in_database; };
	bool get_even_when_removed() const { return x_even_when_removed; };

    private:
	bool x_early_release;
	bool x_info_details;
	std::vector<std::string> x_extra_options_for_dar;
	infinint x_date;
	bool x_ignore_dar_options_in_database;
	bool x_even_when_removed;
    };


	/// options for file "used" in archive

    class database_used_options
    {
    public:
	database_used_options() { x_revert_archive_numbering = false; };

	    /// defines whether the archive number is counted from the beginning or from the end of the database
	void set_revert_archive_numbering(bool revert) { x_revert_archive_numbering = revert; };

	bool get_revert_archive_numbering() const { return x_revert_archive_numbering; };

    private:
	bool x_revert_archive_numbering;

    };


	/// @}


} // end of namespace
#endif
