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
// $Id: libdar.hpp,v 1.12.2.2 2003/11/29 09:18:49 edrusb Rel $
//
/*********************************************************************/
//

#ifndef LIBDAR_H
#define LIBDAR_H

#include "../my_config.h"
#include <string>
#include "compressor.hpp"
#include "path.hpp"
#include "mask.hpp"
#include "integers.hpp"
#include "infinint.hpp"
#include "statistics.hpp"
#include "user_interaction.hpp"
#include "deci.hpp"

namespace libdar
{
    const U_I LIBDAR_COMPILE_TIME_MAJOR = 1;
    const U_I LIBDAR_COMPILE_TIME_MINOR = 2;

    extern void get_version(U_I & major, U_I & minor);
    extern void get_compile_time_features(bool & ea, bool & largefile, bool & nodump, bool & special_alloc, U_I & bits);

///////////////////////
// IMPORTANT !
// two callback functions must be given to darlib for it to be operational
// see user_interaction.hpp file.
//////////////////////

    extern statistics op_create(const path &fs_root,
                                const path &sauv_path,
                                const path *ref_path,
                                const mask &selection,
                                const mask &subtree,
                                const std::string &filename,
                                const std::string & extension,
                                const std::string *ref_filename,
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
                                const std::string & input_pipe,
                                const std::string & output_pipe,
                                const std::string & execute,
                                const std::string & execute_ref,
                                const std::string & pass,
                                const std::string & pass_ref,
                                const mask &compr_mask,
                                const infinint & min_compr_size,
                                bool nodump,
                                bool ignore_owner,
                                const infinint & hourshift);

    extern statistics op_extract(const path &fs_root,
                                 const path &sauv_path,
                                 const mask &selection,
                                 const mask &subtree,
                                 const std::string &filename,
                                 const std::string & extension,
                                 bool allow_over,
                                 bool warn_over,
                                 bool info_details,
                                 bool detruire,
                                 bool only_more_recent,
                                 bool restore_ea_root,
                                 bool restore_ea_user,
                                 const std::string &input_pipe,
                                 const std::string &output_pipe,
                                 const std::string & execute,
                                 const std::string & pass,
                                 bool flat,
                                 bool ignore_owner,
                                 const infinint & hourshift);

    extern statistics op_diff(const path & fs_root,
                              const path &sauv_path,
                              const mask &selection,
                              const mask &subtree,
                              const std::string &filename,
                              const std::string & extension,
                              bool info_details,
                              bool check_ea_root,
                              bool check_ea_user,
                              const std::string &input_pipe,
                              const std::string &output_pipe,
                              const std::string & execute,
                              const std::string & pass,
                              bool ignore_owner);

    extern void op_listing(const path &sauv_path,
                           const std::string & filename,
                           const std::string & extension,
                           bool info_details,
                           bool tar_format,
                           const std::string &input_pipe,
                           const std::string &output_pipe,
                           const std::string & execute,
                           const std::string & pass,
                           const mask &selection);

    extern statistics op_test(const path &sauv_path,
                              const mask &selection,
                              const mask &subtree,
                              const std::string & filename,
                              const std::string & extension,
                              bool info_details,
                              const std::string &input_pipe,
                              const std::string &output_pipe,
                              const std::string & execute,
                              const std::string & pass);

    extern void op_isolate(const path &sauv_path,
                           const path *ref_path,
                           const std::string & filename,
                           const std::string & extension,
                           const std::string *ref_filename,
                           bool allow_over,
                           bool warn_over,
                           bool info_details,
                           bool pause,
                           compression algo,
                           U_I compression_level,
                           const infinint &file_size,
                           const infinint &first_file_size,
                           const std::string &input_pipe,
                           const std::string &output_pipe,
                           const std::string & execute,
                           const std::string & execute_ref,
                           const std::string & pass,
                           const std::string & pass_ref);

        // if callback is not set the tar-like output is displayed using user_interaction callback
        // if necessary you can give a callback function that will receive the split data in its
        // different fields.
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
    extern char *libdar_str2charptr(const std::string & x);

} // end of namespace

#endif
