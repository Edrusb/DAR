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
// $Id: libdar.hpp,v 1.19.2.1 2004/01/28 15:29:47 edrusb Rel $
//
/*********************************************************************/
//

#ifndef LIBDAR_H
#define LIBDAR_H

#include <string>
#include "compressor.hpp"
#include "path.hpp"
#include "mask.hpp"
#include "integers.hpp"
#include "infinint.hpp"
#include "statistics.hpp"
#include "user_interaction.hpp"
#include "deci.hpp"
#include "archive.hpp"
#include "crypto.hpp"

#define LIBDAR_NOEXCEPT 0
#define LIBDAR_EMEMORY 1
#define LIBDAR_EBUG 2
#define LIBDAR_EINFININT 3
#define LIBDAR_ELIMITINT 4
#define LIBDAR_ERANGE 5
#define LIBDAR_EDECI 6
#define LIBDAR_EFEATURE 7
#define LIBDAR_EHARDWARE 8
#define LIBDAR_EUSER_ABORT 9
#define LIBDAR_EDATA 10
#define LIBDAR_ESCRIPT 11
#define LIBDAR_ELIBCALL 12
#define LIBDAR_UNKNOWN 13

namespace libdar
{
    const U_I LIBDAR_COMPILE_TIME_MAJOR = 2;
    const U_I LIBDAR_COMPILE_TIME_MINOR = 0;

    extern void get_version(U_I & major, U_I & minor);
    extern void get_compile_time_features(bool & ea, bool & largefile, bool & nodump, bool & special_alloc, U_I & bits);

///////////////////////
// IMPORTANT !
// two callback functions must be given to darlib for it to be operational
// see user_interaction.hpp file.
//////////////////////

    extern archive* open_archive_noexcept(const path & chem, const std::string & basename,
					  const std::string & extension, crypto_algo crypto, const std::string &pass,
					  const std::string & input_pipe, const std::string & output_pipe,
					  const std::string & execute, bool info_details,
					  U_16 & exception,
					  std::string & except_msg);

    extern void close_archive_noexcept(archive *ptr,
					U_16 & exception,
					std::string & except_msg);

    extern statistics op_create(const path &fs_root,
                                const path &sauv_path,
                                archive *ref_arch,
                                const mask &selection,
                                const mask &subtree,
                                const std::string &filename,
                                const std::string & extension,
                                bool allow_over,
                                bool warn_over,
                                bool info_details,
                                bool pause,
                                bool empty_dir,
                                compression algo,
                                U_I compression_level,
                                const infinint & file_size,
                                const infinint & first_file_size,
                                bool root_ea,
                                bool user_ea,
                                const std::string & execute,
				crypto_algo crypto,
                                const std::string & pass,
                                const mask &compr_mask,
                                const infinint & min_compr_size,
                                bool nodump,
                                bool ignore_owner,
                                const infinint & hourshift,
				bool empty);

    extern statistics op_create_noexcept(const path &fs_root,
					 const path &sauv_path,
					 archive *ref_arch,
					 const mask &selection,
					 const mask &subtree,
					 const std::string &filename,
					 const std::string & extension,
					 bool allow_over,
					 bool warn_over,
					 bool info_details,
					 bool pause,
					 bool empty_dir,
					 compression algo,
					 U_I compression_level,
					 const infinint & file_size,
					 const infinint & first_file_size,
					 bool root_ea,
					 bool user_ea,
					 const std::string & execute,
					 crypto_algo crypto,
					 const std::string & pass,
					 const mask &compr_mask,
					 const infinint & min_compr_size,
					 bool nodump,
					 bool ignore_owner,
					 const infinint & hourshift,
					 bool empty,
					 U_16 & exception,
					 std::string & except_msg);


    extern statistics op_extract(archive *arch,
				 const path &fs_root,
                                 const mask &selection,
                                 const mask &subtree,
                                 bool allow_over,
                                 bool warn_over,
                                 bool info_details,
                                 bool detruire,
                                 bool only_more_recent,
                                 bool restore_ea_root,
                                 bool restore_ea_user,
                                 bool flat,
                                 bool ignore_owner,
				 bool warn_remove_no_match,
                                 const infinint & hourshift,
				 bool empty);

    extern statistics op_extract_noexcept(archive *arch,
					  const path &fs_root,
					  const mask &selection,
					  const mask &subtree,
					  bool allow_over,
					  bool warn_over,
					  bool info_details,
					  bool detruire,
					  bool only_more_recent,
					  bool restore_ea_root,
					  bool restore_ea_user,
					  bool flat,
					  bool ignore_owner,
					  bool warn_remove_no_match,
					  const infinint & hourshift,
					  bool empty,
					  U_16 & exception,
					  std::string & except_msg);

    extern void op_listing(archive *arch,
                           bool info_details,
                           bool tar_format,
                           const mask &selection,
			   bool filter_unsaved);

    extern void op_listing_noexcept(archive  *arch,
				    bool info_details,
				    bool tar_format,
				    const mask &selection,
				    bool filter_unsaved,
				    U_16 & exception,
				    std::string & except_msg);


    extern statistics op_diff(archive *arch,
			      const path & fs_root,
                              const mask &selection,
                              const mask &subtree,
                              bool info_details,
                              bool check_ea_root,
                              bool check_ea_user,
                              bool ignore_owner);

    extern statistics op_diff_noexcept(archive *arch,
				       const path & fs_root,
				       const mask &selection,
				       const mask &subtree,
				       bool info_details,
				       bool check_ea_root,
				       bool check_ea_user,
				       bool ignore_owner,
				       U_16 & exception,
				       std::string & except_msg);

    extern statistics op_test(archive *arch,
                              const mask &selection,
                              const mask &subtree,
                              bool info_details);

    extern statistics op_test_noexcept(archive *arch,
				       const mask &selection,
				       const mask &subtree,
				       bool info_details,
				       U_16 & exception,
				       std::string & except_msg);

    extern void op_isolate(const path &sauv_path,
			   archive *ref_arch,
			   const std::string & filename,
			   const std::string & extension,
			   bool allow_over,
			   bool warn_over,
			   bool info_details,
			   bool pause,
			   compression algo,
			   U_I compression_level,
			   const infinint &file_size,
			   const infinint &first_file_size,
			   const std::string & execute,
			   crypto_algo crypto,
			   const std::string & pass);

    extern void op_isolate_noexcept(const path &sauv_path,
				    archive *ref_arch,
				    const std::string & filename,
				    const std::string & extension,
				    bool allow_over,
				    bool warn_over,
				    bool info_details,
				    bool pause,
				    compression algo,
				    U_I compression_level,
				    const infinint &file_size,
				    const infinint &first_file_size,
				    const std::string & execute,
				    crypto_algo crypto,
				    const std::string & pass,
				    U_16 & exception,
				    std::string & except_msg);

	// the get_childre_of() call need the API to be given a callback
	// function thanks to the set_op_tar_listing_callback()
	//
    extern bool get_children_of(archive *arch, const std::string & dir);
    extern bool get_children_of_noexcept(archive *arch, const std::string & dir,
					 U_16 & exception,
					 std::string & except_msg);


        // if callback is not set the tar-like output is displayed using user_interaction callback
        // if necessary you can give a callback function that will receive the split data in its
        // different fields.
	// if not mandatory for op_listing() call this callback function is required for
	// get_children_of() call.
	//
    extern void set_op_tar_listing_callback(void (*callback)(const std::string & flag,
                                                             const std::string & perm,
                                                             const std::string & uid,
                                                             const std::string & gid,
                                                             const std::string & size,
                                                             const std::string & date,
                                                             const std::string & filename));


        // routine provided to convert std::string to char *
        // returns the address to a newly allocated memory
        // which must be released calling the "delete"
        // operator when no more needed.
        // return null in case of error
    extern char *libdar_str2charptr_noexcept(const std::string & x, U_16 & exception, std::string & except_msg);

} // end of namespace

#endif
