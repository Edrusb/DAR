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

#include <new>

#include "archive_options.hpp"

using namespace std;

namespace libdar
{
	// general default values

    static const U_32 default_crypto_size = 10240;
    static const path default_ref_chem = "/";
    static const U_I default_min_compr_size = 100;
    static const inode::comparison_fields default_comparison_fields = inode::cf_all;
    static const crit_constant_action default_crit_action = crit_constant_action(data_preserve, EA_preserve);
    static const string default_user_comment = "N/A";

	// some local helper functions

    inline void archive_option_destroy_mask(mask * & ptr);
    inline void archive_option_clean_mask(mask * & ptr, bool all = true);
    inline void archive_option_check_mask(const mask & m);
    inline void archive_option_destroy_crit_action(crit_action * & ptr);
    inline void archive_option_clean_crit_action(crit_action * & ptr);
    inline void archive_option_check_crit_action(const crit_action & m);


	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////


    inline void archive_option_clean_mask(mask * & ptr, bool all)
    {
	archive_option_destroy_mask(ptr);
	ptr = new (nothrow) bool_mask(all);
	if(ptr == NULL)
	    throw Ememory("archive_options_clean_mask");
    }

    inline void archive_option_destroy_mask(mask * & ptr)
    {
	if(ptr != NULL)
	{
	    delete ptr;
	    ptr = NULL;
	}
    }

    inline void archive_option_check_mask(const mask & m)
    {
	NLS_SWAP_IN;
	try
	{
	    if(&m == NULL)
		throw Elibcall("archive_options_check_mask", gettext("invalid NULL argument given as mask option"));
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    inline void archive_option_clean_crit_action(crit_action * & ptr)
    {
	archive_option_destroy_crit_action(ptr);
	ptr = default_crit_action.clone();
	if(ptr == NULL)
	    throw Ememory("archive_options::archive_option_clean_crit_action");
    }

    inline void archive_option_destroy_crit_action(crit_action * & ptr)
    {
	if(ptr != NULL)
	{
	    delete ptr;
	    ptr = NULL;
	}
    }

    inline void archive_option_check_crit_action(const crit_action & m)
    {
	NLS_SWAP_IN;
	try
	{
	    if(&m == NULL)
		throw Elibcall("archive_options_check_crit_action", gettext("invalid NULL argument given as crit_action option"));
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }


	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////

    archive_options_read::archive_options_read() : x_ref_chem(default_ref_chem)
    {
	x_entrepot = NULL;
	x_ref_entrepot = NULL;
	clear();
    }

    void archive_options_read::clear()
    {

	    // setting the default values for all options

	detruit();

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
	x_entrepot = new entrepot_local("", "", "", false); // never using furtive_mode to read slices
	if(x_entrepot == NULL)
	    throw Ememory("archive_options_read::clear");

	    //
	external_cat = false;
	x_ref_chem = default_ref_chem;
	x_ref_basename = "";
	x_ref_crypto = crypto_none;
	x_ref_pass.clear();
	x_ref_crypto_size = default_crypto_size;
	x_ref_execute = "";
	x_ref_slice_min_digits = 0;
	x_ref_entrepot = new entrepot_local("", "", "", false); // never using furtive_mode to read slices
	if(x_ref_entrepot == NULL)
	    throw Ememory("archive_options_read::clear");
    }

    void archive_options_read::set_default_crypto_size()
    {
	x_crypto_size = default_crypto_size;
    }

    void archive_options_read::unset_external_catalogue()
    {
	x_ref_chem = default_ref_chem ;
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
	if(ref.x_entrepot == NULL)
	    throw SRC_BUG;
	x_entrepot = ref.x_entrepot->clone();
	if(x_entrepot == NULL)
	    throw Ememory("archive_options_read::copy_from");

	    //

	external_cat = ref.external_cat;
	x_ref_chem = ref.x_ref_chem;
	x_ref_basename = ref.x_ref_basename;
	x_ref_crypto = ref.x_ref_crypto;
	x_ref_pass = ref.x_ref_pass;
	x_ref_crypto_size = ref.x_ref_crypto_size;
	x_ref_execute = ref.x_ref_execute;
	x_ref_slice_min_digits = ref.x_ref_slice_min_digits;
	if(ref.x_ref_entrepot == NULL)
	    throw SRC_BUG;
	x_ref_entrepot = ref.x_ref_entrepot->clone();
	if(x_ref_entrepot == NULL)
	    throw Ememory("archive_options_read::copy_from");
    }

    void archive_options_read::detruit()
    {
	if(x_entrepot != NULL)
	{
	    delete x_entrepot;
	    x_entrepot = NULL;
	}
	if(x_ref_entrepot != NULL)
	{
	    delete x_ref_entrepot;
	    x_ref_entrepot = NULL;
	}
    }




	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////

    archive_options_create::archive_options_create()
    {
	x_selection = x_subtree = x_ea_mask = x_compr_mask = x_backup_hook_file_mask = NULL;
	x_entrepot = NULL;
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
	x_selection = x_subtree = x_ea_mask = x_compr_mask = x_backup_hook_file_mask = NULL;
	x_entrepot = NULL;
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

	    archive_option_clean_mask(x_selection);
	    archive_option_clean_mask(x_subtree);
	    archive_option_clean_mask(x_ea_mask);
	    archive_option_clean_mask(x_compr_mask);
	    archive_option_clean_mask(x_backup_hook_file_mask, false);
	    x_ref_arch = NULL;
	    x_allow_over = true;
	    x_warn_over = true;
	    x_info_details = false;
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
	    x_min_compr_size = default_min_compr_size;
	    x_nodump = false;
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
	    x_display_skipped = false;
	    x_fixed_date = 0;
	    x_slice_permission = "";
	    x_slice_user_ownership = "";
	    x_slice_group_ownership = "";
	    x_repeat_count = 0;
	    x_repeat_byte = 0;
	    x_sequential_marks = true;
	    x_sparse_file_min_size = 15;  // min value to activate the feature (0 means no detection of sparse_file)
	    x_security_check = true;
	    x_user_comment = default_user_comment;
	    x_hash = hash_none;
	    x_slice_min_digits = 0;
	    x_backup_hook_file_execute = "";
	    x_ignore_unknown = false;
	    x_entrepot = new entrepot_local("", "", "", false); // never using furtive_mode to read slices
	    if(x_entrepot == NULL)
		throw Ememory("archive_options_create::clear");
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
	    archive_option_check_mask(selection);
	    archive_option_destroy_mask(x_selection);
	    x_selection = selection.clone();
	    if(x_selection == NULL)
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
	    archive_option_check_mask(subtree);
	    archive_option_destroy_mask(x_subtree);
	    x_subtree = subtree.clone();
	    if(x_subtree == NULL)
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
	    archive_option_check_mask(ea_mask);
	    archive_option_destroy_mask(x_ea_mask);
	    x_ea_mask = ea_mask.clone();
	    if(x_ea_mask == NULL)
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
	    archive_option_check_mask(compr_mask);
	    archive_option_destroy_mask(x_compr_mask);
	    x_compr_mask = compr_mask.clone();
	    if(x_compr_mask == NULL)
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
	    archive_option_check_mask(which_files);
	    archive_option_destroy_mask(x_backup_hook_file_mask);
	    x_backup_hook_file_mask = which_files.clone();
	    if(x_backup_hook_file_mask == NULL)
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
	if(x_entrepot != NULL)
	    delete x_entrepot;

	x_entrepot = entr.clone();
	if(x_entrepot == NULL)
	    throw Ememory("archive_options_create::set_entrepot");
	x_entrepot->set_permission(x_slice_permission);
	x_entrepot->set_user_ownership(x_slice_user_ownership);
	x_entrepot->set_group_ownership(x_slice_group_ownership);
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
	    if(x_entrepot != NULL)
	    {
		delete x_entrepot;
		x_entrepot = NULL;
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
	x_selection = NULL;
	x_subtree = NULL;
	x_ea_mask = NULL;
	x_compr_mask = NULL;
	x_backup_hook_file_mask = NULL;
	x_entrepot = NULL;

	if(ref.x_selection == NULL)
	    throw SRC_BUG;
	if(ref.x_subtree == NULL)
	    throw SRC_BUG;
	if(ref.x_ea_mask == NULL)
	    throw SRC_BUG;
	if(ref.x_compr_mask == NULL)
	    throw SRC_BUG;
	if(ref.x_backup_hook_file_mask == NULL)
	    throw SRC_BUG;
	x_selection = ref.x_selection->clone();
	x_subtree = ref.x_subtree->clone();
	x_ea_mask = ref.x_ea_mask->clone();
	x_compr_mask = ref.x_compr_mask->clone();
	x_backup_hook_file_mask = ref.x_backup_hook_file_mask->clone();

	if(x_selection == NULL || x_subtree == NULL || x_ea_mask == NULL || x_compr_mask == NULL || x_backup_hook_file_mask == NULL)
	    throw Ememory("archive_options_create::copy_from");

	x_ref_arch = ref.x_ref_arch;
	x_allow_over = ref.x_allow_over;
	x_warn_over = ref.x_warn_over;
	x_info_details = ref.x_info_details;
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
	x_display_skipped = ref.x_display_skipped;
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
	if(x_entrepot == NULL)
	    throw SRC_BUG;
	x_entrepot = x_entrepot->clone();
	if(x_entrepot == NULL)
	    throw Ememory("archive_options_create::copy_from");
    }


	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////

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
	    x_empty = false;
	    x_slice_permission = "";
	    x_slice_user_ownership = "";
	    x_slice_group_ownership = "";
	    x_user_comment = default_user_comment;
	    x_hash = hash_none;
	    x_slice_min_digits = 0;
	    x_sequential_marks = true;
	    x_entrepot = new entrepot_local("", "", "", false); // never using furtive_mode to read slices
	    if(x_entrepot == NULL)
		throw Ememory("archive_options_isolate::clear");
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
	if(x_entrepot != NULL)
	    delete x_entrepot;

	x_entrepot = entr.clone();
	if(x_entrepot == NULL)
	    throw Ememory("archive_options_isolate::set_entrepot");
	x_entrepot->set_permission(x_slice_permission);
	x_entrepot->set_user_ownership(x_slice_user_ownership);
	x_entrepot->set_group_ownership(x_slice_group_ownership);
    }

    void archive_options_isolate::destroy()
    {
	if(x_entrepot != NULL)
	{
	    delete x_entrepot;
	    x_entrepot = NULL;
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
	x_empty = ref.x_empty;
	x_slice_permission = ref.x_slice_permission;
	x_slice_user_ownership = ref.x_slice_user_ownership;
	x_slice_group_ownership = ref.x_slice_group_ownership;
	x_user_comment = ref.x_user_comment;
	x_hash = ref.x_hash;
	x_slice_min_digits = ref.x_slice_min_digits;
	x_sequential_marks = ref.x_sequential_marks;
	if(ref.x_entrepot == NULL)
	    throw SRC_BUG;
	x_entrepot = ref.x_entrepot->clone();
	if(x_entrepot == NULL)
	    throw Ememory("archive_options_isolate::copy_from");
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

	    archive_option_clean_mask(x_selection);
	    archive_option_clean_mask(x_subtree);
	    archive_option_clean_mask(x_ea_mask);
	    archive_option_clean_mask(x_compr_mask);
	    archive_option_clean_crit_action(x_overwrite);
	    x_ref = NULL;
	    x_allow_over = true;
	    x_warn_over = true;
	    x_info_details = false;
	    x_pause = 0;
	    x_empty_dir = false;
	    x_compr_algo = none;
	    x_compression_level = 9;
	    x_file_size = 0;
	    x_first_file_size = 0;
	    x_execute = "";
	    x_crypto = crypto_none;
	    x_pass.clear();
	    x_min_compr_size = default_min_compr_size;
	    x_empty = false;
	    x_display_skipped = false;
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
	    x_entrepot = new entrepot_local("", "", "", false); // never using furtive_mode to read slices
	    if(x_entrepot == NULL)
		throw Ememory("archive_options_merge::clear");
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
	    archive_option_check_mask(selection);
	    archive_option_destroy_mask(x_selection);
	    x_selection = selection.clone();
	    if(x_selection == NULL)
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
	    archive_option_check_mask(subtree);
	    archive_option_destroy_mask(x_subtree);
	    x_subtree = subtree.clone();
	    if(x_subtree == NULL)
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
	    archive_option_check_crit_action(overwrite);
	    archive_option_destroy_crit_action(x_overwrite);
	    x_overwrite = overwrite.clone();
	    if(x_overwrite == NULL)
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
	    archive_option_check_mask(ea_mask);
	    archive_option_destroy_mask(x_ea_mask);
	    x_ea_mask = ea_mask.clone();
	    if(x_ea_mask == NULL)
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
	    archive_option_check_mask(compr_mask);
	    archive_option_destroy_mask(x_compr_mask);
	    x_compr_mask = compr_mask.clone();
	    if(x_compr_mask == NULL)
		throw Ememory("archive_options_merge::set_compr_mask");
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
	if(x_entrepot != NULL)
	    delete x_entrepot;

	x_entrepot = entr.clone();
	if(x_entrepot == NULL)
	    throw Ememory("archive_options_merge::set_entrepot");
	x_entrepot->set_permission(x_slice_permission);
	x_entrepot->set_user_ownership(x_slice_user_ownership);
	x_entrepot->set_group_ownership(x_slice_group_ownership);
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
	    archive_option_destroy_crit_action(x_overwrite);
	    if(x_entrepot != NULL)
	    {
		delete x_entrepot;
		x_entrepot = NULL;
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
	x_selection = NULL;
	x_subtree = NULL;
	x_ea_mask = NULL;
	x_compr_mask = NULL;
	x_overwrite = NULL;
	x_entrepot = NULL;

	try
	{
	    if(ref.x_selection == NULL)
		throw SRC_BUG;
	    if(ref.x_subtree == NULL)
		throw SRC_BUG;
	    if(ref.x_ea_mask == NULL)
		throw SRC_BUG;
	    if(ref.x_compr_mask == NULL)
		throw SRC_BUG;
	    if(ref.x_overwrite == NULL)
		throw SRC_BUG;
	    if(ref.x_entrepot == NULL)
		throw SRC_BUG;

	    x_selection = ref.x_selection->clone();
	    x_subtree = ref.x_subtree->clone();
	    x_ea_mask = ref.x_ea_mask->clone();
	    x_compr_mask = ref.x_compr_mask->clone();
	    x_overwrite = ref.x_overwrite->clone();
	    x_entrepot = ref.x_entrepot->clone();

	    if(x_selection == NULL || x_subtree == NULL || x_ea_mask == NULL || x_compr_mask == NULL || x_overwrite == NULL || x_entrepot == NULL)
		throw Ememory("archive_options_extract::copy_from");

	    x_ref = ref.x_ref;
	    x_allow_over = ref.x_allow_over;
	    x_warn_over = ref.x_warn_over;
	    x_overwrite = ref.x_overwrite;
	    x_info_details = ref.x_info_details;
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
	    x_min_compr_size = ref.x_min_compr_size;
	    x_empty = ref.x_empty;
	    x_display_skipped = ref.x_display_skipped;
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
	    archive_option_clean_mask(x_selection);
	    archive_option_clean_mask(x_subtree);
	    archive_option_clean_mask(x_ea_mask);
	    archive_option_clean_crit_action(x_overwrite);
	    x_warn_over = true;
	    x_info_details = false;
	    x_flat = false;
	    x_what_to_check = default_comparison_fields;
	    x_warn_remove_no_match = true;
	    x_empty = false;
	    x_display_skipped = false;
	    x_empty_dir = true;
	    x_dirty = dirty_warn;
	    x_only_deleted = false;
	    x_ignore_deleted = false;
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
	    archive_option_check_mask(selection);
	    archive_option_destroy_mask(x_selection);
	    x_selection = selection.clone();
	    if(x_selection == NULL)
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
	    archive_option_check_mask(subtree);
	    archive_option_destroy_mask(x_subtree);
	    x_subtree = subtree.clone();
	    if(x_subtree == NULL)
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
	    archive_option_check_mask(ea_mask);
	    archive_option_destroy_mask(x_ea_mask);
	    x_ea_mask = ea_mask.clone();
	    if(x_ea_mask == NULL)
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
	    archive_option_check_crit_action(over);
	    archive_option_destroy_crit_action(x_overwrite);
	    x_overwrite = over.clone();
	    if(x_overwrite == NULL)
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
	x_selection = NULL;
	x_subtree = NULL;
	x_ea_mask = NULL;
	x_overwrite = NULL;

	try
	{
	    if(ref.x_selection == NULL)
		throw SRC_BUG;
	    if(ref.x_subtree == NULL)
		throw SRC_BUG;
	    if(ref.x_ea_mask == NULL)
		throw SRC_BUG;
	    if(ref.x_overwrite == NULL)
		throw SRC_BUG;
	    x_selection = ref.x_selection->clone();
	    x_subtree = ref.x_subtree->clone();
	    x_ea_mask = ref.x_ea_mask->clone();
	    x_overwrite = ref.x_overwrite->clone();

	    if(x_selection == NULL || x_subtree == NULL || x_ea_mask == NULL || x_overwrite == NULL)
		throw Ememory("archive_options_extract::copy_from");

	    x_warn_over = ref.x_warn_over;
	    x_info_details = ref.x_info_details;
	    x_flat = ref.x_flat;
	    x_what_to_check = ref.x_what_to_check;
	    x_warn_remove_no_match = ref.x_warn_remove_no_match;
	    x_empty = ref.x_empty;
	    x_display_skipped = ref.x_display_skipped;
	    x_empty_dir = ref.x_empty_dir;
	    x_dirty = ref.x_dirty;
	    x_only_deleted = ref.x_only_deleted;
	    x_ignore_deleted = ref.x_ignore_deleted;
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
	    archive_option_clean_mask(x_selection);
	    archive_option_clean_mask(x_subtree);
	    x_filter_unsaved = false;
	    x_display_ea = false;
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
	    archive_option_check_mask(selection);
	    archive_option_destroy_mask(x_selection);
	    x_selection = selection.clone();
	    if(x_selection == NULL)
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
	    archive_option_check_mask(subtree);
	    archive_option_destroy_mask(x_subtree);
	    x_subtree = subtree.clone();
	    if(x_subtree == NULL)
		throw Ememory("archive_options_listing::set_subtree");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    const mask & archive_options_listing::get_selection() const
    {
	if(x_selection == NULL)
	    throw Erange("archive_option_listing", dar_gettext("No mask available"));
	return *x_selection;
    }

    const mask & archive_options_listing::get_subtree() const
    {
	if(x_subtree == NULL)
	    throw Erange("archive_option_listing", dar_gettext("No mask available"));
	return *x_subtree;
    }

    void archive_options_listing::destroy()
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

    void archive_options_listing::copy_from(const archive_options_listing & ref)
    {
	x_selection = NULL;
	x_subtree = NULL;

	try
	{
	    if(ref.x_selection == NULL)
		throw SRC_BUG;
	    x_selection = ref.x_selection->clone();
	    x_subtree = ref.x_subtree->clone();
	    if(x_selection == NULL || x_subtree == NULL)
		throw Ememory("archive_options_listing::copy_from");

	    x_info_details = ref.x_info_details;
	    x_list_mode = ref.x_list_mode;
	    x_filter_unsaved = ref.x_filter_unsaved;
	    x_display_ea = ref.x_display_ea;
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

	    archive_option_clean_mask(x_selection);
	    archive_option_clean_mask(x_subtree);
	    x_info_details = false;
	    archive_option_clean_mask(x_ea_mask);
	    x_what_to_check = inode::cf_all;
	    x_alter_atime = true;
	    x_old_alter_atime = true;
#if FURTIVE_READ_MODE_AVAILABLE
	    x_furtive_read = true;
#else
	    x_furtive_read = false;
#endif
	    x_display_skipped = false;
	    x_hourshift = 0;
	    x_compare_symlink_date = true;
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
	    archive_option_check_mask(selection);
	    archive_option_destroy_mask(x_selection);
	    x_selection = selection.clone();
	    if(x_selection == NULL)
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
	    archive_option_check_mask(subtree);
	    archive_option_destroy_mask(x_subtree);
	    x_subtree = subtree.clone();
	    if(x_subtree == NULL)
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
	    archive_option_check_mask(ea_mask);
	    archive_option_destroy_mask(x_ea_mask);
	    x_ea_mask = ea_mask.clone();
	    if(x_ea_mask == NULL)
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
	x_selection = NULL;
	x_subtree = NULL;
	x_ea_mask = NULL;

	try
	{
	    if(ref.x_selection == NULL)
		throw SRC_BUG;
	    if(ref.x_subtree == NULL)
		throw SRC_BUG;
	    if(ref.x_ea_mask == NULL)
		throw SRC_BUG;

	    x_selection = ref.x_selection->clone();
	    x_subtree = ref.x_subtree->clone();
	    x_ea_mask = ref.x_ea_mask->clone();

	    if(x_selection == NULL || x_subtree == NULL || x_ea_mask == NULL)
		throw Ememory("archive_options_extract::copy_from");

	    x_info_details = ref.x_info_details;
	    x_what_to_check = ref.x_what_to_check;
	    x_alter_atime = ref.x_alter_atime;
	    x_old_alter_atime = ref.x_old_alter_atime;
	    x_furtive_read = ref.x_furtive_read;
	    x_display_skipped = ref.x_display_skipped;
	    x_hourshift = ref.x_hourshift;
	    x_compare_symlink_date = ref.x_compare_symlink_date;
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

	    archive_option_clean_mask(x_selection);
	    archive_option_clean_mask(x_subtree);
	    x_info_details = false;
	    x_empty = false;
	    x_display_skipped = false;
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
	    archive_option_check_mask(selection);
	    archive_option_destroy_mask(x_selection);
	    x_selection = selection.clone();
	    if(x_selection == NULL)
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
	    archive_option_check_mask(subtree);
	    archive_option_destroy_mask(x_subtree);
	    x_subtree = subtree.clone();
	    if(x_subtree == NULL)
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
	x_selection = NULL;
	x_subtree = NULL;

	try
	{
	    if(ref.x_selection == NULL)
		throw SRC_BUG;
	    if(ref.x_subtree == NULL)
		throw SRC_BUG;

	    x_selection = ref.x_selection->clone();
	    x_subtree = ref.x_subtree->clone();

	    if(x_selection == NULL || x_subtree == NULL)
		throw Ememory("archive_options_extract::copy_from");

	    x_info_details = ref.x_info_details;
	    x_empty = ref.x_empty;
	    x_display_skipped = ref.x_display_skipped;
	}
	catch(...)
	{
	    clear();
	    throw;
	}
    }

} // end of namespace

