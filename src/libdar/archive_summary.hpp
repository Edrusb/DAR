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

    /// \file archive_summary.hpp
    /// \brief datastructure returned by archive::summary_data
    /// \ingroup API


#ifndef ARCHIVE_SUMMARY_HPP
#define ARCHIVE_SUMMARY_HPP

#include "../my_config.h"
#include <string>
#include "infinint.hpp"
#include "header_version.hpp"
#include "cat_entree.hpp"

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
	const infinint & get_slice_number() const { return slice_number; };
	const infinint & get_archive_size() const { return archive_size; };
	const header_version & get_header() const { return header; };
	const infinint & get_catalog_size() const { return catalog_size; };
	const infinint & get_storage_size() const { return storage_size; };
	const infinint & get_data_size() const { return data_size; };
	const entree_stats & get_contents() const { return contents; };

	    // SETTINGS

	void set_slice_size(const infinint & arg) { slice_size = arg; };
	void set_first_slice_size(const infinint & arg) { first_slice_size = arg; };
	void set_last_slice_size(const infinint & arg) { last_slice_size = arg; };
	void set_slice_number(const infinint & arg) { slice_number = arg; };
	void set_archive_size(const infinint & arg) { archive_size = arg; };
	void set_header(const header_version & arg) { header = arg; };
	void set_catalog_size(const infinint & arg) { catalog_size = arg; };
	void set_storage_size(const infinint & arg) { storage_size = arg; };
	void set_data_size(const infinint & arg) { data_size = arg; };
	void set_contents(const entree_stats & arg) { contents = arg; };


	void clear();

    private:
	infinint slice_size;          ///< slice of the first slice or zero if not applicable
	infinint first_slice_size;    ///< slice of the middle slices or zero if not applicable
	infinint last_slice_size;     ///< slice of the last slice or zero if not applicable
	infinint slice_number;        ///< number of slices composing the archive of zero if unknown
	infinint archive_size;        ///< total size of the archive
	header_version header;        ///< information found in the archive header
	infinint catalog_size;        ///< catalogue size if known, zero if not
	infinint storage_size;        ///< amount of byte used to store (compressed/encrypted) data
	infinint data_size;           ///< amount of data saved (once uncompressed/unciphered)
	entree_stats contents;        ///< nature of saved files
    };

} // end of namespace

#endif
