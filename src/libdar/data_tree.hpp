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

    /// \file data_tree.hpp
    /// \brief base classes used to store entree information in dar_manager databases
    /// \ingroup Private


#ifndef DATA_TREE_HPP
#define DATA_TREE_HPP

#include "../my_config.h"

#include <map>
#include <string>
#include <deque>
#include <set>
#include "infinint.hpp"
#include "generic_file.hpp"
#include "user_interaction.hpp"
#include "path.hpp"
#include "database_listing_callback.hpp"
#include "database_aux.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the data_tree class stores presence of a given file in a set of archives

	/// the data associated to a given file are the different modification dates
	/// that this file has been found in the archive the database has been feed by
    class data_tree
    {
    public:
	data_tree(const std::string &name);
	data_tree(generic_file & f, unsigned char db_version); //< constructor does not read signature
	data_tree(const data_tree & ref) = default;
	data_tree(data_tree && ref) noexcept = default;
	data_tree & operator = (const data_tree & ref) = default;
	data_tree & operator = (data_tree && ref) noexcept = default;
	virtual ~data_tree() = default;

	virtual void dump(generic_file & f) const; //< dump signature followed by data constructor will read
	std::string get_name() const { return filename; };
	void set_name(const std::string & name) { filename = name; };

	    /// returns the archives to restore in order to obtain the data that was defined just before (or at) the given date
	    ///
	    /// \param[out] archive is the set of archive to restore in sequence to obtain the requested data
	    /// \param[in] date date above which to ignore data found in the database
	    /// \param[in] even_when_removed is true when user requested to restore the file in its latest state even if it has been removed afterward
	    /// \return the success of failure status of the requested lookup
	db_lookup get_data(std::set<archive_num> & archive, const datetime & date, bool even_when_removed) const;

	    /// if EA has been saved alone later, returns in which version for the state of the file at the given date.
	db_lookup get_EA(archive_num & archive, const datetime & date, bool even_when_removed) const;

	    /// return the date of file's last modification date within the give archive and whether the file has been saved or deleted
	bool read_data(archive_num num,
		       datetime & val,
		       db_etat & present) const;

	    /// return the date of last inode change and whether the EA has been saved or deleted
	bool read_EA(archive_num num, datetime & val, db_etat & present) const;

	void set_data(const archive_num & archive,
		      const datetime & date,
		      db_etat present) { set_data(archive, date, present, nullptr, nullptr); };

	void set_data(const archive_num & archive,
		      const datetime & date,
		      db_etat present,
		      const crc *base,
		      const crc *result) { last_mod[archive] = status_plus(date, present, base, result); (void) check_delta_validity(); };

	void set_EA(const archive_num & archive, const datetime & date, db_etat present) { status sta(date, present); last_change[archive] = sta; };

	    /// check date order between archives withing the database ; throw Erange if problem found with date order
	virtual bool check_order(user_interaction & dialog, const path & current_path, bool & initial_warn) const { return check_map_order(dialog, last_mod, current_path, "data", initial_warn) && check_map_order(dialog, last_change, current_path, "EA", initial_warn); };

	    /// add deleted entry if no object of the current archive exist and the entry of the previous archive is already present.

	    /// \param[in] archive is the number of the archive to finalize
	    /// \param[in] deleted_date date of deletion to use for inode removal when no
	    /// information can be grabbed from the archive (this date is taken from the
	    /// parent dir last modification date)
	    /// \param[in] ignore_archive_greater_or_equal ignore archives which number
	    /// is greater or equal than "ignore_archive_greater_or_equal" as if they were not
	    /// present in the database. If set to zero, no archive is ignored.
	virtual void finalize(const archive_num & archive,
			      const datetime & deleted_date,
			      const archive_num & ignore_archive_greater_or_equal);

	    /// return true if the corresponding file is no more located in any archive (thus, the object is no more usefull in the base)
	virtual bool remove_all_from(const archive_num & archive_to_remove, const archive_num & last_archive);

	    /// list where is saved this file
	void listing(database_listing_get_version_callback callback,
		     void *tag) const;
	virtual void apply_permutation(archive_num src, archive_num dst);

	    /// decrement archive numbers above num
	virtual void skip_out(archive_num num);

	    /// provide a summary of the latest version of file and ea per archive number of the database

	    /// \param[out] data is a table indexed by archive num providing the number latest data version per archive
	    /// \param[out] ea is a table indexed by archive num providing the number latest EA version per archive
	    /// \param[out] total_data is a table indexed by archive providing the total number of entries with data, archive per archive (most recent and older)
	    /// \param[out] total_ea is a table indexed by archive num providing the total number of entries with EA, archive per archive (most recent and older)
	    /// \param[in] ignore_older_than_that, if not null date, ignore data et EA versions strictly more recent than this date
	    /// \note the provided four fields that will receive table of counters per archive should be initialized by the caller
	    /// as this is a recursive call with data_dir that increment the counter recursively.
	virtual void compute_most_recent_stats(std::deque<infinint> & data,
					       std::deque<infinint> & ea,
					       std::deque<infinint> & total_data,
					       std::deque<infinint> & total_ea,
					       const datetime & ignore_older_than_that) const;

	virtual char obj_signature() const { return signature(); };
	static char signature() { return 't'; };

	    // fix corruption case that was brought by bug in release 2.4.0 to 2.4.9
	virtual bool fix_corruption(); // return true whether corruption could be fixed (meaning this entry can be safely removed from base)

    private:

	static constexpr const char * const ETAT_SAVED = "S";
	static constexpr const char * const ETAT_PATCH = "O";
	static constexpr const char * const ETAT_PATCH_UNUSABLE = "U";
	static constexpr const char * const ETAT_PRESENT = "P";
	static constexpr const char * const ETAT_REMOVED = "R";
	static constexpr const char * const ETAT_ABSENT = "A";
	static constexpr const char * const ETAT_INODE = "I";

	static constexpr unsigned char STATUS_PLUS_FLAG_ME = 0x01;
	static constexpr unsigned char STATUS_PLUS_FLAG_REF = 0x02;

	class status
	{
	public:
	    status(): date(0) { present = db_etat::et_absent; };
	    status(const datetime & d, db_etat p) { date = d; present = p; };
	    status(const status & ref) = default;
	    status(status && ref) noexcept = default;
	    status & operator = (const status & ref) = default;
	    status & operator = (status && ref) noexcept = default;
	    virtual ~status() = default;

	    datetime date;                             //< date of the event
	    db_etat present;                              //< file's status in the archive

	    virtual void dump(generic_file & f) const; //< write the struct to file
	    virtual void read(generic_file &f,         //< set the struct from file
			      unsigned char db_version);
	};


	class status_plus : public status
	{
	public:
	    status_plus() { base = result = nullptr; };
	    status_plus(const datetime & d, db_etat p, const crc *xbase, const crc *xresult);
	    status_plus(const status_plus & ref): status(ref) { copy_from(ref); };
	    status_plus(status_plus && ref) noexcept: status(std::move(ref)) { nullifyptr(); move_from(std::move(ref)); };
	    status_plus & operator = (const status_plus & ref) { detruit(); copy_from(ref); return *this; };
	    status_plus & operator = (status_plus && ref) noexcept { status::operator = (std::move(ref)); move_from(std::move(ref)); return *this; };
	    ~status_plus() { detruit(); };

	    crc *base; //< only present for s_delta status, to have a link with the file to apply the patch to
	    crc *result; //< present for s_delta, s_saved, s_not_saved this is the crc of the data (or crc of the data resulting from the patch)

	    void dump(generic_file & f) const; //< write the struct to file
	    void read(generic_file &f,         //< set the struct from file
		      unsigned char db_version);

	private:
	    void nullifyptr() noexcept { base = result = nullptr; };
	    void copy_from(const status_plus & ref);
	    void move_from(status_plus && ref) noexcept;
	    void detruit();
	};


	std::string filename;
	std::map<archive_num, status_plus> last_mod; //< key is archive number ; value is last_mod time
	std::map<archive_num, status> last_change;   //< key is archive number ; value is last_change time


	    // when false is returned, this means that the user wants to ignore subsequent error of the same type
	    // else either no error yet met or user want to continue receiving the same type of error for other files
	    // in that later case initial_warn is set to false (first warning has been shown).
	template <class T> bool check_map_order(user_interaction & dialog,
						const std::map<archive_num, T> the_map,
						const path & current_path,
						const std::string & field_nature,
						bool & initial_warn) const;

	bool check_delta_validity(); // return true if no error has been met about delta patch (no delta is broken, missing its reference)

	    /// gives new archive number when an database has its archive reordered

	    /// \param[in] src the archive number to move
	    /// \param[in] dst the new position of the archive number given by src
	    /// \param[in] x any archive number in the database, which new position is to be calculated in regard to the src -> dst move
	    /// \return the new archive number of archive x in regard to the src -> dst move
	static archive_num data_tree_permutation(archive_num src, archive_num dst, archive_num x);

	    /// helper method to provide information to a database_listing_get_version_callback
	static void display_line(database_listing_get_version_callback callback,
				 void *tag,
				 archive_num num,
				 const datetime *data,
				 db_etat data_presence,
				 const datetime *ea,
				 db_etat ea_presence);

    };



	/// @}

} // end of namespace

#endif
