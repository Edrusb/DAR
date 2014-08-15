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

    /// \file header_version.hpp
    /// \brief archive global header/trailer structure is defined here
    /// \ingroup Private

#ifndef HEADER_VERSION_HPP
#define HEADER_VERSION_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "tools.hpp"
#include "archive_version.hpp"
#include "on_pool.hpp"
#include "crypto.hpp"
#include "slice_layout.hpp"
#include "compressor.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// manages the archive header and trailer
    class header_version : public on_pool
    {
    public:
	header_version();
	header_version(const header_version & ref) { copy_from(ref); };
	const header_version & operator = (const header_version & ref) { detruit(); copy_from(ref); return * this; };
	~header_version() { detruit(); };

	    /// read the header or trailer from the archive
        void read(generic_file &f, user_interaction & dialog, bool lax_mode);

	    /// write down the object to the archive (as header if wrote at the beginning of the archive, as trailer is at the end)
        void write(generic_file &f) const;

	    // settings

	void set_edition(const archive_version & ed) { edition = ed; };
	void set_compression_algo(const compression & zip) { algo_zip = zip; };
	void set_command_line(const std::string & line) { cmd_line = line; };
	void set_initial_offset(const infinint & offset) { initial_offset = offset; };
	void set_sym_crypto_algo(const crypto_algo & algo) { sym = algo; };

	    /// the object pointed to by key is passed under the responsibility of this header_version object
	void set_crypted_key(memory_file *key) { if(key == NULL) throw SRC_BUG; clear_crypted_key(); crypted_key = key; };
	void clear_crypted_key() { if(crypted_key != NULL) { delete crypted_key; crypted_key = NULL; } };

	    /// the object pointed to by layout is passed under the responsibility of this header_version object
	void set_slice_layout(slice_layout *layout) { if(layout == NULL) throw SRC_BUG; clear_slice_layout(); ref_layout = layout; };
	void clear_slice_layout() { if(ref_layout != NULL) { delete ref_layout; ref_layout = NULL; } };

	void set_tape_marks(bool presence) { has_tape_marks = presence; };

	    // gettings

	const archive_version & get_edition() const { return edition; };
	compression get_compression_algo() const { return algo_zip; };
	const std::string & get_command_line() const { return cmd_line; };
	const infinint & get_initial_offset() const { return initial_offset; };

	bool is_ciphered() const { return ciphered || sym != crypto_none; };
	crypto_algo get_sym_crypto_algo() const { return sym; };
	memory_file *get_crypted_key() const { return crypted_key; };
	const slice_layout *get_slice_layout() const { return ref_layout; };
	bool get_tape_marks() const { return has_tape_marks; };

    private:
        archive_version edition; //< archive format
        compression algo_zip;    //< compression algorithm used
        std::string cmd_line;    //< used long ago to store cmd_line, then abandonned, then recycled as a user comment field
	infinint initial_offset; //< defines at which offset starts the archive (passed the archive header), this field is obiously only used in the trailer not in the header
	    // has to be set to zero when it is unknown, in that case this field is not dump to archive
	crypto_algo sym;         //< strong encryption algorithm used for symmetrical encryption
	memory_file *crypted_key;//< optional field containing the asymmetrically ciphered key used for strong encryption ciphering
	slice_layout *ref_layout;//< optional field used in isolated catalogues to record the slicing layout of their archive of reference
	bool has_tape_marks;     //< whether the archive contains tape marks aka escape marks aka sequence marks

	bool ciphered;   // whether the archive is ciphered, even if we do not know its crypto algorithm (old archives)

	void copy_from(const header_version & ref);
	void detruit();

	// FLAG VALUES

	static const U_I FLAG_SAVED_EA_ROOT = 0x80;      //< no more used since version "05"
	static const U_I FLAG_SAVED_EA_USER = 0x40;      //< no more used since version "05"
	static const U_I FLAG_SCRAMBLED     = 0x20;      //< scrambled or strong encryption used
	static const U_I FLAG_SEQUENCE_MARK = 0x10;      //< escape sequence marks present for sequential reading
	static const U_I FLAG_INITIAL_OFFSET = 0x08;     //< whether the header contains the initial offset (size of clear data before encrypted) NOTE : This value is set internally by header_version, no need to set flag with it! But that's OK to set it or not, it will be updated according to initial_offset's value.
	static const U_I FLAG_HAS_CRYPTED_KEY = 0x04;    //< the header contains a symmetrical key encrypted with asymmetrical algorithm
	static const U_I FLAG_HAS_REF_SLICING = 0x02;    //< the header contains the slicing information of the archive of reference (used for isolated catalogue)
	static const U_I FLAG_HAS_AN_EXTENDED_SIZE = 0x01; //< reserved for future use
    };

} // end of namespace

#endif
