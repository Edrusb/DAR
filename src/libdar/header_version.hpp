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

    /// \file header_version.hpp
    /// \brief archive global header/trailer structure is defined here
    /// \ingroup Private

#ifndef HEADER_VERSION_HPP
#define HEADER_VERSION_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "archive_version.hpp"
#include "crypto.hpp"
#include "slice_layout.hpp"
#include "compression.hpp"
#include "user_interaction.hpp"
#include "memory_file.hpp"
#include "archive_aux.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// manages the archive header and trailer
    class header_version
    {
    public:
	header_version();
	header_version(const header_version & ref) { copy_from(ref); };
	header_version(header_version && ref) noexcept { nullifyptr(); move_from(std::move(ref)); };
	header_version & operator = (const header_version & ref) { detruit(); copy_from(ref); return * this; };
	header_version & operator = (header_version && ref) noexcept { move_from(std::move(ref)); return *this; };
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

	    /// the object pointed to by key passes to the responsibility of this header_version object
	void set_crypted_key(memory_file *key) { if(key == nullptr) throw SRC_BUG; clear_crypted_key(); crypted_key = key; };
	void clear_crypted_key() { if(crypted_key != nullptr) { delete crypted_key; crypted_key = nullptr; } };

	    /// the object pointed to by layout is passed under the responsibility of this header_version object
	void set_slice_layout(slice_layout *layout) { if(layout == nullptr) throw SRC_BUG; clear_slice_layout(); ref_layout = layout; };
	void clear_slice_layout() { if(ref_layout != nullptr) { delete ref_layout; ref_layout = nullptr; } };

	void set_tape_marks(bool presence) { has_tape_marks = presence; };
	void set_signed(bool is_signed) { arch_signed = is_signed; };

	void set_salt(const std::string & arg) { salt = arg; has_kdf_params = true; };
	void set_iteration_count(const infinint & arg) { iteration_count = arg; has_kdf_params = true; };
	void set_kdf_hash(hash_algo algo);

	void set_compression_block_size(const infinint & bs) { compr_bs = bs; };

	    // gettings

	const archive_version & get_edition() const { return edition; };
	compression get_compression_algo() const { return algo_zip; };
	const std::string & get_command_line() const { return cmd_line; };
	const infinint & get_initial_offset() const { return initial_offset; };

	bool is_ciphered() const { return ciphered || sym != crypto_algo::none; };
	bool is_signed() const { return arch_signed; };
	crypto_algo get_sym_crypto_algo() const { return sym; };
	std::string get_sym_crypto_name() const;
	std::string get_asym_crypto_name() const;
	memory_file *get_crypted_key() const { return crypted_key; };
	const slice_layout *get_slice_layout() const { return ref_layout; };
	bool get_tape_marks() const { return has_tape_marks; };
	const std::string & get_salt() const { return salt; };
	const infinint & get_iteration_count() const { return iteration_count; };
	hash_algo get_kdf_hash() const { return kdf_hash; };
	const infinint & get_compression_block_size() const { return compr_bs; };

	    // display

	void display(user_interaction & dialg) const;

	    // clear

	void clear();

    private:
        archive_version edition; ///< archive format
        compression algo_zip;    ///< compression algorithm used
        std::string cmd_line;    ///< used long ago to store cmd_line, then abandonned, then recycled as a user comment field
	infinint initial_offset; ///< defines at which offset starts the archive (passed the archive header), this field is obiously only used in the trailer not in the header
	    // has to be set to zero when it is unknown, in that case this field is not dump to archive
	crypto_algo sym;         ///< strong encryption algorithm used for symmetrical encryption
	memory_file *crypted_key;///< optional field containing the asymmetrically ciphered key used for strong encryption ciphering
	slice_layout *ref_layout;///< optional field used in isolated catalogues to record the slicing layout of their archive of reference
	bool has_tape_marks;     ///< whether the archive contains tape marks aka escape marks aka sequence marks

	bool ciphered;           ///< whether the archive is ciphered, even if we do not know its crypto algorithm (old archives)
	bool arch_signed;        ///< whether the archive is signed
	bool has_kdf_params;     ///< has salt/ineration/kdf_hash fields valid
	std::string salt;        ///< used for key derivation
	infinint iteration_count;///< used for key derivation
	hash_algo kdf_hash;      ///< used for key derivation
	infinint compr_bs;       ///< the compression block size (0 for legacy compression mode)

	void nullifyptr() noexcept { crypted_key = nullptr; ref_layout = nullptr; };
	void copy_from(const header_version & ref);
	void move_from(header_version && ref) noexcept;
	void detruit();


	static constexpr U_I PRE_FORMAT_10_ITERATION = 2000;   ///< fixed value used for key derivation before archive format 10

    };

} // end of namespace

#endif
