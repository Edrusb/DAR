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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

#include "archive_options.hpp"
#include "entrepot_local.hpp"

using namespace std;

namespace libdar
{
	// general default values

    static const U_32 default_crypto_size = 10240;
    static const path default_ref_chem = "/";
    static const U_I default_min_compr_size = 100;
    static const cat_inode::comparison_fields default_comparison_fields = cat_inode::cf_all;
    static const crit_constant_action default_crit_action = crit_constant_action(data_preserve, EA_preserve);
    static const string default_user_comment = "N/A";
    static const U_32 default_delta_sig_min_size = 10240;

	// some local helper functions

    inline void archive_option_destroy_mask(mask * & ptr);
    inline void archive_option_clean_mask(mask * & ptr, memory_pool *pool, bool all = true);
    inline void archive_option_destroy_crit_action(crit_action * & ptr);
    inline void archive_option_clean_crit_action(crit_action * & ptr);


	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////


    inline void archive_option_clean_mask(mask * & ptr, memory_pool *pool, bool all)
    {
	archive_option_destroy_mask(ptr);
	ptr = new (pool) bool_mask(all);
	if(ptr == nullptr)
	    throw Ememory("archive_option_clean_mask");
    }

    inline void archive_option_destroy_mask(mask * & ptr)
    {
	if(ptr != nullptr)
	{
	    delete ptr;
	    ptr = nullptr;
	}
    }

    inline void archive_option_clean_crit_action(crit_action * & ptr)
    {
	archive_option_destroy_crit_action(ptr);
	ptr = default_crit_action.clone();
	if(ptr == nullptr)
	    throw Ememory("archive_options::archive_option_clean_crit_action");
    }

    inline void archive_option_destroy_crit_action(crit_action * & ptr)
    {
	if(ptr != nullptr)
	{
	    delete ptr;
	    ptr = nullptr;
	}
    }


	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////

    archive_options_read::archive_options_read() : x_ref_chem(default_ref_chem)
    {
	x_entrepot = nullptr;
	x_ref_entrepot = nullptr;
	clear();
    }

    void archive_options_read::clear()
    {

	    // setting the default values for all options

	destroy();

	x_crypto = crypto_none;
	x_pass.clear();
	x_crypto_size = default_crypto_size;
	x_input_pipe = "";
	x_output_pipe = "";
	x_execute = "";
	x_info_details = false;
	x_lax = false;
	x_sequential_read = false;
	x_slice_min_digits = 0;
	x_entrepot = new (get_pool()) entrepot_local("", "", false); // never using furtive_mode to read slices
	if(x_entrepot == nullptr)
	    throw Ememory("archive_options_read::clear");
	x_ignore_signature_check_failure = false;
	x_multi_threaded = true;

	    //
	external_cat = false;
	x_ref_chem = default_ref_chem;
	x_ref_basename = "";
	x_ref_crypto = crypto_none;
	x_ref_pass.clear();
	x_ref_crypto_size = default_crypto_size;
	x_ref_execute = "";
	x_ref_slice_min_digits = 0;
	x_ref_entrepot = new (get_pool()) entrepot_local("", "", false); // never using furtive_mode to read slices
	if(x_ref_entrepot == nullptr)
	    throw Ememory("archive_options_read::clear");
	x_header_only = false;
    }

    void archive_options_read::set_default_crypto_size()
    {
	x_crypto_size = default_crypto_size;
	x_ref_crypto_size = default_crypto_size;
    }

    void archive_options_read::unset_external_catalogue()
    {
	x_ref_chem = default_ref_chem;
	x_ref_basename = "";
	external_cat = false;
    }

    const path & archive_options_read::get_ref_path() const
    {
        NLS_SWAP_IN;
        try
        {
	    if(!external_cat)
		throw Elibcall("archive_options_read::get_external_catalogue", gettext("Cannot get catalogue of reference as it has not been provided"));
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

	return x_ref_chem;
    }

    const string & archive_options_read::get_ref_basename() const
    {
	NLS_SWAP_IN;
	try
	{
	    if(!external_cat)
		throw Elibcall("archive_options_read::get_external_catalogue", gettext("Error, catalogue of reference has not been provided"));
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

	return x_ref_basename;

    }

    void archive_options_read::copy_from(const archive_options_read & ref)
    {
	x_crypto = ref.x_crypto;
	x_pass = ref.x_pass;
	x_crypto_size = ref.x_crypto_size;
	x_input_pipe = ref.x_input_pipe;
	x_output_pipe = ref.x_output_pipe;
	x_execute = ref.x_execute;
	x_info_details = ref.x_info_details;
	x_lax = ref.x_lax;
	x_sequential_read = ref.x_sequential_read;
	x_slice_min_digits = ref.x_slice_min_digits;
	if(ref.x_entrepot == nullptr)
	    throw SRC_BUG;
	x_entrepot = ref.x_entrepot->clone();
	if(x_entrepot == nullptr)
	    throw Ememory("archive_options_read::copy_from");
	x_ignore_signature_check_failure = ref.x_ignore_signature_check_failure;
	x_multi_threaded = ref.x_multi_threaded;
	    //

	external_cat = ref.external_cat;
	x_ref_chem = ref.x_ref_chem;
	x_ref_basename = ref.x_ref_basename;
	x_ref_crypto = ref.x_ref_crypto;
	x_ref_pass = ref.x_ref_pass;
	x_ref_crypto_size = ref.x_ref_crypto_size;
	x_ref_execute = ref.x_ref_execute;
	x_ref_slice_min_digits = ref.x_ref_slice_min_digits;
	if(ref.x_ref_entrepot == nullptr)
	    throw SRC_BUG;
	x_ref_entrepot = ref.x_ref_entrepot->clone();
	if(x_ref_entrepot == nullptr)
	    throw Ememory("archive_options_read::copy_from");
	x_header_only = ref.x_header_only;
    }

    void archive_options_read::destroy()
    {
	if(x_entrepot != nullptr)
	{
	    delete x_entrepot;
	    x_entrepot = nullptr;
	}
	if(x_ref_entrepot != nullptr)
	{
	    delete x_ref_entrepot;
	    x_ref_entrepot = nullptr;
	}
    }




	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////

    archive_options_create::archive_options_create()
    {
	x_selection = x_subtree = x_ea_mask = x_compr_mask = x_backup_hook_file_mask = x_delta_mask = nullptr;
	x_entrepot = nullptr;
	try
	{
	    clear();
	}
	catch(...)
	{
	    destroy();
	    throw;
	}
    }

    archive_options_create::archive_options_create(const archive_options_create & ref)
    {
	x_selection = x_subtree = x_ea_mask = x_compr_mask = x_backup_hook_file_mask = x_delta_mask = nullptr;
	x_entrepot = nullptr;
	try
	{
	    copy_from(ref);
	}
	catch(...)
	{
	    destroy();
	    throw;
	}
    }

    void archive_options_create::clear()
    {
	NLS_SWAP_IN;
	try
	{

	    destroy();

	    archive_option_clean_mask(x_selection, get_pool());
	    archive_option_clean_mask(x_subtree, get_pool());
	    archive_option_clean_mask(x_ea_mask, get_pool());
	    archive_option_clean_mask(x_compr_mask, get_pool());
	    archive_option_clean_mask(x_backup_hook_file_mask, get_pool(), false);
	    archive_option_clean_mask(x_delta_mask, get_pool());
	    x_ref_arch = nullptr;
	    x_allow_over = true;
	    x_warn_over = true;
	    x_info_details = false;
	    x_display_treated = false;
	    x_display_treated_only_dir = false;
	    x_display_skipped = false;
	    x_display_finished = false;
	    x_pause = 0;
	    x_empty_dir = false;
	    x_compr_algo = none;
	    x_compression_level = 9;
	    x_file_size = 0;
	    x_first_file_size = 0;
	    x_execute = "";
	    x_crypto = crypto_none;
	    x_pass.clear();
	    x_crypto_size = default_crypto_size;
	    x_gnupg_recipients.clear();
	    x_gnupg_signatories.clear();
	    x_min_compr_size = default_min_compr_size;
	    x_nodump = false;
	    exclude_by_ea = "";
	    x_what_to_check = default_comparison_fields;
	    x_hourshift = 0;
	    x_empty = false;
	    x_alter_atime = true;
	    x_old_alter_atime = true;
#if FURTIVE_READ_MODE_AVAILABLE
	    x_furtive_read = true;
#else
	    x_furtive_read = false;
#endif
	    x_same_fs = false;
	    x_snapshot = false;
	    x_cache_directory_tagging = false;
	    x_fixed_date = 0;
	    x_slice_permission = "";
	    x_slice_user_ownership = "";
	    x_slice_group_ownership = "";
	    x_repeat_count = 3;
	    x_repeat_byte = 1;
	    x_sequential_marks = true;
	    x_sparse_file_min_size = 15;  // min value to activate the feature (0 means no detection of sparse_file)
	    x_security_check = true;
	    x_user_comment = default_user_comment;
	    x_hash = hash_none;
	    x_slice_min_digits = 0;
	    x_backup_hook_file_execute = "";
	    x_ignore_unknown = false;
	    x_entrepot = new (get_pool()) entrepot_local( "", "", false); // never using furtive_mode to read slices
	    if(x_entrepot == nullptr)
		throw Ememory("archive_options_create::clear");
	    x_scope = all_fsa_families();
	    x_multi_threaded = true;
	    x_delta_diff = true;
	    x_delta_signature = false;
	    has_delta_mask_been_set = false;
	    x_delta_sig_min_size = default_delta_sig_min_size;
	    x_ignored_as_symlink.clear();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_create::set_selection(const mask & selection)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_selection);
	    x_selection = selection.clone();
	    if(x_selection == nullptr)
		throw Ememory("archive_options_create::set_selection");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_create::set_subtree(const mask & subtree)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_subtree);
	    x_subtree = subtree.clone();
	    if(x_subtree == nullptr)
		throw Ememory("archive_options_create::set_subtree");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_create::set_ea_mask(const mask & ea_mask)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_ea_mask);
	    x_ea_mask = ea_mask.clone();
	    if(x_ea_mask == nullptr)
		throw Ememory("archive_options_create::set_ea_mask");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_create::set_compr_mask(const mask & compr_mask)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_compr_mask);
	    x_compr_mask = compr_mask.clone();
	    if(x_compr_mask == nullptr)
		throw Ememory("archive_options_create::set_compr_mask");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_create::set_backup_hook(const std::string & execute, const mask & which_files)
    {

	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_backup_hook_file_mask);
	    x_backup_hook_file_mask = which_files.clone();
	    if(x_backup_hook_file_mask == nullptr)
		throw Ememory("archive_options_create::set_backup_hook");

	    x_backup_hook_file_execute = execute;
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_create::set_entrepot(const entrepot & entr)
    {
	if(x_entrepot != nullptr)
	    delete x_entrepot;

	x_entrepot = entr.clone();
	if(x_entrepot == nullptr)
	    throw Ememory("archive_options_create::set_entrepot");
    }

    void archive_options_create::set_delta_mask(const mask & delta_mask)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_delta_mask);
	    x_delta_mask = delta_mask.clone();
	    if(x_delta_mask == nullptr)
		throw Ememory("archive_options_create::set_delta_mask");
	    has_delta_mask_been_set = true;
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_create::destroy()
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_selection);
	    archive_option_destroy_mask(x_subtree);
	    archive_option_destroy_mask(x_ea_mask);
	    archive_option_destroy_mask(x_compr_mask);
	    archive_option_destroy_mask(x_backup_hook_file_mask);
	    archive_option_destroy_mask(x_delta_mask);
	    if(x_entrepot != nullptr)
	    {
		delete x_entrepot;
		x_entrepot = nullptr;
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_create::copy_from(const archive_options_create & ref)
    {
	x_selection = nullptr;
	x_subtree = nullptr;
	x_ea_mask = nullptr;
	x_compr_mask = nullptr;
	x_backup_hook_file_mask = nullptr;
	x_entrepot = nullptr;
	x_delta_mask = nullptr;

	if(ref.x_selection == nullptr)
	    throw SRC_BUG;
	if(ref.x_subtree == nullptr)
	    throw SRC_BUG;
	if(ref.x_ea_mask == nullptr)
	    throw SRC_BUG;
	if(ref.x_compr_mask == nullptr)
	    throw SRC_BUG;
	if(ref.x_backup_hook_file_mask == nullptr)
	    throw SRC_BUG;
	x_selection = ref.x_selection->clone();
	x_subtree = ref.x_subtree->clone();
	x_ea_mask = ref.x_ea_mask->clone();
	x_compr_mask = ref.x_compr_mask->clone();
	x_backup_hook_file_mask = ref.x_backup_hook_file_mask->clone();

	if(x_selection == nullptr || x_subtree == nullptr || x_ea_mask == nullptr || x_compr_mask == nullptr || x_backup_hook_file_mask == nullptr)
	    throw Ememory("archive_options_create::copy_from");

	x_ref_arch = ref.x_ref_arch;
	x_allow_over = ref.x_allow_over;
	x_warn_over = ref.x_warn_over;
	x_info_details = ref.x_info_details;
	x_display_treated = ref.x_display_treated;
	x_display_treated_only_dir = ref.x_display_treated_only_dir;
	x_display_skipped = ref.x_display_skipped;
	x_display_finished = ref.x_display_finished;
	x_pause = ref.x_pause;
	x_empty_dir = ref.x_empty_dir;
	x_compr_algo = ref.x_compr_algo;
	x_compression_level = ref.x_compression_level;
	x_file_size = ref.x_file_size;
	x_first_file_size = ref.x_first_file_size;
	x_execute = ref.x_execute;
	x_crypto = ref.x_crypto;
	x_pass = ref.x_pass;
	x_crypto_size = ref.x_crypto_size;
	x_gnupg_recipients = ref.x_gnupg_recipients;
	x_gnupg_signatories = ref.x_gnupg_signatories;
	x_min_compr_size = ref.x_min_compr_size;
	x_nodump = ref.x_nodump;
	x_what_to_check = ref.x_what_to_check;
	x_hourshift = ref.x_hourshift;
	x_empty = ref.x_empty;
	x_alter_atime = ref.x_alter_atime;
	x_old_alter_atime = ref.x_old_alter_atime;
	x_furtive_read = ref.x_furtive_read;
	x_same_fs = ref.x_same_fs;
	x_snapshot = ref.x_snapshot;
	x_cache_directory_tagging = ref.x_cache_directory_tagging;
	x_fixed_date = ref.x_fixed_date;
	x_slice_permission = ref.x_slice_permission;
	x_slice_user_ownership = ref.x_slice_user_ownership;
	x_slice_group_ownership = ref.x_slice_group_ownership;
	x_repeat_count = ref.x_repeat_count;
	x_repeat_byte = ref.x_repeat_byte;
	x_sequential_marks = ref.x_sequential_marks;
	x_sparse_file_min_size = ref.x_sparse_file_min_size;
	x_security_check = ref.x_security_check;
	x_user_comment = ref.x_user_comment;
	x_hash = ref.x_hash;
	x_slice_min_digits = ref.x_slice_min_digits;
	x_backup_hook_file_execute = ref.x_backup_hook_file_execute;
	x_ignore_unknown = ref.x_ignore_unknown;
	if(x_entrepot != nullptr)
	    throw SRC_BUG;
	x_entrepot = ref.x_entrepot->clone();
	if(x_entrepot == nullptr)
	    throw Ememory("archive_options_create::copy_from");
	x_scope = ref.x_scope;
	x_multi_threaded = ref.x_multi_threaded;
	x_delta_diff = ref.x_delta_diff;
	x_delta_signature = ref.x_delta_signature;
	x_delta_mask = ref.x_delta_mask->clone();
	has_delta_mask_been_set = ref.has_delta_mask_been_set;
	x_delta_sig_min_size = ref.x_delta_sig_min_size;
	x_ignored_as_symlink = ref.x_ignored_as_symlink;
    }


	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////

    archive_options_isolate::archive_options_isolate()
    {
	x_entrepot = nullptr;
	x_delta_mask = nullptr;
	try
	{
	    clear();
	}
	catch(...)
	{
	    destroy();
	    throw;
	}
    }

    archive_options_isolate::archive_options_isolate(const archive_options_isolate & ref)
    {
	x_entrepot = nullptr;
	x_delta_mask = nullptr;
	try
	{
	    copy_from(ref);
	}
	catch(...)
	{
	    destroy();
	    throw;
	}
    }

    void archive_options_isolate::clear()
    {
	NLS_SWAP_IN;
	try
	{
	    destroy();

	    x_allow_over = true;
	    x_warn_over = true;
	    x_info_details = false;
	    x_pause = 0;
	    x_algo = none;
	    x_compression_level = 9;
	    x_file_size = 0;
	    x_first_file_size = 0;
	    x_execute = "";
	    x_crypto = crypto_none;
	    x_pass.clear();
	    x_crypto_size = default_crypto_size;
	    x_gnupg_recipients.clear();
	    x_gnupg_signatories.clear();
	    x_empty = false;
	    x_slice_permission = "";
	    x_slice_user_ownership = "";
	    x_slice_group_ownership = "";
	    x_user_comment = default_user_comment;
	    x_hash = hash_none;
	    x_slice_min_digits = 0;
	    x_sequential_marks = true;
	    x_entrepot = new (get_pool()) entrepot_local("", "", false); // never using furtive_mode to read slices
	    if(x_entrepot == nullptr)
		throw Ememory("archive_options_isolate::clear");
	    x_multi_threaded = true;
	    x_delta_signature = false;
	    archive_option_clean_mask(x_delta_mask, get_pool());
	    has_delta_mask_been_set = false;
	    x_delta_sig_min_size = default_delta_sig_min_size;
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_isolate::set_entrepot(const entrepot & entr)
    {
	if(x_entrepot != nullptr)
	    delete x_entrepot;

	x_entrepot = entr.clone();
	if(x_entrepot == nullptr)
	    throw Ememory("archive_options_isolate::set_entrepot");
    }

    void archive_options_isolate::set_delta_mask(const mask & delta_mask)
    {
	NLS_SWAP_IN;
	try
	{
	    if(!compile_time::librsync())
		throw Ecompilation("librsync");
	    else
	    {
		archive_option_destroy_mask(x_delta_mask);
		x_delta_mask = delta_mask.clone();
		if(x_delta_mask == nullptr)
		    throw Ememory("archive_options_create::set_delta_mask");
		has_delta_mask_been_set = true;
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_isolate::destroy()
    {
	archive_option_destroy_mask(x_delta_mask);
	if(x_entrepot != nullptr)
	{
	    delete x_entrepot;
	    x_entrepot = nullptr;
	}
    }

    void archive_options_isolate::copy_from(const archive_options_isolate & ref)
    {
	x_allow_over = ref.x_allow_over;
	x_warn_over = ref.x_warn_over;
	x_info_details = ref.x_info_details;
	x_pause = ref.x_pause;
	x_algo = ref.x_algo;
	x_compression_level = ref.x_compression_level;
	x_file_size = ref.x_file_size;
	x_first_file_size = ref.x_first_file_size;
	x_execute = ref.x_execute;
	x_crypto = ref.x_crypto;
	x_pass = ref.x_pass;
	x_crypto_size = ref.x_crypto_size;
	x_gnupg_recipients = ref.x_gnupg_recipients;
	x_gnupg_signatories = ref.x_gnupg_signatories;
	x_empty = ref.x_empty;
	x_slice_permission = ref.x_slice_permission;
	x_slice_user_ownership = ref.x_slice_user_ownership;
	x_slice_group_ownership = ref.x_slice_group_ownership;
	x_user_comment = ref.x_user_comment;
	x_hash = ref.x_hash;
	x_slice_min_digits = ref.x_slice_min_digits;
	x_sequential_marks = ref.x_sequential_marks;
	if(ref.x_entrepot == nullptr)
	    throw SRC_BUG;
	x_entrepot = ref.x_entrepot->clone();
	if(x_entrepot == nullptr)
	    throw Ememory("archive_options_isolate::copy_from");
	x_multi_threaded = ref.x_multi_threaded;
	x_delta_signature = ref.x_delta_signature;
	x_delta_mask = ref.x_delta_mask->clone();
	has_delta_mask_been_set = ref.has_delta_mask_been_set;
	x_delta_sig_min_size = ref.x_delta_sig_min_size;
    }


	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////


    void archive_options_merge::clear()
    {
	NLS_SWAP_IN;
	try
	{
	    destroy();

	    archive_option_clean_mask(x_selection, get_pool());
	    archive_option_clean_mask(x_subtree, get_pool());
	    archive_option_clean_mask(x_ea_mask, get_pool());
	    archive_option_clean_mask(x_compr_mask, get_pool());
	    archive_option_clean_mask(x_delta_mask, get_pool());
	    archive_option_clean_crit_action(x_overwrite);
	    x_ref = nullptr;
	    x_allow_over = true;
	    x_warn_over = true;
	    x_info_details = false;
	    x_display_treated = false;
	    x_display_treated_only_dir = false;
	    x_display_skipped = false;
	    x_pause = 0;
	    x_empty_dir = false;
	    x_compr_algo = none;
	    x_compression_level = 9;
	    x_file_size = 0;
	    x_first_file_size = 0;
	    x_execute = "";
	    x_crypto = crypto_none;
	    x_pass.clear();
	    x_crypto_size = default_crypto_size;
	    x_gnupg_recipients.clear();
	    x_gnupg_signatories.clear();
	    x_min_compr_size = default_min_compr_size;
	    x_empty = false;
	    x_keep_compressed = false;
	    x_slice_permission = "";
	    x_slice_user_ownership = "";
	    x_slice_group_ownership = "";
	    x_decremental = false;
	    x_sequential_marks = true;
	    x_sparse_file_min_size = 0; // disabled by default
	    x_user_comment = default_user_comment;
	    x_hash = hash_none;
	    x_slice_min_digits = 0;
	    x_entrepot = new (get_pool()) entrepot_local("", "", false); // never using furtive_mode to read slices
	    if(x_entrepot == nullptr)
		throw Ememory("archive_options_merge::clear");
	    x_scope = all_fsa_families();
	    x_multi_threaded = true;
	    x_delta_signature = true;
	    has_delta_mask_been_set = false;
	    x_delta_sig_min_size = default_delta_sig_min_size;
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_merge::set_selection(const mask & selection)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_selection);
	    x_selection = selection.clone();
	    if(x_selection == nullptr)
		throw Ememory("archive_options_merge::set_selection");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_merge::set_subtree(const mask & subtree)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_subtree);
	    x_subtree = subtree.clone();
	    if(x_subtree == nullptr)
		throw Ememory("archive_options_merge::set_subtree");
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void archive_options_merge::set_overwriting_rules(const crit_action & overwrite)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_crit_action(x_overwrite);
	    x_overwrite = overwrite.clone();
	    if(x_overwrite == nullptr)
		throw Ememory("archive_options_merge::set_overwriting_rules");
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void archive_options_merge::set_ea_mask(const mask & ea_mask)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_ea_mask);
	    x_ea_mask = ea_mask.clone();
	    if(x_ea_mask == nullptr)
		throw Ememory("archive_options_merge::set_ea_mask");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_merge::set_compr_mask(const mask & compr_mask)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_compr_mask);
	    x_compr_mask = compr_mask.clone();
	    if(x_compr_mask == nullptr)
		throw Ememory("archive_options_merge::set_compr_mask");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

    }

    void archive_options_merge::set_delta_mask(const mask & delta_mask)
    {
	NLS_SWAP_IN;
	try
	{
	    if(!compile_time::librsync())
		throw Ecompilation("librsync");
	    else
	    {
		archive_option_destroy_mask(x_delta_mask);
		x_delta_mask = delta_mask.clone();
		if(x_delta_mask == nullptr)
		    throw Ememory("archive_options_create::set_delta_mask");
		has_delta_mask_been_set = true;
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_merge::set_entrepot(const entrepot & entr)
    {
	if(x_entrepot != nullptr)
	    delete x_entrepot;

	x_entrepot = entr.clone();
	if(x_entrepot == nullptr)
	    throw Ememory("archive_options_merge::set_entrepot");
    }


    void archive_options_merge::destroy()
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_selection);
	    archive_option_destroy_mask(x_subtree);
	    archive_option_destroy_mask(x_ea_mask);
	    archive_option_destroy_mask(x_compr_mask);
	    archive_option_destroy_mask(x_delta_mask);
	    archive_option_destroy_crit_action(x_overwrite);
	    if(x_entrepot != nullptr)
	    {
		delete x_entrepot;
		x_entrepot = nullptr;
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_merge::copy_from(const archive_options_merge & ref)
    {
	x_selection = nullptr;
	x_subtree = nullptr;
	x_ea_mask = nullptr;
	x_compr_mask = nullptr;
	x_overwrite = nullptr;
	x_entrepot = nullptr;
	x_delta_mask = nullptr;

	try
	{
	    if(ref.x_selection == nullptr)
		throw SRC_BUG;
	    if(ref.x_subtree == nullptr)
		throw SRC_BUG;
	    if(ref.x_ea_mask == nullptr)
		throw SRC_BUG;
	    if(ref.x_compr_mask == nullptr)
		throw SRC_BUG;
	    if(ref.x_overwrite == nullptr)
		throw SRC_BUG;
	    if(ref.x_entrepot == nullptr)
		throw SRC_BUG;
	    if(ref.x_delta_mask == nullptr)
		throw SRC_BUG;

	    x_selection = ref.x_selection->clone();
	    x_subtree = ref.x_subtree->clone();
	    x_ea_mask = ref.x_ea_mask->clone();
	    x_compr_mask = ref.x_compr_mask->clone();
	    x_overwrite = ref.x_overwrite->clone();
	    x_entrepot = ref.x_entrepot->clone();
	    x_delta_mask = ref.x_delta_mask->clone();

	    if(x_selection == nullptr
	       || x_subtree == nullptr
	       || x_ea_mask == nullptr
	       || x_compr_mask == nullptr
	       || x_overwrite == nullptr
	       || x_entrepot == nullptr
	       || x_delta_mask == nullptr)
		throw Ememory("archive_options_extract::copy_from");

	    x_ref = ref.x_ref;
	    x_allow_over = ref.x_allow_over;
	    x_warn_over = ref.x_warn_over;
	    x_info_details = ref.x_info_details;
	    x_display_treated = ref.x_display_treated;
	    x_display_treated_only_dir = ref.x_display_treated_only_dir;
	    x_display_skipped = ref.x_display_skipped;
	    x_pause = ref.x_pause;
	    x_empty_dir = ref.x_empty_dir;
	    x_compr_algo = ref.x_compr_algo;
	    x_compression_level = ref.x_compression_level;
	    x_file_size = ref.x_file_size;
	    x_first_file_size = ref.x_first_file_size;
	    x_execute = ref.x_execute;
	    x_crypto = ref.x_crypto;
	    x_pass = ref.x_pass;
	    x_crypto_size = ref.x_crypto_size;
	    x_gnupg_recipients = ref.x_gnupg_recipients;
	    x_gnupg_signatories = ref.x_gnupg_signatories;
	    x_min_compr_size = ref.x_min_compr_size;
	    x_empty = ref.x_empty;
	    x_keep_compressed = ref.x_keep_compressed;
	    x_slice_permission = ref.x_slice_permission;
	    x_slice_user_ownership = ref.x_slice_user_ownership;
	    x_slice_group_ownership = ref.x_slice_group_ownership;
	    x_decremental = ref.x_decremental;
	    x_sequential_marks = ref.x_sequential_marks;
	    x_sparse_file_min_size = ref.x_sparse_file_min_size;
	    x_user_comment = ref.x_user_comment;
	    x_hash = ref.x_hash;
	    x_slice_min_digits = ref.x_slice_min_digits;
	    x_scope = ref.x_scope;
	    x_multi_threaded = ref.x_multi_threaded;
	    x_delta_signature = ref.x_delta_signature;
	    has_delta_mask_been_set = ref.has_delta_mask_been_set;
	    x_delta_sig_min_size = ref.x_delta_sig_min_size;
	}
	catch(...)
	{
	    clear();
	    throw;
	}
    }

	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////


    void archive_options_extract::clear()
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_clean_mask(x_selection, get_pool());
	    archive_option_clean_mask(x_subtree, get_pool());
	    archive_option_clean_mask(x_ea_mask, get_pool());
	    archive_option_clean_crit_action(x_overwrite);
	    x_warn_over = true;
	    x_info_details = false;
	    x_display_treated = false;
	    x_display_treated_only_dir = false;
	    x_display_skipped = false;
	    x_flat = false;
	    x_what_to_check = default_comparison_fields;
	    x_warn_remove_no_match = true;
	    x_empty = false;
	    x_empty_dir = true;
	    x_dirty = dirty_warn;
	    x_only_deleted = false;
	    x_ignore_deleted = false;
	    x_scope = all_fsa_families();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_extract::set_selection(const mask & selection)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_selection);
	    x_selection = selection.clone();
	    if(x_selection == nullptr)
		throw Ememory("archive_options_extract::set_selection");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_extract::set_subtree(const mask & subtree)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_subtree);
	    x_subtree = subtree.clone();
	    if(x_subtree == nullptr)
		throw Ememory("archive_options_extract::set_subtree");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_extract::set_ea_mask(const mask & ea_mask)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_ea_mask);
	    x_ea_mask = ea_mask.clone();
	    if(x_ea_mask == nullptr)
		throw Ememory("archive_options_extract::set_ea_mask");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_extract::set_overwriting_rules(const crit_action & over)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_crit_action(x_overwrite);
	    x_overwrite = over.clone();
	    if(x_overwrite == nullptr)
		throw Ememory("archive_options_extract::set_overwriting_rules");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_extract::destroy()
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_selection);
	    archive_option_destroy_mask(x_subtree);
	    archive_option_destroy_mask(x_ea_mask);
	    archive_option_destroy_crit_action(x_overwrite);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_extract::copy_from(const archive_options_extract & ref)
    {
	x_selection = nullptr;
	x_subtree = nullptr;
	x_ea_mask = nullptr;
	x_overwrite = nullptr;

	try
	{
	    if(ref.x_selection == nullptr)
		throw SRC_BUG;
	    if(ref.x_subtree == nullptr)
		throw SRC_BUG;
	    if(ref.x_ea_mask == nullptr)
		throw SRC_BUG;
	    if(ref.x_overwrite == nullptr)
		throw SRC_BUG;
	    x_selection = ref.x_selection->clone();
	    x_subtree = ref.x_subtree->clone();
	    x_ea_mask = ref.x_ea_mask->clone();
	    x_overwrite = ref.x_overwrite->clone();

	    if(x_selection == nullptr || x_subtree == nullptr || x_ea_mask == nullptr || x_overwrite == nullptr)
		throw Ememory("archive_options_extract::copy_from");

	    x_warn_over = ref.x_warn_over;
	    x_info_details = ref.x_info_details;
	    x_display_treated = ref.x_display_treated;
	    x_display_treated_only_dir = ref.x_display_treated_only_dir;
	    x_display_skipped = ref.x_display_skipped;
	    x_flat = ref.x_flat;
	    x_what_to_check = ref.x_what_to_check;
	    x_warn_remove_no_match = ref.x_warn_remove_no_match;
	    x_empty = ref.x_empty;
	    x_empty_dir = ref.x_empty_dir;
	    x_dirty = ref.x_dirty;
	    x_only_deleted = ref.x_only_deleted;
	    x_ignore_deleted = ref.x_ignore_deleted;
	    x_scope = ref.x_scope;
	}
	catch(...)
	{
	    clear();
	    throw;
	}
    }

	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////

    void archive_options_listing::clear()
    {
	NLS_SWAP_IN;
	try
	{
	    destroy();

	    x_info_details = false;
	    x_list_mode = normal;
	    archive_option_clean_mask(x_selection, get_pool());
	    archive_option_clean_mask(x_subtree, get_pool());
	    x_filter_unsaved = false;
	    x_display_ea = false;
	    x_sizes_in_bytes = false;
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    void archive_options_listing::set_selection(const mask & selection)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_selection);
	    x_selection = selection.clone();
	    if(x_selection == nullptr)
		throw Ememory("archive_options_listing::set_selection");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_listing::set_subtree(const mask & subtree)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_subtree);
	    x_subtree = subtree.clone();
	    if(x_subtree == nullptr)
		throw Ememory("archive_options_listing::set_subtree");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_listing::set_user_slicing(const infinint & slicing_first, const infinint & slicing_others)
    {
	if(x_slicing_first == nullptr)
	{
	    x_slicing_first = new (get_pool()) infinint(slicing_first);
	    if(x_slicing_first == nullptr)
		throw Ememory("archive_options_listing::set_user_slicing");
	}
	else
	    *x_slicing_first = slicing_first;

	if(x_slicing_others == nullptr)
	{
	    x_slicing_others = new (get_pool()) infinint(slicing_others);
	    if(x_slicing_others == nullptr)
		throw Ememory("archive_options_listing::set_user_slicing");
	}
	else
	    *x_slicing_others = slicing_others;
    }

    const mask & archive_options_listing::get_selection() const
    {
	if(x_selection == nullptr)
	    throw Erange("archive_option_listing", dar_gettext("No mask available"));
	return *x_selection;
    }

    const mask & archive_options_listing::get_subtree() const
    {
	if(x_subtree == nullptr)
	    throw Erange("archive_option_listing", dar_gettext("No mask available"));
	return *x_subtree;
    }

    bool archive_options_listing::get_user_slicing(infinint & slicing_first, infinint & slicing_others) const
    {
	if(x_slicing_first != nullptr && x_slicing_others != nullptr)
	{
	    slicing_first = *x_slicing_first;
	    slicing_others = *x_slicing_others;
	    return true;
	}
	else
	    return false;
    }


    void archive_options_listing::destroy()
    {
	NLS_SWAP_IN;
	try
	{
	    if(x_slicing_first != nullptr)
	    {
		delete x_slicing_first;
		x_slicing_first = nullptr;
	    }
	    if(x_slicing_others != nullptr)
	    {
		delete x_slicing_others;
		x_slicing_others = nullptr;
	    }
	    archive_option_destroy_mask(x_selection);
	    archive_option_destroy_mask(x_subtree);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_listing::copy_from(const archive_options_listing & ref)
    {
	x_selection = nullptr;
	x_subtree = nullptr;
	x_slicing_first = nullptr;
	x_slicing_others = nullptr;

	try
	{
	    if(ref.x_selection == nullptr)
		throw SRC_BUG;
	    x_selection = ref.x_selection->clone();
	    x_subtree = ref.x_subtree->clone();
	    if(x_selection == nullptr || x_subtree == nullptr)
		throw Ememory("archive_options_listing::copy_from");
	    if(ref.x_slicing_first != nullptr)
	    {
		x_slicing_first = new (get_pool()) infinint(*ref.x_slicing_first);
		if(x_slicing_first == nullptr)
		    throw Ememory("archive_options_listing::copy_from");
	    }
	    if(ref.x_slicing_others != nullptr)
	    {
		x_slicing_others = new (get_pool()) infinint(*ref.x_slicing_others);
		if(x_slicing_others == nullptr)
		    throw Ememory("archive_options_listing::copy_from");
	    }

	    x_info_details = ref.x_info_details;
	    x_list_mode = ref.x_list_mode;
	    x_filter_unsaved = ref.x_filter_unsaved;
	    x_display_ea = ref.x_display_ea;
	    x_sizes_in_bytes = ref.x_sizes_in_bytes;
	}
	catch(...)
	{
	    clear();
	    throw;
	}
    }


	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////

    void archive_options_diff::clear()
    {
	NLS_SWAP_IN;
	try
	{
	    destroy();

	    archive_option_clean_mask(x_selection, get_pool());
	    archive_option_clean_mask(x_subtree, get_pool());
	    x_info_details = false;
	    x_display_treated = false;
	    x_display_treated_only_dir = false;
	    x_display_skipped = false;
	    archive_option_clean_mask(x_ea_mask, get_pool());
	    x_what_to_check = cat_inode::cf_all;
	    x_alter_atime = true;
	    x_old_alter_atime = true;
#if FURTIVE_READ_MODE_AVAILABLE
	    x_furtive_read = true;
#else
	    x_furtive_read = false;
#endif
	    x_hourshift = 0;
	    x_compare_symlink_date = true;
	    x_scope = all_fsa_families();
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }


    void archive_options_diff::set_selection(const mask & selection)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_selection);
	    x_selection = selection.clone();
	    if(x_selection == nullptr)
		throw Ememory("archive_options_diff::set_selection");
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void archive_options_diff::set_subtree(const mask & subtree)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_subtree);
	    x_subtree = subtree.clone();
	    if(x_subtree == nullptr)
		throw Ememory("archive_options_diff::set_subtree");
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void archive_options_diff::set_ea_mask(const mask & ea_mask)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_ea_mask);
	    x_ea_mask = ea_mask.clone();
	    if(x_ea_mask == nullptr)
		throw Ememory("archive_options_dif::set_ea_mask");
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void archive_options_diff::set_furtive_read_mode(bool furtive_read)
    {
	NLS_SWAP_IN;
	try
	{
#if FURTIVE_READ_MODE_AVAILABLE
	    x_furtive_read = furtive_read;
	    if(furtive_read)
	    {
		x_old_alter_atime = x_alter_atime;
		x_alter_atime = true;
		    // this is required to avoid libdar manipulating ctime of inodes
	    }
	    else
		x_alter_atime = x_old_alter_atime;
#else
	    if(furtive_read)
		throw Ecompilation(gettext("Furtive read mode"));
	    x_furtive_read = false;
#endif
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void archive_options_diff::destroy()
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_selection);
	    archive_option_destroy_mask(x_subtree);
	    archive_option_destroy_mask(x_ea_mask);
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void archive_options_diff::copy_from(const archive_options_diff & ref)
    {
	x_selection = nullptr;
	x_subtree = nullptr;
	x_ea_mask = nullptr;

	try
	{
	    if(ref.x_selection == nullptr)
		throw SRC_BUG;
	    if(ref.x_subtree == nullptr)
		throw SRC_BUG;
	    if(ref.x_ea_mask == nullptr)
		throw SRC_BUG;

	    x_selection = ref.x_selection->clone();
	    x_subtree = ref.x_subtree->clone();
	    x_ea_mask = ref.x_ea_mask->clone();

	    if(x_selection == nullptr || x_subtree == nullptr || x_ea_mask == nullptr)
		throw Ememory("archive_options_extract::copy_from");

	    x_info_details = ref.x_info_details;
	    x_display_treated = ref.x_display_treated;
	    x_display_treated_only_dir = ref.x_display_treated_only_dir;
	    x_display_skipped = ref.x_display_skipped;
	    x_what_to_check = ref.x_what_to_check;
	    x_alter_atime = ref.x_alter_atime;
	    x_old_alter_atime = ref.x_old_alter_atime;
	    x_furtive_read = ref.x_furtive_read;
	    x_hourshift = ref.x_hourshift;
	    x_compare_symlink_date = ref.x_compare_symlink_date;
	    x_scope = ref.x_scope;
	}
	catch(...)
	{
	    clear();
	    throw;
	}
    }



	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////

    void archive_options_test::clear()
    {
	NLS_SWAP_IN;
	try
	{
	    destroy();

	    archive_option_clean_mask(x_selection, get_pool());
	    archive_option_clean_mask(x_subtree, get_pool());
	    x_info_details = false;
	    x_display_treated = false;
	    x_display_treated_only_dir = false;
	    x_display_skipped = false;
	    x_empty = false;
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }


    void archive_options_test::set_selection(const mask & selection)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_selection);
	    x_selection = selection.clone();
	    if(x_selection == nullptr)
		throw Ememory("archive_options_test::set_selection");
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void archive_options_test::set_subtree(const mask & subtree)
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_subtree);
	    x_subtree = subtree.clone();
	    if(x_subtree == nullptr)
		throw Ememory("archive_option_test::set_subtree");
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }


    void archive_options_test::destroy()
    {
	NLS_SWAP_IN;
	try
	{
	    archive_option_destroy_mask(x_selection);
	    archive_option_destroy_mask(x_subtree);
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

    }

    void archive_options_test::copy_from(const archive_options_test & ref)
    {
	x_selection = nullptr;
	x_subtree = nullptr;

	try
	{
	    if(ref.x_selection == nullptr)
		throw SRC_BUG;
	    if(ref.x_subtree == nullptr)
		throw SRC_BUG;

	    x_selection = ref.x_selection->clone();
	    x_subtree = ref.x_subtree->clone();

	    if(x_selection == nullptr || x_subtree == nullptr)
		throw Ememory("archive_options_extract::copy_from");

	    x_info_details = ref.x_info_details;
	    x_display_treated = ref.x_display_treated;
	    x_display_treated_only_dir = ref.x_display_treated_only_dir;
	    x_display_skipped = ref.x_display_skipped;
	    x_empty = ref.x_empty;
	}
	catch(...)
	{
	    clear();
	    throw;
	}
    }

} // end of namespace

