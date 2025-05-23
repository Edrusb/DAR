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

    /// \file archive_summary.hpp
    /// \brief datastructure returned by archive::summary_data
    /// \ingroup API


#ifndef ARCHIVE_SUMMARY_HPP
#define ARCHIVE_SUMMARY_HPP

#include "../my_config.h"
#include <string>
#include "infinint.hpp"
#include "entree_stats.hpp"

namespace libdar
{

	/// \addtogroup API
        /// @{


	/// the archive_summary class provides information about a given archive

    class archive_summary
    {
    public:
	archive_summary() { clear(); };
	archive_summary(const archive_summary & ref) = default;
	archive_summary(archive_summary && ref) noexcept = default;
	archive_summary & operator = (const archive_summary & ref) = default;
	archive_summary & operator = (archive_summary && ref) noexcept = default;
	~archive_summary() = default;

	    // GETTINGS

	const infinint & get_slice_size() const { return slice_size; };
	const infinint & get_first_slice_size() const { return first_slice_size; };
	const infinint & get_last_slice_size() const { return last_slice_size; };
	const infinint & get_ref_slice_size() const { return ref_slice_size; };
	const infinint & get_ref_first_slice_size() const { return ref_first_slice_size; };
	const infinint & get_slice_number() const { return slice_number; };
	const infinint & get_archive_size() const { return archive_size; };
	const infinint & get_catalog_size() const { return catalog_size; };
	const infinint & get_storage_size() const { return storage_size; };
	const infinint & get_data_size() const { return data_size; };
	const entree_stats & get_contents() const { return contents; };
	const std::string & get_edition() const { return edition; };
	const std::string & get_compression_algo() const { return algo_zip; };
	const std::string & get_user_comment() const { return user_comment; };
	const std::string & get_cipher() const { return cipher; };
	const std::string & get_asym() const { return asym; };
	bool get_signed() const { return is_signed; };
	bool get_tape_marks() const { return tape_marks; };
	const std::string & get_in_place() const { return in_place; };
	const infinint & get_compression_block_size() const { return compr_block_size; };
	const std::string & get_salt() const { return salt; };
	const infinint & get_iteration_count() const { return iteration_count; };
	const std::string & get_kdf_hash() const { return kdf_hash; };


	    // SETTINGS

	void set_slice_size(const infinint & arg) { slice_size = arg; };
	void set_first_slice_size(const infinint & arg) { first_slice_size = arg; };
	void set_last_slice_size(const infinint & arg) { last_slice_size = arg; };
	void set_ref_slice_size(const infinint & arg) { ref_slice_size = arg; };
	void set_ref_first_slice_size(const infinint & arg) { ref_first_slice_size = arg; };
	void set_slice_number(const infinint & arg) { slice_number = arg; };
	void set_archive_size(const infinint & arg) { archive_size = arg; };
	void set_catalog_size(const infinint & arg) { catalog_size = arg; };
	void set_storage_size(const infinint & arg) { storage_size = arg; };
	void set_data_size(const infinint & arg) { data_size = arg; };
	void set_contents(const entree_stats & arg) { contents = arg; };
	void set_edition(const std::string & arg) { edition = arg; };
	void set_compression_algo(const std::string & arg) { algo_zip = arg; };
	void set_user_comment(const std::string & arg) { user_comment = arg; };
	void set_cipher(const std::string & arg) { cipher = arg; };
	void set_asym(const std::string & arg) { asym = arg; };
	void set_signed(bool arg) { is_signed = arg; };
	void set_tape_marks(bool arg) { tape_marks = arg; };
	void set_in_place(const std::string & arg) { in_place = arg; };
	void set_compression_block_size(const infinint & arg) { compr_block_size = arg; };
	void set_salt(const std::string & arg) { salt = arg; };
	void set_iteration_count(const infinint & arg) { iteration_count = arg; };
	void set_kdf_hash(const std::string & arg) { kdf_hash = arg; };

	void clear();

    private:
	infinint slice_size;          ///< slice of the middle slice or zero if not applicable
	infinint first_slice_size;    ///< slice of the first slices or zero if not applicable
	infinint last_slice_size;     ///< slice of the last slice or zero if not applicable
	infinint ref_slice_size;      ///< slice of the slice of the archive of reference
	infinint ref_first_slice_size;///< slice of the first slice of the archive of reference
	infinint slice_number;        ///< number of slices composing the archive of zero if unknown
	infinint archive_size;        ///< total size of the archive
	infinint catalog_size;        ///< catalogue size if known, zero if not
	infinint storage_size;        ///< amount of byte used to store (compressed/encrypted) data
	infinint data_size;           ///< amount of data saved (once uncompressed/unciphered)
	entree_stats contents;        ///< nature of saved files
	std::string edition;          ///< archive format
	std::string algo_zip;         ///< compression algorithm
	std::string user_comment;     ///< user comment
	std::string cipher;           ///< encryption algorithm
	std::string asym;             ///< asymetrical encryption
	bool is_signed;               ///< whether the archive is signed
	bool tape_marks;              ///< whether the archive has tape marks (for sequential reading)
	std::string in_place;         ///< in_place path empty string if absent
	infinint compr_block_size;    ///< compression block size, or zero if stream compression is used
	std::string salt;             ///< the salt
	infinint iteration_count;     ///< iteration count for KDF routine
	std::string kdf_hash;         ///< kdf hash algo
    };

} // end of namespace

#endif
