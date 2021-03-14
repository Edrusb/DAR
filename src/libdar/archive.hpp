/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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
#include <memory>

#include "erreurs.hpp"
#include "path.hpp"
#include "statistics.hpp"
#include "archive_options.hpp"
#include "list_entry.hpp"
#include "crypto.hpp"
#include "archive_summary.hpp"
#include "archive_listing_callback.hpp"
#include "user_interaction.hpp"


namespace libdar
{
	// need to declare class database friend of class archive.
	// class database need to access the archive table of content
	// though exposing it by mean of a public method of class
	// archive would let it visibile to the API, thing we do not
	// want
    class database;

        /// \addtogroup API
        /// @{

	/// the archive class realizes the most general operations on archives

	/// the operations corresponds to the one the final user expects, these
	/// are the same abstraction level as the operation realized by the DAR
	/// command line tool.

    class archive
    {
    public:

	    /// this constructor opens an already existing archive (for reading) [this is the "read" constructor]

	    /// \param[in,out] dialog for user- interaction
	    /// \param[in] chem the path where to look for slices
	    /// \param[in] basename the slices basename of the archive to read
	    /// ("-" means standard input, and activates the output_pipe and input_pipe arguments)
	    /// \param[in] extension the slice extension (should always be "dar")
	    /// \param[in] options A set of option to use to read the archive
	archive(const std::shared_ptr<user_interaction> & dialog,
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
	archive(const std::shared_ptr<user_interaction> & dialog,
		const path & fs_root,
		const path & sauv_path,
		const std::string & filename,
		const std::string & extension,
		const archive_options_create & options,
		statistics * progressive_report);

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

	archive(const std::shared_ptr<user_interaction> & dialog,
		const path & sauv_path,
		std::shared_ptr<archive> ref_arch1,
		const std::string & filename,
		const std::string & extension,
		const archive_options_merge & options,
		statistics * progressive_report);

	    /// this constructor create a new archive from a damaged one [this is the "repair" constructor]

	    /// \param[in,out] dialog for user interaction
	    /// \param[in] chem_src the path where to look for slices of the archive to repair
	    /// \param[in] basename_src the slices basename of the archive to repair
	    /// \param[in] extension_src the slices extension of the archive to repair
	    /// \param[in] options_read the set of option to use to read the archive repair
	    /// \param[in] chem_dst the path where to write the repaired archive
	    /// \param[in] basename_dst the slices basename of the repaired archive
	    /// \param[in] extension_dst the slices extension of the repaired archive
	    /// \param[in] options_repair the set of option to use to write the repaired archive

	archive(const std::shared_ptr<user_interaction> & dialog,
		const path & chem_src,
		const std::string & basename_src,
		const std::string & extension_src,
		const archive_options_read & options_read,
		const path & chem_dst,
		const std::string & basename_dst,
		const std::string & extension_dst,
		const archive_options_repair & options_repair);


	    /// copy constructor (not implemented, throw an exception if called explicitely or implicitely)

	    /// \note this lack of implementation is intentionnal, Archive should rather be manipulated
	    /// using pointers, or passed as constant reference (const &) in arguments or returned values.
	    /// Moreover, having two objets one copy of the other may lead to unexpected behaviors while
	    /// merging or creating, isolating or merging archives.

	archive(const archive & ref) = delete;
	archive(archive && ref) = delete;
	archive & operator = (const archive & ref) = delete;
	archive & operator = (archive && ref) = delete;

	    /// the destructor
	~archive();


	    /// extraction of data from an archive

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
	statistics op_extract(const path &fs_root,
			      const archive_options_extract & options,
			      statistics *progressive_report);

	    /// display a summary of the archive

	    /// \note see also get_stats() method
	void summary();

	    /// same information as summary() but as broken out data
	archive_summary summary_data();


	    /// listing of the archive contents

	    /// \param[in] callback callback function used to provide data in splitted field (not used if null is given)
	    /// \param[in] context will be passed as is to the last argument of the provided callback
	    /// \param[in] options list of optional parameters to use for the operation
	    /// \note if callback is nullptr (or NULL), the output is done using user_interaction provided with archive constructor
	    /// \note alternative way to get archive contents:
	    /// . archive::get_children_of() method
	    /// . archive::init_catalogue()+get_children_in_table()
	void op_listing(archive_listing_callback callback,
			void *context,
			const archive_options_listing & options) const;

	    /// archive comparison with filesystem

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
	statistics op_diff(const path & fs_root,
			   const archive_options_diff & options,
			   statistics * progressive_report);


	    /// test the archive integrity

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
	statistics op_test(const archive_options_test & options,
			   statistics * progressive_report);


	    /// this methodes isolates the catalogue of a the current archive into a separated archive

	    /// \param[in] sauv_path the path where to create slices
	    /// \param[in] filename base name of the slices ("-" for standard output)
	    /// \param[in] extension slices extension ("dar")
	    /// \param[in] options optional parameters to use for the operation
	    /// \note that if the archive contains delta sig and isolation options, specifying not
	    /// to keep them in the resulting isolated catalogue leads the current archive object
	    /// (but not the corresponding archive stored on filesystem) to be modified
	    /// (delta signature are removed) --- this is not a const method.
	void op_isolate(const path &sauv_path,
			const std::string & filename,
			const std::string & extension,
			const archive_options_isolate & options);


	    /// getting information about a given directory

	    /// \param[in] callback callback function used to provide data in splitted field
	    /// \param[in] context will be passed as first argument of the callback as is provided here
	    /// \param[in] dir relative path the directory to get information about
	    /// \param[in] fetch_ea whether to look for EA (not possible in sequential read mode)
	    /// \return true if some children have been found and the callback has been run at least once
	bool get_children_of(archive_listing_callback callback,
			     void *context,
			     const std::string & dir,
			     bool fetch_ea = false);

	    /// getting information about the given directory (alternative to get_children_of)

	    /// \param[in] dir relative path the directory to get information about, use empty string for root directory
	    /// \param[in] fetch_ea whether to fetch Extended Attributes relative information in each returned list_entry (not possible in sequential read mode)
	    /// \return a table information about all subdir and subfile for the given directory
	    /// \note at the difference of get_children_of, this call does not rely on a user_interaction class
	    /// to provide the information, but rather returns a table of children. To allow new fields to
	    /// be added to the future the table contains an object that provide a method per field.
	    /// \note before calling this method on this object, a single call to init_catalogue() is
	    /// mandatory
	const std::vector<list_entry> get_children_in_table(const std::string & dir, bool fetch_ea = false) const;

	    /// returns true if the pointed directory has one or more subdirectories
	bool has_subdirectory(const std::string & dir) const;

	    /// retrieving statistics about archive contents
	const entree_stats get_stats() const;

	    /// retrieving signature information about the archive
	const std::list<signator> & get_signatories() const;

	    /// necessary to get the catalogue fully loaded in memory in any situation
	    /// in particular in sequential reading mode
	void init_catalogue() const;

	    /// closes all filedescriptors and associated even when in sequential read mode

	    /// \note once this method has been called, the archive object can only be used
	    /// as reference for a differential archive.
	void drop_all_filedescriptors();

	    /// change all inode as unsaved (equal to differential backup with no change met)
	void set_to_unsaved_data_and_FSA();

	    /// get the first slice header

	    /// get_first_slice_header_size() and get_non_first_slice_header_size() can be used to translate
	    /// from archive offset as reported by the list_entry::get_archive_offset_*() methods to file
	    /// offset. This can be done by adding the first_slice_header_size to the archive offset, if the
	    /// resulting number is larger than the first slice size, substract the it and add the non_first_slice
	    /// header_size, and so on. This way you can determin the slice number to look into and the file offset
	    /// in that file. Unit for all value is the byte (= octet).
	    /// \note may return 0 if the slice header is not known
	U_64 get_first_slice_header_size() const;

	    /// get the non first slice header

	    /// \note may return 0 if the slice header is not known
	U_64 get_non_first_slice_header_size() const;


    private:
	class i_archive;
	std::shared_ptr<i_archive> pimpl;

	    // see the comment near the forward declaration
	    // of class database above for explanations
	friend class database;
    };

} // end of namespace

#endif
