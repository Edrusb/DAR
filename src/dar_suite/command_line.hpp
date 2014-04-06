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

    /// \file command_line.hpp
    /// \brief contains routing in charge of the command-line and included files parsing
    /// \ingroup CMDLINE

#ifndef COMMAND_LINE_HPP
#define COMMAND_LINE_HPP

#include "../my_config.h"
#include <string>
#include <vector>
#include "infinint.hpp"
#include "compressor.hpp"
#include "mask.hpp"
#include "path.hpp"
#include "catalogue.hpp"
#include "archive.hpp"
#include "criterium.hpp"

using namespace std;
using namespace libdar;

    /// \addtogroup CMDLINE
    /// @{

enum operation { noop, extract, create, diff, test, listing, isolate, merging, version_or_help };
    // noop stands for no-operation. get_args() never returns such value,
    // it is just necessary within the command_line module

enum dirty_behavior { dirtyb_ignore, dirtyb_warn, dirtyb_ok };

    // all parameters retreived from command-line
struct line_param
{
    operation op;                 //< which operation to perform
    path * fs_root;               //< filesystem root
    path * sauv_root;             //< where is the archive to operate on (create, read, etc.)
    string filename;              //< basename of the archive to operate on
    path * ref_root;              //< where is the archive of reference
    string * ref_filename;        //< basename of the archive of reference (NULL => no archive of reference)
    infinint file_size;           //< size of the slices to create (except the first)
    infinint first_file_size;     //< sice of the first slice to create
    mask * selection;             //< filter files for the operation based on filename only
    mask * subtree;               //< filter files for the operation based on path+filename
    bool allow_over;              //< whether to allow slice overwriting
    bool warn_over;               //< whether to warn before overwriting files or slices
    bool info_details;            //< whether to show processing messages
    bool display_treated;         //< whether to show treated files
    bool display_skipped;         //< whether to display skipped files
    compression algo;             //< compression algorithm to use when generating an archive
    U_I compression_level;        //< compression level to use when generating an archive
    infinint pause;               //< whether to pause between slices
    bool beep;                    //< whether to ring the terminal upon user interaction request
    bool empty_dir;               //< whether to store skipped directories as empty, whether to avoid restoring directory where no data is to be restored
    mask * ea_mask;               //< which EA to work on
    string input_pipe;            //< if not an empty string, name of the pipe through which to read data from dar_slave
    string output_pipe;           //< if not an empty string, name of the pipe through which to write orders to dar_slave
    inode::comparison_fields what_to_check; //< what fields to take into account when comparing/restoring files,
    string execute;               //< if not an empty string, the command to execute between slices
    string execute_ref;           //< if not an empty string, the command to execute between slices of the archive of reference
    secu_string pass;             //< if not an empty string, encrypt the archive with the given algo:pass string
    secu_string pass_ref;         //< if not an empty string, use the provided encryption scheme to read the archive of reference
    mask * compress_mask;         //< which file to compress
    bool flat;                    //< whether to ignore directory structure when restoring data
    infinint min_compr_size;      //< below which size to never try compressing files
    bool nodump;                  //< whether to ignore files having the "nodump" flag set when performing a backup
    bool exclude_by_ea;           //< whether inode have to be check against a given EA before backup
    string ea_name_for_exclusion; //< EA name to use for file exclusion, or empty string for the default EA name
    infinint hourshift;           //< consider equal two dates that have an integer hour of difference equal or less than hourshift
    bool warn_remove_no_match;    //< whether to warn file about to be removed during a restoration, when they to no match the expected type of file
    bool filter_unsaved;          //< whether to not list files that are not saved in the archive
    bool empty;                   //< whether to do a dry-run execution
    bool alter_atime;             //< whether to reset the atime of file read during backup to their original value (resetting atime does modify ctime)
    bool same_fs;                 //< whether to stick to a same filesystem
    bool snapshot;                //< whether to perform a snapshot backup
    bool cache_directory_tagging; //< whether to ignore directory contents where a the cache directory tagging files is found
    U_32 crypto_size;             //< block size by which to cypher data
    U_32 crypto_size_ref;         //< block size by which to uncypher data from the archive of reference
    archive_options_listing::listformat list_mode; //< type of listing to follow
    path * aux_root;              //< where is the auxiliary archive of reference [used for merging but also when creating an archive, for the on-fly isolation]
    string * aux_filename;        //< basename of the auxiliary archive if reference (NULL => no auxiliary of reference)
    secu_string aux_pass;         //< crypto to use for the auxiliary archive
    string aux_execute;           //< command to be run between the slice of the auxiliary archive of reference
    U_32 aux_crypto_size;         //< block size by which to cypher/uncypher data to/from the auxiliary archive of reference
    bool keep_compressed;         //< when merging, whether to not uncompress/re-compress data in the process
    infinint fixed_date;          //< the data for the snapshot backup
    bool quiet;                   //< whether to display final summary for the operation
    const crit_action * overwrite;//< the overwriting policy
    string slice_perm;            //< permission to set when creating a slice
    string slice_user;            //< user to set when creating a slice
    string slice_group;           //< group to set when creating a slice
    infinint repeat_count;        //< number of time to try saving a file if it changes at the time it is read for backup
    infinint repeat_byte;         //< archive total maximum amount of byte to waste re-saving changing files
    bool decremental;             //< whether to produce a decremental backup (when merging)
    bool furtive_read_mode;       //< whether to use the furtive read mode
    bool lax;                     //< whether to activate the last chance recovery mode (use with caution!)
    bool use_sequential_marks;    //< whether to add escape sequential marks in the archive
    bool sequential_read;         //< whether to follow escape sequential marks to achieve a sequential reading of the archive
    infinint sparse_file_min_size;//< minimum size of a zeroed byte sequence to be considered as a hole and stored this way in the archive
    dirty_behavior dirty;         //< what to do when comes the time to restore a file that is flagged as dirty
    bool  security_check;         //< whether to signal possible root-kit presence
    string user_comment;          //< user comment to add to the archive
    hash_algo hash;               //< whether to produce a hash file, and which algoritm to use for that hash
    infinint num_digits;          //< minimum number of decimal for the slice number
    infinint ref_num_digits;      //< minimum number of decimal for the slice number of the archive of reference
    infinint aux_num_digits;      //< minimum number of decimal for the slice number of the auxiliary archive of reference
    bool only_deleted;            //< whether to only consider deleted files
    bool not_deleted;             //< whether to ignore deleted files
    mask * backup_hook_mask;      //< which file have to considered for backup hook
    string backup_hook_execute;   //< which command to execute as backup hook
    bool list_ea;                 //< whether to list Extended Attribute of files
    bool ignore_unknown_inode;    //< whether to ignore unknown inode types
    bool no_compare_symlink_date; //< whether to report difference in dates of symlinks while diffing an archive with filesystem
    fsa_scope scope;              //< FSA scope to consider for the operation

	// constructor for line_param
    line_param()
    {
	fs_root= NULL;
	sauv_root = NULL;
	ref_root = NULL;
	selection = NULL;
	subtree = NULL;
	ref_filename = NULL;
	ea_mask = NULL;
	compress_mask = NULL;
	aux_root = NULL;
	aux_filename = NULL;
	overwrite = NULL;
	backup_hook_mask = NULL;
    };

	// destructor for line_param
    ~line_param()
    {
	if(fs_root != NULL)
	    delete fs_root;
	if(sauv_root != NULL)
	    delete sauv_root;
	if(ref_root != NULL)
	    delete ref_root;
	if(selection != NULL)
	    delete selection;
	if(subtree != NULL)
	    delete subtree;
	if(ref_filename != NULL)
	    delete ref_filename;
	if(ea_mask != NULL)
	    delete ea_mask;
	if(compress_mask != NULL)
	    delete compress_mask;
	if(aux_root != NULL)
	    delete aux_root;
	if(aux_filename != NULL)
	    delete aux_filename;
	if(overwrite != NULL)
	    delete overwrite;
	if(backup_hook_mask != NULL)
	    delete backup_hook_mask;
    };
};

    /// main routine to extract parameters from command-line and included files

extern bool get_args(user_interaction & dialog,
		     const char *home,
		     const vector<string> & dar_dcf_path,
		     const vector<string> & dar_duc_path,
                     S_I argc,
		     char * const argv[],
		     line_param & param);

    /// @}

#if HAVE_GETOPT_LONG
const struct option *get_long_opt();
#endif

const char *get_short_opt();
#endif
