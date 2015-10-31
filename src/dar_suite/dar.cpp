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

#include <string>
#include <iostream>
#include <new>
#include <vector>

#include "erreurs.hpp"
#include "user_interaction.hpp"
#include "command_line.hpp"
#include "tools.hpp"
#include "dar.hpp"
#include "dar_suite.hpp"
#include "integers.hpp"
#include "deci.hpp"
#include "libdar.hpp"
#include "shell_interaction.hpp"
#include "criterium.hpp"
#include "line_tools.hpp"

#ifndef DAR_VERSION
#define DAR_VERSION "unknown (BUG at compilation time?)"
#endif

using namespace std;
using namespace libdar;

static void display_sauv_stat(user_interaction & dialog, const statistics & st);
static void display_rest_stat(user_interaction & dialog, const statistics & st);
static void display_diff_stat(user_interaction & dialog, const statistics &st);
static void display_test_stat(user_interaction & dialog, const statistics & st);
static void display_merge_stat(user_interaction & dialog, const statistics & st);
static S_I little_main(shell_interaction & dialog, S_I argc, char * const argv[], const char **env);

int main(S_I argc, char * const argv[], const char **env)
{
    return dar_suite_global(argc,
			    argv,
			    env,
			    get_short_opt(),
#if HAVE_GETOPT_LONG
			    get_long_opt(),
#endif
			    '\0', // should never be met as option, thus early read the whole command-line for -j and -Q options
			    &little_main);
}

static S_I little_main(shell_interaction & dialog, S_I argc, char * const argv[], const char **env)
{
    S_I ret = EXIT_OK;
    line_param param;
    const char *home = tools_get_from_env(env, "HOME");
    vector<string> dar_dcf_path = line_tools_explode_PATH(tools_get_from_env(env, "DAR_DCF_PATH"));
    vector<string> dar_duc_path = line_tools_explode_PATH(tools_get_from_env(env, "DAR_DUC_PATH"));

    if(home == nullptr)
        home = "/";
    if(! get_args(dialog,
		  home,
		  dar_dcf_path,
		  dar_duc_path,
		  argc,
		  argv,
		  param))
    {
	if(param.op == version_or_help)
	    return EXIT_OK;
	else
	    return EXIT_SYNTAX;
    }
    else // get_args is OK, we've got a valid command line
    {
	archive *arch = nullptr;
	archive *aux = nullptr;
	archive *cur = nullptr;
        dialog.set_beep(param.beep);

        if(param.filename != "-"
	   || (param.output_pipe != "" && param.op != create && param.op != isolate && param.op != merging))
            dialog.change_non_interactive_output(&cout);
            // standart output can be used to send non interactive
            // messages

	try
	{
	    statistics st = false;
	    secu_string tmp_pass;
	    crypto_algo crypto, aux_crypto;
	    archive_options_read read_options;
	    archive_options_create create_options;
	    archive_options_isolate isolate_options;
	    archive_options_merge merge_options;
	    archive_options_extract extract_options;
	    archive_options_listing listing_options;
	    archive_options_diff diff_options;
	    archive_options_test test_options;
	    bool no_cipher_given;
	    vector<string> recipients;

            switch(param.op)
            {
            case create:
	    case merging:
		if(param.ref_filename != nullptr && param.ref_root != nullptr)
		{
		    line_tools_crypto_split_algo_pass(param.pass_ref,
						      crypto,
						      tmp_pass,
						      no_cipher_given,
						      recipients);
		    if(param.op == merging && param.aux_root != nullptr && param.info_details)
			dialog.warning(gettext("Considering the (first) archive of reference:"));
		    read_options.clear();
		    if(no_cipher_given)
			    // since archive format 9 crypto algo used
			    // is stored in the archive, it will be used
			    // unless we specify explicitely the cipher to use
			read_options.set_crypto_algo(libdar::crypto_none);
		    else
			read_options.set_crypto_algo(crypto);
		    read_options.set_crypto_pass(tmp_pass);
		    read_options.set_crypto_size(param.crypto_size_ref);
		    read_options.set_input_pipe(param.input_pipe);
		    read_options.set_output_pipe(param.output_pipe);
		    read_options.set_execute(param.execute_ref);
		    read_options.set_info_details(param.info_details);
		    read_options.set_lax(param.lax);
		    read_options.set_slice_min_digits(param.ref_num_digits);
		    read_options.set_ignore_signature_check_failure(param.blind_signatures);
		    read_options.set_multi_threaded(param.multi_threaded);
		    if(param.sequential_read)
		    {
			if(param.op == merging)
			    throw Erange("little_main", gettext("Using sequential reading mode for archive source is not possible for merging operation"));
			else
			    read_options.set_sequential_read(true);
		    }
		    arch = new (nothrow) archive(dialog, *param.ref_root, *param.ref_filename, EXTENSION,
						 read_options);
		    if(arch == nullptr)
			throw Ememory("little_main");
		}

		if(param.aux_root != nullptr && param.aux_filename != nullptr)
		{
		    if(param.op != merging && param.op != create)
			throw SRC_BUG;
		    if(param.op == merging)
		    {
			if(param.info_details)
			    dialog.warning(gettext("Considering the second (alias auxiliary) archive of reference:"));
			line_tools_crypto_split_algo_pass(param.aux_pass,
							  aux_crypto,
							  tmp_pass,
							  no_cipher_given,
							  recipients);
			read_options.clear();
			if(no_cipher_given)
				// since archive format 9 crypto algo used
				// is stored in the archive, it will be used
				// unless we specify explicitely the cipher to use
			    read_options.set_crypto_algo(libdar::crypto_none);
			else
			    read_options.set_crypto_algo(aux_crypto);
			read_options.set_crypto_pass(tmp_pass);
			read_options.set_crypto_size(param.aux_crypto_size);
			read_options.set_execute(param.aux_execute);
			read_options.set_info_details(param.info_details);
			read_options.set_lax(param.lax);
			read_options.set_slice_min_digits(param.aux_num_digits);
			read_options.set_ignore_signature_check_failure(param.blind_signatures);
			read_options.set_multi_threaded(param.multi_threaded);
			if(param.sequential_read)
			    throw Erange("little_main", gettext("Using sequential reading mode for archive source is not possible for merging operation"));
			aux = new (nothrow) archive(dialog, *param.aux_root, *param.aux_filename, EXTENSION,
						    read_options);
			if(aux == nullptr)
			    throw Ememory("little_main");
		    }
		}

		line_tools_crypto_split_algo_pass(param.pass,
						  crypto,
						  tmp_pass,
						  no_cipher_given,
						  recipients);

		switch(param.op)
		{
		case create:
		    create_options.clear();
		    if(arch != nullptr)
		    {
			arch->drop_all_filedescriptors(dialog);
			create_options.set_reference(arch);
		    }
		    create_options.set_selection(*param.selection);
		    create_options.set_subtree(*param.subtree);
		    create_options.set_allow_over(param.allow_over);
		    create_options.set_warn_over(param.warn_over);
		    create_options.set_info_details(param.info_details);
		    create_options.set_display_treated(param.display_treated,
						       param.display_treated_only_dir);
		    create_options.set_display_skipped(param.display_skipped);
		    create_options.set_display_finished(param.display_finished);
		    create_options.set_pause(param.pause);
		    create_options.set_empty_dir(param.empty_dir);
		    create_options.set_compression(param.algo);
		    create_options.set_compression_level(param.compression_level);
		    create_options.set_slicing(param.file_size, param.first_file_size);
		    create_options.set_ea_mask(*param.ea_mask);
		    create_options.set_execute(param.execute);
		    create_options.set_crypto_algo(crypto);
		    create_options.set_crypto_pass(tmp_pass);
		    create_options.set_crypto_size(param.crypto_size);
		    create_options.set_gnupg_recipients(recipients);
		    if(recipients.empty() && !param.signatories.empty())
			throw Erange("little_main", gettext("Archive signature is only possible with gnupg encryption"));
		    create_options.set_gnupg_signatories(param.signatories);
		    create_options.set_compr_mask(*param.compress_mask);
		    create_options.set_min_compr_size(param.min_compr_size);
		    create_options.set_nodump(param.nodump);
		    if(param.exclude_by_ea)
			create_options.set_exclude_by_ea(param.ea_name_for_exclusion);
		    create_options.set_what_to_check(param.what_to_check);
		    create_options.set_hourshift(param.hourshift);
		    create_options.set_empty(param.empty);
		    create_options.set_alter_atime(param.alter_atime);
		    create_options.set_furtive_read_mode(param.furtive_read_mode);
		    create_options.set_same_fs(param.same_fs);
		    create_options.set_snapshot(param.snapshot);
		    create_options.set_cache_directory_tagging(param.cache_directory_tagging);
		    create_options.set_fixed_date(param.fixed_date);
		    create_options.set_slice_permission(param.slice_perm);
		    create_options.set_slice_user_ownership(param.slice_user);
		    create_options.set_slice_group_ownership(param.slice_group);
		    create_options.set_retry_on_change(param.repeat_count, param.repeat_byte);
		    create_options.set_sequential_marks(param.use_sequential_marks);
		    create_options.set_sparse_file_min_size(param.sparse_file_min_size);
		    create_options.set_security_check(param.security_check);
		    create_options.set_user_comment(param.user_comment);
		    create_options.set_hash_algo(param.hash);
		    create_options.set_slice_min_digits(param.num_digits);
		    create_options.set_fsa_scope(param.scope);
		    create_options.set_multi_threaded(param.multi_threaded);

		    if(param.backup_hook_mask != nullptr)
			create_options.set_backup_hook(param.backup_hook_execute, *param.backup_hook_mask);
		    create_options.set_ignore_unknown_inode_type(param.ignore_unknown_inode);
		    cur = new (nothrow) archive(dialog, *param.fs_root, *param.sauv_root, param.filename, EXTENSION,
						create_options,
						&st);
		    if(cur == nullptr)
			throw Ememory("little_main");
		    if(!param.quiet)
			display_sauv_stat(dialog, st);
		    break;
		case merging:
		    merge_options.clear();
		    merge_options.set_auxilliary_ref(aux);
		    merge_options.set_selection(*param.selection);
		    merge_options.set_subtree(*param.subtree);
		    merge_options.set_allow_over(param.allow_over);
		    merge_options.set_overwriting_rules(*param.overwrite);
		    merge_options.set_warn_over(param.warn_over);
		    merge_options.set_info_details(param.info_details);
		    merge_options.set_display_treated(param.display_treated,
						      param.display_treated_only_dir);
		    merge_options.set_display_skipped(param.display_skipped);
		    merge_options.set_pause(param.pause);
		    merge_options.set_empty_dir(param.empty_dir);
		    merge_options.set_compression(param.algo);
		    merge_options.set_compression_level(param.compression_level);
		    merge_options.set_slicing(param.file_size, param.first_file_size);
		    merge_options.set_ea_mask(*param.ea_mask);
		    merge_options.set_execute(param.execute);
		    merge_options.set_crypto_algo(crypto);
		    merge_options.set_crypto_pass(tmp_pass);
		    merge_options.set_crypto_size(param.crypto_size);
		    merge_options.set_gnupg_recipients(recipients);
		    if(recipients.empty() && !param.signatories.empty())
			throw Erange("little_main", gettext("Archive signature is only possible with gnupg encryption"));
		    merge_options.set_gnupg_signatories(param.signatories);
		    merge_options.set_compr_mask(*param.compress_mask);
		    merge_options.set_min_compr_size(param.min_compr_size);
		    merge_options.set_empty(param.empty);
		    merge_options.set_keep_compressed(param.keep_compressed);
		    merge_options.set_slice_permission(param.slice_perm);
		    merge_options.set_slice_user_ownership(param.slice_user);
		    merge_options.set_slice_group_ownership(param.slice_group);
		    merge_options.set_decremental_mode(param.decremental);
		    merge_options.set_sequential_marks(param.use_sequential_marks);
		    merge_options.set_sparse_file_min_size(param.sparse_file_min_size);
		    merge_options.set_user_comment(param.user_comment);
		    merge_options.set_hash_algo(param.hash);
		    merge_options.set_slice_min_digits(param.num_digits);
		    merge_options.set_fsa_scope(param.scope);
		    merge_options.set_multi_threaded(param.multi_threaded);

		    cur = new (nothrow) archive(dialog,  // user_interaction &
						*param.sauv_root,  //const path &
						arch,              // archive *
						param.filename,    // const string &
						EXTENSION,         // const string &
						merge_options,
						&st);              // statistics*
		    if(cur == nullptr)
			throw Ememory("little_main");
		    if(!param.quiet)
			display_merge_stat(dialog, st);
		    break;
		default:
		    throw SRC_BUG;
		}

		    // making some room in memory

		if(param.info_details)
		    dialog.warning(gettext("Making room in memory (releasing memory used by archive of reference)..."));

		if(arch != nullptr)
		{
		    delete arch;
		    arch = nullptr;
		}
		if(aux != nullptr)
		{
		    delete aux;
		    aux = nullptr;
		}

		    // checking for onfly isolation

		if(!st.get_errored().is_zero())
		    ret = EXIT_DATA_ERROR;
		if(!st.get_tooold().is_zero())
		    ret = EXIT_SAVED_MODIFIED;

		if(param.op == create)
		    if(param.aux_root != nullptr && param.aux_filename != nullptr)
		    {
			if(param.op != merging && param.op != create)
			    throw SRC_BUG;
			if(param.op == create)
			{
			    if(!param.quiet)
				dialog.warning(gettext("Now performing on-fly isolation..."));
			    if(cur == nullptr)
				throw SRC_BUG;
			    line_tools_crypto_split_algo_pass(param.aux_pass,
							      aux_crypto,
							      tmp_pass,
							      no_cipher_given,
							      recipients);
			    isolate_options.clear();
			    isolate_options.set_allow_over(param.allow_over);
			    isolate_options.set_warn_over(param.warn_over);
			    isolate_options.set_info_details(param.info_details);
			    isolate_options.set_pause(param.pause);
			    if(compile_time::libbz2())
				isolate_options.set_compression(bzip2);
			    else
				if(compile_time::libz())
				    isolate_options.set_compression(gzip);
				else
				    if(compile_time::liblzo())
					isolate_options.set_compression(lzo);
				    else // no compression
					isolate_options.set_compression(none);
			    isolate_options.set_execute(param.aux_execute);
			    isolate_options.set_crypto_algo(aux_crypto);
			    isolate_options.set_crypto_pass(tmp_pass);
			    isolate_options.set_crypto_size(param.aux_crypto_size);
			    isolate_options.set_gnupg_recipients(recipients);
			    if(recipients.empty() && !param.signatories.empty())
				throw Erange("little_main", gettext("Archive signature is only possible with gnupg encryption"));
			    isolate_options.set_gnupg_signatories(param.signatories);
			    isolate_options.set_empty(param.empty);
			    isolate_options.set_slice_permission(param.slice_perm);
			    isolate_options.set_slice_user_ownership(param.slice_user);
			    isolate_options.set_slice_group_ownership(param.slice_group);
			    isolate_options.set_hash_algo(param.hash);
			    isolate_options.set_slice_min_digits(param.aux_num_digits);
			    isolate_options.set_user_comment(param.user_comment);
			    isolate_options.set_sequential_marks(param.use_sequential_marks);
			    isolate_options.set_multi_threaded(param.multi_threaded);

			    cur->op_isolate(dialog,
					    *param.aux_root,
					    *param.aux_filename,
					    EXTENSION,
					    isolate_options);
			}
		    }
		break;
            case isolate:
		line_tools_crypto_split_algo_pass(param.pass_ref,
						  crypto,
						  tmp_pass,
						  no_cipher_given,
						  recipients);
		read_options.clear();
		if(no_cipher_given)
			// since archive format 9 crypto algo used
			// is stored in the archive, it will be used
			// unless we specify explicitely the cipher to use
		    read_options.set_crypto_algo(libdar::crypto_none);
		else
		    read_options.set_crypto_algo(crypto);
		read_options.set_crypto_pass(tmp_pass);
		read_options.set_crypto_size(param.crypto_size_ref);
		read_options.set_input_pipe(param.input_pipe);
		read_options.set_output_pipe(param.output_pipe);
		read_options.set_execute(param.execute_ref);
		read_options.set_info_details(param.info_details);
		read_options.set_lax(param.lax);
		read_options.set_sequential_read(param.sequential_read);
		read_options.set_slice_min_digits(param.ref_num_digits);
		read_options.set_ignore_signature_check_failure(param.blind_signatures);
		read_options.set_multi_threaded(param.multi_threaded);
		arch = new (nothrow) archive(dialog, *param.ref_root, *param.ref_filename, EXTENSION,
					     read_options);
		if(arch == nullptr)
		    throw Ememory("little_main");
		else
		    arch->drop_all_filedescriptors(dialog);

		line_tools_crypto_split_algo_pass(param.pass,
						  crypto,
						  tmp_pass,
						  no_cipher_given,
						  recipients);
		isolate_options.clear();
		isolate_options.set_allow_over(param.allow_over);
		isolate_options.set_warn_over(param.warn_over);
		isolate_options.set_info_details(param.info_details);
		isolate_options.set_pause(param.pause);
		isolate_options.set_compression(param.algo);
		isolate_options.set_compression_level(param.compression_level);
		isolate_options.set_slicing(param.file_size, param.first_file_size);
		isolate_options.set_execute(param.execute);
		isolate_options.set_crypto_algo(crypto);
		isolate_options.set_crypto_pass(tmp_pass);
		isolate_options.set_crypto_size(param.crypto_size);
		isolate_options.set_gnupg_recipients(recipients);
		if(recipients.empty() && !param.signatories.empty())
		    throw Erange("little_main", gettext("Archive signature is only possible with gnupg encryption"));
		isolate_options.set_gnupg_signatories(param.signatories);
		isolate_options.set_empty(param.empty);
		isolate_options.set_slice_permission(param.slice_perm);
		isolate_options.set_slice_user_ownership(param.slice_user);
		isolate_options.set_slice_group_ownership(param.slice_group);
		isolate_options.set_user_comment(param.user_comment);
		isolate_options.set_hash_algo(param.hash);
		isolate_options.set_slice_min_digits(param.num_digits);
		isolate_options.set_sequential_marks(param.use_sequential_marks);
		isolate_options.set_multi_threaded(param.multi_threaded);

                arch->op_isolate(dialog,
				 *param.sauv_root,
				 param.filename,
				 EXTENSION,
				 isolate_options);
		break;
            case extract:
		line_tools_crypto_split_algo_pass(param.pass,
						  crypto,
						  tmp_pass,
						  no_cipher_given,
						  recipients);
		read_options.clear();
		if(no_cipher_given)
			// since archive format 9 crypto algo used
			// is stored in the archive, it will be used
			// unless we specify explicitely the cipher to use
		    read_options.set_crypto_algo(libdar::crypto_none);
		else
		    read_options.set_crypto_algo(crypto);
		read_options.set_crypto_pass(tmp_pass);
		read_options.set_crypto_size(param.crypto_size);
		read_options.set_input_pipe(param.input_pipe);
		read_options.set_output_pipe(param.output_pipe);
		read_options.set_execute(param.execute);
		read_options.set_info_details(param.info_details);
		read_options.set_lax(param.lax);
		read_options.set_sequential_read(param.sequential_read);
		read_options.set_slice_min_digits(param.num_digits);
		read_options.set_ignore_signature_check_failure(param.blind_signatures);
		read_options.set_multi_threaded(param.multi_threaded);

		if(param.ref_filename != nullptr && param.ref_root != nullptr)
		{
		    secu_string ref_tmp_pass;
		    crypto_algo ref_crypto;

		    line_tools_crypto_split_algo_pass(param.pass_ref,
						      ref_crypto,
						      ref_tmp_pass,
						      no_cipher_given,
						      recipients);
		    read_options.set_external_catalogue(*param.ref_root, *param.ref_filename);
		    if(no_cipher_given)
			    // since archive format 9 crypto algo used
			    // is stored in the archive, it will be used
			    // unless we specify explicitely the cipher to use
			read_options.set_ref_crypto_algo(libdar::crypto_none);
		    else
  		        read_options.set_ref_crypto_algo(ref_crypto);
		    read_options.set_ref_crypto_pass(ref_tmp_pass);
		    read_options.set_ref_crypto_size(param.crypto_size_ref);
		    read_options.set_ref_execute(param.execute_ref);
		    read_options.set_ref_slice_min_digits(param.ref_num_digits);
		}

		arch = new (nothrow) archive(dialog,
					     *param.sauv_root,
					     param.filename,
					     EXTENSION,
					     read_options);
		if(arch == nullptr)
		    throw Ememory("little_main");

		extract_options.clear();
		extract_options.set_selection(*param.selection);
		extract_options.set_subtree(*param.subtree);
		extract_options.set_warn_over(param.warn_over);
		extract_options.set_info_details(param.info_details);
		extract_options.set_display_treated(param.display_treated,
						    param.display_treated_only_dir);
		extract_options.set_display_skipped(param.display_skipped);
		extract_options.set_ea_mask(*param.ea_mask);
		extract_options.set_flat(param.flat);
		extract_options.set_what_to_check(param.what_to_check);
		extract_options.set_warn_remove_no_match(param.warn_remove_no_match);
		extract_options.set_empty(param.empty);
		extract_options.set_empty_dir(!param.empty_dir);
		    // the inversion above (!) is due to the fact this option is
		    // set with the same -D option that is used at backup time to
		    // save as empty directories those which have been excluded
		    // by filters, option, which default is 'false', but here
		    // in a restoration context, unless -D is provided (which set the
		    // option to "true"), we want that all directories, empty or not, be restored
		extract_options.set_overwriting_rules(*param.overwrite);
		switch(param.dirty)
		{
		case dirtyb_ignore:
		    extract_options.set_dirty_behavior(true, false);
		    break;
		case dirtyb_warn:
		    extract_options.set_dirty_behavior(false, true);
		    break;
		case dirtyb_ok:
		    extract_options.set_dirty_behavior(false, false);
		    break;
		default:
		    throw SRC_BUG;
		}
		extract_options.set_only_deleted(param.only_deleted);
		extract_options.set_ignore_deleted(param.not_deleted);
		extract_options.set_fsa_scope(param.scope);

                st = arch->op_extract(dialog,
				      *param.fs_root,
				      extract_options,
				      nullptr);
		if(!param.quiet)
		    display_rest_stat(dialog, st);
                if(st.get_errored() > 0)
                    throw Edata(gettext("All files asked could not be restored"));
                break;
            case diff:
		line_tools_crypto_split_algo_pass(param.pass,
						  crypto,
						  tmp_pass,
						  no_cipher_given,
						  recipients);
		read_options.clear();
		if(no_cipher_given)
			// since archive format 9 crypto algo used
			// is stored in the archive, it will be used
			// unless we specify explicitely the cipher to use
		    read_options.set_crypto_algo(libdar::crypto_none);
		else
 		    read_options.set_crypto_algo(crypto);
		read_options.set_crypto_pass(tmp_pass);
		read_options.set_crypto_size(param.crypto_size);
		read_options.set_input_pipe(param.input_pipe);
		read_options.set_output_pipe(param.output_pipe);
		read_options.set_execute(param.execute);
		read_options.set_info_details(param.info_details);
		read_options.set_lax(param.lax);
		read_options.set_sequential_read(param.sequential_read);
		read_options.set_slice_min_digits(param.num_digits);
		read_options.set_ignore_signature_check_failure(param.blind_signatures);
		read_options.set_multi_threaded(param.multi_threaded);

		if(param.ref_filename != nullptr && param.ref_root != nullptr)
		{
		    secu_string ref_tmp_pass;
		    crypto_algo ref_crypto;

		    line_tools_crypto_split_algo_pass(param.pass_ref,
						      ref_crypto,
						      ref_tmp_pass,
						      no_cipher_given,
						      recipients);
		    read_options.set_external_catalogue(*param.ref_root, *param.ref_filename);
		    if(no_cipher_given)
			    // since archive format 9 crypto algo used
			    // is stored in the archive, it will be used
			    // unless we specify explicitely the cipher to use
			read_options.set_ref_crypto_algo(libdar::crypto_none);
		    else
			read_options.set_ref_crypto_algo(ref_crypto);
		    read_options.set_ref_crypto_pass(ref_tmp_pass);
		    read_options.set_ref_crypto_size(param.crypto_size_ref);
		    read_options.set_ref_execute(param.execute_ref);
		    read_options.set_ref_slice_min_digits(param.ref_num_digits);
		}
		arch = new (nothrow) archive(dialog,
					     *param.sauv_root,
					     param.filename,
					     EXTENSION,
					     read_options);
		if(arch == nullptr)
		    throw Ememory("little_main");

		diff_options.clear();
		diff_options.set_selection(*param.selection);
		diff_options.set_subtree(*param.subtree);
		diff_options.set_info_details(param.info_details);
		diff_options.set_display_treated(param.display_treated,
						 param.display_treated_only_dir);
		diff_options.set_display_skipped(param.display_skipped);
		diff_options.set_ea_mask(*param.ea_mask);
		diff_options.set_what_to_check(param.what_to_check);
		diff_options.set_alter_atime(param.alter_atime);
		diff_options.set_furtive_read_mode(param.furtive_read_mode);
		diff_options.set_hourshift(param.hourshift);
		diff_options.set_compare_symlink_date(param.no_compare_symlink_date);
		diff_options.set_fsa_scope(param.scope);
                st = arch->op_diff(dialog, *param.fs_root,
				   diff_options,
				   nullptr);
		if(!param.quiet)
		    display_diff_stat(dialog, st);
                if(st.get_errored() > 0 || st.get_deleted() > 0)
                    throw Edata(gettext("Some file comparisons failed"));
                break;
	    case test:
		line_tools_crypto_split_algo_pass(param.pass,
						  crypto,
						  tmp_pass,
						  no_cipher_given,
						  recipients);
		read_options.clear();
		if(no_cipher_given)
			// since archive format 9 crypto algo used
			// is stored in the archive, it will be used
			// unless we specify explicitely the cipher to use
		    read_options.set_crypto_algo(libdar::crypto_none);
		else
		    read_options.set_crypto_algo(crypto);
		read_options.set_crypto_pass(tmp_pass);
		read_options.set_crypto_size(param.crypto_size);
		read_options.set_input_pipe(param.input_pipe);
		read_options.set_output_pipe(param.output_pipe);
		read_options.set_execute(param.execute);
		read_options.set_info_details(param.info_details);
		read_options.set_lax(param.lax);
		read_options.set_sequential_read(param.sequential_read);
		read_options.set_slice_min_digits(param.num_digits);
		read_options.set_ignore_signature_check_failure(param.blind_signatures);
		read_options.set_multi_threaded(param.multi_threaded);

		if(param.ref_filename != nullptr && param.ref_root != nullptr)
		{
		    secu_string ref_tmp_pass;
		    crypto_algo ref_crypto;

		    line_tools_crypto_split_algo_pass(param.pass_ref,
						      ref_crypto,
						      ref_tmp_pass,
						      no_cipher_given,
						      recipients);
		    read_options.set_external_catalogue(*param.ref_root, *param.ref_filename);
		    if(no_cipher_given)
			    // since archive format 9 crypto algo used
			    // is stored in the archive, it will be used
			    // unless we specify explicitely the cipher to use
			read_options.set_ref_crypto_algo(libdar::crypto_none);
		    else
			read_options.set_ref_crypto_algo(ref_crypto);
		    read_options.set_ref_crypto_pass(ref_tmp_pass);
		    read_options.set_ref_crypto_size(param.crypto_size_ref);
		    read_options.set_ref_execute(param.execute_ref);
		    read_options.set_ref_slice_min_digits(param.ref_num_digits);
		}
		arch = new (nothrow) archive(dialog,
					     *param.sauv_root,
					     param.filename,
					     EXTENSION,
					     read_options);
		if(arch == nullptr)
		    throw Ememory("little_main");

		test_options.clear();
		test_options.set_selection(*param.selection);
		test_options.set_subtree(*param.subtree);
		test_options.set_info_details(param.info_details);
		test_options.set_display_treated(param.display_treated,
						 param.display_treated_only_dir);
		test_options.set_display_skipped(param.display_skipped);
		test_options.set_empty(param.empty);
                st = arch->op_test(dialog, test_options, nullptr);
		if(!param.quiet)
		    display_test_stat(dialog, st);
                if(st.get_errored() > 0)
                    throw Edata(gettext("Some files are corrupted in the archive and it will not be possible to restore them"));
                break;

            case listing:
		line_tools_crypto_split_algo_pass(param.pass,
						  crypto,
						  tmp_pass,
						  no_cipher_given,
						  recipients);
		read_options.clear();
		if(no_cipher_given)
			// since archive format 9 crypto algo used
			// is stored in the archive, it will be used
			// unless we specify explicitely the cipher to use
		    read_options.set_crypto_algo(libdar::crypto_none);
		else
		    read_options.set_crypto_algo(crypto);
		read_options.set_crypto_pass(tmp_pass);
		read_options.set_crypto_size(param.crypto_size);
		read_options.set_input_pipe(param.input_pipe);
		read_options.set_output_pipe(param.output_pipe);
		read_options.set_execute(param.execute);
		read_options.set_info_details(param.info_details);
		read_options.set_lax(param.lax);
		read_options.set_sequential_read(param.sequential_read);
		read_options.set_slice_min_digits(param.num_digits);
		read_options.set_ignore_signature_check_failure(param.blind_signatures);
		read_options.set_multi_threaded(param.multi_threaded);

		arch = new (nothrow) archive(dialog,
					     *param.sauv_root,
					     param.filename,
					     EXTENSION,
					     read_options);
		if(arch == nullptr)
		    throw Ememory("little_main");

		if(param.quiet)
		{
		    const list<signator> & gnupg_signed = arch->get_signatories();
		    arch->summary(dialog);
		    line_tools_display_signatories(dialog, gnupg_signed);
		}
		else
		{
		    if(param.info_details)
		    {
			const list<signator> & gnupg_signed = arch->get_signatories();
			arch->summary(dialog);
			line_tools_display_signatories(dialog, gnupg_signed);
			dialog.pause(gettext("Continue listing archive contents?"));
		    }
		    listing_options.clear();
		    listing_options.set_info_details(param.info_details);
		    listing_options.set_list_mode(param.list_mode);
		    listing_options.set_selection(*param.selection);
		    listing_options.set_subtree(*param.subtree);
		    listing_options.set_filter_unsaved(param.filter_unsaved);
		    listing_options.set_display_ea(param.list_ea);
		    if(!param.file_size.is_zero() && param.list_mode == archive_options_listing::slicing)
			listing_options.set_user_slicing(param.first_file_size, param.file_size);
		    arch->op_listing(dialog, listing_options);
		}
                break;
            default:
                throw SRC_BUG;
            }
	}
	catch(...)
	{
	    if(!param.quiet)
		dialog.warning(gettext("Final memory cleanup..."));

	    if(arch != nullptr)
	    {
		delete arch;
		arch = nullptr;
	    }
	    if(cur != nullptr)
	    {
		delete cur;
		cur = nullptr;
	    }
	    if(aux != nullptr)
	    {
		delete aux;
		aux = nullptr;
	    }
	    throw;
	}

	if(param.info_details)
	    dialog.warning(gettext("Final memory cleanup..."));

	string mesg;

	if(arch != nullptr)
	{
	    mesg = arch->free_and_check_memory();
	    if(!mesg.empty())
		cerr << mesg << endl;
	}
	if(aux != nullptr)
	{
	    mesg = aux->free_and_check_memory();
	    if(!mesg.empty())
		cerr << mesg << endl;
	}
	if(cur != nullptr)
	{
	    mesg = cur->free_and_check_memory();
	    if(!mesg.empty())
		cerr << mesg << endl;
	}

	if(arch != nullptr)
	{
	    delete arch;
	    arch = nullptr;
	}
	if(cur != nullptr)
	{
	    delete cur;
	    cur = nullptr;
	}
	if(aux != nullptr)
	{
	    delete aux;
	    aux = nullptr;
	}

        return ret;
    }
}

static void display_sauv_stat(user_interaction & dialog, const statistics & st)
{
    infinint total = st.total();
    infinint treated = st.get_treated();
    infinint hard_links = st.get_hard_links();
    infinint tooold = st.get_tooold();
    infinint skipped = st.get_skipped();
    infinint ignored = st.get_ignored();
    infinint errored = st.get_errored();
    infinint deleted = st.get_deleted();
    infinint ea_treated = st.get_ea_treated();
    infinint fsa_treated = st.get_fsa_treated();
    infinint byte_count = st.get_byte_amount();

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i inode(s) saved\n"), &treated);
    dialog.printf(gettext("   including %i hard link(s) treated\n"), &hard_links);
    dialog.printf(gettext(" %i inode(s) changed at the moment of the backup and could not be saved properly\n"), &tooold);
    dialog.printf(gettext(" %i byte(s) have been wasted in the archive to resave changing files"), & byte_count);
    dialog.printf(gettext(" %i inode(s) not saved (no inode/file change)\n"), &skipped);
    dialog.printf(gettext(" %i inode(s) failed to be saved (filesystem error)\n"), &errored);
    dialog.printf(gettext(" %i inode(s) ignored (excluded by filters)\n"), &ignored);
    dialog.printf(gettext(" %i inode(s) recorded as deleted from reference backup\n"), &deleted);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of inode(s) considered: %i\n"), &total);
#ifdef EA_SUPPORT
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" EA saved for %i inode(s)\n"), &ea_treated);
#endif
    dialog.printf(gettext(" FSA saved for %i inode(s)\n"), &fsa_treated);
    dialog.printf(" --------------------------------------------\n");
}

static void display_rest_stat(user_interaction & dialog, const statistics & st)
{
    infinint total = st.total();
    infinint treated = st.get_treated();
    infinint hard_links = st.get_hard_links();
    infinint tooold = st.get_tooold();
    infinint skipped = st.get_skipped();
    infinint ignored = st.get_ignored();
    infinint errored = st.get_errored();
    infinint deleted = st.get_deleted();
    infinint ea_treated = st.get_ea_treated();
    infinint fsa_treated = st.get_fsa_treated();

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i inode(s) restored\n"), &treated);
    dialog.printf(gettext("    including %i hard link(s)\n"), &hard_links);
    dialog.printf(gettext(" %i inode(s) not restored (not saved in archive)\n"), &skipped);
    dialog.printf(gettext(" %i inode(s) not restored (overwriting policy decision)\n"), &tooold);
    dialog.printf(gettext(" %i inode(s) ignored (excluded by filters)\n"), &ignored);
    dialog.printf(gettext(" %i inode(s) failed to restore (filesystem error)\n"), &errored);
    dialog.printf(gettext(" %i inode(s) deleted\n"), &deleted);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of inode(s) considered: %i\n"), &total);
#ifdef EA_SUPPORT
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" EA restored for %i inode(s)\n"), &ea_treated);
#endif
    dialog.printf(gettext(" FSA restored for %i inode(s)\n"), &fsa_treated);
    dialog.printf(" --------------------------------------------\n");
}

static void display_diff_stat(user_interaction & dialog, const statistics &st)
{
    infinint total = st.total();
    infinint treated = st.get_treated();
    infinint ignored = st.get_ignored();
    infinint errored = st.get_errored();

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i item(s) treated\n"), &treated);
    dialog.printf(gettext(" %i item(s) do not match those on filesystem\n"), &errored);
    dialog.printf(gettext(" %i item(s) ignored (excluded by filters)\n"), &ignored);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of items considered: %i\n"), &total);
    dialog.printf(" --------------------------------------------\n");
}

static void display_test_stat(user_interaction & dialog, const statistics & st)
{
    infinint total = st.total();

    infinint treated = st.get_treated();
    infinint errored = st.get_errored();
    infinint skipped = st.get_skipped();

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i item(s) treated\n"), &treated);
    dialog.printf(gettext(" %i item(s) with error\n"), &errored);
    dialog.printf(gettext(" %i item(s) ignored (excluded by filters)\n"), &skipped);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of items considered: %i\n"), &total);
    dialog.printf(" --------------------------------------------\n");
}

static void display_merge_stat(user_interaction & dialog, const statistics & st)
{
    infinint total = st.total();

    infinint treated = st.get_treated();
    infinint hard_links = st.get_hard_links();
    infinint deleted = st.get_deleted();
    infinint ignored = st.get_ignored();
    infinint ea_treated = st.get_ea_treated();
    infinint fsa_treated = st.get_fsa_treated();

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i inode(s) added to archive\n"), &treated);
    dialog.printf(gettext(" with %i hard link(s) recorded\n"), &hard_links);
    dialog.printf(gettext(" %i inode(s) ignored (excluded by filters)\n"), &ignored);
    dialog.printf(gettext(" %i inode(s) recorded as deleted\n"), &deleted);
#ifdef EA_SUPPORT
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" EA saved for %i inode(s)\n"), &ea_treated);
#endif
    dialog.printf(gettext(" FSA saved for %i inode(s)\n"), &fsa_treated);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of inode(s) considered: %i\n"), &total);
    dialog.printf(" --------------------------------------------\n");
}

const char *dar_version()
{
    return DAR_VERSION;
}
