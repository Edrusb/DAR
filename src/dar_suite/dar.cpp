/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

#include <string>
#include <iostream>
#include <new>
#include <vector>

#include "libdar.hpp"
#include "shell_interaction.hpp"
#include "dar_suite.hpp"
#include "command_line.hpp"
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
static S_I little_main(shared_ptr<user_interaction> & dialog, S_I argc, char * const argv[], const char **env);

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

static S_I little_main(shared_ptr<user_interaction> & dialog, S_I argc, char * const argv[], const char **env)
{
    S_I ret = EXIT_OK;
    line_param param;
    const char *home = line_tools_get_from_env(env, "HOME");
    deque<string> dar_dcf_path = line_tools_explode_PATH(line_tools_get_from_env(env, "DAR_DCF_PATH"));
    deque<string> dar_duc_path = line_tools_explode_PATH(line_tools_get_from_env(env, "DAR_DUC_PATH"));
    string sftp_known_hosts;
    string sftp_pub_filekey;
    string sftp_prv_filekey;
    const char *env_knownhosts = line_tools_get_from_env(env, "DAR_SFTP_KNOWNHOSTS_FILE");
    const char *env_pub_filekey = line_tools_get_from_env(env, "DAR_SFTP_PUBLIC_KEYFILE");
    const char *env_prv_filekey = line_tools_get_from_env(env, "DAR_SFTP_PRIVATE_KEYFILE");
    const char *env_ignored_as_symlink = line_tools_get_from_env(env, "DAR_IGNORED_AS_SYMLINK");
    shell_interaction *shelli = dynamic_cast<shell_interaction *>(dialog.get());
    string home_pref;

    if(!dialog)
	throw SRC_BUG;
    if(shelli == nullptr)
	throw SRC_BUG;
    if(home == nullptr)
    {
        home = "/";
	home_pref = "";
    }
    else
	home_pref = string(home);

    if(env_knownhosts != nullptr)
	sftp_known_hosts = string(env_knownhosts);
    else
	sftp_known_hosts = home_pref + "/.ssh/known_hosts";
    if(env_pub_filekey != nullptr)
	sftp_pub_filekey = string(env_pub_filekey);
    else
	sftp_pub_filekey = home_pref + "/.ssh/id_rsa.pub";
    if(env_prv_filekey != nullptr)
	sftp_prv_filekey = string(env_prv_filekey);
    else
	sftp_prv_filekey = home_pref + "/.ssh/id_rsa";

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
	shared_ptr<entrepot> repo;
	shared_ptr<entrepot> ref_repo;
	shared_ptr<entrepot> aux_repo;
	shell_interaction *ptr = dynamic_cast<shell_interaction *>(dialog.get());

	if(ptr != nullptr)
	{
	    ptr->set_beep(param.beep);
	    ptr->set_fully_detailed_datetime(param.fully_detailed_dates);
	}

	if(param.display_masks)
	{
	    const string initial_prefix = "    ";
	    if(param.subtree != nullptr)
	    {
		string res = param.subtree->dump(initial_prefix);
		dialog->message("directory tree filter:");
		dialog->message(res);
		dialog->message("");
	    }

	    if(param.selection != nullptr)
	    {
		string res = param.selection->dump(initial_prefix);
		dialog->message("filename filter:");
		dialog->message(res);
		dialog->message("");
	    }

	    if(param.ea_mask != nullptr)
	    {
		string res = param.ea_mask->dump(initial_prefix);
		dialog->message("EA filter:");
		dialog->message(res);
		dialog->message("");
	    }

	    if(param.compress_mask != nullptr)
	    {
		string res = param.compress_mask->dump(initial_prefix);
		dialog->message("Compression filter:");
		dialog->message(res);
		dialog->message("");
	    }

	    if(param.backup_hook_mask != nullptr)
	    {
		string res = param.backup_hook_mask->dump(initial_prefix);
		dialog->message("backup hook filter:");
		dialog->message(res);
		dialog->message("");
	    }
	}

        if(param.filename != "-"
	   || (param.output_pipe != "" && param.op != create && param.op != isolate && param.op != merging))
	{
	    shell_interaction *ptr = dynamic_cast<shell_interaction *>(dialog.get());

	    if(ptr != nullptr)
		ptr->change_non_interactive_output(cout);
	}
            // standard output can now be used to send non interactive
            // messages

	try
	{
	    shared_ptr<archive> arch;
	    shared_ptr<archive> aux;
	    unique_ptr<archive> cur;
	    statistics st = false;
	    secu_string tmp_pass;
	    crypto_algo crypto, aux_crypto;
	    archive_options_read read_options;
	    archive_options_create create_options;
	    archive_options_isolate isolate_options;
	    archive_options_merge merge_options;
	    archive_options_extract extract_options;
	    archive_options_listing_shell listing_options;
	    archive_options_diff diff_options;
	    archive_options_test test_options;
	    archive_options_repair repair_options;
	    bool no_cipher_given;
	    vector<string> recipients;
	    set<string> ignored_as_symlink_listing;
	    shared_ptr<database> extractdb;

	    if(param.remote.ent_host.size() != 0)
	    {
		repo.reset(new (nothrow) entrepot_libcurl(dialog,
							  string_to_mycurl_protocol(param.remote.ent_proto),
							  param.remote.ent_login,
							  param.remote.ent_pass,
							  param.remote.ent_host,
							  param.remote.ent_port,
							  param.remote.auth_from_file,
							  sftp_pub_filekey,
							  sftp_prv_filekey,
							  sftp_known_hosts,
							  param.remote.network_retry,
							  param.remote_verbose));
		if(!repo)
		    throw Ememory("little_main");
	    }

	    if(param.ref_remote.ent_host.size() != 0)
	    {
		ref_repo.reset(new (nothrow) entrepot_libcurl(dialog,
							      string_to_mycurl_protocol(param.ref_remote.ent_proto),
							      param.ref_remote.ent_login,
							      param.ref_remote.ent_pass,
							      param.ref_remote.ent_host,
							      param.ref_remote.ent_port,
							      param.ref_remote.auth_from_file,
							      sftp_pub_filekey,
							      sftp_prv_filekey,
							      sftp_known_hosts,
							      param.ref_remote.network_retry,
							      param.remote_verbose));
		if(!ref_repo)
		    throw Ememory("little_main");
	    }

	    if(param.aux_remote.ent_host.size() != 0)
	    {
		aux_repo.reset(new (nothrow) entrepot_libcurl(dialog,
							      string_to_mycurl_protocol(param.aux_remote.ent_proto),
							      param.aux_remote.ent_login,
							      param.aux_remote.ent_pass,
							      param.aux_remote.ent_host,
							      param.aux_remote.ent_port,
							      param.aux_remote.auth_from_file,
							      sftp_pub_filekey,
							      sftp_prv_filekey,
							      sftp_known_hosts,
							      param.aux_remote.network_retry,
							      param.remote_verbose));
		if(!aux_repo)
		    throw Ememory("little_main");
	    }

            switch(param.op)
            {
            case create:
	    case merging:
	    case repairing:
		if(param.ref_filename != nullptr && param.ref_root != nullptr)
		{
		    line_tools_crypto_split_algo_pass(param.pass_ref,
						      crypto,
						      tmp_pass,
						      no_cipher_given,
						      recipients);
		    if(param.op == merging && param.aux_root != nullptr && param.info_details)
			dialog->message(gettext("Considering the (first) archive of reference:"));
		    if(param.sequential_read && param.delta_diff)
			throw Erange("little_main",gettext("Sequential reading of the archive of reference is not possible when delta difference is requested, you need either to read the archive of reference in direct access mode (without \'--sequential-read\' option) or disable binary delta (adding \'--delta no-patch\' option)"));

		    read_options.clear();
		    if(no_cipher_given)
			    // since archive format 9 crypto algo used
			    // is stored in the archive, it will be used
			    // unless we specify explicitely the cipher to use
			read_options.set_crypto_algo(crypto_algo::none);
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
		    read_options.set_multi_threaded_crypto(param.multi_threaded_crypto);
		    read_options.set_multi_threaded_compress(param.multi_threaded_compress);
		    read_options.set_silent(param.quiet_crypto);

		    if(param.sequential_read)
		    {
			if(param.op == merging)
			    throw Erange("little_main", gettext("Using sequential reading mode for archive source is not possible for merging operation"));
			else
			    read_options.set_sequential_read(true);
		    }
		    if(ref_repo)
			read_options.set_entrepot(ref_repo);

		    if(param.op != repairing)
		    {
			arch.reset(new (nothrow) archive(dialog,
							 *param.ref_root,
							 *param.ref_filename,
							 EXTENSION,
							 read_options));
			if(!arch)
			    throw Ememory("little_main");
		    }
		    else // repairing
			arch.reset();
		}

		if(param.aux_root != nullptr && param.aux_filename != nullptr)
		{
		    if(param.op != merging && param.op != create)
			throw SRC_BUG;
		    if(param.op == merging)
		    {
			if(param.info_details)
			    dialog->message(gettext("Considering the second (alias auxiliary) archive of reference:"));
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
			    read_options.set_crypto_algo(crypto_algo::none);
			else
			    read_options.set_crypto_algo(aux_crypto);

			read_options.set_crypto_pass(tmp_pass);
			read_options.set_crypto_size(param.aux_crypto_size);
			read_options.set_execute(param.aux_execute);
			read_options.set_info_details(param.info_details);
			read_options.set_lax(param.lax);
			read_options.set_slice_min_digits(param.aux_num_digits);
			read_options.set_ignore_signature_check_failure(param.blind_signatures);
			read_options.set_multi_threaded_crypto(param.multi_threaded_crypto);
			read_options.set_multi_threaded_compress(param.multi_threaded_compress);
			read_options.set_silent(param.quiet_crypto);

			if(param.sequential_read)
			    throw Erange("little_main", gettext("Using sequential reading mode for archive source is not possible for merging operation"));
			if(aux_repo)
			    read_options.set_entrepot(aux_repo);

			aux.reset(new (nothrow) archive(dialog,
							*param.aux_root,
							*param.aux_filename,
							EXTENSION,
							read_options));
			if(!aux)
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
		    if(arch)
		    {
			if(!param.delta_sig && !param.delta_diff)
				// we may need to copy delta_sig
				// from the archive of reference
				// to the resulting archive so we
				// cannot drop fd when delta_sig
				// is requested
				// We may also need to read delta_sig
				// to perform the delta difference
			    arch->drop_all_filedescriptors(false);
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
		    create_options.set_compression_block_size(param.compression_block_size);
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

		    if(param.same_fs_incl.empty() && param.same_fs_excl.empty()) // legacy -M option
			create_options.set_same_fs(param.same_fs);
		    else // new 2.7.0 -M option syntax
		    {
			deque<string>::iterator it = param.same_fs_incl.begin();

			while(it != param.same_fs_incl.end())
			{
			    create_options.set_same_fs_include(*it);
			    ++it;
			}

			it = param.same_fs_excl.begin();
			while(it != param.same_fs_excl.end())
			{
			    create_options.set_same_fs_exclude(*it);
			    ++it;
			}
		    }

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
		    create_options.set_multi_threaded_crypto(param.multi_threaded_crypto);
		    create_options.set_multi_threaded_compress(param.multi_threaded_compress);
		    create_options.set_delta_signature(param.delta_sig);
		    if(param.delta_sig_min_size > 0)
			create_options.set_delta_sig_min_size(param.delta_sig_min_size);
		    if(compile_time::librsync())
			create_options.set_delta_diff(param.delta_diff);
		    create_options.set_auto_zeroing_neg_dates(param.zeroing_neg_dates);

		    if(param.backup_hook_mask != nullptr)
			create_options.set_backup_hook(param.backup_hook_execute, *param.backup_hook_mask);
		    create_options.set_ignore_unknown_inode_type(param.ignore_unknown_inode);
		    if(param.delta_mask != nullptr)
			create_options.set_delta_mask(*param.delta_mask);
		    if(repo)
			create_options.set_entrepot(repo);
		    if(param.ignored_as_symlink.size() > 0)
		    {
			deque<string> tmp;

			line_tools_split(param.ignored_as_symlink, ':', tmp);
			ignored_as_symlink_listing = line_tools_deque_to_set(tmp);
		    }
		    else
		    {
			if(env_ignored_as_symlink != nullptr)
			{
			    deque<string> tmp;

			    line_tools_split(env_ignored_as_symlink, ':', tmp);
			    ignored_as_symlink_listing = line_tools_deque_to_set(tmp);
			}
		    }
		    create_options.set_ignored_as_symlink(ignored_as_symlink_listing);
		    create_options.set_modified_data_detection(param.modet);
		    if(param.iteration_count > 0)
			create_options.set_iteration_count(param.iteration_count);
		    if(param.kdf_hash != hash_algo::none)
			create_options.set_kdf_hash(param.kdf_hash);
		    create_options.set_sig_block_len(param.delta_sig_len);
		    create_options.set_never_resave_uncompressed(param.never_resave_uncompr);

		    cur.reset(new (nothrow) archive(dialog,
						    *param.fs_root,
						    *param.sauv_root,
						    param.filename,
						    EXTENSION,
						    create_options,
						    &st));
		    if(!cur)
			throw Ememory("little_main");
		    if(!param.quiet)
			display_sauv_stat(*dialog, st);
		    break;
		case merging:
		    merge_options.clear();
		    merge_options.set_auxiliary_ref(aux);
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
		    merge_options.set_compression_block_size(param.compression_block_size);
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
		    merge_options.set_multi_threaded_crypto(param.multi_threaded_crypto);
		    merge_options.set_multi_threaded_compress(param.multi_threaded_compress);
		    merge_options.set_delta_signature(param.delta_sig);
		    if(param.delta_mask != nullptr)
			merge_options.set_delta_mask(*param.delta_mask);
		    if(param.delta_sig_min_size > 0)
			merge_options.set_delta_sig_min_size(param.delta_sig_min_size);
		    if(repo)
			merge_options.set_entrepot(repo);
		    if(param.iteration_count > 0)
			merge_options.set_iteration_count(param.iteration_count);
		    if(param.kdf_hash != hash_algo::none)
			merge_options.set_kdf_hash(param.kdf_hash);
		    merge_options.set_sig_block_len(param.delta_sig_len);
		    merge_options.set_never_resave_uncompressed(param.never_resave_uncompr);

		    cur.reset(new (nothrow) archive(dialog,            // user_interaction &
						    *param.sauv_root,  //const path &
						    arch,              // the mandatory archive of reference
						    param.filename,    // const string &
						    EXTENSION,         // const string &
						    merge_options,
						    &st));             // statistics*
		    if(!cur)
			throw Ememory("little_main");
		    if(!param.quiet)
			display_merge_stat(*dialog, st);
		    break;
		case repairing:
		    repair_options.clear();
		    repair_options.set_warn_over(param.warn_over);
		    repair_options.set_allow_over(param.allow_over);
		    repair_options.set_info_details(param.info_details);
		    repair_options.set_display_treated(param.display_treated,
						      param.display_treated_only_dir);
		    repair_options.set_display_skipped(param.display_skipped);
		    repair_options.set_pause(param.pause);
		    repair_options.set_slicing(param.file_size, param.first_file_size);
		    repair_options.set_execute(param.execute);
		    repair_options.set_crypto_algo(crypto);
		    repair_options.set_crypto_pass(tmp_pass);
		    repair_options.set_crypto_size(param.crypto_size);
		    repair_options.set_gnupg_recipients(recipients);
		    if(recipients.empty() && !param.signatories.empty())
			throw Erange("little_main", gettext("Archive signature is only possible with gnupg encryption"));
		    repair_options.set_gnupg_signatories(param.signatories);
		    repair_options.set_empty(param.empty);
		    repair_options.set_slice_permission(param.slice_perm);
		    repair_options.set_slice_user_ownership(param.slice_user);
		    repair_options.set_slice_group_ownership(param.slice_group);
		    repair_options.set_user_comment(param.user_comment);
		    repair_options.set_hash_algo(param.hash);
		    repair_options.set_slice_min_digits(param.num_digits);
		    repair_options.set_multi_threaded_crypto(param.multi_threaded_crypto);
		    repair_options.set_multi_threaded_compress(param.multi_threaded_compress);
		    if(param.iteration_count > 0)
			repair_options.set_iteration_count(param.iteration_count);
		    if(param.kdf_hash != hash_algo::none)
			repair_options.set_kdf_hash(param.kdf_hash);

		    if(repo)
			repair_options.set_entrepot(repo);

		    cur.reset(new (nothrow) archive(dialog,
						    *param.ref_root,
						    *param.ref_filename,
						    EXTENSION,
						    read_options,
						    *param.sauv_root,
						    param.filename,
						    EXTENSION,
						    repair_options));
		    if(!cur)
			throw Ememory("little_main");
		    break;
		default:
		    throw SRC_BUG;
		}

		    // making some room in memory

		if(param.info_details)
		    dialog->message(gettext("Making room in memory (releasing memory used by archive of reference)..."));
		arch.reset();
		aux.reset();

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
				dialog->message(gettext("Now performing on-fly isolation..."));
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
				isolate_options.set_compression(compression::bzip2);
			    else
				if(compile_time::libz())
				    isolate_options.set_compression(compression::gzip);
				else
				    if(compile_time::liblzo())
					isolate_options.set_compression(compression::lzo);
				    else // no compression
					isolate_options.set_compression(compression::none);
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
			    isolate_options.set_multi_threaded_crypto(param.multi_threaded_crypto);
			    isolate_options.set_multi_threaded_compress(param.multi_threaded_compress);

				// copying delta sig is not possible in on-fly isolation,
				// archive must be closed and re-open in read mode to be able
				// to fetch delta signatures
			    isolate_options.set_delta_signature(false);
			    if(aux_repo)
				isolate_options.set_entrepot(aux_repo);
			    if(param.iteration_count > 0)
				isolate_options.set_iteration_count(param.iteration_count);
			    if(param.kdf_hash != hash_algo::none)
				isolate_options.set_kdf_hash(param.kdf_hash);
			    isolate_options.set_sig_block_len(param.delta_sig_len);

			    cur->op_isolate(*param.aux_root,
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
		    read_options.set_crypto_algo(crypto_algo::none);
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
		read_options.set_multi_threaded_crypto(param.multi_threaded_crypto);
		read_options.set_multi_threaded_compress(param.multi_threaded_compress);
		if(ref_repo)
		    read_options.set_entrepot(ref_repo);
		    // yes this is "ref_repo" where is located the -A-pointed-to archive
		    // -C-pointed-to archive is located in the "repo" entrepot
		read_options.set_silent(param.quiet_crypto);
		read_options.set_force_first_slice(param.force_first_slice);
		if(param.isolation_repair) // this field is used for isolation options
		{
			// forced options
		    read_options.set_force_first_slice(true);
		    read_options.set_sequential_read(true);
		}


		arch.reset(new (nothrow) archive(dialog,
						 *param.ref_root,
						 *param.ref_filename,
						 EXTENSION,
						 read_options));
		if(!arch)
		    throw Ememory("little_main");

		if(!param.delta_sig)
		    arch->drop_all_filedescriptors(param.isolation_repair);

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
		isolate_options.set_compression_block_size(param.compression_block_size);
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
		isolate_options.set_multi_threaded_crypto(param.multi_threaded_crypto);
		isolate_options.set_multi_threaded_compress(param.multi_threaded_compress);
		isolate_options.set_delta_signature(param.delta_sig);
		if(param.delta_mask != nullptr)
		    isolate_options.set_delta_mask(*param.delta_mask);
		if(param.delta_sig_min_size > 0)
		    isolate_options.set_delta_sig_min_size(param.delta_sig_min_size);
		if(repo)
		    isolate_options.set_entrepot(repo);
		    // yes this is "ref_repo" where is located the -A-pointed-to archive
		    // -C-pointed-to archive is located in the "repo" entrepot
		if(param.iteration_count > 0)
		    isolate_options.set_iteration_count(param.iteration_count);
		if(param.kdf_hash != hash_algo::none)
		    isolate_options.set_kdf_hash(param.kdf_hash);
		isolate_options.set_sig_block_len(param.delta_sig_len);
		isolate_options.set_repair_mode(param.isolation_repair);

                arch->op_isolate(*param.sauv_root,
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
		    read_options.set_crypto_algo(crypto_algo::none);
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
		read_options.set_multi_threaded_crypto(param.multi_threaded_crypto);
		read_options.set_multi_threaded_compress(param.multi_threaded_compress);
		if(repo)
		    read_options.set_entrepot(repo);
		if(param.sequential_read)
		    read_options.set_early_memory_release(true);
		    // early memory release is useless out of sequential read
		    // as the whole catalogue gets loaded into memory, releasing
		    // it earlier during the reading process does not free memory
		    // from at the heap level of the process for most (if not all)
		    // operating systems.
		read_options.set_silent(param.quiet_crypto);

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
			read_options.set_ref_crypto_algo(crypto_algo::none);
		    else
  		        read_options.set_ref_crypto_algo(ref_crypto);
		    read_options.set_ref_crypto_pass(ref_tmp_pass);
		    read_options.set_ref_crypto_size(param.crypto_size_ref);
		    read_options.set_ref_execute(param.execute_ref);
		    read_options.set_ref_slice_min_digits(param.ref_num_digits);
		    if(ref_repo)
			read_options.set_ref_entrepot(ref_repo);
		    read_options.set_silent(param.quiet_crypto);
		    read_options.set_force_first_slice(param.force_first_slice);
		}

		if(param.extract_from_database)
		{
		    database_open_options datopt;
		    path tmp = *param.sauv_root;
		    tmp.append(param.filename);

		    datopt.set_warn_order(false);
		    datopt.set_partial(false);

		    extractdb.reset(new (nothrow) database(dialog,
							   tmp.display(),
							   datopt));

		    if(!extractdb)
			throw Ememory("little_main");
		}
		else
		{
		    arch.reset(new (nothrow) archive(dialog,
						     *param.sauv_root,
						     param.filename,
						     EXTENSION,
						     read_options));
		    if(!arch)
			throw Ememory("little_main");
		}

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
		extract_options.set_ignore_unix_sockets(param.unix_sockets);
		extract_options.set_in_place(param.in_place);

		if(param.extract_from_database)
		{
		    database_restore_options datopt;

		    datopt.set_early_release(false);
		    datopt.set_info_details(param.info_details);
		    datopt.set_ignore_dar_options_in_database(true); // this is not used anyway by database::restore()
		    datopt.set_date(param.fixed_date);
		    datopt.set_even_when_removed(false); // do not keep file restored if they had been deleted at the requested time

		    extractdb->restore(read_options,
				       *param.fs_root,
				       extract_options,
				       datopt,
				       nullptr);
		}
		else
		{
		    st = arch->op_extract(*param.fs_root,
					  extract_options,
					  nullptr);
		    if(!param.quiet)
			display_rest_stat(*dialog, st);
		    if(st.get_errored() > 0)
			throw Edata(gettext("All files asked could not be restored"));
		}
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
		    read_options.set_crypto_algo(crypto_algo::none);
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
		read_options.set_multi_threaded_crypto(param.multi_threaded_crypto);
		read_options.set_multi_threaded_compress(param.multi_threaded_compress);
		if(repo)
		    read_options.set_entrepot(repo);
		if(param.sequential_read)
		    read_options.set_early_memory_release(true);
		read_options.set_silent(param.quiet_crypto);

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
			read_options.set_ref_crypto_algo(crypto_algo::none);
		    else
			read_options.set_ref_crypto_algo(ref_crypto);
		    read_options.set_ref_crypto_pass(ref_tmp_pass);
		    read_options.set_ref_crypto_size(param.crypto_size_ref);
		    read_options.set_ref_execute(param.execute_ref);
		    read_options.set_ref_slice_min_digits(param.ref_num_digits);
		    if(ref_repo)
			read_options.set_ref_entrepot(ref_repo);
		    read_options.set_force_first_slice(param.force_first_slice);
		}
		arch.reset(new (nothrow) archive(dialog,
						 *param.sauv_root,
						 param.filename,
						 EXTENSION,
						 read_options));
		if(!arch)
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
		diff_options.set_in_place(param.in_place);
		diff_options.set_auto_zeroing_neg_dates(param.zeroing_neg_dates);

                st = arch->op_diff(*param.fs_root,
				   diff_options,
				   nullptr);
		if(!param.quiet)
		    display_diff_stat(*dialog, st);
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
		    read_options.set_crypto_algo(crypto_algo::none);
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
		read_options.set_multi_threaded_crypto(param.multi_threaded_crypto);
		read_options.set_multi_threaded_compress(param.multi_threaded_compress);
		if(repo)
		    read_options.set_entrepot(repo);
		if(param.sequential_read)
		    read_options.set_early_memory_release(true);
		read_options.set_silent(param.quiet_crypto);

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
			read_options.set_ref_crypto_algo(crypto_algo::none);
		    else
			read_options.set_ref_crypto_algo(ref_crypto);
		    read_options.set_ref_crypto_pass(ref_tmp_pass);
		    read_options.set_ref_crypto_size(param.crypto_size_ref);
		    read_options.set_ref_execute(param.execute_ref);
		    read_options.set_ref_slice_min_digits(param.ref_num_digits);
		    if(ref_repo)
			read_options.set_ref_entrepot(ref_repo);
		    read_options.set_force_first_slice(param.force_first_slice);
		}
		arch.reset(new (nothrow) archive(dialog,
						 *param.sauv_root,
						 param.filename,
						 EXTENSION,
						 read_options));
		if(!arch)
		    throw Ememory("little_main");

		test_options.clear();
		test_options.set_selection(*param.selection);
		test_options.set_subtree(*param.subtree);
		test_options.set_info_details(param.info_details);
		test_options.set_display_treated(param.display_treated,
						 param.display_treated_only_dir);
		test_options.set_display_skipped(param.display_skipped);
		test_options.set_empty(param.empty);
                st = arch->op_test(test_options, nullptr);
		if(!param.quiet)
		    display_test_stat(*dialog, st);
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
		    read_options.set_crypto_algo(crypto_algo::none);
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
		read_options.set_multi_threaded_crypto(param.multi_threaded_crypto);
		read_options.set_multi_threaded_compress(param.multi_threaded_compress);
		if(repo)
		    read_options.set_entrepot(repo);
		read_options.set_header_only(param.header_only);
		if(param.sequential_read)
		    read_options.set_early_memory_release(true);
		read_options.set_silent(param.quiet_crypto);

		arch.reset(new (nothrow) archive(dialog,
						 *param.sauv_root,
						 param.filename,
						 EXTENSION,
						 read_options));
		if(!arch)
		    throw Ememory("little_main");

		if(param.quiet)
		{
		    const list<signator> & gnupg_signed = arch->get_signatories();
		    arch->summary();
		    line_tools_display_signatories(*dialog, gnupg_signed);
		}
		else
		{
		    if(param.info_details)
		    {
			const list<signator> & gnupg_signed = arch->get_signatories();
			arch->summary();
			line_tools_display_signatories(*dialog, gnupg_signed);
			dialog->pause(gettext("Continue listing archive contents?"));
		    }
		    listing_options.clear();
		    listing_options.set_info_details(param.info_details);
		    listing_options.set_list_mode(param.list_mode);
		    listing_options.set_selection(*param.selection);
		    listing_options.set_subtree(*param.subtree);
		    listing_options.set_filter_unsaved(param.filter_unsaved);
		    listing_options.set_display_ea(param.list_ea);
		    if(!param.file_size.is_zero() && param.list_mode == archive_options_listing_shell::slicing)
			listing_options.set_user_slicing(param.first_file_size, param.file_size);
		    listing_options.set_sizes_in_bytes(param.sizes_in_bytes);
		    shelli->archive_show_contents(*arch, listing_options);
		}
                break;
            default:
                throw SRC_BUG;
            }
	}
	catch(...)
	{
	    if(!param.quiet)
		dialog->message(gettext("Final memory cleanup..."));
	    throw;
	}

	if(param.info_details)
	    dialog->message(gettext("Final memory cleanup..."));

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
    infinint inode_only = st.get_inode_only();
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
    dialog.printf(gettext(" %i inode(s) with only metadata changed\n"), &inode_only);
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
