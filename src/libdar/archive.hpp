//*********************************************************************/
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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: archive.hpp,v 1.32.2.2 2009/04/07 08:45:29 edrusb Rel $
//
/*********************************************************************/
//

    /// \file archive.hpp
    /// \brief the archive class is defined in this module


#ifndef ARCHIVE_HPP
#define ARCHIVE_HPP

#include "../my_config.h"

#include "crypto.hpp"
#include "path.hpp"
#include "catalogue.hpp"
#include "scrambler.hpp"
#include "statistics.hpp"

namespace libdar
{

	/// the archive class realizes the most general operations on archives

	/// the operations corresponds to the one the final user expects, these
	/// are the same abstraction level as the operation realized by the DAR
	/// command line tool.
	/// \ingroup API
    class archive
    {
    public:

	    /// defines the way archive listing is done:
	enum listformat
	{
	    normal,   //< the tar-like listing (should be the default to use)
	    tree,     //< the original dar's tree listing (for those that like forest)
	    xml       //< the xml catalogue output
	};

	    /// this constructor opens an already existing archive (for reading) [this is the "read" constructor]

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] chem the path where to look for slices
	    /// \param[in] basename the slices basename of the archive to read
	    /// ("-" means standard input, and activates the output_pipe and input_pipe arguments)
	    /// \param[in] extension the slice extension (should always be "dar")
	    /// \param[in] crypto the crypto cypher to use to read the archive
	    /// \param[in] pass the password or passphrase to decrypt (unused if encryption is not set)
	    /// if an empty string is given and encryption is set, the password will be asked through the user_interaction object
	    /// \param[in] crypto_size the encryption block size to use to decrypt (unused if encrytion is not set)
	    /// \param[in] input_pipe the name of the input pipe to read data from (when basename is set to "-")
	    /// if input_pipe is set to "" the information from dar_slave are expected in standard input else the given string
	    /// must be the path to the a named pipe which will relay the information from dar_slave
	    /// \param[in] output_pipe the name of the output pipe to send orders to (when basenale is set to "-")
	    /// if output_pipe is set to "" the orders sent to dar_slave will exit by the standard output else the given string
	    /// must be the path to a named pipe which will relay the orders to dar_slave
	    /// \param[in] execute the command to execute before reading each slice (empty string for no script)
	    /// several macros are available:
	    /// - %%n : the slice number to be read
	    /// - %%b : the archive basename
	    /// - %%p : the slices path
	    /// - %%e : the archive extension (usually "dar")
	    /// - %%% : substitued by %%
	    /// .
	    /// \param[in] info_details whether the user needs detailed output of the operation
	archive(user_interaction & dialog,
		const path & chem,
		const std::string & basename,
		const std::string & extension,
		crypto_algo crypto,
		const std::string &pass,
		U_32 crypto_size,
		const std::string & input_pipe,
		const std::string & output_pipe,
		const std::string & execute,
		bool info_details);


	    /// this constuctor create an archive (full or differential) [this is the "create" constructor]

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] fs_root the filesystem to take as root for the backup
	    /// \param[in] sauv_path the path where to create slices
	    /// \param[in] ref_arch the archive to take as reference (NULL for a full backup)
	    /// \param[in] selection to only save file (except directory) that match the given mask
	    /// \param[in] subtree define the directory and files to consider (this mask will be applied to the absolute path of files being proceeded)
	    /// \param[in] filename base name of the slices. If "-" is given the archive will be produced in standard output
	    /// \param[in] extension slices extension ("dar")
	    /// \param[in] allow_over whether overwritting is allowed
	    /// \param[in] warn_over whether a warning shall be issued before overwriting
	    /// \param[in] info_details whether the user needs detailed output of the operation
	    /// \param[in] pause Pause beteween slices. Set to zero does not pause at all, set to 1 makes libdar pauses each slice, set to 2 makes libdar pause each 2 slices and so on.
	    /// \param[in] empty_dir whether we need to store ignored directories as empty
	    /// \param[in] algo the compression algorithm used
	    /// \param[in] compression_level the compression level (from 1 to 9)
	    /// \param[in] file_size the slice size in byte (0 for a single slice whatever its size is)
	    /// \param[in] first_file_size the first file size (value 0 is forbidden unless file_size is also set to zero).
	    /// \param[in] ea_mask defines which Extended Attributes to save
	    /// \param[in] execute command to execute after each slice creation
	    /// (see the "read" constructor for the available macros)
	    /// \param[in] crypto cypher to use
	    /// \param[in] pass the password / passphrase to encrypt the data with. Giving an empty string makes the password asked
	    /// interactively through the dialog argument if encryption has been set.
	    /// \param[in] crypto_size the size of the encryption by block to use
	    /// \param[in] compr_mask files to compress
	    /// \param[in] min_compr_size file size under which to never compress
	    /// \param[in] nodump whether to ignore files with the nodump flag set
	    /// \param[in] what_to_check fields to consider when comparing inodes with reference archive (see inode::comparison_fields enumeration in catalogue.hpp)
	    /// \param[in] hourshift ignore differences of at most this integer number of hours while looking for changes in dates
	    /// \param[in] empty whether to make a dry-run operation
	    /// \param[in] alter_atime whether to alter atime or ctime in the filesystem when reading files to save
	    /// \param[in] same_fs whether to limit the backup to files located on the same filesystem as the directory taken as root of the backup
	    /// \param[in] snapshot whether to make an emtpy archive only referencing the current state of files in the filesystem
	    /// \param[in] cache_directory_tagging whether to consider the Cache Directory Tagging Standard
	    /// \param[in] display_skipped whether to display files that have been excluded by filters
	    /// \param[in] fixed_date whether to ignore any archive of reference and only save file which modification is more recent that the given "fixed_date". To not use this feature set fixed_date to zero.
	    /// \param[out] progressive_report statistics about the operation, considering the treated files (NULL can be given if you don't want to use this feature)
	    /// \note the statistics fields used are:
	    /// - .treated: the total number of files seen
	    /// - .hard_link: the number of hard linked inodes
	    /// - .tooold: the number of files that changed at the time they were saved
	    /// - .skipped: number of files not changed (differential backup)
	    /// - .errored: number of files concerned by filesystem error
	    /// - .ignored: number of files excluded by filters
	    /// - .deleted: number of files recorded as deleted
	    /// .
	archive(user_interaction & dialog,
		const path & fs_root,
		const path & sauv_path,
		archive *ref_arch,
		const mask & selection,
		const mask & subtree,
		const std::string & filename,
		const std::string & extension,
		bool allow_over,
		bool warn_over,
		bool info_details,
		const infinint & pause,
		bool empty_dir,
		compression algo,
		U_I compression_level,
		const infinint &file_size,
		const infinint &first_file_size,
		const mask & ea_mask,
		const std::string & execute,
		crypto_algo crypto,
		const std::string & pass,
		U_32 crypto_size,
		const mask & compr_mask,
		const infinint & min_compr_size,
		bool nodump,
		inode::comparison_fields what_to_check,
		const infinint & hourshift,
		bool empty,
		bool alter_atime,
		bool same_fs,
		bool snapshot,
		bool cache_directory_tagging,
		bool display_skipped,
		const infinint & fixed_date,
		statistics * progressive_report);


	    /// this constructor isolates a catalogue of a given archive [this is the "isolate" constructor]

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] sauv_path the path where to create slices
	    /// \param[in] ref_arch the archive to take as reference (NULL for a full backup)
	    /// \param[in] filename base name of the slices ("-" for standard output)
	    /// \param[in] extension slices extension ("dar")
	    /// \param[in] allow_over whether overwritting is allowed
	    /// \param[in] warn_over whether a warning shall be issued before overwriting
	    /// \param[in] info_details whether the user needs detailed output of the operation
	    /// \param[in] pause Pause beteween slices. Set to zero does not pause at all, set to 1 makes libdar pauses each slice, set to 2 makes libdar pause each 2 slices and so on.
	    /// \param[in] algo the compression algorithm used
	    /// \param[in] compression_level the compression level (from 1 to 9)
	    /// \param[in] file_size the slice size in byte (0 for a single slice whatever its size is)
	    /// \param[in] first_file_size the first file size (zero is allowed only if file_size is set to zero)
	    /// \param[in] execute command to execute after each slice creation
	    /// \param[in] crypto cypher to use
	    /// \param[in] pass the password / passphrase to encrypt the data with (empty string for interactive question)
	    /// \param[in] crypto_size the size of the encryption by block to use
	    /// \param[in] empty whether to make a dry-run operation
	archive(user_interaction & dialog,
		const path &sauv_path,
		archive *ref_arch,
		const std::string & filename,
		const std::string & extension,
		bool allow_over,
		bool warn_over,
		bool info_details,
		const infinint & pause,
		compression algo,
		U_I compression_level,
		const infinint &file_size,
		const infinint &first_file_size,
		const std::string & execute,
		crypto_algo crypto,
		const std::string & pass,
		U_32 crypto_size,
		bool empty);


	    /// this constructor builds an archive from two given archive [this is the "merge" constructor]

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] sauv_path the path where to create slices
	    /// \param[in] ref_arch1 the first input archive (NULL if no archive to give, thus building a subset of ref_arch2)
	    /// \param[in] ref_arch2 the first input archive (NULL if no archive to give, thus building a subset of ref_arch1)
	    /// \param[in] selection to only consider files (except directory) that match the given mask
	    /// \param[in] subtree define the directory and files to consider (this mask will be applied to the absolute path of files being proceeded, *
            /// assuming a "<ROOT>" is the root of all paths)
	    /// \param[in] filename base name of the slices. If "-" is given the archive will be produced in standard output
	    /// \param[in] extension slices extension ("dar")
	    /// \param[in] allow_over whether to allow slice overwriting
	    /// \param[in] warn_over whether to warn before overwriting a slice
	    /// \param[in] info_details whether the user needs detailed output of the operation
	    /// \param[in] pause Pause beteween slices. Set to zero does not pause at all, set to 1 makes libdar pauses each slice, set to 2 makes libdar pause each 2 slices and so on.
	    /// \param[in] empty_dir whether we need to store ignored directories as empty
	    /// \param[in] algo is the compression algorithm used
	    /// \param[in] compression_level is the compression level (from 1 to 9)
	    /// \param[in] file_size the slice size in byte (0 for a single slice whatever its size is)
	    /// \param[in] first_file_size the first file size (value 0 is forbidden unless file_size is also set to zero).
	    /// \param[in] ea_mask defines which Extended Attributes to save
	    /// \param[in] execute command to execute after each slice creation
	    /// (see the "read" constructor for the available macros)
	    /// \param[in] crypto cypher to use
	    /// \param[in] pass the password / passphrase to encrypt the data with. Giving an empty string makes the password asked
	    /// interactively through the dialog argument if encryption has been set.
	    /// \param[in] crypto_size the size of the encryption by block to use
	    /// \param[in] compr_mask files to compress
	    /// \param[in] min_compr_size file size under which to never compress
	    /// \param[in] empty whether to make a dry-run operation
	    /// \param[in] display_skipped whether to display files that have been excluded by filters
	    /// \param[in] keep_compressed make dar ignore the 'algo' argument and do not uncompress / compress files that are selected for merging
	    /// \param[out] progressive_report statistics about the operation, considering the treated files (NULL can be given if you don't want to use this feature)
	    /// \note the statistics fields used are:
	    /// - .treated: the total number of files seen
	    /// - .hard_link: the number of hard linked inodes
	    /// - .ignored: number of files excluded by filters
	    /// - .deleted: number of files recorded as deleted
	    /// .

	archive(user_interaction & dialog,
		const path & sauv_path,
		archive *ref_arch1,
		archive *ref_arch2,
		const mask & selection,
		const mask & subtree,
		const std::string & filename,
		const std::string & extension,
                bool allow_over,
		bool warn_over,
		bool info_details,
		const infinint & pause,
		bool empty_dir,
		compression algo,
		U_I compression_level,
		const infinint & file_size,
		const infinint & first_file_size,
		const mask & ea_mask,
		const std::string & execute,
		crypto_algo crypto,
		const std::string & pass,
		U_32 crypto_size,
		const mask & compr_mask,
		const infinint & min_compr_size,
		bool empty,
		bool display_skipped,
		bool keep_compressed,
		statistics * progressive_report);


	    /// the destructor
	~archive() { free(); };


	    /// extraction of data from an archive

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] fs_root the filesystem to take as root for the restoration
	    /// \param[in] selection to only restore file (except directory) that match the given mask
	    /// \param[in] subtree define the directory and files to consider (this mask will be applied to the absolute path of files being proceeded)
	    /// \param[in] allow_over whether overwritting is allowed
	    /// \param[in] warn_over whether a warning shall be issued before overwriting
	    /// \param[in] info_details whether the user needs detailed output of the operation
	    /// \param[in] detruire whether the files recorded as removed from the archive of reference shall be removed from filesystem
	    /// \param[in] only_more_recent whether to restore only file more recent than those on filesystem
	    /// \param[in] ea_mask mask which defines which EA to restore
	    /// \param[in] flat whether to ignore directory structure and restore all files in the same directory
	    /// \param[in] what_to_check fields to consider when comparing inodes with those on filesystem to determine if it is more recent (see inode::comparison_fields enumeration), also defines which if mtime has to be restored (cf_mtime) if permission have to be too (cf_ignore_owner) if ownership has to be restored too (cf_all)
	    /// \param[in] warn_remove_no_match whether a warning must be issue if a file to remove does not match the expected type of file
	    /// \param[in] hourshift ignore differences of at most this integer number of hours while looking for file to restore
	    /// \param[in] empty whether to make a dry-run operation
	    /// \param[in] ea_erase if set, all EA are first erased before being restored
	    /// \param[in] display_skipped whether to display files that have been excluded by filters
	    /// \param[in,out] progressive_report points to an already existing statistics object that can be consulted at any time
	    /// during the call (see the returned value to know the useful fields and their meining),
	    /// NULL can be given in argument if you only need the result at the end of the operation through the returned value of this call
	    /// this should speed up the operation by a little amount.
	    /// \return the statistics about the operation, considering the treated files
	    /// \note the statistics fields used are:
	    /// - .treated: the total number of files seen
	    /// - .skipped: number of files not saved in the archive
	    /// - .tooold: number of file older than the one on filesystem
	    /// - .errored: number of files concerned by filesystem error
	    /// - .ignored: number of files excluded by filters
	    /// - .deleted: number of files deleted
	    /// .
	statistics op_extract(user_interaction & dialog,
			      const path &fs_root,
			      const mask &selection,
			      const mask &subtree,
			      bool allow_over,
			      bool warn_over,
			      bool info_details,
			      bool detruire,
			      bool only_more_recent,
			      const mask & ea_mask,
			      bool flat,
			      inode::comparison_fields what_to_check,
			      bool warn_remove_no_match,
			      const infinint & hourshift,
			      bool empty,
			      bool ea_erase,
			      bool display_skipped,
			      statistics *progressive_report);


	    /// listing of the archive contents

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] info_details whether the user needs detailed output of the operation
	    /// \param[in] list_mode whether to list ala tar or in a tree like view
	    /// \param[in] selection to only view some files (directories will always be seen)
	    /// \param[in] filter_unsaved whether to ignore unsaved files
	void op_listing(user_interaction & dialog,
			bool info_details,
			archive::listformat list_mode,
			const mask &selection,
			bool filter_unsaved);


	    /// archive comparison with filesystem

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] fs_root the filesystem to take as root for the comparison
	    /// \param[in] selection to only consider file (except directory) that match the given mask
	    /// \param[in] subtree define the directory and files to consider (this mask will be applied to the absolute path of files being proceeded)
	    /// \param[in] info_details whether the user needs detailed output of the operation
	    /// \param[in] ea_mask is a mask that defines the Extended Attributes to compare
	    /// \param[in] what_to_check fields to consider wien comparing inodes with those on filesystem (see inode::comparison_fields enumeration)
	    /// \param[in] alter_atime whether to alter atime or ctime in the filesystem when reading files to compare
	    /// \param[in] display_skipped whether to display files that have been excluded by filters
	    /// \param[in,out] progressive_report points to an already existing statistics object that can be consulted at any time
	    /// during the call (see the returned value to know the useful fields and their meining),
	    /// NULL can be given in argument if you only need the result at the end of the operation through the returned value of this call
	    /// this should speed up the operation by a little amount.
	    /// \return the statistics about the operation, considering the treated files
	    /// \note the statistics fields used are:
	    /// - .treated: the total number of files seen
	    /// - .errored: number of files that do not match or could not be read
	    /// - .ignored: number of files excluded by filters
	    /// .
	statistics op_diff(user_interaction & dialog,
			   const path & fs_root,
			   const mask &selection,
			   const mask &subtree,
			   bool info_details,
			   const mask & ea_mask,
			   inode::comparison_fields what_to_check,
			   bool alter_atime,
			   bool display_skipped,
			   statistics * progressive_report);


	    /// test the archive integrity

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] selection to only test file (except directory) that match the given mask
	    /// \param[in] subtree define the directory and files to consider (this mask will be applied to the string "<ROOT>/<relative filename>" of the files being proceeded, where "<ROOT>" is the litteral string "<ROOT>" without the quotes)
	    /// \param[in] info_details whether the user needs detailed output of the operation
	    /// \param[in] display_skipped whether to display files that have been excluded by filters
	    /// \param[in,out] progressive_report points to an already existing statistics object that can be consulted at any time
	    /// during the call (see the returned value to know the useful fields and their meining),
	    /// NULL can be given in argument if you only need the result at the end of the operation through the returned value of this call
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
			   const mask &selection,
			   const mask &subtree,
			   bool info_details,
			   bool display_skipped,
			   statistics * progressive_report);


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

	    /// retrieving statistics about archive contents
	const entree_stats get_stats() const { if(cat == NULL) throw SRC_BUG; return cat->get_stats(); };

	    /// gives access to internal catalogue (not to be used from the API)

	    /// \return the catalogue reference contained in this archive
	    /// \note this method is not to be used directly from external application, it is
	    /// not part of the API but must remain a public method for been usable by the database class
	const catalogue & get_catalogue() const { if(cat == NULL) throw SRC_BUG; return *cat; };

    private:
	enum operation { oper_create, oper_isolate, oper_merge };

	generic_file *level1;
	generic_file *scram;
	compressor *level2;
	header_version ver;
	catalogue *cat;
	infinint local_cat_size;
	path *local_path;
	bool exploitable; // is false if only the catalogue is available (for reference backup or isolation).

	void free();
	catalogue & get_cat() { if(cat == NULL) throw SRC_BUG; else return *cat; };
	const header_version & get_header() const { return ver; };
	const path & get_path() { if(local_path == NULL) throw SRC_BUG; else return *local_path; };

	bool get_sar_param(infinint & sub_file_size, infinint & first_file_size, infinint & last_file_size,
			   infinint & total_file_number);
	infinint get_level2_size();
	infinint get_cat_size() const { return local_cat_size; };

	statistics op_create_in(user_interaction & dialog,
				operation op,
				const path & fs_root,
				const path & sauv_path,
				archive *ref_arch,
				const mask & selection,
				const mask & subtree,
				const std::string & filename,
				const std::string & extension,
				bool allow_over,
				bool warn_over,
				bool info_details,
				const infinint & pause,
				bool empty_dir,
				compression algo,
				U_I compression_level,
				const infinint & file_size,
				const infinint & first_file_size,
				const mask & ea_mask,
				const std::string & execute,
				crypto_algo crypto,
				const std::string & pass,
				U_32 crypto_size,
				const mask & compr_mask,
				const infinint & min_compr_size,
				bool nodump,
				const infinint & hourshift,
				bool empty,
				bool alter_atime,
				bool same_fs,
				inode::comparison_fields what_to_check,
				bool snapshot,
				bool cache_directory_tagging,
				bool display_skipped,
				const infinint & fixed_date,
				statistics * progressive_report);

	void op_create_in_sub(user_interaction & dialog,        ///< interaction with user
			      operation op,                     ///< the filter operation to bind to
			      const path & fs_root,             ///< root of the filesystem to act on
			      const path & sauv_path_t,         ///< where to create the archive
			      catalogue  * ref_arch1,           ///< catalogue of the archive of reference (a catalogue must be provided in any case, a empty one shall fit for no reference)
			      catalogue  * ref_arch2,           ///< secondary catalogue used for merging, can be NULL if not used
			      const path * ref_path,            ///< path of the archive of archive of reference (NULL if there is no archive of reference used, thus ref_arch (previous arg) is probably an empty archive)
			      const mask & selection,           ///< filter on filenames
			      const mask & subtree,             ///< filter on directory tree and filenames
			      const std::string & filename,     ///< basename of the archive to create
			      const std::string & extension,    ///< extension of the archives
			      bool allow_over,                  ///< whether to allow overwriting (of slices)
			      bool warn_over,                   ///< whether to warn before overwriting
			      bool info_details,                ///< whether to display detailed informations
			      const infinint & pause,           ///< whether to pause between slices
			      bool empty_dir,                   ///< whether to store excluded dir as empty directories
			      compression algo,                 ///< compression algorithm
			      U_I compression_level,            ///< compression level (range 1 to 9)
			      const infinint & file_size,       ///< slice size
			      const infinint & first_file_size, ///< first slice size
			      const mask & ea_mask,             ///< Extended Attribute to consider
			      const std::string & execute,      ///< Command line to execute between slices
			      crypto_algo crypto,               ///< crypt algorithm
			      const std::string & pass,         ///< password ("" for onfly request of password)
			      U_32 crypto_size,                 ///< size of crypto blocks
			      const mask & compr_mask,          ///< files to compress
			      const infinint & min_compr_size,  ///< file size under which to not compress files
			      bool nodump,                      ///< whether to consider the "nodump" filesystem flag
			      const infinint & hourshift,       ///< hourshift (see man page -H option)
			      bool empty,                       ///< whether to make an "dry-run" execution
			      bool alter_atime,                 ///< whether to alter atime date (by opposition to ctime) when reading files
			      bool same_fs,                     ///< confin the files consideration to a single filesystem
			      inode::comparison_fields what_to_check,  ///< fields to consider wien comparing inodes (see inode::comparison_fields enumeration)
			      bool snapshot,                    ///< make as if all file had not changed
			      bool cache_directory_tagging,     ///< avoid saving directory which follow the cache directory tagging
			      bool display_skipped,             ///< display skipped files for the operation
			      bool keep_compressed,             ///< keep file compressed when merging
			      const infinint & fixed_date,      ///< whether to ignore any archive of reference and only save file which modification is more recent that the given "fixed_date" date
			      statistics * st_ptr);             ///< statistics must not be NULL !

	void disable_natural_destruction();
	void enable_natural_destruction();
    };

} // end of namespace

#endif
