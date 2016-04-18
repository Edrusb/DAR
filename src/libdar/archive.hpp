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

    /// \file archive.hpp
    /// \brief the archive class is defined in this module
    /// \ingroup API


#ifndef ARCHIVE_HPP
#define ARCHIVE_HPP

#include "../my_config.h"
#include <vector>
#include <string>

#include "erreurs.hpp"
#include "path.hpp"
#include "scrambler.hpp"
#include "statistics.hpp"
#include "archive_options.hpp"
#include "escape.hpp"
#include "escape_catalogue.hpp"
#include "pile.hpp"
#include "list_entry.hpp"
#include "on_pool.hpp"
#include "crypto.hpp"
#include "slice_layout.hpp"

namespace libdar
{

	/// the archive class realizes the most general operations on archives

	/// the operations corresponds to the one the final user expects, these
	/// are the same abstraction level as the operation realized by the DAR
	/// command line tool.
	/// \ingroup API
    class archive : public on_pool
    {
    public:

	    /// this constructor opens an already existing archive (for reading) [this is the "read" constructor]

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] chem the path where to look for slices
	    /// \param[in] basename the slices basename of the archive to read
	    /// ("-" means standard input, and activates the output_pipe and input_pipe arguments)
	    /// \param[in] extension the slice extension (should always be "dar")
	    /// \param[in] options A set of option to use to read the archive
	archive(user_interaction & dialog,
		const path & chem,
		const std::string & basename,
		const std::string & extension,
		const archive_options_read & options);


	    /// this constuctor create an archive (full or differential) [this is the "create" constructor]

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] fs_root the filesystem to take as root for the backup
	    /// \param[in] sauv_path the path where to create slices
	    /// \param[in] filename base name of the slices. If "-" is given the archive will be produced in standard output
	    /// \param[in] extension slices extension ("dar")
	    /// \param[in] options optional parameters to use for the operation
 	    /// \param[out] progressive_report statistics about the operation, considering the treated files (nullptr can be given if you don't want to use this feature)
	    /// \note the statistics fields used are:
	    /// - .treated: the total number of files seen
	    /// - .hard_link: the number of hard linked inodes
	    /// - .tooold: the number of files that changed at the time they were saved and that could not be resaved (due to repeat limit or byte limit)
	    /// - .skipped: number of files not changed (differential backup)
	    /// - .errored: number of files concerned by filesystem error
	    /// - .ignored: number of files excluded by filters
	    /// - .deleted: number of files recorded as deleted
	    /// - .ea_treated: number of entry having some EA
	    /// - .byte_amount : number of wasted bytes due to repeat on change feature
	    /// .
	archive(user_interaction & dialog,
		const path & fs_root,
		const path & sauv_path,
		const std::string & filename,
		const std::string & extension,
		const archive_options_create & options,
		statistics * progressive_report);


	    /// WARNING this is a deprecated constructor, use the op_isolate() method for better performances
	    /// \note this method will be remove from the API in a future version of libdar
	archive(user_interaction & dialog,
		const path & sauv_path,
		archive *ref_arch,
		const std::string & filename,
		const std::string & extension,
		const archive_options_isolate & options);


	    /// this constructor builds an archive from two given archive [this is the "merge" constructor]

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] sauv_path the path where to create slices
	    /// \param[in] ref_arch1 the first mandatory input archive (the second is optional and provided within the 'option' argument
	    /// \param[in] filename base name of the slices. If "-" is given the archive will be produced in standard output
	    /// \param[in] extension slices extension ("dar")
	    /// \param[in] options optional parameters to be used for the operation
	    /// \param[out] progressive_report statistics about the operation, considering the treated files (nullptr can be given if you don't want to use this feature)
	    /// \note the statistics fields used are:
	    /// - .treated: the total number of files seen
	    /// - .hard_link: the number of hard linked inodes
	    /// - .ignored: number of files excluded by filters
	    /// - .deleted: number of files recorded as deleted
	    /// - .ea_treated: number of entry with EA
	    /// .

	archive(user_interaction & dialog,
		const path & sauv_path,
		archive *ref_arch1,
		const std::string & filename,
		const std::string & extension,
		const archive_options_merge & options,
		statistics * progressive_report);

	    /// copy constructor (not implemented, throw an exception if called explicitely or implicitely)

	    /// \note this lack of implementation is intentionnal, Archive should rather be manipulated
	    /// using pointers, or passed as constant reference (const &) in arguments or returned values.
	    /// Moreover, having two objets one copy of the other may lead to unexpected behaviors while
	    /// merging or creating, isolating or merging archives.

	archive(const archive & ref) : stack(ref.stack) { throw Efeature(dar_gettext("Archive copy constructor is not implemented")); };
	archive & operator = (const archive & ref) { throw Efeature(dar_gettext("Archive assignment operator is not implemented")); };

	    /// the destructor
	~archive() throw(Ebug) { free_all(); };


	    /// extraction of data from an archive

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] fs_root the filesystem to take as root for the restoration
	    /// \param[in] options optional parameter to be used for the operation
	    /// \param[in,out] progressive_report points to an already existing statistics object that can be consulted at any time
	    /// during the call (see the returned value to know the useful fields and their meining),
	    /// nullptr can be given in argument if you only need the result at the end of the operation through the returned value of this call
	    /// this should speed up the operation by a little amount.
	    /// \return the statistics about the operation, considering the treated files
	    /// \note the statistics fields used are:
	    /// - .treated: the total number of files restored
	    /// - .skipped: number of files not saved in the archive
	    /// - .tooold: number of file not restored due to overwriting policy decision
	    /// - .errored: number of files concerned by filesystem error
	    /// - .ignored: number of files excluded by filters
	    /// - .deleted: number of files deleted
	    /// - .hard_links: number of hard link restored
	    /// - .ea_treated: number of entry having some EA
	    /// .
	statistics op_extract(user_interaction & dialog,
			      const path &fs_root,
			      const archive_options_extract & options,
			      statistics *progressive_report);

	    /// display a summary of the archive
	    ///
	    /// \note see also get_stats() method
	void summary(user_interaction & dialog);


	    /// listing of the archive contents

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] options list of optional parameters to use for the operation
	    /// \note this call covers the whole listing and uses the user_interaction
	    /// to provide the archive contents (in particular using its listing() method
	    /// if set and activated by its set_use_listing() method). There is however two
	    /// alternative way to get archive contents:
	    /// . archive::get_children_of() method
	    /// . archive::init_catalogue()+get_children_in_table()
	void op_listing(user_interaction & dialog,
			const archive_options_listing & options);

	    /// archive comparison with filesystem

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] fs_root the filesystem to take as root for the comparison
	    /// \param[in] options optional parameters to be used with the operation
	    /// \param[in,out] progressive_report points to an already existing statistics object that can be consulted at any time
	    /// during the call (see the returned value to know the useful fields and their meining),
	    /// nullptr can be given in argument if you only need the result at the end of the operation through the returned value of this call
	    /// this should speed up the operation by a little amount.
	    /// \return the statistics about the operation, considering the treated files
	    /// \note the statistics fields used are:
	    /// - .treated: the total number of files seen
	    /// - .errored: number of files that do not match or could not be read
	    /// - .ignored: number of files excluded by filters
	    /// .
	statistics op_diff(user_interaction & dialog,
			   const path & fs_root,
			   const archive_options_diff & options,
			   statistics * progressive_report);


	    /// test the archive integrity

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] options optional parameter to use for the operation
	    /// \param[in,out] progressive_report points to an already existing statistics object that can be consulted at any time
	    /// during the call (see the returned value to know the useful fields and their meining),
	    /// nullptr can be given in argument if you only need the result at the end of the operation through the returned value of this call
	    /// this should speed up the operation by a little amount.
	    /// \note op_test will generate an error message if used on an archive
	    /// that has been created by the isolate or creation constructor
	    /// this is not only an implementation limitation but also a choice.
	    /// testing an file archive using the C++ object used to create
	    /// the file is not a good idea. You need to first destroy this
	    /// C++ object then create a new one with the reading constructor
	    /// this way only you can be sure your archive is properly tested.
	    /// \return the statistics about the operation, considering the treated files
	    /// \note the statistics fields used are:
	    /// - .treated: the total number of files seen
	    /// - .skipped: number of file older than the one on filesystem
	    /// - .errored: number of files with error
	    /// .
	statistics op_test(user_interaction & dialog,
			   const archive_options_test & options,
			   statistics * progressive_report);


	    /// this methodes isolates the catalogue of a the current archive into a separated archive

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] sauv_path the path where to create slices
	    /// \param[in] filename base name of the slices ("-" for standard output)
	    /// \param[in] extension slices extension ("dar")
	    /// \param[in] options optional parameters to use for the operation
	void op_isolate(user_interaction & dialog,
			const path &sauv_path,
			const std::string & filename,
			const std::string & extension,
			const archive_options_isolate & options);


	    /// getting information about a given directory

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] dir relative path the directory to get information about
	    /// \return true if some children have been found and thus if
	    /// the dialog.listing() method has been called at least once.
	    /// \note the get_children_of() call uses the listing() method
	    /// to send back data to the user. If it is not redifined in the
	    /// dialog object nothing will get sent back to the user
	bool get_children_of(user_interaction & dialog,
			     const std::string & dir);

	    /// getting information about the given directory (alternative to get_children_of)
	    ///
	    /// \param[in] dir relative path the directory to get information about, use empty string for root directory
	    /// \return a table information about all subdir and subfile for the given directory
	    /// \note at the difference of get_children_of, this call does not rely on a user_interaction class
	    /// to provide the information, but rather returns a table of children. To allow new fields to
	    /// be added to the future the table contains an object that provide a method per field.
	    /// \note before calling this method on this object, a single call to init_catalogue() is
	    /// mandatory
	const std::vector<list_entry> get_children_in_table(const std::string & dir) const;

	    /// returns true if the pointed directory has one or more subdirectories
	bool has_subdirectory(const std::string & dir) const;

	    /// retrieving statistics about archive contents
	const entree_stats get_stats() const { if(cat == nullptr) throw SRC_BUG; return cat->get_stats(); };

	    /// retrieving signature information about the archive
	const std::list<signator> & get_signatories() const { return gnupg_signed; };

	    /// necessary to get the catalogue fully loaded in memory in any situation
	    /// in particular in sequential reading mode
	void init_catalogue(user_interaction & dialog) const;

	    /// gives access to internal catalogue (not to be used from the API)

	    /// \return the catalogue reference contained in this archive
	    /// \note this method is not to be used directly from external application, it is
	    /// not part of the API but must remain a public method for been usable by the database class
	    /// \note this method is not usable (throws an exception) if the archive has been
	    /// open in sequential read mode and the catalogue has not yet been read; use the
	    /// same method but with user_interaction argument instead, in that situation
	    /// or call init_catalogue() first.
	const catalogue & get_catalogue() const;

	    /// gives access to internal catalogue (not to be used from the API) even in sequential read mode
	const catalogue & get_catalogue(user_interaction & dialog) const;

	    /// closes all filedescriptors and associated data, just keep the catalogue

	    /// \note once this method has been called, the archive object can only be used
	    /// as reference for a differential archive.
	    /// \note this method is not usable (throws an exception) if the archive has been
	    /// open in sequential read mode and the catalogue has not yet been read; use the
	    /// same method but with user_interaction argument instead in that situation
	void drop_all_filedescriptors();

	    /// closes all filedescriptors and associated even when in sequential read mode

	void drop_all_filedescriptors(user_interaction & dialog);

	    /// change all inode as unsaved (equal to differential backup with no change met)
	void set_to_unsaved_data_and_FSA() { if(cat == nullptr) throw SRC_BUG; cat->set_to_unsaved_data_and_FSA(); };

	    /// check that the internal memory_pool has all its blocks properly freeed
	    ///
	    /// \return an empty string in normal situation, else a status report about the
	    /// still allocated blocks that remain in the internal memory_pool
	    /// \note this must be the last call done to that object before destruction and it must
	    /// be done only once
	std::string free_and_check_memory() const;

	    /// get the first slice header
	    ///
	    /// get_first_slice_header_size() and get_non_first_slice_header_size() can be used to translate
	    /// from archive offset as reported by the list_entry::get_archive_offset_*() methods to file
	    /// offset. This can be done by adding the first_slice_header_size to the archive offset, if the
	    /// resulting number is larger than the first slice size, substract the it and add the non_first_slice
	    /// header_size, and so on. This way you can determin the slice number to look into and the file offset
	    /// in that file. Unit for all value is the byte (= octet).
	    /// \note may return 0 if the slice header is not known
	U_64 get_first_slice_header_size() const;

	    /// get the non first slice header
	    ///
	    /// \note may return 0 if the slice header is not known
	U_64 get_non_first_slice_header_size() const;


    private:
	enum operation { oper_create, oper_isolate, oper_merge };

	pile stack;              //< the different layer through which the archive contents is read or wrote
	header_version ver;      //< information for the archive header
	memory_pool *pool;       //< points to local_pool or inherited pool or to nullptr if no memory_pool has to be used
	catalogue *cat;          //< archive contents
	infinint local_cat_size; //< size of the catalogue on disk
	bool exploitable;        //< is false if only the catalogue is available (for reference backup or isolation).
	bool lax_read_mode;      //< whether the archive has been openned in lax mode (unused for creation/merging/isolation)
	bool sequential_read;    //< whether the archive is read in sequential mode
	bool freed_and_checked;  //< whether free_and_check has been run
	std::list<signator> gnupg_signed; //< list of signature found in the archive (reading an existing archive)
	slice_layout slices;     //< slice layout, archive is not sliced <=> first_size or other_size fields is set to zero (in practice both are set to zero, but one being set is enought to determine the archive is not sliced)

	void free_except_memory_pool();
	void free_all();
	void init_pool();
	void check_gnupg_signed(user_interaction & dialog) const;

	const catalogue & get_cat() const { if(cat == nullptr) throw SRC_BUG; else return *cat; };
	const header_version & get_header() const { return ver; };

	bool get_sar_param(infinint & sub_file_size, infinint & first_file_size, infinint & last_file_size,
			   infinint & total_file_number);
	const entrepot *get_entrepot(); //< this method may return nullptr if no entrepot is used (pipes used for archive building, etc.)
	infinint get_level2_size();
	infinint get_cat_size() const { return local_cat_size; };

	statistics op_create_in(user_interaction & dialog,
				operation op,
				const path & fs_root,
				const entrepot & sauv_path_t,
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
				cat_inode::comparison_fields what_to_check,
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
				statistics * progressive_report);

	void op_create_in_sub(user_interaction & dialog,        //< interaction with user
			      operation op,                     //< the filter operation to bind to
			      const path & fs_root,             //< root of the filesystem to act on
			      const entrepot & sauv_path_t,     //< where to create the archive
			      const catalogue * ref_cat1,       //< catalogue of the archive of reference, (cannot be nullptr if ref_cat2 is not nullptr)
			      const catalogue * ref_cat2,       //< secondary catalogue used for merging, can be nullptr if not used
			      bool initial_pause,               //< whether we shall pause before starting the archive creation
			      const mask & selection,           //< filter on filenames
			      const mask & subtree,             //< filter on directory tree and filenames
			      const std::string & filename,     //< basename of the archive to create
			      const std::string & extension,    //< extension of the archives
			      bool allow_over,                  //< whether to allow overwriting (of slices)
			      const crit_action & overwrite,    //< whether and how to allow overwriting (for files inside the archive)
			      bool warn_over,                   //< whether to warn before overwriting
			      bool info_details,                //< whether to display detailed informations
			      bool display_treated,             //< whether to display treated files
			      bool display_treated_only_dir,    //< whether to only display current directory of treated files
			      bool display_skipped,             //< display skipped files for the operation
			      bool display_finished,            //< display space and compression ratio summary for each completed directory
			      const infinint & pause,           //< whether to pause between slices
			      bool empty_dir,                   //< whether to store excluded dir as empty directories
			      compression algo,                 //< compression algorithm
			      U_I compression_level,            //< compression level (range 1 to 9)
			      const infinint & file_size,       //< slice size
			      const infinint & first_file_size, //< first slice size
			      const mask & ea_mask,             //< Extended Attribute to consider
			      const std::string & execute,      //< Command line to execute between slices
			      crypto_algo crypto,               //< crypt algorithm
			      const secu_string & pass,         //< password ("" for onfly request of password)
			      U_32 crypto_size,                 //< size of crypto blocks
			      const std::vector<std::string> & gnupg_recipients, //< list of email recipients to encrypted a randomly chosen key inside the archive
			      const std::vector<std::string> & gnupg_signatories, //< list of email recipients to use for signature
			      const mask & compr_mask,          //< files to compress
			      const infinint & min_compr_size,  //< file size under which to not compress files
			      bool nodump,                      //< whether to consider the "nodump" filesystem flag
			      const std::string & exclude_by_ea,//< if not empty the ea to use for inode exclusion from backup operation
			      const infinint & hourshift,       //< hourshift (see man page -H option)
			      bool empty,                       //< whether to make an "dry-run" execution
			      bool alter_atime,                 //< whether to alter atime date (by opposition to ctime) when reading files
			      bool furtive_read_mode,           //< whether to neither alter atime nor ctome (if true alter_atime is ignored)
			      bool same_fs,                     //< confin the files consideration to a single filesystem
			      cat_inode::comparison_fields what_to_check,  //< fields to consider wien comparing inodes (see cat_inode::comparison_fields enumeration)
			      bool snapshot,                    //< make as if all file had not changed
			      bool cache_directory_tagging,     //< avoid saving directory which follow the cache directory tagging
			      bool keep_compressed,             //< keep file compressed when merging
			      const infinint & fixed_date,      //< whether to ignore any archive of reference and only save file which modification is more recent that the given "fixed_date" date
			      const std::string & slice_permission,      //< permissions of slices that will be created
			      const infinint & repeat_count,             //< max number of retry to save a file that have changed while it was read for backup
			      const infinint & repeat_byte,              //< max amount of wasted data used to save a file that have changed while it was read for backup
			      bool decremental,                          //< in the merging context only, whether to build a decremental backup from the two archives of reference
			      bool add_marks_for_sequential_reading,     //< whether to add marks for sequential reading
			      bool security_check,                       //< whether to check for ctime change with no reason (rootkit ?)
			      const infinint & sparse_file_min_size,     //< starting which size to consider looking for holes in sparse files (0 for no detection)
			      const std::string & user_comment,          //< user comment to put in the archive
			      hash_algo hash,                            //< whether to produce hash file, and which algo to use
			      const infinint & slice_min_digits,         //< minimum digit for slice number
			      const std::string & backup_hook_file_execute, //< command to execute before and after files to backup
			      const mask & backup_hook_file_mask,         //< files elected to have a command executed before and after their backup
			      bool ignore_unknown,                        //< whether to warn when an unknown inode type is met
			      const fsa_scope & scope,                    //< FSA scope for the operation
			      bool multi_threaded,              //< whether libdar is allowed to spawn several thread to possibily work faster on multicore CPU
			      statistics * st_ptr);             //< statistics must not be nullptr !

	void disable_natural_destruction();
	void enable_natural_destruction();
	const label & get_layer1_data_name() const;
	const label & get_catalogue_data_name() const;
	bool only_contains_an_isolated_catalogue() const; //< true if the current archive only contains an isolated catalogue
	void check_against_isolation(user_interaction & dialog, bool lax) const; //< throw Erange exception if the archive only contains an isolated catalogue
	const cat_directory *get_dir_object(const std::string & dir) const;
    };

} // end of namespace

#endif
