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

    /// \file i_archive.hpp
    /// \brief the archive class implementation is defined in this module
    /// \ingroup Private


#ifndef I_ARCHIVE_HPP
#define I_ARCHIVE_HPP

#include "../my_config.h"
#include <vector>
#include <string>
#include <memory>

#include "erreurs.hpp"
#include "path.hpp"
#include "statistics.hpp"
#include "archive_options.hpp"
#include "pile.hpp"
#include "list_entry.hpp"
#include "crypto.hpp"
#include "slice_layout.hpp"
#include "mem_ui.hpp"
#include "archive_summary.hpp"
#include "archive_listing_callback.hpp"
#include "catalogue.hpp"
#include "archive.hpp"
#include "header_version.hpp"

namespace libdar
{

        /// \addtogroup API
        /// @{

	/// the archive::i_archive class implements the most general operations on archives

    class archive::i_archive: public mem_ui
    {
    public:

	    /// this constructor opens an already existing archive (for reading) [this is the "read" constructor]

	i_archive(const std::shared_ptr<user_interaction> & dialog,
		const path & chem,
		const std::string & basename,
		const std::string & extension,
		const archive_options_read & options);


	    /// this constuctor create an archive (full or differential) [this is the "create" constructor]

	i_archive(const std::shared_ptr<user_interaction> & dialog,
		const path & fs_root,
		const path & sauv_path,
		const std::string & filename,
		const std::string & extension,
		const archive_options_create & options,
		statistics * progressive_report);

	    /// this constructor builds an archive from two given archive [this is the "merge" constructor]

	i_archive(const std::shared_ptr<user_interaction> & dialog,
		const path & sauv_path,
		std::shared_ptr<archive> ref_arch1,
		const std::string & filename,
		const std::string & extension,
		const archive_options_merge & options,
		statistics * progressive_report);

	    /// this constructor create a new archive from a damaged one [this is the "repair" constructor]

	i_archive(const std::shared_ptr<user_interaction> & dialog,
		const path & chem_src,
		const std::string & basename_src,
		const std::string & extension_src,
		const archive_options_read & options_read,
		const path & chem_dst,
		const std::string & basename_dst,
		const std::string & extension_dst,
		const archive_options_repair & options_repair);

	    /// copy constructor (not implemented, throw an exception if called explicitely or implicitely)

	i_archive(const i_archive & ref) = delete;
	i_archive(i_archive && ref) = delete;
	i_archive & operator = (const i_archive & ref) = delete;
	i_archive & operator = (i_archive && ref) = delete;

	    /// the destructor

	~i_archive() { free_mem(); };


	    /// extraction of data from an archive

	statistics op_extract(const path &fs_root,
			      const archive_options_extract & options,
			      statistics *progressive_report);

	    /// display a summary of the archive

	void summary();

	    /// same information as summary() but as broken out data
	archive_summary summary_data();


	    /// listing of the archive contents

	void op_listing(archive_listing_callback callback,
			void *context,
			const archive_options_listing & options) const;

	    /// archive comparison with filesystem

	statistics op_diff(const path & fs_root,
			   const archive_options_diff & options,
			   statistics * progressive_report);


	    /// test the archive integrity

	statistics op_test(const archive_options_test & options,
			   statistics * progressive_report);


	    /// this methodes isolates the catalogue of a the current archive into a separated archive

	void op_isolate(const path &sauv_path,
			const std::string & filename,
			const std::string & extension,
			const archive_options_isolate & options);


	    /// getting information about a given directory

	bool get_children_of(archive_listing_callback callback,
			     void *context,
			     const std::string & dir,
			     bool fetch_ea = false);

	    /// getting information about the given directory (alternative to get_children_of)

	const std::vector<list_entry> get_children_in_table(const std::string & dir, bool fetch_ea = false) const;

	    /// returns true if the pointed directory has one or more subdirectories
	bool has_subdirectory(const std::string & dir) const;

	    /// retrieving statistics about archive contents
	const entree_stats get_stats() const { if(cat == nullptr) throw SRC_BUG; return cat->get_stats(); };

	    /// retrieving signature information about the archive
	const std::list<signator> & get_signatories() const { return gnupg_signed; };

	    /// necessary to get the catalogue fully loaded in memory in any situation
	    /// in particular in sequential reading mode
	void init_catalogue() const;

	    /// gives access to internal catalogue (not to be used from the API)

	const catalogue & get_catalogue() const;

	    /// closes all filedescriptors and associated even when in sequential read mode

	void drop_all_filedescriptors();

	    /// change all inode as unsaved (equal to differential backup with no change met)
	void set_to_unsaved_data_and_FSA() { if(cat == nullptr) throw SRC_BUG; cat->set_to_unsaved_data_and_FSA(); };

	    /// returns the slice layout of the archive, or of the archive of reference in case of isolated catalogue

	bool get_catalogue_slice_layout(slice_layout & slicing) const;

	    /// get the first slice header

	U_64 get_first_slice_header_size() const;

	    /// get the non first slice header

	U_64 get_non_first_slice_header_size() const;


    private:
	enum operation { oper_create, oper_isolate, oper_merge, oper_repair };

	pile stack;              ///< the different layer through which the archive contents is read or wrote
	header_version ver;      ///< information for the archive header
	catalogue *cat;          ///< archive contents
	infinint local_cat_size; ///< size of the catalogue on disk
	bool exploitable;        ///< is false if only the catalogue is available (for reference backup or isolation).
	bool lax_read_mode;      ///< whether the archive has been openned in lax mode (unused for creation/merging/isolation)
	bool sequential_read;    ///< whether the archive is read in sequential mode
	std::list<signator> gnupg_signed; ///< list of signature found in the archive (reading an existing archive)
	slice_layout slices;     ///< slice layout, archive is not sliced <=> first_size or other_size fields is set to zero (in practice both are set to zero, but one being set is enought to determine the archive is not sliced)

	void free_mem();
	void check_gnupg_signed() const;

	const catalogue & get_cat() const { if(cat == nullptr) throw SRC_BUG; else return *cat; };
	const header_version & get_header() const { return ver; };

	bool get_sar_param(infinint & sub_file_size, infinint & first_file_size, infinint & last_file_size,
			   infinint & total_file_number);
	std::shared_ptr<entrepot> get_entrepot(); ///< this method may return nullptr if no entrepot is used (pipes used for archive building, etc.)
	infinint get_level2_size();
	infinint get_cat_size() const { return local_cat_size; };

	statistics op_create_in(operation op,
				const path & fs_root,
				const std::shared_ptr<entrepot> & sauv_path_t,
				archive *ref_arch,
				const mask & selection,
				const mask & subtree,
				const std::string & filename,
				const std::string & extension,
				bool allow_over,
				bool warn_over,
				bool info_details,
				bool display_treated,
				bool display_treated_only_dir,
				bool display_skipped,
				bool display_finished,
				const infinint & pause,
				bool empty_dir,
				compression algo,
				U_I compression_level,
				const infinint & file_size,
				const infinint & first_file_size,
				const mask & ea_mask,
				const std::string & execute,
				crypto_algo crypto,
				const secu_string & pass,
				U_32 crypto_size,
				const std::vector<std::string> & gnupg_recipients,
				const std::vector<std::string> & gnupg_signatories,
				const mask & compr_mask,
				const infinint & min_compr_size,
				bool nodump,
				const std::string & exclude_by_ea,
				const infinint & hourshift,
				bool empty,
				bool alter_atime,
				bool furtive_read_mode,
				bool same_fs,
				comparison_fields what_to_check,
				bool snapshot,
				bool cache_directory_tagging,
				const infinint & fixed_date,
				const std::string & slice_permission,
				const infinint & repeat_count,
				const infinint & repeat_byte,
				bool add_marks_for_sequential_reading,
				bool security_check,
				const infinint & sparse_file_min_size,
				const std::string & user_comment,
				hash_algo hash,
				const infinint & slice_min_digits,
				const std::string & backup_hook_file_execute,
				const mask & backup_hook_file_mask,
				bool ignore_unknown,
				const fsa_scope & scope,
				bool multi_threaded,
				bool delta_signature,
				bool build_delta_sig,
				const mask & delta_mask,
				const infinint & delta_sig_min_size,
				bool delta_diff,
				bool zeroing_neg_date,
				const std::set<std::string> & ignored_symlinks,
				modified_data_detection mod_data_detect,
				const infinint & iteration_count,
				hash_algo kdf_hash,
				statistics * progressive_report);

	void op_create_in_sub(operation op,                     ///< the filter operation to bind to
			      const path & fs_root,             ///< root of the filesystem to act on
			      const std::shared_ptr<entrepot> & sauv_path_t,     ///< where to create the archive
			      catalogue * ref_cat1,             ///< catalogue of the archive of reference, (cannot be nullptr if ref_cat2 is not nullptr)
			      const catalogue * ref_cat2,       ///< secondary catalogue used for merging, can be nullptr if not used
			      bool initial_pause,               ///< whether we shall pause before starting the archive creation
			      const mask & selection,           ///< filter on filenames
			      const mask & subtree,             ///< filter on directory tree and filenames
			      const std::string & filename,     ///< basename of the archive to create
			      const std::string & extension,    ///< extension of the archives
			      bool allow_over,                  ///< whether to allow overwriting (of slices)
			      const crit_action & overwrite,    ///< whether and how to allow overwriting (for files inside the archive)
			      bool warn_over,                   ///< whether to warn before overwriting
			      bool info_details,                ///< whether to display detailed informations
			      bool display_treated,             ///< whether to display treated files
			      bool display_treated_only_dir,    ///< whether to only display current directory of treated files
			      bool display_skipped,             ///< display skipped files for the operation
			      bool display_finished,            ///< display space and compression ratio summary for each completed directory
			      const infinint & pause,           ///< whether to pause between slices
			      bool empty_dir,                   ///< whether to store excluded dir as empty directories
			      compression algo,                 ///< compression algorithm
			      U_I compression_level,            ///< compression level (range 1 to 9)
			      const infinint & file_size,       ///< slice size
			      const infinint & first_file_size, ///< first slice size
			      const mask & ea_mask,             ///< Extended Attribute to consider
			      const std::string & execute,      ///< Command line to execute between slices
			      crypto_algo crypto,               ///< crypt algorithm
			      const secu_string & pass,         ///< password ("" for onfly request of password)
			      U_32 crypto_size,                 ///< size of crypto blocks
			      const std::vector<std::string> & gnupg_recipients, ///< list of email recipients to encrypted a randomly chosen key inside the archive
			      const std::vector<std::string> & gnupg_signatories, ///< list of email recipients to use for signature
			      const mask & compr_mask,          ///< files to compress
			      const infinint & min_compr_size,  ///< file size under which to not compress files
			      bool nodump,                      ///< whether to consider the "nodump" filesystem flag
			      const std::string & exclude_by_ea,///< if not empty the ea to use for inode exclusion from backup operation
			      const infinint & hourshift,       ///< hourshift (see man page -H option)
			      bool empty,                       ///< whether to make an "dry-run" execution
			      bool alter_atime,                 ///< whether to alter atime date (by opposition to ctime) when reading files
			      bool furtive_read_mode,           ///< whether to neither alter atime nor ctome (if true alter_atime is ignored)
			      bool same_fs,                     ///< confin the files consideration to a single filesystem
			      comparison_fields what_to_check,  ///< fields to consider wien comparing inodes (see comparison_fields enumeration)
			      bool snapshot,                    ///< make as if all file had not changed
			      bool cache_directory_tagging,     ///< avoid saving directory which follow the cache directory tagging
			      bool keep_compressed,             ///< keep file compressed when merging
			      const infinint & fixed_date,      ///< whether to ignore any archive of reference and only save file which modification is more recent that the given "fixed_date" date
			      const std::string & slice_permission,      ///< permissions of slices that will be created
			      const infinint & repeat_count,             ///< max number of retry to save a file that have changed while it was read for backup
			      const infinint & repeat_byte,              ///< max amount of wasted data used to save a file that have changed while it was read for backup
			      bool decremental,                          ///< in the merging context only, whether to build a decremental backup from the two archives of reference
			      bool add_marks_for_sequential_reading,     ///< whether to add marks for sequential reading
			      bool security_check,                       ///< whether to check for ctime change with no reason (rootkit ?)
			      const infinint & sparse_file_min_size,     ///< starting which size to consider looking for holes in sparse files (0 for no detection)
			      const std::string & user_comment,          ///< user comment to put in the archive
			      hash_algo hash,                            ///< whether to produce hash file, and which algo to use
			      const infinint & slice_min_digits,         ///< minimum digit for slice number
			      const std::string & backup_hook_file_execute, ///< command to execute before and after files to backup
			      const mask & backup_hook_file_mask,         ///< files elected to have a command executed before and after their backup
			      bool ignore_unknown,                        ///< whether to warn when an unknown inode type is met
			      const fsa_scope & scope,                    ///< FSA scope for the operation
			      bool multi_threaded,              ///< whether libdar is allowed to spawn several thread to possibily work faster on multicore CPU
			      bool delta_signature,             ///< whether to calculate and store binary delta signature for each saved file
			      bool build_delta_sig,             ///< whether to rebuild delta sig accordingly to delta_mask
			      const mask & delta_mask,          ///< which files to consider delta signature for
			      const infinint & delta_sig_min_size, ///< minimum file size for which to calculate delta signature
			      bool delta_diff,                     ///< whether to allow delta diff backup when delta sig is present
			      bool zeroing_neg_date,            ///< if true just warn before zeroing neg date, dont ask user
			      const std::set<std::string> & ignored_symlinks, ///< list of symlink pointed to directory to recurse into
			      modified_data_detection mod_data_detect, ///< how to verify data has not changed upon inode metadata change
			      const infinint & iteration_count, ///< for key derivation
			      hash_algo kdf_hash,               ///< hash used for key derivation
			      statistics * st_ptr             ///< statistics must not be nullptr !
	    );

	void disable_natural_destruction();
	void enable_natural_destruction();
	const label & get_layer1_data_name() const;
	const label & get_catalogue_data_name() const;
	bool only_contains_an_isolated_catalogue() const; ///< true if the current archive only contains an isolated catalogue
	void check_against_isolation(bool lax) const; ///< throw Erange exception if the archive only contains an isolated catalogue
	const cat_directory *get_dir_object(const std::string & dir) const;
	void load_catalogue();
    };

} // end of namespace

#endif
