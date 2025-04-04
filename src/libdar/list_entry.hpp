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

    /// \file list_entry.hpp
    /// \brief class of objects describing an entry in the archive, used by archive::get_children_in_table
    /// \ingroup API


#ifndef LIST_ENTRY_HPP
#define LIST_ENTRY_HPP

#include <string>
#include <set>

#include "../my_config.h"
#include "infinint.hpp"
#include "deci.hpp"
#include "compression.hpp"
#include "integers.hpp"
#include "datetime.hpp"
#include "range.hpp"
#include "cat_status.hpp"
#include "ea.hpp"
#include "fsa_family.hpp"

namespace libdar
{

        /// \addtogroup API
        /// @{

    class crc;

	/// the list_entry class provides mean to get information about a particular entry in the archive
	///
	/// it provides methods for libdar to fill up such object and methods for API user
	/// to read the information. Each information uses its own method, thus it will require
	/// several call to different method to get the full description of the object.
	/// This has the advantage to let the possiblity to add new fields in the future
	/// without breaking anything in API, and in consequences in user programs.
    class list_entry
    {
    public:
	list_entry() { clear(); };
	list_entry(const list_entry & ref) = default;
	list_entry(list_entry && ref) = default;
	list_entry & operator = (const list_entry & ref) = default;
	list_entry & operator = (list_entry && ref) noexcept = default;
	~list_entry() = default;

	    /// method used to know whether the returned entry signals a End of Directory

	    /// \note such entry does not contain any valid information, it signals the next
	    /// entry will be taken from the parent directory, instead of the current directory.
	    /// Such entry is necessary for archive::op_listing
	bool is_eod() const { return type == 'z'; };

	    // methods for API users
	    // field that are not set are returned as empty string

	const std::string & get_name() const { return my_name; };
	unsigned char get_type() const { return type; };
	bool is_dir() const { return type == 'd'; };
	bool is_file() const { return type == 'f'; };
	bool is_symlink() const { return type == 'l'; };
	bool is_char_device() const { return type == 'c'; };
	bool is_block_device() const { return type == 'b'; };
	bool is_unix_socket() const { return type == 's'; };
	bool is_named_pipe() const { return type == 'p'; };
	bool is_hard_linked() const { return hard_link; };
	bool is_removed_entry() const { return type == 'x'; };
	bool is_door_inode() const { return type == 'o'; };
	bool is_empty_dir() const { return empty_dir; };

	unsigned char get_removed_type() const;  ///< valid only for removed_entries

	bool has_data_present_in_the_archive() const { return data_status == saved_status::saved || data_status == saved_status::delta; };
	std::string get_data_flag() const;
	saved_status get_data_status() const { return data_status; };

	bool has_EA() const { return ea_status != ea_saved_status::none && ea_status != ea_saved_status::removed; };
	bool has_EA_saved_in_the_archive() const { return ea_status == ea_saved_status::full; };
	std::string get_ea_flag() const;
	ea_saved_status get_ea_status() const { return ea_status; };

	bool has_FSA() const { return fsa_status != fsa_saved_status::none; };
	bool has_FSA_saved_in_the_archive() const { return fsa_status == fsa_saved_status::full; };
	std::string get_fsa_flag() const;

	std::string get_uid(bool try_resolving_name = false) const;
	std::string get_gid(bool try_resolving_name = false) const;
	std::string get_perm() const;

	    /// \note this is a const method because it only changes
	    /// the display not the value of dates
	void set_fully_detailed_dates(bool mode) const { full_date = mode; };
	std::string get_last_access() const;
	std::string get_last_modif() const;
	std::string get_last_change() const;
	std::string get_removal_date() const; ///< for removed_entry only
	time_t get_last_access_s() const { return datetime2time_t(last_access); };
	time_t get_last_modif_s() const;
	time_t get_last_change_s() const { return datetime2time_t(last_change); };
	time_t get_removal_date_s() const; ///< for removed_entry only

	    /// yet an alternative method to get last access time

	    /// \param[in] tu time unit to be used to store fraction (libdar::datetime::tu_microsecond, libdar::datetime::tu_nanosecond,...)
	    /// \param[out] second integer number of second
	    /// \param[out] fraction remaining part of the time (expressed as tu unit) to be added to "second" to get the exact time
	void get_last_access(datetime::time_unit tu, time_t & second, time_t & fraction) const
	{ last_access.get_value(second, fraction, tu); }

	    /// yet an alternative method to get the last modification date (see get_last_access() for details)
	void get_last_modif(datetime::time_unit tu, time_t & second, time_t & fraction)	const
	{ last_modif.get_value(second, fraction, tu); }

	    /// yet an alternative method to get the last change date (see get_last_access() for details)
	void get_last_change(datetime::time_unit tu, time_t & second, time_t & fraction) const
	{ last_change.get_value(second, fraction, tu); }

	std::string get_file_size(bool size_in_bytes = true) const;
	std::string get_compression_ratio() const;
	std::string get_compression_ratio_flag() const;
	bool is_sparse() const { return sparse_file; };
	std::string get_sparse_flag() const { return sparse_file ? "[X]" : "[ ]"; };
	std::string get_compression_algo() const { return compression2string(compression_algo); };
	bool is_dirty() const { return dirty; };
	std::string get_link_target() const { return target; };
	std::string get_major() const;
	std::string get_minor() const;

	    /// provides slice information

	    /// \note you must activate slice lookup from archive options for this
	    /// field to me set by libdar, this operation has a additional cost in
	    /// storage and computation
	const range & get_slices() const { return slices; };
	bool has_delta_signature() const { return delta_sig; };
	std::string get_delta_flag() const;


	    /// offset in byte where to find first byte of data

	    /// \note: return false if no data is present, else set the argument
	    /// \note: offset is counted whatever is the number of slice as if there all slice were sticked toghether. But
	    /// the first bytes of each slice do not count as they hold the slice header. This one is variable
	    /// but can be known using the archive::get_first_slice_header_size() and archive::get_non_first_slice_header_size()
	    /// methods from the current archive class. If encryption is used it is not possible to translate precisely from
	    /// archive offset to slice offset, the encryption layer depending on the algorithm used may introduce an additional
	    /// shift between clear data offset an corresponding ciphered data offset.
	    /// \note if an U_64 cannot handle such large value, false is returned, you should use the infinint of std::string
	    /// version of this method
	bool get_archive_offset_for_data(infinint & val) const { val = offset_for_data; return !val.is_zero(); };
	bool get_archive_offset_for_data(U_64 & val) const;
	std::string get_archive_offset_for_data() const { return offset_for_data.is_zero() ? "" : deci(offset_for_data).human(); };

	    /// amount of byte used to store the file's data

	    /// \note if an U_64 cannot handle such large value, false is returned, you should use the
	    /// infinint of std::string version of this method
	bool get_storage_size_for_data(infinint & val) const { val = storage_size_for_data; return !val.is_zero(); };
	bool get_storage_size_for_data(U_64 & val) const;
	std::string get_storage_size_for_data(bool size_in_bytes = true) const;

	    /// offset in byte whert to find the first byte of Extended Attributes

	    /// \note see note for get_archive_offset_for_data(infinint)
	    /// \note if an U_64 cannot handle such large value, false is returned, you should use the infinint of
	    /// std::string version of this method
	bool get_archive_offset_for_EA(infinint & val) const { val = offset_for_EA; return !val.is_zero(); };
	bool get_archive_offset_for_EA(U_64 & val) const;
	std::string get_archive_offset_for_EA() const { return offset_for_EA.is_zero() ? "" : deci(offset_for_EA).human(); };

	    /// amount of byte used to store the file's EA
	bool get_storage_size_for_EA(infinint & val) const { val = storage_size_for_EA; return !val.is_zero(); };
	bool get_storage_size_for_EA(U_64 & val) const;
	std::string get_storage_size_for_EA() const { return storage_size_for_EA.is_zero() ? "" : deci(storage_size_for_EA).human(); };

	    /// offset in byte where to find the first byte of Filesystem Specific Attributes

	    /// \note see note for get_archive_offset_for_data(infinint)
	    /// \note if an U_64 cannot handle such large value, false is returned, you should use the
	    /// infinint of std::string version of this method
	bool get_archive_offset_for_FSA(infinint & val) const { val = offset_for_FSA; return !val.is_zero(); };
	bool get_archive_offset_for_FSA(U_64 & val) const;
	std::string get_archive_offset_for_FSA() const { return offset_for_FSA.is_zero() ? "" : deci(offset_for_FSA).human(); };

	    /// amount of byte used to store the file's FSA
	bool get_storage_size_for_FSA(infinint & val) const { val = storage_size_for_FSA; return !val.is_zero(); };
	bool get_storage_size_for_FSA(U_64 & val) const;
	std::string get_storage_size_for_FSA() const { return storage_size_for_FSA.is_zero() ? "" : deci(storage_size_for_FSA).human(); };

	    /// reset the reading of Extended Attributes names

	    /// \note see get_ea_read_next()
	void get_ea_reset_read() const { it_ea = ea.begin(); };

	    /// read the next Extended Attribute name

	    /// \param[out] key is set to the EA name
	    /// \return true if some other attribute can be read, false if this one is the last
	    /// \note to have list_entry objects filled with this information you need to
	    /// activate it in archive options listing (archive::op_listing) of set fetch_ea to true
	    /// in archive::get_children_of() and in archive_get_children_in_table()
	bool get_ea_read_next(std::string & key) const;

	std::string get_etiquette() const { return deci(etiquette).human(); }; ///< this is the hard-link ID, only valid for hard linked entries

	fsa_scope get_fsa_scope() const { return fsa_sc; };

	std::string get_data_crc() const { return data_crc; };
	std::string get_delta_patch_base_crc() const { return patch_base_crc; };
	std::string get_delta_patch_result_crc() const { return patch_result_crc; };

	    // methods for libdar to setup the object

	void set_name(const std::string & val) { my_name = val; };
	void set_type(unsigned char val) { type = val; };
	void set_removed_type(unsigned char val);
	void set_hard_link(bool val) { hard_link = val; };
	void set_uid(const infinint & val) { uid = val; };
	void set_gid(const infinint & val) { gid = val; };
	void set_perm(U_16 val) { perm = val; };
	void set_last_access(const datetime & val) { last_access = val; };
	void set_last_modif(const datetime & val) { last_modif = val; };
	void set_removal_date(const datetime & val);
	void set_saved_status(saved_status val) { data_status = val; };
	void set_ea_status(ea_saved_status val) { ea_status = val; };
	void set_last_change(const datetime & val) { last_change = val; };
	void set_fsa_status(fsa_saved_status val) { fsa_status = val; };
	void set_file_size(const infinint & val) { file_size = val; };
	void set_is_sparse_file(bool val) { sparse_file = val; };
	void set_compression_algo(compression val) { compression_algo = val; };
	void set_dirtiness(bool val) { dirty = val; };
	void set_link_target(const std::string & val) { target = val; };
	void set_major(int val) { major = val; };
	void set_minor(int val) { minor = val; };
	void set_slices(const range & sl) { slices = sl; };
	void set_delta_sig(bool val) { delta_sig = val; };
	void set_archive_offset_for_data(const infinint & val) { offset_for_data = val; };
	void set_storage_size_for_data(const infinint & val) { storage_size_for_data = val; };
	void set_archive_offset_for_EA(const infinint & val) { offset_for_EA = val; };
	void set_storage_size_for_EA(const infinint & val) { storage_size_for_EA = val; };
	void set_archive_offset_for_FSA(const infinint & val) { offset_for_FSA = val; };
	void set_storage_size_for_FSA(const infinint & val) { storage_size_for_FSA = val; };
	void set_ea(const ea_attributs & arg);
	void set_etiquette(const infinint & arg) { etiquette = arg; };
	void set_fsa_scope(const fsa_scope & arg) { fsa_sc = arg; };
	void set_data_crc(const crc & ptr);
	void set_delta_patch_base_crc(const crc & ptr);
	void set_delta_patch_result_crc(const crc & ptr);
	void set_empty_dir(bool val) { if(!is_dir()) throw SRC_BUG; empty_dir = val; };

	void clear();

    private:
	std::string my_name;
	bool hard_link;
	unsigned char type;
	infinint uid;
	infinint gid;
	U_16 perm;
	mutable bool full_date;
	datetime last_access;
	datetime last_modif;     ///< also used to store removal_date for removed entries
	saved_status data_status;
	ea_saved_status ea_status;
	datetime last_change;
	fsa_saved_status fsa_status;
	fsa_scope fsa_sc;
	infinint file_size;
	bool sparse_file;
	compression compression_algo;
	bool dirty;
	std::string target;    ///< target[0] used to store signature of removed entry for removed entries
	int major;
	int minor;
	range slices;
	bool delta_sig;
	infinint offset_for_data;
	infinint storage_size_for_data;
	infinint offset_for_EA;
	infinint storage_size_for_EA;
	infinint offset_for_FSA;
	infinint storage_size_for_FSA;
	std::deque<std::string> ea;
	mutable std::deque<std::string>::const_iterator it_ea;
	infinint etiquette;  ///< hard link identification
	std::string data_crc;
	std::string patch_base_crc;
	std::string patch_result_crc;
	bool empty_dir;

	static time_t datetime2time_t(const datetime & val);
    };

	/// @}

} // end of namespace

#endif
