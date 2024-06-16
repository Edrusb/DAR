/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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

    /// \file archive_options.hpp
    /// \brief this file contains a set of classes used to transmit options to archive operation
    /// \ingroup API

#ifndef ARCHIVE_OPTIONS_HPP
#define ARCHIVE_OPTIONS_HPP

#include "../my_config.h"
#include "crypto.hpp"
#include "integers.hpp"
#include "mask.hpp"
#include "mask_list.hpp"
#include "crit_action.hpp"
#include "secu_string.hpp"
#include "entrepot.hpp"
#include "fsa_family.hpp"
#include "compile_time_features.hpp"
#include "archive_aux.hpp"
#include "compression.hpp"
#include "delta_sig_block_size.hpp"
#include "filesystem_ids.hpp"

#include <string>
#include <vector>
#include <set>
#include <memory>

namespace libdar
{
    class archive; // needed to be able to use pointer on archive object.


	/////////////////////////////////////////////////////////
	////////////// OPTIONS FOR OPENNING AN ARCHIVE //////////
	/////////////////////////////////////////////////////////

	/// \addtogroup API
	/// @{


	/// class holding optional parameters used to read an existing archive

    class archive_options_read
    {
    public:
	    /// build an object and set options to their default values
	archive_options_read();

	    /// the copy constructor, assignment operator and destructor
	archive_options_read(const archive_options_read & ref) : x_ref_chem(ref.x_ref_chem) { copy_from(ref); };
	archive_options_read(archive_options_read && ref) noexcept;
	archive_options_read & operator = (const archive_options_read & ref) { copy_from(ref); return *this; };
	archive_options_read & operator = (archive_options_read && ref) noexcept { move_from(std::move(ref)); return *this; };
	~archive_options_read() = default;


	    /////////////////////////////////////////////////////////////////////
	    // set back to default (this is the state just after the object is constructed
	    // this method is to be used to reuse a given object

	    /// reset all the options to their default values
	void clear();


	    /////////////////////////////////////////////////////////////////////
	    // setting methods


	    /// defines the the crypto cypher to use to read the archive (default is crypto_none)

	    /// \note since release 2.5.0 you may and should provide crypto_none in any case (the default value)
	    /// for libdar uses the algorithm stored in the archive. However you way override this
	    /// (in case of corruption for example) by explicitely specifying a crypto algorithm
	    /// in that case the value of the crypto algorithm stored in the archive is ignored
	    /// and the algorithm used to decipher is the one specified here.
	void set_crypto_algo(crypto_algo val) { x_crypto = val; };

	    /// defines the password or passphrase to decrypt (unused if encryption is not set)
	void set_crypto_pass(const secu_string & pass) { x_pass = pass; };

	    /// the encryption block size to use to decrypt
	void set_crypto_size(U_32 crypto_size) { x_crypto_size = crypto_size; };

	    /// set the encryption block size to the default value
	void set_default_crypto_size();

	    /// set the name of the input pipe to read data from (when basename is set to "-")

	    /// if input_pipe is set to "" (empty string) the information from dar_slave are expected
	    /// in standard input else the given string
	void set_input_pipe(const std::string & input_pipe) { x_input_pipe = input_pipe; };

	    /// set the name of the output pipe to send orders to (when basenale is set to "-")

	    /// if output_pipe is set to "" the orders sent to dar_slave will exit by the standard output else the given string
	    /// must be the path to a named pipe which will relay the orders to dar_slave
	void set_output_pipe(const std::string & output_pipe) { x_output_pipe = output_pipe; };

	    /// set the command to execute before reading each slice (empty string for no script)

	    /// several macros are available:
	    /// - %%n : the slice number to be read
	    /// - %%N : the slice number with padded zeros according to slice_min_digits given option
	    /// - %%b : the archive basename
	    /// - %%p : the slices path
	    /// - %%e : the archive extension (usually "dar")
	    /// - %%c : the archive context ("init", "operation" or "last_slice") see dar(1) man page for more details
	    /// - %%% : substitued by %%
	    /// .
	void set_execute(const std::string & execute) { x_execute = execute; };

	    /// defines whether the user needs detailed output of the operation
	void set_info_details(bool info_details) { x_info_details = info_details; };

	    /// defines whether any archive coherence error, system error or media error lead to the abortion of the operation

	    /// lax mode is false by default.
	    /// setting it to true, may allow more data to be restored, but may lead the user to get corrupted data
	    /// the user will be warned and asked upon what to do if such case arrives.
	void set_lax(bool val) { x_lax = val; };

	    /// defines whether to try reading the archive sequentially (ala tar) or using the final catalogue

	    /// the sequential reading must not has been disabled at creation time and the archive must be of minimum format "08" for the operation not to fail

	void set_sequential_read(bool val) { x_sequential_read = val; };

	    /// defines the minimum digit a slice must have concerning its number, zeros will be prepended as much as necessary to respect this

	void set_slice_min_digits(infinint val) { x_slice_min_digits = val; };


	    /// defines the protocol to use to retrieve slices
	void set_entrepot(const std::shared_ptr<entrepot> & entr) { if(!entr) throw Erange("archive_options_read::set_entrepot", "null entrepot pointer given in argument"); x_entrepot = entr; };

	    /// whether to warn (true) or ignore (false) signature failure (default is true)
	void set_ignore_signature_check_failure(bool val) { x_ignore_signature_check_failure = val; };

	    /// whether libdar is allowed to create several thread to work possibly faster on multicore CPU (need libthreadar to be effective)

	    /// \deprecated this call is deprecated, see set_multi_threaded_*() more specific calls
	    /// \note setting this to true is equivalent to calling set_mutli_threaded_crypto(2)
	void set_multi_threaded(bool val) { x_multi_threaded_crypto = 2; x_multi_threaded_compress = 2; };

	    /// how much thread libdar will use for cryptography (need libthreadar to be effective)
	void set_multi_threaded_crypto(U_I num) { x_multi_threaded_crypto = num; };

	    /// how much thread libdar will use for compression (need libthreadar too and compression_block_size > 0)
	void set_multi_threaded_compress(U_I num) { x_multi_threaded_compress = num; };

	    /// whether we only read the archive header and exit
	void set_header_only(bool val) { x_header_only = val; };

	    /// whether to avoid display low importance messages
	void set_silent(bool val) { x_silent = val; };

	    /// whether to perform early memory release of the catalogue
	void set_early_memory_release(bool val) { x_early_memory_release = val; };

	    /// whether to ask the first slice in place of the last slice when reading an archive with the help of an isolated catalogue
	void set_force_first_slice(bool val) { x_force_first_slice = val; };


	    //////// what follows concerne the use of an external catalogue instead of the archive's internal one

	    /// defines whether or not to use the catalogue from an extracted catalogue (instead of the one embedded in the archive) and which one to use
	void set_external_catalogue(const path & ref_chem, const std::string & ref_basename) { x_ref_chem = ref_chem, x_ref_basename = ref_basename; external_cat = true; };
	    /// clear any reference to an external catalogue
	void unset_external_catalogue();

	    /// defines the crypto algo for the reference catalogue
	void set_ref_crypto_algo(crypto_algo ref_crypto) { x_ref_crypto = ref_crypto; };

	    /// defines the pass for the reference catalogue
	void set_ref_crypto_pass(const secu_string & ref_pass) { x_ref_pass = ref_pass; };

	    /// defines the crypto size for the reference catalogue
	void set_ref_crypto_size(U_32 ref_crypto_size) { x_ref_crypto_size = ref_crypto_size; };

	    /// set the command to execute before reading each slice of the reference catalogue

	    /// several macros are available:
	    /// - %%n : the slice number to be read
	    /// - %%N : the slice number with padded zeros according to slice_min_digits given option
	    /// - %%b : the archive basename
	    /// - %%p : the slices path
	    /// - %%e : the archive extension (usually "dar")
	    /// - %%c : the archive context ("init", "operation" or "last_slice") see dar(1) man page for more details
	    /// - %%% : substitued by %%
	    /// .
	void set_ref_execute(const std::string & ref_execute) { x_ref_execute = ref_execute; };


	    /// defines the minim digit for slice number of the archive of reference (where the external catalogue is read from)

	void set_ref_slice_min_digits(infinint val) { x_ref_slice_min_digits = val; };

	    /// defines the protocol to use to retrieve slices of the reference archive (where the external catalogue resides)
	void set_ref_entrepot(const std::shared_ptr<entrepot> & entr) { if(!entr) throw Erange("archive_options_read::set_ref_entrepot", "null entrepot pointer given in argument"); x_ref_entrepot = entr; };


	    /////////////////////////////////////////////////////////////////////
	    // getting methods (mainly used inside libdar, but kept public and part of the API in the case it is needed)


	crypto_algo get_crypto_algo() const { return x_crypto; };
	const secu_string & get_crypto_pass() const { return x_pass; };
	U_32 get_crypto_size() const { return x_crypto_size; };
	const std::string & get_input_pipe() const { return x_input_pipe; };
	const std::string & get_output_pipe() const { return x_output_pipe; };
	const std::string & get_execute() const { return x_execute; };
	bool get_info_details() const { return x_info_details; };
	bool get_lax() const { return x_lax; };
	bool get_sequential_read() const { return x_sequential_read; };
	infinint get_slice_min_digits() const { return x_slice_min_digits; };
	const std::shared_ptr<entrepot> & get_entrepot() const { return x_entrepot; };
	bool get_ignore_signature_check_failure() const { return x_ignore_signature_check_failure; };
	U_I get_multi_threaded_crypto() const { return x_multi_threaded_crypto; };
	U_I get_multi_threaded_compress() const { return x_multi_threaded_compress; };
	bool get_header_only() const { return x_header_only; };
	bool get_silent() const { return x_silent; };
	bool get_early_memory_release() const { return x_early_memory_release; };
	bool get_force_first_slice() const { return x_force_first_slice; };

	    // All methods that follow concern the archive where to fetch the (isolated) catalogue from
	bool is_external_catalogue_set() const { return external_cat; };
	const path & get_ref_path() const;
	const std::string & get_ref_basename() const;
	crypto_algo get_ref_crypto_algo() const { return x_ref_crypto; };
	const secu_string & get_ref_crypto_pass() const { return x_ref_pass; };
	U_32 get_ref_crypto_size() const { return x_ref_crypto_size; };
	const std::string & get_ref_execute() const { return x_ref_execute; };
	infinint get_ref_slice_min_digits() const { return x_ref_slice_min_digits; };
	const std::shared_ptr<entrepot> & get_ref_entrepot() const { return x_ref_entrepot; };

    private:
	crypto_algo x_crypto;
	secu_string x_pass;
	U_32 x_crypto_size;
	std::string x_input_pipe;
	std::string x_output_pipe;
	std::string x_execute;
	bool x_info_details;
	bool x_lax;
	bool x_sequential_read;
	infinint x_slice_min_digits;
	std::shared_ptr<entrepot> x_entrepot;
	bool x_ignore_signature_check_failure;
	U_I x_multi_threaded_crypto;
	U_I x_multi_threaded_compress;
	bool x_header_only;
	bool x_silent;
	bool x_early_memory_release;
	bool x_force_first_slice;

	    // external catalogue relative fields
	bool external_cat;
	path x_ref_chem;
	std::string x_ref_basename;
	crypto_algo x_ref_crypto;
	secu_string x_ref_pass;
	U_32 x_ref_crypto_size;
	std::string x_ref_execute;
	infinint x_ref_slice_min_digits;
	std::shared_ptr<entrepot> x_ref_entrepot;

	void copy_from(const archive_options_read & ref);
	void move_from(archive_options_read && ref) noexcept;
    };


	/////////////////////////////////////////////////////////
	///////// OPTIONS FOR CREATING AN ARCHIVE ///////////////
	/////////////////////////////////////////////////////////

	/// class holding optional parameters used to create an archive

    class archive_options_create
    {
    public:
	    // default constructors and destructor.

	archive_options_create();
	archive_options_create(const archive_options_create & ref);
	archive_options_create(archive_options_create && ref) noexcept { nullifyptr(); move_from(std::move(ref)); };
	archive_options_create & operator = (const archive_options_create & ref) { destroy(); copy_from(ref); return *this; };
	archive_options_create & operator = (archive_options_create && ref) noexcept { move_from(std::move(ref)); return *this; };
	~archive_options_create() { destroy(); };

	    /////////////////////////////////////////////////////////////////////
	    // set back to default (this is the state just after the object is constructed
	    // this method is to be used to reuse a given object

	    /// reset all the options to their default values
	void clear();


	    /////////////////////////////////////////////////////////////////////
	    // setting methods

	    /// set the archive to take as reference (nullptr for a full backup)
	void set_reference(std::shared_ptr<archive> ref_arch) { x_ref_arch = ref_arch; };

	    /// defines the filenames to only save (except directory) as those that match the given mask
	void set_selection(const mask & selection);

	    /// defines the directories and files to consider

	    /// \note WARNING: this mask will be applied to the absolute path of files being proceeded.
	    /// We speak here about the root of the filesystem under which the fs_root directory contains
	    /// the files to backup. This is independent from the fs_root argument of class archive
	    /// constructor which objective is to reduce the perimeter of the backup. The subtree filters
	    /// do not compare only to the path inside the fs_root directory but to the full path,
	    /// including the fs_root directory. In other words, if the subtree mask do not accept
	    /// anything under fs_root path, the resulting backup will be empty.
	void set_subtree(const mask & subtree);

	    /// defines whether overwritting is allowed or not
	void set_allow_over(bool allow_over) { x_allow_over = allow_over; };

	    /// defines whether a warning shall be issued before overwriting
	void set_warn_over(bool warn_over) { x_warn_over = warn_over; };

	    /// defines whether the user needs detailed output of the operation

	    /// \note in API 5.5.x and before this switch drove the displaying
	    /// of processing messages and treated files. now it only drives the
	    /// display of processing messages, use set_display_treated to define
	    /// whether files under treatement should be display or not
	void set_info_details(bool info_details) { x_info_details = info_details; };

	    /// defines whether to show treated files

	    /// \param[in] display_treated true to display processed inodes
	    /// \param[in] only_dir only display the current directory under process, not its individual files
	void set_display_treated(bool display_treated, bool only_dir) { x_display_treated = display_treated; x_display_treated_only_dir = only_dir; };

	    /// whether to display files that have been excluded by filters
	void set_display_skipped(bool display_skipped) { x_display_skipped = display_skipped; };

	    /// whether to display a summary for each completed directory with total saved data and compression ratio
	void set_display_finished(bool display_finished) { x_display_finished = display_finished; };

	    /// set a pause beteween slices. Set to zero does not pause at all, set to 1 makes libdar pauses each slice, set to 2 makes libdar pause each 2 slices and so on.
	void set_pause(const infinint & pause) { x_pause = pause; };

	    /// defines whether we need to store ignored directories as empty
	void set_empty_dir(bool empty_dir) { x_empty_dir = empty_dir; };

	    /// set the compression algorithm to be used
	void set_compression(compression compr_algo) { x_compr_algo = compr_algo; };

	    /// set the compression level (from 1 to 9)
	void set_compression_level(U_I compression_level) { x_compression_level = compression_level; };

	    /// set the compression block size

	    /// \param[in] compression_block_size if set to zero (which is the default value)
	    /// compression is performed without block in one single stream per file.
	    /// This is the historical way used by libdar, it gives the best result
	    /// and the lowest compute overhead, though it cannot be parallelized.
	    /// At the opposite using compresion per block reduce the compression ratio
	    /// but allows the block to be compressed/decompressed in parallel and thus
	    /// leverage multi-core systems. When the block size increase you tend to the
	    /// same compression ratio as compression ration without block
	void set_compression_block_size(U_I compression_block_size) { x_compression_block_size = compression_block_size; };

	    /// define the archive slicing

	    /// \param[in] file_size set the slice size in byte (0 for a single slice whatever its size is)
	    /// \param[in] first_file_size set the first file size
	    /// \note if not specified or set to zero, first_file_size is considered equal to file_size
	void set_slicing(const infinint & file_size, const infinint & first_file_size = 0)
	{
	    x_file_size = file_size;
	    if(first_file_size.is_zero())
		x_first_file_size = file_size;
	    else
		x_first_file_size = first_file_size;
	};


	    /// defines which Extended Attributes to save
	void set_ea_mask(const mask & ea_mask);

	    /// set the command to execute after each slice creation

	    /// several macros are available:
	    /// - %%n : the number of the just created slice
	    /// - %%N : the slice number with padded zeros according to slice_min_digits given option
	    /// - %%b : the archive basename
	    /// - %%p : the slices path
	    /// - %%e : the archive extension (usually "dar")
	    /// - %%c : the archive context ("init", "operation" or "last_slice") see dar(1) man page for more details
	    /// - %%% : substitued by %%
	    /// .
	void set_execute(const std::string & execute) { x_execute = execute; };

	    /// set the cypher to use
	void set_crypto_algo(crypto_algo crypto) { x_crypto = crypto; };

	    /// set the pass the password / passphrase to use. Giving an empty string makes the password asked
	    /// interactively through the dialog argument if encryption has been set.
	void set_crypto_pass(const secu_string & pass) { x_pass = pass; };

	    /// set the size of the encryption by block to use
	void set_crypto_size(U_32 crypto_size) { x_crypto_size = crypto_size; };

	    /// set the list of recipients that will be able to read the archive

	    /// \note this is based on GnuPG keyring and assumes the user running libdar has its keyring
	    /// containing for each recipient a valid public key. If a list of recipient is given the crypto_pass
	    /// (see above) is not used, but the crypto_algo stays used to encrypt the archive using a randomly generated key
	    /// which is encrypted using the public keys of the recipients and dropped that way encrypted inside the archive.
	    /// \note if crypto_algo is not set while a list of recipient is given, the crypto algo will default to blowfish
	    /// \note since release 2.7.0 if a given std::string in the list contains an '@' the string is assumed to be an
	    /// email and search is done in the keyring for that field type, else it is assumed to be a keyid.
	void set_gnupg_recipients(const std::vector<std::string> & gnupg_recipients) { x_gnupg_recipients = gnupg_recipients; };


	    /// the private keys matching the email or the keyid of the provided list are used to sign the archive random key
	    /// \note since release 2.7.0 if a given std::string in the list contains an '@' the string is assumed to be an
	    /// email and search is done in the keyring for that field type, else it is assumed to be a keyid.
	void set_gnupg_signatories(const std::vector<std::string> & gnupg_signatories) { x_gnupg_signatories = gnupg_signatories; };

	    /// defines files to compress
	void set_compr_mask(const mask & compr_mask);

	    /// defines file size under which to never compress
	void set_min_compr_size(const infinint & min_compr_size) { x_min_compr_size = min_compr_size; };

	    /// defines whether to ignore files with the nodump flag set
	void set_nodump(bool nodump) { x_nodump = nodump; };

	    /// defines whether to ignore files having a given EA
	    /// \note if ea_name is set to "" the default ea_name "user.libdar_no_backup" is used.
	void set_exclude_by_ea(const std::string & ea_name)
	{ exclude_by_ea = (ea_name == "" ? "user.libdar_no_backup" : ea_name); };

	    /// set the fields to consider when comparing inodes with reference archive (see comparison_fields enumeration in catalogue.hpp)
	void set_what_to_check(comparison_fields what_to_check) { x_what_to_check = what_to_check; };

	    /// ignore differences of at most this integer number of hours while looking for changes in dates
	void set_hourshift(const infinint & hourshift) { x_hourshift = hourshift; };

	    /// whether to make a dry-run operation
	void set_empty(bool empty) { x_empty = empty; };

	    /// whether to alter atime or ctime in the filesystem when reading files to save

	    /// \param[in] alter_atime whether to change atime (true) or ctime (false)
	    /// \note this parameter is used only when furtive_read_mode() is not activated
	void set_alter_atime(bool alter_atime)
	{
	    if(x_furtive_read)
		x_old_alter_atime = alter_atime;
	    else
		x_alter_atime = alter_atime;
	};

	    /// whether to use furtive read mode (if activated, alter_atime() has no meaning/use)
	void set_furtive_read_mode(bool furtive_read);

	    /// whether to limit the backup to files located on the same filesystem as the directory taken as root of the backup

	    /// \note if using this method, all value set using set_same_fs_include and set_same_fs_exclude so far are
	    /// forgotten and ignored.
	void set_same_fs(bool same_fs) { x_same_fs = same_fs; x_same_fs_include.clear(); x_same_fs_exclude.clear(); };

	    /// files on the filesystem pointed to by the given path will be considered for the backup operation if not excluded by set_same_fs_exclude()

	    /// \note if using this method any previous call to the legacy set_same_fs(bool) invocation is ignored
	void set_same_fs_include(const std::string & included_path_to_fs) { x_same_fs_include.push_back(included_path_to_fs); };

	    /// files on the filesystem pointed to by the given path will not be considered for backup operation

	    /// \note if using this method any previous call to the legacy set_same_fs(bool) invocation is ignored
	void set_same_fs_exclude(const std::string & excluded_path_to_fs) { x_same_fs_exclude.push_back(excluded_path_to_fs); };

	    /// whether to make an emtpy archive only referencing the current state of files in the filesystem
	void set_snapshot(bool snapshot) { x_snapshot = snapshot; };

	    /// whether to consider the Cache Directory Tagging Standard
	void set_cache_directory_tagging(bool cache_directory_tagging) { x_cache_directory_tagging = cache_directory_tagging; };

	    /// whether to ignore any archive of reference and only save file which modification is more recent that the given "fixed_date". To not use this feature set fixed_date value of zero (which is the value by default)
	void set_fixed_date(const infinint & fixed_date) { x_fixed_date = fixed_date; };

	    /// if not an empty string set the slice permission according to the octal value given.
	void set_slice_permission(const std::string & slice_permission) { x_slice_permission = slice_permission; };

	    /// if not an empty string set the user ownership of slices accordingly
	void set_slice_user_ownership(const std::string & slice_user_ownership) { x_slice_user_ownership = slice_user_ownership; };

	    /// if not an empty string set the group ownership of slices accordingly
	void set_slice_group_ownership(const std::string & slice_group_ownership) { x_slice_group_ownership = slice_group_ownership; };

	    /// how much time to retry saving a file if it changed while being read
	void set_retry_on_change(const infinint & count_max_per_file, const infinint & global_max_byte_overhead = 0) { x_repeat_count = count_max_per_file; x_repeat_byte = global_max_byte_overhead; };

	    /// whether to add escape sequence aka tape marks to allow sequential reading of the archive
	void set_sequential_marks(bool sequential) { x_sequential_marks = sequential; };

	    /// whether to try to detect sparse files
	void set_sparse_file_min_size(const infinint & size) { x_sparse_file_min_size = size; };

	    /// whether to check for ctime changes since with the archive of reference
	void set_security_check(bool check) { x_security_check = check; };

	    /// specify a user comment in the archive (always in clear text!)
	void set_user_comment(const std::string & comment) { x_user_comment = comment; };

	    /// specify whether to produce a hash file of the slice and which hash algo to use

	    /// \note the libdar::hash_algo data type is defined in hash_fichier.hpp, valid values
	    /// are for examle libdar::hash_none, libdar::hash_md5, libdar::hash_sha1, libdar::hash_sha512...
	void set_hash_algo(hash_algo hash);

	    /// defines the minimum digit a slice must have concerning its number, zeros will be prepended as much as necessary to respect this
	void set_slice_min_digits(infinint val) { x_slice_min_digits = val; };

	    /// defines the backup hook for files
	void set_backup_hook(const std::string & execute, const mask & which_files);

	    /// whether to ignore unknown inode types instead of issuing a warning
	void set_ignore_unknown_inode_type(bool val) { x_ignore_unknown = val; };

	    /// defines the protocol to use for slices
	void set_entrepot(const std::shared_ptr<entrepot> & entr) { if(!entr) throw Erange("archive_options_create::set_entrepot", "null entrepot pointer given in argument"); x_entrepot = entr; };

	    /// defines the FSA (Filesystem Specific Attribute) to only consider (by default all FSA activated at compilation time are considered)
	void set_fsa_scope(const fsa_scope & scope) { x_scope = scope; };

	    /// whether libdar is allowed to spawn several threads to possibily work faster on multicore CPU (requires libthreadar)

	    /// \deprecated this call is deprecated, see set_multi_threaded_*() more specific calls
	    /// \note setting this to true is equivalent to calling set_mutli_threaded_crypto(2)
	void set_multi_threaded(bool val) { x_multi_threaded_crypto = 2; x_multi_threaded_compress = 2; };

	    /// how much thread libdar will use for cryptography (need libthreadar to be effective)
	void set_multi_threaded_crypto(U_I num) { x_multi_threaded_crypto = num; };

		    /// how much thread libdar will use for compression (need libthreadar too and compression_block_size > 0)
	void set_multi_threaded_compress(U_I num) { x_multi_threaded_compress = num; };

	    /// whether binary delta has to be computed for differential/incremental backup

	    /// \note this requires delta signature to be present in the archive of reference
	void set_delta_diff(bool val) { if(val && !compile_time::librsync()) throw Ecompilation("librsync"); x_delta_diff = val; };

	    /// whether signature to base binary delta on the future has to be calculated and stored beside saved files
	void set_delta_signature(bool val) { x_delta_signature = val; };

	    /// whether to derogate to defaut delta file consideration while calculation delta signatures
	void set_delta_mask(const mask & delta_mask);

	    /// whether to never calculate delta signature for files which size is smaller or equal to the given argument

	    /// \note by default a min size of 10 kiB is used
	void set_delta_sig_min_size(const infinint & val) { x_delta_sig_min_size = val; };

	    /// whether to automatically zeroing negative dates read from the filesystem (just warn, don't ask whether to pursue)
	void set_auto_zeroing_neg_dates(bool val) { x_auto_zeroing_neg_dates = val; };

	    /// provide a list of full path which if are symlinks will be considered as the inode they point to

	    /// \note this is espetially intended for use with symlinks pointing to directories
	    /// to have dar recursing in such pointed to directory instead of just recording that directory
	void set_ignored_as_symlink(const std::set<std::string> & list) { x_ignored_as_symlink = list; };

	    /// defines when to resave a file's data which inode metadata changed

	void set_modified_data_detection(modified_data_detection val) { x_modified_data_detection = val; };

	    /// key derivation
	void set_iteration_count(const infinint & val) { x_iteration_count = val; };

	    /// hash algo used for key derivation
	void set_kdf_hash(hash_algo algo) { x_kdf_hash = algo; };

	    /// block size to use to build delta signatures
	void set_sig_block_len(delta_sig_block_size val) { val.check(); x_sig_block_len = val; };

	    /// never try resaving uncompressed when compression ratio is bad
	void set_never_resave_uncompressed(bool val) { x_never_resave_uncompressed = val; };

	    /////////////////////////////////////////////////////////////////////
	    // getting methods

	std::shared_ptr<archive> get_reference() const { return x_ref_arch; };
	const mask & get_selection() const { if(x_selection == nullptr) throw SRC_BUG; return *x_selection; };
	const mask & get_subtree() const { if(x_subtree == nullptr) throw SRC_BUG; return *x_subtree; };
	bool get_allow_over() const { return x_allow_over; };
	bool get_warn_over() const { return x_warn_over; };
	bool get_info_details() const { return x_info_details; };
	bool get_display_treated() const { return x_display_treated; };
	bool get_display_treated_only_dir() const { return x_display_treated_only_dir; };
	bool get_display_skipped() const { return x_display_skipped; };
	bool get_display_finished() const { return x_display_finished; };
	const infinint & get_pause() const { return x_pause; };
	bool get_empty_dir() const { return x_empty_dir; };
	compression get_compression() const { return x_compr_algo; };
	U_I get_compression_level() const { return x_compression_level; };
	U_I get_compression_block_size() const { return x_compression_block_size; };
	const infinint & get_slice_size() const { return x_file_size; };
	const infinint & get_first_slice_size() const { return x_first_file_size; };
	const mask & get_ea_mask() const { if(x_ea_mask == nullptr) throw SRC_BUG; return *x_ea_mask; };
	const std::string & get_execute() const { return x_execute; };
	crypto_algo get_crypto_algo() const { return x_crypto; };
	const secu_string & get_crypto_pass() const { return x_pass; };
	U_32 get_crypto_size() const { return x_crypto_size; };
	const std::vector<std::string> & get_gnupg_recipients() const { return x_gnupg_recipients; };
	const std::vector<std::string> & get_gnupg_signatories() const { return x_gnupg_signatories; };
	const mask & get_compr_mask() const { if(x_compr_mask == nullptr) throw SRC_BUG; return *x_compr_mask; };
	const infinint & get_min_compr_size() const { return x_min_compr_size; };
	bool get_nodump() const { return x_nodump; };
	const std::string & get_exclude_by_ea() const { return exclude_by_ea; };
	comparison_fields get_comparison_fields() const { return x_what_to_check; };
	const infinint & get_hourshift() const { return x_hourshift; };
	bool get_empty() const { return x_empty; };
	bool get_alter_atime() const { return x_alter_atime; };
	bool get_furtive_read_mode() const { return x_furtive_read; };
	bool get_same_fs() const { return x_same_fs; };
	std::deque<std::string> get_same_fs_include() const { return x_same_fs_include; };
	std::deque<std::string> get_same_fs_exclude() const { return x_same_fs_exclude; };
	bool get_snapshot() const { return x_snapshot; };
	bool get_cache_directory_tagging() const { return x_cache_directory_tagging; };
	const infinint & get_fixed_date() const { return x_fixed_date; };
	const std::string & get_slice_permission() const { return x_slice_permission; };
	const std::string & get_slice_user_ownership() const { return x_slice_user_ownership; };
	const std::string & get_slice_group_ownership() const { return x_slice_group_ownership; };
	const infinint & get_repeat_count() const { return x_repeat_count; };
	const infinint & get_repeat_byte() const { return x_repeat_byte; };
	bool get_sequential_marks() const { return x_sequential_marks; };
	infinint get_sparse_file_min_size() const { return x_sparse_file_min_size; };
	bool get_security_check() const { return  x_security_check; };
	const std::string & get_user_comment() const { return x_user_comment; };
	hash_algo get_hash_algo() const { return x_hash; };
	infinint get_slice_min_digits() const { return x_slice_min_digits; };
	const std::string & get_backup_hook_file_execute() const { return x_backup_hook_file_execute; };
	const mask & get_backup_hook_file_mask() const { return *x_backup_hook_file_mask; };
	bool get_ignore_unknown_inode_type() const { return x_ignore_unknown; };
	const std::shared_ptr<entrepot> & get_entrepot() const { return x_entrepot; };
	const fsa_scope & get_fsa_scope() const { return x_scope; };
	U_I get_multi_threaded_crypto() const { return x_multi_threaded_crypto; };
	U_I get_multi_threaded_compress() const { return x_multi_threaded_compress; };
	bool get_delta_diff() const { return x_delta_diff; };
	bool get_delta_signature() const { return x_delta_signature; };
	const mask & get_delta_mask() const { return *x_delta_mask; }
	bool get_has_delta_mask_been_set() const { return has_delta_mask_been_set; };
	const infinint & get_delta_sig_min_size() const { return x_delta_sig_min_size; };
	bool get_auto_zeroing_neg_dates() const { return x_auto_zeroing_neg_dates; };
	const std::set<std::string> & get_ignored_as_symlink() const { return x_ignored_as_symlink; };
	modified_data_detection get_modified_data_detection() const { return x_modified_data_detection; };
	const infinint & get_iteration_count() const { return x_iteration_count; };
	hash_algo get_kdf_hash() const { return x_kdf_hash; };
	delta_sig_block_size get_sig_block_len() const { return x_sig_block_len; };
	bool get_never_resave_uncompressed() const { return x_never_resave_uncompressed; };

    private:
	std::shared_ptr<archive> x_ref_arch; ///< just contains the address of an existing object, no local copy of object is done here
	mask * x_selection;  ///< points to a local copy of mask (must be allocated / releases by the archive_option_create object)
	mask * x_subtree;    ///< points to a local copy of mask (must be allocated / releases by the archive_option_create objects)
	bool x_allow_over;
	bool x_warn_over;
	bool x_info_details;
	bool x_display_treated;
	bool x_display_treated_only_dir;
	bool x_display_skipped;
	bool x_display_finished;
	infinint x_pause;
	bool x_empty_dir;
	compression x_compr_algo;
	U_I x_compression_level;
	U_I x_compression_block_size;
	infinint x_file_size;
	infinint x_first_file_size;
	mask * x_ea_mask;    ///< points to a local copy of mask (must be allocated / releases by the archive_option_create objects)
	std::string x_execute;
	crypto_algo x_crypto;
	secu_string x_pass;
	U_32 x_crypto_size;
	std::vector<std::string> x_gnupg_recipients;
	std::vector<std::string> x_gnupg_signatories;
	mask * x_compr_mask; ///< points to a local copy of mask (must be allocated / releases by the archive_option_create objects)
	infinint x_min_compr_size;
	bool x_nodump;
	std::string exclude_by_ea;
	comparison_fields x_what_to_check;
	infinint x_hourshift;
	bool x_empty;
	bool x_alter_atime;
	bool x_old_alter_atime; ///< used to backup origina alter_atime value when activating furtive read mode
	bool x_furtive_read;
	bool x_same_fs;
	std::deque<std::string> x_same_fs_include;
	std::deque<std::string> x_same_fs_exclude;
	bool x_snapshot;
	bool x_cache_directory_tagging;
	infinint x_fixed_date;
	std::string x_slice_permission;
	std::string x_slice_user_ownership;
	std::string x_slice_group_ownership;
	infinint x_repeat_count;
	infinint x_repeat_byte;
	bool x_sequential_marks;
	infinint x_sparse_file_min_size;
	bool x_security_check;
	std::string x_user_comment;
	hash_algo x_hash;
	infinint x_slice_min_digits;
	mask * x_backup_hook_file_mask;
	std::string x_backup_hook_file_execute;
	bool x_ignore_unknown;
	std::shared_ptr<entrepot> x_entrepot;
	fsa_scope x_scope;
	U_I x_multi_threaded_crypto;
	U_I x_multi_threaded_compress;
	bool x_delta_diff;
	bool x_delta_signature;
	mask *x_delta_mask;
	bool has_delta_mask_been_set;
	infinint x_delta_sig_min_size;
	bool x_auto_zeroing_neg_dates;
	std::set<std::string> x_ignored_as_symlink;
	modified_data_detection x_modified_data_detection;
	infinint x_iteration_count;
	hash_algo x_kdf_hash;
	delta_sig_block_size x_sig_block_len;
	bool x_never_resave_uncompressed;

	void nullifyptr() noexcept;
	void destroy() noexcept;
	void copy_from(const archive_options_create & ref);
	void move_from(archive_options_create && ref) noexcept;
	void destroy_mask(mask * & ptr);
	void clean_mask(mask * & ptr);
	void check_mask(const mask & m);
    };






	/////////////////////////////////////////////////////////
	//////////// OPTIONS FOR ISOLATING A CATALOGUE //////////
	/////////////////////////////////////////////////////////

    	/// class holding optional parameters used to isolate an existing archive

    class archive_options_isolate
    {
    public:
	archive_options_isolate();
	archive_options_isolate(const archive_options_isolate & ref);
	archive_options_isolate(archive_options_isolate && ref) noexcept { nullifyptr(); move_from(std::move(ref)); };
	archive_options_isolate & operator = (const archive_options_isolate & ref) { destroy(); copy_from(ref); return *this; };
	archive_options_isolate & operator = (archive_options_isolate && ref) noexcept { move_from(std::move(ref)); return *this; };
	~archive_options_isolate() { destroy(); };


	void clear();

	    /////////////////////////////////////////////////////////////////////
	    // setting methods

	    /// whether overwritting is allowed
	void set_allow_over(bool allow_over) { x_allow_over = allow_over; };

	    /// whether a warning shall be issued before overwriting
	void set_warn_over(bool warn_over) { x_warn_over = warn_over; };

	    /// whether the user needs detailed output of the operation
	void set_info_details(bool info_details) { x_info_details = info_details; };

	    /// Pause beteween slices. Set to zero does not pause at all, set to 1 makes libdar pauses each slice, set to 2 makes libdar pause each 2 slices and so on.
	void set_pause(const infinint & pause) { x_pause = pause; };

	    /// the compression algorithm used
	void set_compression(compression algo) { x_algo = algo; };

	    /// the compression level (from 1 to 9)
	void set_compression_level(U_I compression_level) { x_compression_level = compression_level; };

		    /// set the compression block size

	void set_compression_block_size(U_I compression_block_size) { x_compression_block_size = compression_block_size; };

	    /// define the archive slicing

	    /// \param[in] file_size set the slice size in byte (0 for a single slice whatever its size is)
	    /// \param[in] first_file_size set the first file size
	    /// \note if not specified or set to zero, first_file_size is considered equal to file_size
	void set_slicing(const infinint & file_size, const infinint & first_file_size = 0)
	{
	    x_file_size = file_size;
	    if(first_file_size.is_zero())
		x_first_file_size = file_size;
	    else
		x_first_file_size = first_file_size;
	};

	    /// command to execute after each slice creation

	    /// several macros are available:
	    /// - %%n : the number of the just created slice
	    /// - %%N : the slice number with padded zeros according to slice_min_digits given option
	    /// - %%b : the archive basename
	    /// - %%p : the slices path
	    /// - %%e : the archive extension (usually "dar")
	    /// - %%c : the archive context ("init", "operation" or "last_slice") see dar(1) man page for more details
	    /// - %%% : substitued by %%
	    /// .
	void set_execute(const std::string & execute) { x_execute = execute; };

	    /// cypher to use
	void set_crypto_algo(crypto_algo crypto) { x_crypto = crypto; };

	    /// password / passphrase to encrypt the data with (empty string for interactive question)
	void set_crypto_pass(const secu_string & pass) { x_pass = pass; };

	    /// size of the encryption by block to use
	void set_crypto_size(U_32 crypto_size) { x_crypto_size = crypto_size; };

	    /// set the list of recipients that will be able to read the archive
	    /// \note more details for the same option of archive_options_create
	    /// \note since release 2.7.0 if a given std::string in the list contains an '@' the string is assumed to be an
	    /// email and search is done in the keyring for that field type, else it is assumed to be a keyid.
	void set_gnupg_recipients(const std::vector<std::string> & gnupg_recipients) { x_gnupg_recipients = gnupg_recipients; };

	    /// the private keys matching the email of the provided list are used to sign the archive random key
	    /// \note since release 2.7.0 if a given std::string in the list contains an '@' the string is assumed to be an
	    /// email and search is done in the keyring for that field type, else it is assumed to be a keyid.
	void set_gnupg_signatories(const std::vector<std::string> & gnupg_signatories) { x_gnupg_signatories = gnupg_signatories; };

	    /// whether to make a dry-run operation
	void set_empty(bool empty) { x_empty = empty; };

	    /// if not an empty string set the slice permission according to the octal value given.
	void set_slice_permission(const std::string & slice_permission) { x_slice_permission = slice_permission; };

	    /// if not an empty string set the user ownership of slices accordingly
	void set_slice_user_ownership(const std::string & slice_user_ownership) { x_slice_user_ownership = slice_user_ownership; };

	    /// if not an empty string set the group ownership of slices accordingly
	void set_slice_group_ownership(const std::string & slice_group_ownership) { x_slice_group_ownership = slice_group_ownership; };

	    /// specify a user comment in the archive (always in clear text!)
	void set_user_comment(const std::string & comment) { x_user_comment = comment; };

	    /// specify whether to produce a hash file of the slice and which hash algo to use
	void set_hash_algo(hash_algo hash);

	    /// defines the minimum digit a slice must have concerning its number, zeros will be prepended as much as necessary to respect this
	void set_slice_min_digits(infinint val) { x_slice_min_digits = val; };

	    /// whether to add escape sequence aka tape marks to allow sequential reading of the archive
	void set_sequential_marks(bool sequential) { x_sequential_marks = sequential; };

	    /// defines the protocol to use for slices
	void set_entrepot(const std::shared_ptr<entrepot> & entr) { if(!entr) throw Erange("archive_options_isolated::set_entrepot", "null entrepot pointer given in argument"); x_entrepot = entr; };

	    /// whether libdar is allowed to created several thread to work possibily faster on multicore CPU (require libthreadar)

	    /// \deprecated this call is deprecated, see set_multi_threaded_*() more specific calls
	    /// \note setting this to true is equivalent to calling set_mutli_threaded_crypto(2)
	void set_multi_threaded(bool val) { x_multi_threaded_crypto = 2; x_multi_threaded_compress = 2; };

	    /// how much thread libdar will use for cryptography (need libthreadar to be effective)
	void set_multi_threaded_crypto(U_I num) { x_multi_threaded_crypto = num; };

	    /// how much thread libdar will use for compression (need libthreadar too and compression_block_size > 0)
	void set_multi_threaded_compress(U_I num) { x_multi_threaded_compress = num; };


	    /// whether signature to base binary delta on the future has to be calculated and stored beside saved files
	void set_delta_signature(bool val) { x_delta_signature = val; };

	    /// whether to derogate to defaut delta file consideration while calculation delta signatures
	void set_delta_mask(const mask & delta_mask);

	    /// whether to never calculate delta signature for files which size is smaller or equal to the given argument
	    ///
	    /// \note by default a min size of 10 kiB is used
	void set_delta_sig_min_size(const infinint & val) { x_delta_sig_min_size = val; };

	    /// key derivation
	void set_iteration_count(const infinint & val) { x_iteration_count = val; };

	    /// hash algo used for key derivation
	void set_kdf_hash(hash_algo algo) { x_kdf_hash = algo; };

	    /// block size to use to build delta signatures
	void set_sig_block_len(delta_sig_block_size val) { val.check(); x_sig_block_len = val; };

	    /////////////////////////////////////////////////////////////////////
	    // getting methods

	bool get_allow_over() const { return x_allow_over; };
	bool get_warn_over() const { return x_warn_over; };
	bool get_info_details() const { return x_info_details; };
	const infinint & get_pause() const { return x_pause; };
	compression get_compression() const { return x_algo; };
	U_I get_compression_level() const { return x_compression_level; };
	U_I get_compression_block_size() const { return x_compression_block_size; };
	const infinint & get_slice_size() const { return x_file_size; };
	const infinint & get_first_slice_size() const { return x_first_file_size; };
	const std::string & get_execute() const { return x_execute; };
	crypto_algo get_crypto_algo() const { return x_crypto; };
	const secu_string & get_crypto_pass() const { return x_pass; };
	U_32 get_crypto_size() const { return x_crypto_size; };
	const std::vector<std::string> & get_gnupg_recipients() const { return x_gnupg_recipients; };
	const std::vector<std::string> & get_gnupg_signatories() const { return x_gnupg_signatories; };
	bool get_empty() const { return x_empty; };
	const std::string & get_slice_permission() const { return x_slice_permission; };
	const std::string & get_slice_user_ownership() const { return x_slice_user_ownership; };
	const std::string & get_slice_group_ownership() const { return x_slice_group_ownership; };
	const std::string & get_user_comment() const { return x_user_comment; };
	hash_algo get_hash_algo() const { return x_hash; };
	infinint get_slice_min_digits() const { return x_slice_min_digits; };
	bool get_sequential_marks() const { return x_sequential_marks; };
	const std::shared_ptr<entrepot> & get_entrepot() const { return x_entrepot; };
	U_I get_multi_threaded_crypto() const { return x_multi_threaded_crypto; };
	U_I get_multi_threaded_compress() const { return x_multi_threaded_compress; };
	bool get_delta_signature() const { return x_delta_signature; };
	const mask & get_delta_mask() const { return *x_delta_mask; }
	bool get_has_delta_mask_been_set() const { return has_delta_mask_been_set; };
	const infinint & get_delta_sig_min_size() const { return x_delta_sig_min_size; };
	const infinint & get_iteration_count() const { return x_iteration_count; };
	hash_algo get_kdf_hash() const { return x_kdf_hash; };
	delta_sig_block_size get_sig_block_len() const { return x_sig_block_len; };


    private:
	bool x_allow_over;
	bool x_warn_over;
	bool x_info_details;
	infinint x_pause;
	compression x_algo;
	U_I x_compression_level;
	U_I x_compression_block_size;
	infinint x_file_size;
	infinint x_first_file_size;
	std::string x_execute;
	crypto_algo x_crypto;
	secu_string x_pass;
	U_32 x_crypto_size;
	std::vector<std::string> x_gnupg_recipients;
	std::vector<std::string> x_gnupg_signatories;
	bool x_empty;
	std::string x_slice_permission;
	std::string x_slice_user_ownership;
	std::string x_slice_group_ownership;
	std::string x_user_comment;
	hash_algo x_hash;
	infinint x_slice_min_digits;
	bool x_sequential_marks;
	std::shared_ptr<entrepot> x_entrepot;
	U_I x_multi_threaded_crypto;
	U_I x_multi_threaded_compress;
	bool x_delta_signature;
	mask *x_delta_mask;
	bool has_delta_mask_been_set;
	infinint x_delta_sig_min_size;
	infinint x_iteration_count;
	hash_algo x_kdf_hash;
	delta_sig_block_size x_sig_block_len;

	void copy_from(const archive_options_isolate & ref);
	void move_from(archive_options_isolate && ref) noexcept;
	void destroy() noexcept;
	void nullifyptr() noexcept;
    };



	/////////////////////////////////////////////////////////
	////////// OPTONS FOR MERGING ARCHIVES //////////////////
	/////////////////////////////////////////////////////////

    	/// class holding optional parameters used to proceed to the merge operation
    class archive_options_merge
    {
    public:

	archive_options_merge() { nullifyptr(); clear(); };
	archive_options_merge(const archive_options_merge & ref) { copy_from(ref); };
	archive_options_merge(archive_options_merge && ref) noexcept { nullifyptr(); move_from(std::move(ref)); };
	archive_options_merge & operator = (const archive_options_merge & ref) { destroy(); copy_from(ref); return *this; };
	archive_options_merge & operator = (archive_options_merge && ref) noexcept { move_from(std::move(ref)); return *this; };
	~archive_options_merge() { destroy(); };

	void clear();

	    /////////////////////////////////////////////////////////////////////
	    // setting methods

	void set_auxiliary_ref(std::shared_ptr<archive> ref) { x_ref = ref; };

	    /// defines the filenames to only save (except directory) as those that match the given mask
	void set_selection(const mask & selection);

	    /// defines the directories and files to consider

	    /// \note WARNING: The filter mechanism used here is common to all operations and works
	    /// by comparing full paths, while in the other hand, paths stored in the libdar archive are
	    /// all relative to what was provided as fs_root at backup time. To have this filter mecanism
	    /// working in the context of merging, where no fs_root can be provided to transform relative
	    /// paths to absolute paths, libdar will emulate an fs_root with the pseudo root value of the
	    /// path::FAKE_ROOT field. This is invisible form the libdar API user except when relying on
	    /// mask_list objects, which cannot thus receive full path as those would not be prepended
	    /// by path::FAKE_ROOT as fs_root value, because fs_root prefixing is only performed for relative
	    /// paths.
	void set_subtree(const mask & subtree);

	    /// defines whether overwritting is allowed or not
	void set_allow_over(bool allow_over) { x_allow_over = allow_over; };

	    /// defines whether a warning shall be issued before overwriting
	void set_warn_over(bool warn_over) { x_warn_over = warn_over; };

	    /// policy to solve merging conflict
	void set_overwriting_rules(const crit_action & overwrite);

	    /// defines whether the user needs detailed output of the operation

	    /// \note in API 5.5.x and before this switch drove the displaying
	    /// of processing messages and treated files. now it only drives the
	    /// display of processing messages, use set_display_treated to define
	    /// whether files under treatement should be display or not
	void set_info_details(bool info_details) { x_info_details = info_details; };

	    /// defines whether to show treated files

	    /// \param[in] display_treated true to display processed inodes
	    /// \param[in] only_dir only display the current directory under processing, not its individual files
	void set_display_treated(bool display_treated, bool only_dir) { x_display_treated = display_treated; x_display_treated_only_dir = only_dir; };

	    /// whether to display files that have been excluded by filters
	void set_display_skipped(bool display_skipped) { x_display_skipped = display_skipped; };

	    /// set a pause beteween slices. Set to zero does not pause at all, set to 1 makes libdar pauses each slice, set to 2 makes libdar pause each 2 slices and so on.
	void set_pause(const infinint & pause) { x_pause = pause; };

	    /// defines whether we need to store ignored directories as empty
	void set_empty_dir(bool empty_dir) { x_empty_dir = empty_dir; };

	    /// set the compression algorithm to be used
	void set_compression(compression compr_algo) { x_compr_algo = compr_algo; };

	    /// set the compression level (from 1 to 9)
	void set_compression_level(U_I compression_level) { x_compression_level = compression_level; };

	    /// set the compression block size (0 for streamed compression)
	void set_compression_block_size(U_I compression_block_size) { x_compression_block_size = compression_block_size; };

	    /// define the archive slicing

	    /// \param[in] file_size set the slice size in byte (0 for a single slice whatever its size is)
	    /// \param[in] first_file_size set the first file size
	    /// \note if not specified or set to zero, first_file_size is considered equal to file_size
	void set_slicing(const infinint & file_size, const infinint & first_file_size = 0)
	{
	    x_file_size = file_size;
	    if(first_file_size.is_zero())
		x_first_file_size = file_size;
	    else
		x_first_file_size = first_file_size;
	};

	    /// defines which Extended Attributes to save
	void set_ea_mask(const mask & ea_mask);

	    /// set the command to execute after each slice creation

	    /// several macros are available:
	    /// - %%n : the number of the just created slice
	    /// - %%N : the slice number with padded zeros according to slice_min_digits given option
	    /// - %%b : the archive basename
	    /// - %%p : the slices path
	    /// - %%e : the archive extension (usually "dar")
	    /// - %%c : the archive context ("init", "operation" or "last_slice") see dar(1) man page for more details
	    /// - %%% : substitued by %%
	    /// .
	void set_execute(const std::string & execute) { x_execute = execute; };

	    /// set the cypher to use
	void set_crypto_algo(crypto_algo crypto) { x_crypto = crypto; };

	    /// set the pass the password / passphrase to use. Giving an empty string makes the password asked
	    /// interactively through the dialog argument if encryption has been set.
	void set_crypto_pass(const secu_string & pass) { x_pass = pass; };

	    /// set the size of the encryption by block to use
	void set_crypto_size(U_32 crypto_size) { x_crypto_size = crypto_size; };

	    /// set the list of recipients that will be able to read the archive
	    /// \note more details for the same option of archive_options_create
	    /// \note since release 2.7.0 if a given std::string in the list contains an '@' the string is assumed to be an
	    /// email and search is done in the keyring for that field type, else it is assumed to be a keyid.
	void set_gnupg_recipients(const std::vector<std::string> & gnupg_recipients) { x_gnupg_recipients = gnupg_recipients; };

	    /// the private keys matching the email of the provided list are used to sign the archive random key
	    /// \note since release 2.7.0 if a given std::string in the list contains an '@' the string is assumed to be an
	    /// email and search is done in the keyring for that field type, else it is assumed to be a keyid.
	void set_gnupg_signatories(const std::vector<std::string> & gnupg_signatories) { x_gnupg_signatories = gnupg_signatories; };

	    /// defines files to compress
	void set_compr_mask(const mask & compr_mask);

	    /// defines file size under which to never compress
	void set_min_compr_size(const infinint & min_compr_size) { x_min_compr_size = min_compr_size; };

	    /// defines whether we do a dry-run execution
	void set_empty(bool empty) { x_empty = empty; };

	    /// make dar ignore the 'algo' argument and do not uncompress / compress files that are selected for merging
	void set_keep_compressed(bool keep_compressed) { x_keep_compressed = keep_compressed; };

	    /// if not an empty string set the slice permission according to the octal value given.
	void set_slice_permission(const std::string & slice_permission) { x_slice_permission = slice_permission; };

	    /// if not an empty string set the user ownership of slices accordingly
	void set_slice_user_ownership(const std::string & slice_user_ownership) { x_slice_user_ownership = slice_user_ownership; };

	    /// if not an empty string set the group ownership of slices accordingly
	void set_slice_group_ownership(const std::string & slice_group_ownership) { x_slice_group_ownership = slice_group_ownership; };

	    /// if set to true use a merging mode suitable to build a decremental backup from two full backups (see Notes)
	void set_decremental_mode(bool mode) { x_decremental = mode; };

	    /// whether to activate escape sequence aka tape marks to allow sequential reading of the archive
	void set_sequential_marks(bool sequential) { x_sequential_marks = sequential; };

	    /// whether to try to detect sparse files
	void set_sparse_file_min_size(const infinint & size) { x_sparse_file_min_size = size; };

	    /// specify a user comment in the archive (always in clear text!)
	void set_user_comment(const std::string & comment) { x_user_comment = comment; };

	    /// specify whether to produce a hash file of the slice and which hash algo to use
	void set_hash_algo(hash_algo hash);

	    /// defines the minimum digit a slice must have concerning its number, zeros will be prepended as much as necessary to respect this
	void set_slice_min_digits(infinint val) { x_slice_min_digits = val; };

	    /// defines the protocol to use for slices
	void set_entrepot(const std::shared_ptr<entrepot> & entr) { if(!entr) throw Erange("archive_options_merge::set_entrepot", "null entrepot pointer given in argument"); x_entrepot = entr; };

	    /// defines the FSA (Filesystem Specific Attribute) to only consider (by default all FSA are considered)
	void set_fsa_scope(const fsa_scope & scope) { x_scope = scope; };

	    /// whether libdar is allowed to spawn several threads to possibily work faster on multicore CPU (requires libthreadar)

	    /// \deprecated this call is deprecated, see set_multi_threaded_*() more specific calls
	    /// \note setting this to true is equivalent to calling set_mutli_threaded_crypto(2)
	void set_multi_threaded(bool val) { x_multi_threaded_crypto = 2; x_multi_threaded_compress = 2; };

	    /// how much thread libdar will use for cryptography (need libthreadar to be effective)
	void set_multi_threaded_crypto(U_I num) { x_multi_threaded_crypto = num; };

	    /// how much thread libdar will use for compression (need libthreadar too and compression_block_size > 0)
	void set_multi_threaded_compress(U_I num) { x_multi_threaded_compress = num; };

	    /// whether signature to base binary delta on the future has to be calculated and stored beside saved files
	    /// \note the default is true, which lead to preserve delta signature over merging, but not to calculate new ones
	    /// unless a mask is given to set_delta_mask() in which case signature are dropped / preserved / added in regard to
	    /// this mask. If set_delta_signature() is false, providing a mask has no effect, no signature will be transfered nor
	    /// calculated in the resulting merged archive.
	void set_delta_signature(bool val) { x_delta_signature = val; };

	    /// whether to derogate to defaut delta file consideration while calculation delta signatures
	void set_delta_mask(const mask & delta_mask);

	    /// whether to never calculate delta signature for files which size is smaller or equal to the given argument

	    /// \note by default a min size of 10 kiB is used
	void set_delta_sig_min_size(const infinint & val) { x_delta_sig_min_size = val; };

	    /// key derivation
	void set_iteration_count(const infinint & val) { x_iteration_count = val; };

	    /// hash algo used for key derivation
	void set_kdf_hash(hash_algo algo) { x_kdf_hash = algo; };

	    /// block size to use to build delta signatures
	void set_sig_block_len(delta_sig_block_size val) { val.check(); x_sig_block_len = val; };

	    /// never try resaving uncompressed when compression ratio is bad
	void set_never_resave_uncompressed(bool val) { x_never_resave_uncompressed = val; };


	    /////////////////////////////////////////////////////////////////////
	    // getting methods

	std::shared_ptr<archive>  get_auxiliary_ref() const { return x_ref; };
	const mask & get_selection() const { if(x_selection == nullptr) throw SRC_BUG; return *x_selection; };
	const mask & get_subtree() const { if(x_subtree == nullptr) throw SRC_BUG; return *x_subtree; };
	bool get_allow_over() const { return x_allow_over; };
	bool get_warn_over() const { return x_warn_over; };
	const crit_action & get_overwriting_rules() const { if(x_overwrite == nullptr) throw SRC_BUG; return *x_overwrite; };
	bool get_info_details() const { return x_info_details; };
	bool get_display_treated() const { return x_display_treated; };
	bool get_display_treated_only_dir() const { return x_display_treated_only_dir; };
	bool get_display_skipped() const { return x_display_skipped; };
	const infinint & get_pause() const { return x_pause; };
	bool get_empty_dir() const { return x_empty_dir; };
	compression get_compression() const { return x_compr_algo; };
	U_I get_compression_level() const { return x_compression_level; };
	U_I get_compression_block_size() const { return x_compression_block_size; };
	const infinint & get_slice_size() const { return x_file_size; };
	const infinint & get_first_slice_size() const { return x_first_file_size; };
	const mask & get_ea_mask() const { if(x_ea_mask == nullptr) throw SRC_BUG; return *x_ea_mask; };
	const std::string & get_execute() const { return x_execute; };
	crypto_algo get_crypto_algo() const { return x_crypto; };
	const secu_string & get_crypto_pass() const { return x_pass; };
	U_32 get_crypto_size() const { return x_crypto_size; };
	const std::vector<std::string> & get_gnupg_recipients() const { return x_gnupg_recipients; };
	const std::vector<std::string> & get_gnupg_signatories() const { return x_gnupg_signatories; };
	const mask & get_compr_mask() const { if(x_compr_mask == nullptr) throw SRC_BUG; return *x_compr_mask; };
	const infinint & get_min_compr_size() const { return x_min_compr_size; };
	bool get_empty() const { return x_empty; };
	bool get_keep_compressed() const { return x_keep_compressed; };
	const std::string & get_slice_permission() const { return x_slice_permission; };
	const std::string & get_slice_user_ownership() const { return x_slice_user_ownership; };
	const std::string & get_slice_group_ownership() const { return x_slice_group_ownership; };
	bool get_decremental_mode() const { return x_decremental; };
	bool get_sequential_marks() const { return x_sequential_marks; };
	infinint get_sparse_file_min_size() const { return x_sparse_file_min_size; };
	const std::string & get_user_comment() const { return x_user_comment; };
	hash_algo get_hash_algo() const { return x_hash; };
	infinint get_slice_min_digits() const { return x_slice_min_digits; };
	const std::shared_ptr<entrepot> & get_entrepot() const { return x_entrepot; };
	const fsa_scope & get_fsa_scope() const { return x_scope; };
	U_I get_multi_threaded_crypto() const { return x_multi_threaded_crypto; };
	U_I get_multi_threaded_compress() const { return x_multi_threaded_compress; };
	bool get_delta_signature() const { return x_delta_signature; };
	const mask & get_delta_mask() const { return *x_delta_mask; }
	bool get_has_delta_mask_been_set() const { return has_delta_mask_been_set; };
	const infinint & get_delta_sig_min_size() const { return x_delta_sig_min_size; };
	const infinint & get_iteration_count() const { return x_iteration_count; };
	hash_algo get_kdf_hash() const { return x_kdf_hash; };
	delta_sig_block_size get_sig_block_len() const { return x_sig_block_len; };
	bool get_never_resave_uncompressed() const { return x_never_resave_uncompressed; };

    private:
	std::shared_ptr<archive> x_ref;
	mask * x_selection;
	mask * x_subtree;
	bool x_allow_over;
	bool x_warn_over;
	crit_action * x_overwrite;
	bool x_info_details;
	bool x_display_treated;
	bool x_display_treated_only_dir;
	bool x_display_skipped;
	infinint x_pause;
	bool x_empty_dir;
	compression x_compr_algo;
	U_I x_compression_level;
	U_I x_compression_block_size;
	infinint x_file_size;
	infinint x_first_file_size;
	mask * x_ea_mask;
	std::string x_execute;
	crypto_algo x_crypto;
	secu_string x_pass;
	U_32 x_crypto_size;
	std::vector<std::string> x_gnupg_recipients;
	std::vector<std::string> x_gnupg_signatories;
	mask * x_compr_mask;
	infinint x_min_compr_size;
	bool x_empty;
	bool x_keep_compressed;
	std::string x_slice_permission;
	std::string x_slice_user_ownership;
	std::string x_slice_group_ownership;
	bool x_decremental;
	bool x_sequential_marks;
	infinint x_sparse_file_min_size;
	std::string x_user_comment;
	hash_algo x_hash;
	infinint x_slice_min_digits;
	std::shared_ptr<entrepot> x_entrepot;
	fsa_scope x_scope;
	U_I x_multi_threaded_crypto;
	U_I x_multi_threaded_compress;
	bool x_delta_signature;
	mask *x_delta_mask;
	bool has_delta_mask_been_set;
	infinint x_delta_sig_min_size;
	infinint x_iteration_count;
	hash_algo x_kdf_hash;
	delta_sig_block_size x_sig_block_len;
	bool x_never_resave_uncompressed;

	void destroy() noexcept;
	void copy_from(const archive_options_merge & ref);
	void move_from(archive_options_merge && ref) noexcept;
	void nullifyptr() noexcept;
    };


	/////////////////////////////////////////////////////////
	////////// OPTONS FOR EXTRACTING FILES FROM ARCHIVE /////
	/////////////////////////////////////////////////////////

    	/// class holding optional parameters used to extract files from an existing archive
    class archive_options_extract
    {
    public:
	enum t_dirty { dirty_ignore, dirty_warn, dirty_ok };

	archive_options_extract() { nullifyptr(); clear(); };
	archive_options_extract(const archive_options_extract & ref) { copy_from(ref); };
	archive_options_extract(archive_options_extract && ref) noexcept { nullifyptr(); move_from(std::move(ref)); };
	archive_options_extract & operator = (const archive_options_extract & ref) { destroy(); copy_from(ref); return *this; };
	archive_options_extract & operator = (archive_options_extract && ref) noexcept { move_from(std::move(ref)); return *this; };
	~archive_options_extract() { destroy(); };

	void clear();

	    /////////////////////////////////////////////////////////////////////
	    // setting methods

	    /// defines the filenames to only save (except directory) as those that match the given mask
	void set_selection(const mask & selection);

	    /// defines the directories and files to consider

	    /// \note WARNING: The filter mechanism used here is common to all operations and works
	    /// by comparing full paths, while in the other hand, paths stored in the libdar archive are
	    /// all relative paths to what was provided as fs_root at backup time. To have this filter mecanism
	    /// working in the context of restoration, libdar prepends the relative path found in the libdar
	    /// archive with  the fs_root argument given to archive::op_extract.
	    /// In consequence the provided filter here, for extraction operation should be build taking into
	    /// account that files to restore will be seen as subdirectories of this provided "fs_root" where
	    /// the data will be restored. In other words, if the subtree mask do not accept anything under
	    /// fs_root path, the resulting backup will be empty.
	void set_subtree(const mask & subtree);

	    /// defines whether a warning shall be issued before overwriting
	void set_warn_over(bool warn_over) { x_warn_over = warn_over; };

	    /// defines whether the user needs detailed output of the operation
	    ///
	    /// \note in API 5.5.x and before this switch drove the displaying
	    /// of processing messages and treated files. now it only drives the
	    /// display of processing messages, use set_display_treated to define
	    /// whether files under treatement should be display or not
	void set_info_details(bool info_details) { x_info_details = info_details; };

	    /// defines whether to show treated files
	    /// \param[in] display_treated true to display processed inodes
	    /// \param[in] only_dir only display the current directory under processing, not its individual files
	void set_display_treated(bool display_treated, bool only_dir) { x_display_treated = display_treated; x_display_treated_only_dir = only_dir; };

	    /// whether to display files that have been excluded by filters
	void set_display_skipped(bool display_skipped) { x_display_skipped = display_skipped; };

	    /// defines which Extended Attributes to save
	void set_ea_mask(const mask & ea_mask);

	    /// whether to ignore directory structure and restore all files in the same directory
	void set_flat(bool flat) { x_flat = flat; };

	    /// fields to consider when comparing inodes with those on filesystem to determine if it is more recent (see comparison_fields enumeration), also defines whether mtime has to be restored (cf_mtime) whether permission have to be too (cf_ignore_owner) whether ownership has to be restored too (cf_all)
	void set_what_to_check(comparison_fields what_to_check) { x_what_to_check = what_to_check; };

	    /// whether a warning must be issue if a file to remove does not match the expected type of file
	void set_warn_remove_no_match(bool warn_remove_no_match) { x_warn_remove_no_match = warn_remove_no_match; };

	    /// defines whether we need to store ignored directories as empty
	void set_empty(bool empty) { x_empty = empty; };

	    /// whether to restore directories where no file has been triggered for backup (no file/inode change, excluded files,...)
	void set_empty_dir(bool empty_dir) { x_empty_dir = empty_dir; };

	    /// whether to restore dirty files (those that changed during backup), warn before restoring or ignoring them

	    ///\param[in] ignore if set to true, dirty files are not restored at all
	    ///\param[in] warn useless if ignored is false. If warn is true, a warning is issued before restoring each dirty file (default behavior)
	void set_dirty_behavior(bool ignore, bool warn) { x_dirty = ignore ? dirty_ignore : (warn ? dirty_warn : dirty_ok); };

	    /// alternative method to modify dirty behavior
	void set_dirty_behavior(t_dirty val) { x_dirty = val; };

	    /// overwriting policy
	void set_overwriting_rules(const crit_action & over);

	    /// only consider deleted files (if set, no data get restored)

	    /// \note setting both set_only_deleted() and set_ignore_deleted() will not restore anything, almost equivalent to a dry-run execution
	void set_only_deleted(bool val) { x_only_deleted = val; };


	    /// do not consider deleted files (if set, no inode will be removed)

	    /// \note setting both set_only_deleted() and set_ignore_deleted() will not restore anything, almost equivalent to a dry-run execution
	void set_ignore_deleted(bool val) { x_ignore_deleted = val; };

	    /// defines the FSA (Filesystem Specific Attribute) to only consider (by default all FSA activated at compilation time are considered)
	void set_fsa_scope(const fsa_scope & scope) { x_scope = scope; };

	    /// whether to ignore unix sockets while restoring
	void set_ignore_unix_sockets(bool val) { x_ignore_unix_sockets = val; };

	    /// whether to ignore fs_root and use in-place path stored in the archive
	void set_in_place(bool arg) { x_in_place = arg; };


	    /////////////////////////////////////////////////////////////////////
	    // getting methods

	const mask & get_selection() const { if(x_selection == nullptr) throw SRC_BUG; return *x_selection; };
	const mask & get_subtree() const { if(x_subtree == nullptr) throw SRC_BUG; return *x_subtree; };
	bool get_warn_over() const { return x_warn_over; };
	bool get_info_details() const { return x_info_details; };
	bool get_display_treated() const { return x_display_treated; };
	bool get_display_treated_only_dir() const { return x_display_treated_only_dir; };
	bool get_display_skipped() const { return x_display_skipped; };
	const mask & get_ea_mask() const { if(x_ea_mask == nullptr) throw SRC_BUG; return *x_ea_mask; };
	bool get_flat() const { return x_flat; };
	comparison_fields get_what_to_check() const { return x_what_to_check; };
	bool get_warn_remove_no_match() const { return x_warn_remove_no_match; };
	bool get_empty() const { return x_empty; };
	bool get_empty_dir() const { return x_empty_dir; };
	t_dirty get_dirty_behavior() const { return x_dirty; }
	const crit_action & get_overwriting_rules() const { if(x_overwrite == nullptr) throw SRC_BUG; return *x_overwrite; };
	bool get_only_deleted() const { return x_only_deleted; };
	bool get_ignore_deleted() const { return x_ignore_deleted; };
	const fsa_scope & get_fsa_scope() const { return x_scope; };
	bool get_ignore_unix_sockets() const { return x_ignore_unix_sockets; };
	bool get_in_place() const { return x_in_place; };

    private:
	mask * x_selection;
	mask * x_subtree;
	bool x_warn_over;
	bool x_info_details;
	bool x_display_treated;
	bool x_display_treated_only_dir;
	bool x_display_skipped;
	mask * x_ea_mask;
	bool x_flat;
	comparison_fields x_what_to_check;
	bool x_warn_remove_no_match;
	bool x_empty;
	bool x_empty_dir;
	t_dirty x_dirty;
	crit_action *x_overwrite;
	bool x_only_deleted;
	bool x_ignore_deleted;
	fsa_scope x_scope;
	bool x_ignore_unix_sockets;
	bool x_in_place;

	void destroy() noexcept;
	void nullifyptr() noexcept;
	void copy_from(const archive_options_extract & ref);
	void move_from(archive_options_extract && ref) noexcept;
    };




	/////////////////////////////////////////////////////////
	////////// OPTIONS FOR LISTING AN ARCHIVE ///////////////
	/////////////////////////////////////////////////////////

    	/// class holding optional parameters used to list the contents of an existing archive
    class archive_options_listing
    {
    public:
	archive_options_listing() { nullifyptr(); clear(); };
	archive_options_listing(const archive_options_listing & ref) { copy_from(ref); };
	archive_options_listing(archive_options_listing && ref) noexcept { nullifyptr(); move_from(std::move(ref)); };
	archive_options_listing & operator = (const archive_options_listing & ref) { destroy(); copy_from(ref); return *this; };
	archive_options_listing & operator = (archive_options_listing && ref) noexcept { move_from(std::move(ref)); return *this; };
	virtual ~archive_options_listing() { destroy(); };

	virtual void clear();


	    /////////////////////////////////////////////////////////////////////
	    // setting methods

	    /// whether output should be verbosed --> to be moved to shell output
	void set_info_details(bool info_details) { x_info_details = info_details; };

	    /// mask applied to filename, only those marching it will be listed

	    /// \note this mask does not reject directory (it does not apply to it)
	void set_selection(const mask & selection);

	    /// defines the directories and files to consider

	    /// \note WARNING: The filter mechanism used here is common to all operations and works
	    /// by comparing full paths, while in the other hand, paths stored in the libdar archive are
	    /// all relative to what was provided as fs_root at backup time. To have this filter mecanism
	    /// working in the context of listing, where no fs_root can be provided to transform relative
	    /// paths to absolute paths, libdar will emulate an fs_root with the pseudo root value of the
	    /// path::FAKE_ROOT field. This is invisible form the libdar API user except when relying on
	    /// mask_list objects, which cannot thus receive full path as those would not be prepended
	    /// by path::FAKE_ROOT as fs_root value, because fs_root prefixing is only performed for relative
	    /// paths.
	void set_subtree(const mask & subtree);

	    /// whether to only show entries that have their data fully saved
	void set_filter_unsaved(bool filter_unsaved) { x_filter_unsaved = filter_unsaved; };

	    /// whether to calculate the slice location of each file
	void set_slicing_location(bool val) { x_slicing_location = val; };

	    /// when slice location is performed, user may modify the slice layout of the archive

	    /// \note this is needed for archive format older than 8 and when listing an
	    /// isolated catalogue which original archive has been resized after the isolation
	    /// operation. This option is not used if set_slicing_location is set to false
	void set_user_slicing(const infinint & slicing_first, const infinint & slicing_others);

	    /// whether to fetch EA for listing

	    /// \note this operation implies a lot of processing
	void set_display_ea(bool display_ea) { x_display_ea = display_ea; };


	    /////////////////////////////////////////////////////////////////////
	    // getting methods

	bool get_info_details() const { return x_info_details; };
	const mask & get_selection() const;
	const mask & get_subtree() const;
	bool get_filter_unsaved() const { return x_filter_unsaved; };
	bool get_user_slicing(infinint & slicing_first, infinint & slicing_others) const;
	bool get_slicing_location() const { return x_slicing_location; };
	bool get_display_ea() const { return x_display_ea; };

    private:
	bool x_info_details;
	mask * x_selection;
	mask * x_subtree;
	bool x_filter_unsaved;
	infinint *x_slicing_first;
	infinint *x_slicing_others;
	bool x_slicing_location;
	bool x_display_ea;

	void destroy() noexcept;
	void nullifyptr() noexcept;
	void copy_from(const archive_options_listing & ref);
	void move_from(archive_options_listing && ref) noexcept;
    };

	/////////////////////////////////////////////////////////
	////////// OPTONS FOR DIFFING AN ARCHIVE ////////////////
	/////////////////////////////////////////////////////////


    class archive_options_diff
    {
    public:
	archive_options_diff() { nullifyptr(); clear(); };
	archive_options_diff(const archive_options_diff & ref) { copy_from(ref); };
	archive_options_diff(archive_options_diff && ref) noexcept { nullifyptr(); move_from(std::move(ref)); };
	archive_options_diff & operator = (const archive_options_diff & ref) { destroy(); copy_from(ref); return *this; };
	archive_options_diff & operator = (archive_options_diff && ref) noexcept { move_from(std::move(ref)); return *this; };
	~archive_options_diff() { destroy(); };

	void clear();

	    /////////////////////////////////////////////////////////////////////
	    // setting methods

	    /// list of filenames to consider (directory not concerned by this fiter)
	void set_selection(const mask & selection);

	    /// defines the directories and files to consider

	    /// \note WARNING: The filter mechanism used here is common to all operations and works
	    /// by comparing full paths, while in the other hand, paths stored in the libdar archive are
	    /// all relative paths to what was provided as fs_root at backup time. To have this filter mecanism
	    /// working in the context of restoration, libdar prepends the relative path found in the libdar
	    /// archive with the fs_root argument given to archive::op_diff method.
	    /// In consequence the provided filter here, for comparison operation should be build taking into
	    /// account that files to compare will be seen as subdirectories of this provided "fs_root" the
	    /// the archive will be compared to. In other words, if the subtree mask do not accept anything under
	    /// the provided fs_root path, no file will be compared to what is on the filesystem.
	void set_subtree(const mask & subtree);

	    /// whether the user needs detailed output of the operation
	    ///
	    /// \note in API 5.5.x and before this switch drove the displaying
	    /// of processing messages and treated files. now it only drives the
	    /// display of processing messages, use set_display_treated to define
	    /// whether files under treatement should be display or not
	void set_info_details(bool info_details) { x_info_details = info_details; };

	    /// defines whether to show treated files
	    /// \param[in] display_treated true to display processed inodes
	    /// \param[in] only_dir only display the current directory under processing, not its individual files
	void set_display_treated(bool display_treated, bool only_dir) { x_display_treated = display_treated; x_display_treated_only_dir = only_dir; };

	    /// whether to display files that have been excluded by filters
	void set_display_skipped(bool display_skipped) { x_display_skipped = display_skipped; };

	    /// defines the Extended Attributes to compare
	void set_ea_mask(const mask & ea_mask);

	    /// fields to consider wien comparing inodes with those on filesystem (see comparison_fields enumeration in catalogue.hpp)
	void set_what_to_check(comparison_fields what_to_check) { x_what_to_check = what_to_check; };

	    /// whether to alter atime or ctime in the filesystem when reading files to compare

	    /// \param[in] alter_atime whether to change atime (true) or ctime (false)
	    /// \note this parameter is used only when furtive_read_mode() is not activated
	void set_alter_atime(bool alter_atime)
	{
	    if(x_furtive_read)
		x_old_alter_atime = alter_atime;
	    else
		x_alter_atime = alter_atime;
	};

	    /// whether to use furtive read mode (if activated, alter_atime() has no meaning/use)
	void set_furtive_read_mode(bool furtive_read);

	    /// ignore differences of at most this integer number of hours while looking for changes in dates
	void set_hourshift(const infinint & hourshift) { x_hourshift = hourshift; };

	    /// whether to compare symlink mtime (symlink mtime)
	void set_compare_symlink_date(bool compare_symlink_date) { x_compare_symlink_date = compare_symlink_date; };

	    /// defines the FSA (Filesystem Specific Attribute) to only consider (by default all FSA activated at compilation time are considered)
	void set_fsa_scope(const fsa_scope & scope) { x_scope = scope; };

	    /// whether to ignore fs_root and use in-place path stored in the archive
	void set_in_place(bool arg) { x_in_place = arg; };


	    /////////////////////////////////////////////////////////////////////
	    // getting methods

	const mask & get_selection() const { if(x_selection == nullptr) throw SRC_BUG; return *x_selection; };
	const mask & get_subtree() const { if(x_subtree == nullptr) throw SRC_BUG; return *x_subtree; };
	bool get_info_details() const { return x_info_details; };
	bool get_display_treated() const { return x_display_treated; };
	bool get_display_treated_only_dir() const { return x_display_treated_only_dir; };
	bool get_display_skipped() const { return x_display_skipped; };
	const mask & get_ea_mask() const { if(x_ea_mask == nullptr) throw SRC_BUG; return *x_ea_mask; };
	comparison_fields get_what_to_check() const { return x_what_to_check; };
	bool get_alter_atime() const { return x_alter_atime; };
	bool get_furtive_read_mode() const { return x_furtive_read; };
	const infinint & get_hourshift() const { return x_hourshift; };
	bool get_compare_symlink_date() const { return x_compare_symlink_date; };
	const fsa_scope & get_fsa_scope() const { return x_scope; };
	bool get_in_place() const { return x_in_place; };

    private:
	mask * x_selection;
	mask * x_subtree;
	bool x_info_details;
	bool x_display_treated;
	bool x_display_treated_only_dir;
	bool x_display_skipped;
	mask * x_ea_mask;
	comparison_fields x_what_to_check;
	bool x_alter_atime;
	bool x_old_alter_atime;
	bool x_furtive_read;
	infinint x_hourshift;
	bool x_compare_symlink_date;
	fsa_scope x_scope;
	bool x_in_place;

	void destroy() noexcept;
	void nullifyptr() noexcept;
	void copy_from(const archive_options_diff & ref);
	void move_from(archive_options_diff && ref) noexcept;
    };




	/////////////////////////////////////////////////////////
	////////// OPTONS FOR TESTING AN ARCHIVE ////////////////
	/////////////////////////////////////////////////////////

    	/// class holding optional parameters used to test the structure coherence of an existing archive
    class archive_options_test
    {
    public:
	archive_options_test() { nullifyptr(); clear(); };
	archive_options_test(const archive_options_test & ref) { copy_from(ref); };
	archive_options_test(archive_options_test && ref) noexcept { nullifyptr(); move_from(std::move(ref)); };
	archive_options_test & operator = (const archive_options_test & ref) { destroy(); copy_from(ref); return *this; };
	archive_options_test & operator = (archive_options_test && ref) noexcept { move_from(std::move(ref)); return *this; };
	~archive_options_test() { destroy(); };

	void clear();

	    /////////////////////////////////////////////////////////////////////
	    // setting methods

	    /// list of filenames to consider (directory not concerned by this fiter)
	void set_selection(const mask & selection);

	    /// defines the directories and files to consider

	    /// \note WARNING: The filter mechanism used here is common to all operations and works
	    /// by comparing full paths, while in the other hand, paths stored in the libdar archive are
	    /// all relative to what was provided as fs_root at backup time. To have this filter mecanism
	    /// working in the context of testing, where no fs_root can be provided to transform relative
	    /// paths to absolute paths, libdar will emulate an fs_root with the pseudo root value of the
	    /// path::FAKE_ROOT field. This is invisible form the libdar API user except when relying on
	    /// mask_list objects, which cannot thus receive full path as those would not be prepended
	    /// by path::FAKE_ROOT as fs_root value, because fs_root prefixing is only performed for relative
	    /// paths.
	void set_subtree(const mask & subtree);

	    /// whether the user needs detailed output of the operation
	    ///
	    /// \note in API 5.5.x and before this switch drove the displaying
	    /// of processing messages and treated files. now it only drives the
	    /// display of processing messages, use set_display_treated to define
	    /// whether files under treatement should be display or not
	void set_info_details(bool info_details) { x_info_details = info_details; };

	    /// whether to display files that have been excluded by filters
	void set_display_skipped(bool display_skipped) { x_display_skipped = display_skipped; };

	    /// defines whether to show treated files
	    /// \param[in] display_treated true to display processed inodes
	    /// \param[in] only_dir only display the current directory under processing, not its individual files
	void set_display_treated(bool display_treated, bool only_dir) { x_display_treated = display_treated; x_display_treated_only_dir = only_dir; };

	    /// dry-run exectution if set to true
	void set_empty(bool empty) { x_empty = empty; };


	    /////////////////////////////////////////////////////////////////////
	    // getting methods

	const mask & get_selection() const { if(x_selection == nullptr) throw SRC_BUG; return *x_selection; };
	const mask & get_subtree() const { if(x_subtree == nullptr) throw SRC_BUG; return *x_subtree; };
	bool get_info_details() const { return x_info_details; };
	bool get_display_treated() const { return x_display_treated; };
	bool get_display_treated_only_dir() const { return x_display_treated_only_dir; };
	bool get_display_skipped() const { return x_display_skipped; };
	bool get_empty() const { return x_empty; };

    private:
	mask * x_selection;
	mask * x_subtree;
	bool x_info_details;
	bool x_display_treated;
	bool x_display_treated_only_dir;
	bool x_display_skipped;
	bool x_empty;

	void destroy() noexcept;
	void nullifyptr() noexcept;
	void copy_from(const archive_options_test & ref);
	void move_from(archive_options_test && ref) noexcept;
    };


	/////////////////////////////////////////////////////////
	///////// OPTIONS FOR REPAIRING AN ARCHIVE //////////////
	/////////////////////////////////////////////////////////

	/// class holding optional parameters used to create an archive
    class archive_options_repair
    {
    public:
	    // default constructors and destructor.

	archive_options_repair();
	archive_options_repair(const archive_options_repair & ref);
	archive_options_repair(archive_options_repair && ref) noexcept { nullifyptr(); move_from(std::move(ref)); };
	archive_options_repair & operator = (const archive_options_repair & ref) { copy_from(ref); return *this; };
	archive_options_repair & operator = (archive_options_repair && ref) noexcept { move_from(std::move(ref)); return *this; };
	~archive_options_repair() = default;

	    /////////////////////////////////////////////////////////////////////
	    // set back to default (this is the state just after the object is constructed
	    // this method is to be used to reuse a given object

	    /// reset all the options to their default values
	void clear();


	    /////////////////////////////////////////////////////////////////////
	    // setting methods

		    /// defines whether overwritting is allowed or not
	void set_allow_over(bool allow_over) { x_allow_over = allow_over; };

	    /// defines whether a warning shall be issued before overwriting
	void set_warn_over(bool warn_over) { x_warn_over = warn_over; };

	    /// defines whether the user needs detailed output of the operation
	    ///
	    /// \note in API 5.5.x and before this switch drove the displaying
	    /// of processing messages and treated files. now it only drives the
	    /// display of processing messages, use set_display_treated to define
	    /// whether files under treatement should be display or not
	void set_info_details(bool info_details) { x_info_details = info_details; };

	    /// defines whether to show treated files
	    ///
	    /// \param[in] display_treated true to display processed inodes
	    /// \param[in] only_dir only display the current directory under process, not its individual files
	void set_display_treated(bool display_treated, bool only_dir) { x_display_treated = display_treated; x_display_treated_only_dir = only_dir; };

	    /// whether to display files that have been excluded by filters
	void set_display_skipped(bool display_skipped) { x_display_skipped = display_skipped; };

	    /// whether to display a summary for each completed directory with total saved data and compression ratio
	void set_display_finished(bool display_finished) { x_display_finished = display_finished; };

	    /// set a pause beteween slices. Set to zero does not pause at all, set to 1 makes libdar pauses each slice, set to 2 makes libdar pause each 2 slices and so on.
	void set_pause(const infinint & pause) { x_pause = pause; };

	    /// define the archive slicing

	    /// \param[in] file_size set the slice size in byte (0 for a single slice whatever its size is)
	    /// \param[in] first_file_size set the first file size
	    /// \note if not specified or set to zero, first_file_size is considered equal to file_size
	void set_slicing(const infinint & file_size, const infinint & first_file_size = 0)
	{
	    x_file_size = file_size;
	    if(first_file_size.is_zero())
		x_first_file_size = file_size;
	    else
		x_first_file_size = first_file_size;
	};

	    /// set the command to execute after each slice creation

	    /// several macros are available:
	    /// - %%n : the number of the just created slice
	    /// - %%N : the slice number with padded zeros according to slice_min_digits given option
	    /// - %%b : the archive basename
	    /// - %%p : the slices path
	    /// - %%e : the archive extension (usually "dar")
	    /// - %%c : the archive context ("init", "operation" or "last_slice") see dar(1) man page for more details
	    /// - %%% : substitued by %%
	    /// .
	void set_execute(const std::string & execute) { x_execute = execute; };

	    /// set the cypher to use
	void set_crypto_algo(crypto_algo crypto) { x_crypto = crypto; };

	    /// set the pass the password / passphrase to use. Giving an empty string makes the password asked
	    /// interactively through the dialog argument if encryption has been set.
	void set_crypto_pass(const secu_string & pass) { x_pass = pass; };

	    /// set the size of the encryption by block to use
	void set_crypto_size(U_32 crypto_size) { x_crypto_size = crypto_size; };

	    /// set the list of recipients that will be able to read the archive
	    /// \note this is based on GnuPG keyring and assumes the user running libdar has its keyring
	    /// containing for each recipient a valid public key. If a list of recipient is given the crypto_pass
	    /// (see above) is not used, but the crypto_algo stays used to encrypt the archive using a randomly generated key
	    /// which is encrypted using the public keys of the recipients and dropped that way encrypted inside the archive.
	    /// \note if crypto_algo is not set while a list of recipient is given, the crypto algo will default to blowfish
	    /// \note since release 2.7.0 if a given std::string in the list contains an '@' the string is assumed to be an
	    /// email and search is done in the keyring for that field type, else it is assumed to be a keyid.
	void set_gnupg_recipients(const std::vector<std::string> & gnupg_recipients) { x_gnupg_recipients = gnupg_recipients; };

	    /// the private keys matching the email of the provided list are used to sign the archive random key
	    /// \note since release 2.7.0 if a given std::string in the list contains an '@' the string is assumed to be an
	    /// email and search is done in the keyring for that field type, else it is assumed to be a keyid.
	void set_gnupg_signatories(const std::vector<std::string> & gnupg_signatories) { x_gnupg_signatories = gnupg_signatories; };

	    /// whether to make a dry-run operation
	void set_empty(bool empty) { x_empty = empty; };

	    /// if not an empty string set the slice permission according to the octal value given.
	void set_slice_permission(const std::string & slice_permission) { x_slice_permission = slice_permission; };

	    /// if not an empty string set the user ownership of slices accordingly
	void set_slice_user_ownership(const std::string & slice_user_ownership) { x_slice_user_ownership = slice_user_ownership; };

	    /// if not an empty string set the group ownership of slices accordingly
	void set_slice_group_ownership(const std::string & slice_group_ownership) { x_slice_group_ownership = slice_group_ownership; };

	    /// specify a user comment in the archive (always in clear text!)
	void set_user_comment(const std::string & comment) { x_user_comment = comment; };

	    /// specify whether to produce a hash file of the slice and which hash algo to use
	    /// \note the libdar::hash_algo data type is defined in hash_fichier.hpp, valid values
	    /// are for examle libdar::hash_none, libdar::hash_md5, libdar::hash_sha1, libdar::hash_sha512...
	void set_hash_algo(hash_algo hash);

	    /// defines the minimum digit a slice must have concerning its number, zeros will be prepended as much as necessary to respect this
	void set_slice_min_digits(infinint val) { x_slice_min_digits = val; };

	    /// defines the protocol to use for slices
	void set_entrepot(const std::shared_ptr<entrepot> & entr) { if(!entr) throw Erange("archive_options_repair::set_entrepot", "null entrepot pointer given in argument"); x_entrepot = entr; };

	    /// whether libdar is allowed to spawn several threads to possibily work faster on multicore CPU (requires libthreadar)

	    /// \deprecated this call is deprecated, see set_multi_threaded_*() more specific calls
	    /// \note setting this to true is equivalent to calling set_mutli_threaded_crypto(2)
	void set_multi_threaded(bool val) { x_multi_threaded_crypto = 2; x_multi_threaded_compress = 2; };

	    /// how much thread libdar will use for cryptography (need libthreadar to be effective)
	void set_multi_threaded_crypto(U_I num) { x_multi_threaded_crypto = num; };

	    /// how much thread libdar will use for compression (need libthreadar too and compression_block_size > 0)
	void set_multi_threaded_compress(U_I num) { x_multi_threaded_compress = num; };


	    /////////////////////////////////////////////////////////////////////
	    // getting methods

	bool get_allow_over() const { return x_allow_over; };
	bool get_warn_over() const { return x_warn_over; };
	bool get_info_details() const { return x_info_details; };
	bool get_display_treated() const { return x_display_treated; };
	bool get_display_treated_only_dir() const { return x_display_treated_only_dir; };
	bool get_display_skipped() const { return x_display_skipped; };
	bool get_display_finished() const { return x_display_finished; };
	const infinint & get_pause() const { return x_pause; };
	const infinint & get_slice_size() const { return x_file_size; };
	const infinint & get_first_slice_size() const { return x_first_file_size; };
	const std::string & get_execute() const { return x_execute; };
	crypto_algo get_crypto_algo() const { return x_crypto; };
	const secu_string & get_crypto_pass() const { return x_pass; };
	U_32 get_crypto_size() const { return x_crypto_size; };
	const std::vector<std::string> & get_gnupg_recipients() const { return x_gnupg_recipients; };
	const std::vector<std::string> & get_gnupg_signatories() const { return x_gnupg_signatories; };
	bool get_empty() const { return x_empty; };
	const std::string & get_slice_permission() const { return x_slice_permission; };
	const std::string & get_slice_user_ownership() const { return x_slice_user_ownership; };
	const std::string & get_slice_group_ownership() const { return x_slice_group_ownership; };
	const std::string & get_user_comment() const { return x_user_comment; };
	hash_algo get_hash_algo() const { return x_hash; };
	infinint get_slice_min_digits() const { return x_slice_min_digits; };
	const std::shared_ptr<entrepot> & get_entrepot() const { return x_entrepot; };
	U_I get_multi_threaded_crypto() const { return x_multi_threaded_crypto; };
	U_I get_multi_threaded_compress() const { return x_multi_threaded_compress; };

    private:
	bool x_allow_over;
	bool x_warn_over;
	bool x_info_details;
	bool x_display_treated;
	bool x_display_treated_only_dir;
	bool x_display_skipped;
	bool x_display_finished;
	infinint x_pause;
	infinint x_file_size;
	infinint x_first_file_size;
	std::string x_execute;
	crypto_algo x_crypto;
	secu_string x_pass;
	U_32 x_crypto_size;
	std::vector<std::string> x_gnupg_recipients;
	std::vector<std::string> x_gnupg_signatories;
	bool x_empty;
	std::string x_slice_permission;
	std::string x_slice_user_ownership;
	std::string x_slice_group_ownership;
	std::string x_user_comment;
	hash_algo x_hash;
	infinint x_slice_min_digits;
	std::shared_ptr<entrepot> x_entrepot;
	U_I x_multi_threaded_crypto;
	U_I x_multi_threaded_compress;

	void nullifyptr() noexcept {};
	void copy_from(const archive_options_repair & ref);
	void move_from(archive_options_repair && ref) noexcept;
    };

	/// @}

} // end of namespace

#endif
