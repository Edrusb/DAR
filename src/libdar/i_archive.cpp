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

extern "C"
{
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // extern "C"

#include "infinint.hpp"
#include "generic_file.hpp"
#include "archive.hpp"
#include "sar.hpp"
#include "macro_tools.hpp"
#include "filtre.hpp"
#include "sar.hpp"
#include "trivial_sar.hpp"
#include "tools.hpp"
#include "header.hpp"
#include "header_version.hpp"
#include "scrambler.hpp"
#include "null_file.hpp"
#include "crypto.hpp"
#include "elastic.hpp"
#include "terminateur.hpp"
#include "thread_cancellation.hpp"
#include "erreurs_ext.hpp"
#include "cache.hpp"
#include "entrepot.hpp"
#include "entrepot_local.hpp"
#include "crypto_sym.hpp"
#include "cat_all_entrees.hpp"
#include "zapette.hpp"
#include "path.hpp"
#include "defile.hpp"
#include "escape.hpp"
#include "escape_catalogue.hpp"
#include "nls_swap.hpp"
#include "i_archive.hpp"
#include "capabilities.hpp"

#define ARCHIVE_NOT_EXPLOITABLE "Archive of reference given is not exploitable"

using namespace std;

namespace libdar
{
	/// checks file is not dirty when reading in sequential mode
    static bool local_check_dirty_seq(escape *ptr);

    static void check_libgcrypt_hash_bug(user_interaction & dialog,
					 hash_algo hash,
					 const infinint & first_file_size,
					 const infinint & file_size);

	// opens an already existing archive

    archive::i_archive::i_archive(const std::shared_ptr<user_interaction> & dialog,
				  const path & chem,
				  const string & basename,
				  const string & extension,
				  const archive_options_read & options): mem_ui(dialog)
    {
	shared_ptr<entrepot> where = options.get_entrepot();
	bool info_details = options.get_info_details();

	if(where == nullptr)
	    throw Ememory("archive::i_archive::archive");

	cat = nullptr;

	try
	{
	    infinint second_terminateur_offset = 0;
	    infinint ref_second_terminateur_offset = 0;
	    header_version ref_ver;
	    pile_descriptor pdesc;
	    list<signator> tmp1_signatories;
	    list<signator> tmp2_signatories;

	    lax_read_mode = options.get_lax();
	    sequential_read = options.get_sequential_read(); // updating the archive object's field
	    where->set_location(chem);

	    try
	    {
		if(info_details)
		    dialog->printf(gettext("Opening archive %s ..."), basename.c_str());

		    // we open the main archive to get the different layers (level1, scram and level2).
		macro_tools_open_archive(get_pointer(),
					 where,
					 basename,
					 options.get_slice_min_digits(),
					 extension,
					 options.get_crypto_algo(),
					 options.get_crypto_pass(),
					 options.get_crypto_size(),
					 stack,
					 ver,
					 options.get_input_pipe(),
					 options.get_output_pipe(),
					 options.get_execute(),
					 second_terminateur_offset,
					 options.get_lax(),
					 options.is_external_catalogue_set(),
					 options.get_sequential_read(),
					 options.get_info_details(),
					 gnupg_signed,
					 slices,
					 options.get_multi_threaded_crypto(),
					 options.get_multi_threaded_compress(),
					 options.get_header_only(),
					 options.get_silent(),
					 options.get_force_first_slice());

		if(options.get_header_only())
		{
		    ver.display(get_ui());
		    throw Erange("archive::i_archive::achive",
				 gettext("header only mode asked"));
		}

		pdesc = pile_descriptor(&stack);

		if(options.is_external_catalogue_set())
		{
		    pile ref_stack;
		    shared_ptr<entrepot> ref_where = options.get_ref_entrepot();
		    if(ref_where == nullptr)
			throw Ememory("archive::i_archive::archive");

		    if(info_details)
			dialog->printf(gettext("Opening the archive of reference %s to retreive the isolated catalog ... "), options.get_ref_basename().c_str());


		    try
		    {
			ref_where->set_location(options.get_ref_path());
			try
			{
			    slice_layout ignored;

			    if(options.get_ref_basename() == "-")
				throw Erange("archive::i_archive::archive", gettext("Reading the archive of reference from pipe or standard input is not possible"));
			    if(options.get_ref_basename() == "+")
				throw Erange("archive::i_archive::archive", gettext("The basename '+' is reserved for special a purpose that has no meaning in this context"));

				// we open the archive of reference also to get its different layers (ref_stack)
			    macro_tools_open_archive(get_pointer(),
						     ref_where,
						     options.get_ref_basename(),
						     options.get_ref_slice_min_digits(),
						     extension,
						     options.get_ref_crypto_algo(),
						     options.get_ref_crypto_pass(),
						     options.get_ref_crypto_size(),
						     ref_stack,
						     ref_ver,
						     "",
						     "",
						     options.get_ref_execute(),
						     ref_second_terminateur_offset,
						     options.get_lax(),
						     false, // has an external catalogue
						     false, // sequential_read is never used to retreive the isolated catalogue (well, that's possible and easy to add this feature), see later ...
						     options.get_info_details(),
						     tmp1_signatories,
						     ignored,
						     options.get_multi_threaded_crypto(),
						     options.get_multi_threaded_compress(),
						     false,
						     options.get_silent(),
						     false);
				// we do not compare the signatories of the archive of reference with the current archive
				// for example the isolated catalogue might be unencrypted and thus not signed

			}
			catch(Euser_abort & e)
			{
			    throw;
			}
			catch(Ebug & e)
			{
			    throw;
			}
			catch(Ethread_cancel & e)
			{
			    throw;
			}
			catch(Egeneric & e)
			{
			    throw Erange("archive::i_archive::archive", string(gettext("Error while opening the archive of reference: ")) + e.get_message());
			}
		    }
		    catch(...)
		    {
			ref_where.reset();
			throw;
		    }
		    ref_where.reset();

			// fetching the catalogue in the archive of reference, making it pointing on the main archive layers.

		    ref_ver.set_compression_algo(ver.get_compression_algo());
			// set the default compression to use to the one of the main archive

		    if(info_details)
			dialog->message(gettext("Loading isolated catalogue in memory..."));

		    cat = macro_tools_get_derivated_catalogue_from(dialog,
								   stack,
								   ref_stack,
								   ref_ver,
								   options.get_info_details(),
								   local_cat_size,
								   ref_second_terminateur_offset,
								   tmp2_signatories,
								   false); // never relaxed checking for external catalogue
		    if(!same_signatories(tmp1_signatories, tmp2_signatories))
			dialog->pause(gettext("Archive of reference is not signed properly (not the same signatories for the archive and the internal catalogue), do we continue?"));
		    if(cat == nullptr)
			throw SRC_BUG;

			// checking for compatibility of the archive of reference with this archive data_name

		    if(get_layer1_data_name() != get_catalogue_data_name())
			throw Erange("archive::i_archive::archive", gettext("The archive and the isolated catalogue do not correspond to the same data, they are thus incompatible between them"));

			// we drop delta signature as they refer to the isolated catalogue container/archive
			// to avoid having to fetch them at wrong offset from the original archive we created
			// this isolated catalogue from. Anyway we do not need them to read an archive, we
			// only need delta signatures for archive differential backup, in which case we use the
			// original archive *or* the isolated catalogue *alone*
		    cat->drop_delta_signatures();

			// we must set the catalogue to know we are using sequential
			// reading mode, as if the catalogue has been here read in direct
			// mode the data and EA have to be read sequentially, and the
			// way to know how to read data from the cat_file and cat_inode
			// objects point of view it owns is to consult the get_escape_layer()
			// method of catalogue class
		    if(sequential_read)
		    {
			if(pdesc.esc == nullptr)
			    throw SRC_BUG;
			cat->set_escape_layer(pdesc.esc);
		    }
		}
		else // no isolated archive to fetch the catalogue from
		{
		    try
		    {
			if(!options.get_sequential_read())
			{
			    if(info_details)
				dialog->message(gettext("Loading catalogue into memory..."));
			    cat = macro_tools_get_catalogue_from(dialog,
								 stack,
								 ver,
								 options.get_info_details(),
								 local_cat_size,
								 second_terminateur_offset,
								 tmp1_signatories,
								 options.get_lax());
			    if(!same_signatories(tmp1_signatories, gnupg_signed))
			    {
				string msg = gettext("Archive internal catalogue is not identically signed as the archive itself, this might be the sign the archive has been compromised");
				if(lax_read_mode)
				    dialog->pause(msg);
				else
				    throw Edata(msg);
			    }
			}
			else // sequentially reading
			{
			    if(pdesc.esc != nullptr) // escape layer is present
			    {
				if(pdesc.esc->skip_to_next_mark(escape::seqt_catalogue, false))
				{
				    if(info_details)
					dialog->message(gettext("No data found in that archive, sequentially reading the catalogue found at the end of the archive..."));
				    pdesc.stack->flush_read_above(pdesc.esc);

				    contextual *layer1 = nullptr;
				    label lab = label_zero;
				    stack.find_first_from_bottom(layer1);
				    if(layer1 != nullptr)
					lab = layer1->get_data_name();

				    cat = macro_tools_read_catalogue(dialog,
								     ver,
								     pdesc,
								     0, // cannot determine cat_size at this stage
								     tmp1_signatories,
								     options.get_lax(),
								     lab,
								     false); // only_detruits

				    if(!same_signatories(tmp1_signatories, gnupg_signed))
				    {
					string msg = gettext("Archive internal catalogue is not identically signed as the archive itself, this might be the sign the archive has been compromised");
					if(lax_read_mode)
					    dialog->pause(msg);
					else
					    throw Edata(msg);
				    }
				}
				else
				{
				    if(info_details)
					dialog->message(gettext("The catalogue will be filled while sequentially reading the archive, preparing the data structure..."));
				    cat = new (nothrow) escape_catalogue(dialog,
									 pdesc,
									 ver,
									 gnupg_signed,
									 options.get_lax());
				}
				if(cat == nullptr)
				    throw Ememory("archive::i_archive::archive");
			    }
			    else
				throw SRC_BUG;
			}
		    }
		    catch(Ebug & e)
		    {
			throw;
		    }
		    catch(Ethread_cancel & e)
		    {
			throw;
		    }
		    catch(Euser_abort & e)
		    {
			throw;
		    }
		    catch(Ememory & e)
		    {
			throw;
		    }
		    catch(...)
		    {
			if(!options.get_lax())
			    throw;
			else // we have tried and failed to read the whole catalogue, now trying to workaround data corruption if possible
			{
			    if(options.get_sequential_read())
				throw;
			    else // legacy extraction of the catalogue (not sequential mode)
			    {
				dialog->printf(gettext("LAX MODE: The end of the archive is corrupted, cannot get the archive contents (the \"catalogue\")"));
				dialog->pause(gettext("LAX MODE: Do you want to bypass some sanity checks and try again reading the archive contents (this may take some time, this may also fail)?"));
				try
				{
				    label tmp;
				    tmp.clear(); // this way we do not modify the catalogue data name even in lax mode
				    cat = macro_tools_lax_search_catalogue(dialog,
									   stack,
									   ver.get_edition(),
									   ver.get_compression_algo(),
									   options.get_info_details(),
									   false, // even partial
									   tmp);
				}
				catch(Erange & e)
				{
				    dialog->printf(gettext("LAX MODE: Could not find a whole catalogue in the archive. If you have an isolated catalogue, stop here and use it as backup of the internal catalogue, else continue but be advised that all data will not be able to be retrieved..."));
				    dialog->pause(gettext("LAX MODE: Do you want to try finding portions of the original catalogue if some remain (this may take even more time and in any case, it will only permit to recover some files, at most)?"));
				    cat = macro_tools_lax_search_catalogue(dialog,
									   stack,
									   ver.get_edition(),
									   ver.get_compression_algo(),
									   options.get_info_details(),
									   true,                     // even partial
									   get_layer1_data_name());
				}
			    }
			}
		    }
		}

		if(!options.get_ignore_signature_check_failure())
		    check_gnupg_signed();
		exploitable = true;

		if(options.get_early_memory_release())
		    cat->set_early_memory_release();
	    }
	    catch(...)
	    {
		where.reset();
		throw;
	    }
	    where.reset();
	}
	catch(...)
	{
	    free_mem();
	    throw;
	}
    }

	// creates a new archive

    archive::i_archive::i_archive(const std::shared_ptr<user_interaction> & dialog,
				  const path & fs_root,
				  const path & sauv_path,
				  const string & filename,
				  const string & extension,
				  const archive_options_create & options,
				  statistics * progressive_report): mem_ui(dialog)
    {
        NLS_SWAP_IN;
        try
        {
	    cat = nullptr;

	    shared_ptr<entrepot> sauv_path_t = options.get_entrepot();
	    if(!sauv_path_t)
		throw Ememory("archive::i_archive::archive");
	    sauv_path_t->set_user_ownership(options.get_slice_user_ownership());
	    sauv_path_t->set_group_ownership(options.get_slice_group_ownership());
	    sauv_path_t->set_location(sauv_path);

	    filesystem_ids same_fs(options.get_same_fs(), fs_root);
	    deque<string> same_fs_inc = options.get_same_fs_include();
	    deque<string> same_fs_exc = options.get_same_fs_exclude();
	    if(!same_fs_inc.empty() || !same_fs_exc.empty())
	    {
		deque<string>::iterator it = same_fs_inc.begin();
		same_fs.clear();

		while(it != same_fs_inc.end())
		{
		    same_fs.include_fs_at(*it);
		    ++it;
		}

		it = same_fs_exc.begin();
		while(it != same_fs_exc.end())
		{
		    same_fs.exclude_fs_at(*it);
		    ++it;
		}
	    }

	    if(options.get_reference().get() != nullptr
	       && options.get_reference().get()->pimpl != nullptr
	       && options.get_reference().get()->pimpl->cat != nullptr
	       && options.get_reference().get()->pimpl->cat->get_early_memory_release())
		throw Erange("i_archive::i_archive (create)", gettext("Early memory release is not possible for a backup of reference"));

	    try
	    {
		sequential_read = false; // updating the archive field
		(void)op_create_in(oper_create,
				   tools_relative2absolute_path(fs_root, tools_getcwd()),
				   sauv_path_t,
				   options.get_reference().get(),
				   options.get_selection(),
				   options.get_subtree(),
				   filename,
				   extension,
				   options.get_allow_over(),
				   options.get_warn_over(),
				   options.get_info_details(),
				   options.get_display_treated(),
				   options.get_display_treated_only_dir(),
				   options.get_display_skipped(),
				   options.get_display_finished(),
				   options.get_pause(),
				   options.get_empty_dir(),
				   options.get_compression(),
				   options.get_compression_level(),
				   options.get_compression_block_size(),
				   options.get_slice_size(),
				   options.get_first_slice_size(),
				   options.get_ea_mask(),
				   options.get_execute(),
				   options.get_crypto_algo(),
				   options.get_crypto_pass(),
				   options.get_crypto_size(),
				   options.get_gnupg_recipients(),
				   options.get_gnupg_signatories(),
				   options.get_compr_mask(),
				   options.get_min_compr_size(),
				   options.get_nodump(),
				   options.get_exclude_by_ea(),
				   options.get_hourshift(),
				   options.get_empty(),
				   options.get_alter_atime(),
				   options.get_furtive_read_mode(),
				   same_fs,
				   options.get_comparison_fields(),
				   options.get_snapshot(),
				   options.get_cache_directory_tagging(),
				   options.get_fixed_date(),
				   options.get_slice_permission(),
				   options.get_repeat_count(),
				   options.get_repeat_byte(),
				   options.get_sequential_marks(),
				   options.get_security_check(),
				   options.get_sparse_file_min_size(),
				   options.get_user_comment(),
				   options.get_hash_algo(),
				   options.get_slice_min_digits(),
				   options.get_backup_hook_file_execute(),
				   options.get_backup_hook_file_mask(),
				   options.get_ignore_unknown_inode_type(),
				   options.get_fsa_scope(),
				   options.get_multi_threaded_crypto(),
				   options.get_multi_threaded_compress(),
				   options.get_delta_signature(),
				   options.get_has_delta_mask_been_set(),
				   options.get_delta_mask(),
				   options.get_delta_sig_min_size(),
				   options.get_delta_diff(),
				   options.get_auto_zeroing_neg_dates(),
				   options.get_ignored_as_symlink(),
				   options.get_modified_data_detection(),
				   options.get_iteration_count(),
				   options.get_kdf_hash(),
				   options.get_sig_block_len(),
				   options.get_never_resave_uncompressed(),
				   progressive_report);
		exploitable = false;
		stack.terminate();
	    }
	    catch(...)
	    {
		sauv_path_t.reset();
		throw;
	    }
	    sauv_path_t.reset();
	}
	catch(...)
	{
	    free_mem();
	    throw;
	}
    }


	// merge constructor

    archive::i_archive::i_archive(const std::shared_ptr<user_interaction> & dialog,
				  const path & sauv_path,
				  shared_ptr<archive> ref_arch1,
				  const string & filename,
				  const string & extension,
				  const archive_options_merge & options,
				  statistics * progressive_report): mem_ui(dialog)
    {
	statistics st = false;
	statistics *st_ptr = progressive_report == nullptr ? &st : progressive_report;
	catalogue *ref_cat1 = nullptr;
	catalogue *ref_cat2 = nullptr;
	shared_ptr<archive> ref_arch2 = options.get_auxiliary_ref();
	compression algo_kept = compression::none;
	U_I comp_bs_kept = 0;
	infinint i_comp_bs_kept;
	shared_ptr<entrepot> sauv_path_t = options.get_entrepot();

	cat = nullptr;

	try
	{
	    if(sauv_path_t == nullptr)
		throw Ememory("archive::i_archive::archive(merge)");
	    sauv_path_t->set_user_ownership(options.get_slice_user_ownership());
	    sauv_path_t->set_group_ownership(options.get_slice_group_ownership());
	    sauv_path_t->set_location(sauv_path);

	    try
	    {
		exploitable = false;
		sequential_read = false; // updating the archive field

		    // sanity checks as much as possible to avoid libdar crashing due to bad arguments
		    // useless arguments are not reported.

		if(options.get_slice_size().is_zero() && !options.get_first_slice_size().is_zero())
		    throw Elibcall("op_merge", gettext("\"first_file_size\" cannot be different from zero if \"file_size\" is equal to zero"));
		if(options.get_crypto_size() < 10 && options.get_crypto_algo() != crypto_algo::none)
		    throw Elibcall("op_merge", gettext("Crypto block size must be greater than 10 bytes"));

		check_libgcrypt_hash_bug(get_ui(), options.get_hash_algo(), options.get_first_slice_size(), options.get_slice_size());

		if(ref_arch1)
		    if(ref_arch1->pimpl->only_contains_an_isolated_catalogue())
			    // convert all data to unsaved
			ref_arch1->set_to_unsaved_data_and_FSA();

		if(ref_arch2)
		    if(ref_arch2->pimpl->only_contains_an_isolated_catalogue())
			ref_arch2->set_to_unsaved_data_and_FSA();

		    // end of sanity checks

		sauv_path_t->set_location(sauv_path);

		tools_avoid_slice_overwriting_regex(get_ui(),
						    *sauv_path_t,
						    filename,
						    extension,
						    options.get_info_details(),
						    options.get_allow_over(),
						    options.get_warn_over(),
						    options.get_empty());

		if(ref_arch1 == nullptr)
		    if(!ref_arch2)
			throw Elibcall("archive::i_archive::archive[merge]", string(gettext("Both reference archive are nullptr, cannot merge archive from nothing")));
		    else
			if(ref_arch2->pimpl->cat == nullptr)
			    throw SRC_BUG; // an archive should always have a catalogue available
			else
			    if(ref_arch2->pimpl->exploitable)
				ref_cat1 = ref_arch2->pimpl->cat;
			    else
				throw Elibcall("archive::i_archive::archive[merge]", gettext(ARCHIVE_NOT_EXPLOITABLE));
		else
		    if(!ref_arch2)
			if(ref_arch1->pimpl->cat == nullptr)
			    throw SRC_BUG; // an archive should always have a catalogue available
			else
			    if(ref_arch1->pimpl->exploitable)
				ref_cat1 = ref_arch1->pimpl->cat;
			    else
				throw Elibcall("archive::i_archive::archive[merge]", gettext(ARCHIVE_NOT_EXPLOITABLE));
		    else // both catalogues available
		    {
			if(!ref_arch1->pimpl->exploitable || !ref_arch2->pimpl->exploitable)
			    throw Elibcall("archive::i_archive::archive[merge]", gettext(ARCHIVE_NOT_EXPLOITABLE));
			if(ref_arch1->pimpl->cat == nullptr)
			    throw SRC_BUG;
			if(ref_arch2->pimpl->cat == nullptr)
			    throw SRC_BUG;
			ref_cat1 = ref_arch1->pimpl->cat;
			ref_cat2 = ref_arch2->pimpl->cat;
			if((ref_arch1->pimpl->ver.get_compression_algo() != ref_arch2->pimpl->ver.get_compression_algo()
			    || ref_arch1->pimpl->ver.get_compression_block_size() != ref_arch2->pimpl->ver.get_compression_block_size())
			   && ref_arch1->pimpl->ver.get_compression_algo() != compression::none
			   && ref_arch2->pimpl->ver.get_compression_algo() != compression::none
			   && options.get_keep_compressed())
			    throw Efeature(gettext("the \"Keep file compressed\" feature is not possible when merging two archives using different compression algorithms (This is for a future version of dar). You can still merge these two archives but without keeping file compressed (thus you will probably like to use compression (-z or -y options) for the resulting archive"));
		    }

		if(options.get_keep_compressed())
		{
		    if(ref_arch1 == nullptr)
			throw SRC_BUG;

		    algo_kept = ref_arch1->pimpl->ver.get_compression_algo();
		    i_comp_bs_kept = ref_arch1->pimpl->ver.get_compression_block_size();
		    if(algo_kept == compression::none && ref_cat2 != nullptr)
		    {
			if(!ref_arch2)
			    throw SRC_BUG;
			else
			{
			    algo_kept = ref_arch2->pimpl->ver.get_compression_algo();
			    i_comp_bs_kept = ref_arch2->pimpl->ver.get_compression_block_size();
			}
		    }

		    comp_bs_kept = 0;
		    i_comp_bs_kept.unstack(comp_bs_kept);
		    if(!i_comp_bs_kept.is_zero())
			throw Erange("archive::i_archive::i_archive(merge)", gettext("compression block size used in the archive exceed integer capacity of the current system"));
		}

		if(ref_cat1 == nullptr)
		    throw SRC_BUG;

		if(ref_cat1->get_early_memory_release())
		    throw Erange("i_archive::i_archive (merge)", gettext("Early memory release is not compatible with the merging operation"));

		if(ref_cat2 != nullptr && ref_cat2->get_early_memory_release())
		    throw Erange("i_archive::i_archive (merge)", gettext("Early memory release is not compatible with the merging operation"));

		if(options.get_delta_signature())
		{
		    if(options.get_keep_compressed() && options.get_has_delta_mask_been_set())
			throw Elibcall("op_merge", gettext("Cannot calculate delta signature when merging if keep compressed is asked"));
		    if(options.get_sparse_file_min_size().is_zero() && options.get_has_delta_mask_been_set())
			dialog->message(gettext("To calculate delta signatures of files saved as sparse files, you need to activate sparse file detection mechanism with merging operation"));
		}

		    // then we call op_create_in_sub which will call filter_merge operation to build the archive described by the catalogue
		op_create_in_sub(oper_merge,
				 FAKE_ROOT,
				 sauv_path_t,
				 ref_cat1,
				 ref_cat2,
				 false,  // initial_pause
				 options.get_selection(),
				 options.get_subtree(),
				 filename,
				 extension,
				 options.get_allow_over(),
				 options.get_overwriting_rules(),
				 options.get_warn_over(),
				 options.get_info_details(),
				 options.get_display_treated(),
				 options.get_display_treated_only_dir(),
				 options.get_display_skipped(),
				 false, // display_finished
				 options.get_pause(),
				 options.get_empty_dir(),
				 options.get_keep_compressed() ? algo_kept : options.get_compression(),
				 options.get_compression_level(),
				 options.get_keep_compressed() ? comp_bs_kept : options.get_compression_block_size(),
				 options.get_slice_size(),
				 options.get_first_slice_size(),
				 options.get_ea_mask(),
				 options.get_execute(),
				 options.get_crypto_algo(),
				 options.get_crypto_pass(),
				 options.get_crypto_size(),
				 options.get_gnupg_recipients(),
				 options.get_gnupg_signatories(),
				 options.get_compr_mask(),
				 options.get_min_compr_size(),
				 false,   // nodump
				 "",      // exclude_by_ea
				 0,       // hourshift
				 options.get_empty(),
				 true,    // alter_atime
				 false,   // furtive_read_mode
				 filesystem_ids(false, path("/")),    // same_fs
				 comparison_fields::all,        // what_to_check
				 false,   // snapshot
				 false,   // cache_directory_tagging
				 options.get_keep_compressed(),
				 0,       // fixed_date
				 options.get_slice_permission(),
				 0,       // repeat_count
				 0,       // repeat_byte
				 options.get_decremental_mode(),
				 options.get_sequential_marks(),
				 false,   // security_check
				 options.get_sparse_file_min_size(),
				 options.get_user_comment(),
				 options.get_hash_algo(),
				 options.get_slice_min_digits(),
				 "",      // backup_hook_file_execute
				 bool_mask(false),         // backup_hook_file_mask
				 false,   // ignore_unknown
				 options.get_fsa_scope(),
				 options.get_multi_threaded_crypto(),
				 options.get_multi_threaded_compress(),
				 options.get_delta_signature(),
				 options.get_has_delta_mask_been_set(), // build delta sig
				 options.get_delta_mask(), // delta_mask
				 options.get_delta_sig_min_size(),
				 false,   // delta diff
				 true,    // zeroing_neg_date
				 set<string>(),            // empty list
				 modified_data_detection::any_inode_change, // not used for merging
				 options.get_iteration_count(),
				 options.get_kdf_hash(),
				 options.get_sig_block_len(),
				 options.get_never_resave_uncompressed(),
				 st_ptr);

		exploitable = false;
		stack.terminate();
	    }
	    catch(...)
	    {
		sauv_path_t.reset();
		throw;
	    }
	    sauv_path_t.reset();
	}
	catch(...)
	{
	    free_mem();
	    throw;
	}
    }

	// reparation constructor

    archive::i_archive::i_archive(const std::shared_ptr<user_interaction> & dialog,
				  const path & chem_src,
				  const string & basename_src,
				  const string & extension_src,
				  const archive_options_read & options_read,
				  const path & chem_dst,
				  const string & basename_dst,
				  const string & extension_dst,
				  const archive_options_repair & options_repair,
				  statistics* progressive_report): mem_ui(dialog)
    {
	statistics st = false;
	statistics* st_ptr = progressive_report == nullptr ? &st : progressive_report;
	archive_options_read my_options_read = options_read;
	bool initial_pause = (*options_read.get_entrepot() == *options_repair.get_entrepot() && chem_src == chem_dst);
	infinint compr_bs;
	U_I compr_bs_ui;

	    ////
	    // initializing object fields

	    // stack will be set by op_create_in_sub()
	    // ver will be set by op_create_in_sub()
	cat = nullptr; // will be set by op_create_in_sub()
	exploitable = false;
	lax_read_mode = false;
	sequential_read = false;
	    // gnupg_signed is not used while creating an archive
	    // slices will be set by op_create_in_sub()
	    // local_cat_size not used while creating an archive

	    ////
	    // initial parameter setup

	shared_ptr<entrepot> sauv_path_t = options_repair.get_entrepot();
	if(sauv_path_t == nullptr)
	    throw Ememory("archive::i_archive::archive(repair)");

	try
	{
	    sauv_path_t->set_user_ownership(options_repair.get_slice_user_ownership());
	    sauv_path_t->set_group_ownership(options_repair.get_slice_group_ownership());
	    sauv_path_t->set_location(chem_dst);

	    tools_avoid_slice_overwriting_regex(get_ui(),
						*sauv_path_t,
						basename_dst,
						extension_dst,
						options_repair.get_info_details(),
						options_repair.get_allow_over(),
						options_repair.get_warn_over(),
						options_repair.get_empty());

		/////
		// opening the source archive in sequential read mode

	    my_options_read.set_sequential_read(true);

	    archive src(dialog,
			chem_src,
			basename_src,
			extension_src,
			my_options_read);

	    if(src.pimpl->cat == nullptr)
		throw SRC_BUG;

	    compr_bs_ui = 0;
	    compr_bs = src.pimpl->ver.get_compression_block_size();
	    compr_bs.unstack(compr_bs_ui);
	    if(! compr_bs.is_zero())
		throw Erange("macro_tools_open_layers", gettext("compression block size used in the archive exceed integer capacity of the current system"));

	    op_create_in_sub(oper_repair,
			     FAKE_ROOT,            // fs_root must be
			     sauv_path_t,
			     src.pimpl->cat,      // ref1
			     nullptr,             // ref2
			     initial_pause,
			     bool_mask(true),     // selection
			     bool_mask(true),     // subtree
			     basename_dst,
			     extension_dst,
			     options_repair.get_allow_over(),
			     crit_constant_action(data_preserve, EA_preserve), // overwrite
			     options_repair.get_warn_over(),
			     options_repair.get_info_details(),
			     options_repair.get_display_treated(),
			     options_repair.get_display_treated_only_dir(),
			     options_repair.get_display_skipped(),
			     options_repair.get_display_finished(),
			     options_repair.get_pause(),
			     false,               // empty_dir
			     src.pimpl->ver.get_compression_algo(),
			     9,                   // we keep the data compressed this parameter has no importance
			     compr_bs_ui,
			     options_repair.get_slice_size(),
			     options_repair.get_first_slice_size(),
			     bool_mask(true),     // ea_mask
			     options_repair.get_execute(),
			     options_repair.get_crypto_algo(),
			     options_repair.get_crypto_pass(),
			     options_repair.get_crypto_size(),
			     options_repair.get_gnupg_recipients(),
			     options_repair.get_gnupg_signatories(),
			     bool_mask(true),     // compr_mask
			     0,                   // min_compr_size
			     false,               // nodump
			     "",                  // exclude_by_ea
			     0,                   // hourshift
			     options_repair.get_empty(),
			     false,               // alter_atime
			     false,               // furtive_read_mode
			     filesystem_ids(false, path("/")), // same_fs
			     comparison_fields::all,   // comparison_fields
			     false,               // snapshot
			     false,               // cache_directory_tagging,
			     true,                // keep_compressed, always
			     0,                   // fixed_date
			     options_repair.get_slice_permission(),
			     0,                   // repeat_count
			     0,                   // repeat_byte
			     false,               // decremental
			     true,                // tape marks are mandatory in repair mode
			     false,               // security_check
			     0,                   // sparse_file_min_size (disabling hole detection)
			     options_repair.get_user_comment(),
			     options_repair.get_hash_algo(),
			     options_repair.get_slice_min_digits(),
			     "",                  // backup_hook_file_execute
			     bool_mask(true),     // backup_hook_file_mask
			     false,               // ignore_unknown
			     all_fsa_families(),  // fsa_scope
			     options_repair.get_multi_threaded_crypto(),
			     options_repair.get_multi_threaded_compress(),
			     true,                // delta_signature
			     false,               // build_delta_signature
			     bool_mask(true),     // delta_mask
			     0,                   // delta_sig_min_size
			     false,               // delta_diff
			     false,               // zeroing_neg_date
			     set<string>(),       // ignored_symlinks
			     modified_data_detection::any_inode_change, // not used for repairing
			     options_repair.get_iteration_count(),
			     options_repair.get_kdf_hash(),
			     delta_sig_block_size(), // sig block size is not used for repairing, build_delta_sig is set to false above
			     false,               // (never_resave_uncompressed) this has not importance as we keep data compressed
			     st_ptr);             // statistics

		// stealing src's catalogue, our's is still empty at this step
	    catalogue *tmp = cat;
	    cat = src.pimpl->cat;
	    src.pimpl->cat = tmp;

	    dialog->printf(gettext("Archive repairing completed. WARNING! it is strongly advised to test the resulting archive before removing the damaged one"));
	}
	catch(...)
	{
	    if(!sauv_path_t)
		sauv_path_t.reset();
	    throw;
	}
	if(!sauv_path_t)
	    sauv_path_t.reset();
    }


    statistics archive::i_archive::op_extract(const path & fs_root,
					      const archive_options_extract & options,
					      statistics * progressive_report)
    {
        statistics st = false;  // false => no lock for this internal object
	statistics *st_ptr = progressive_report == nullptr ? &st : progressive_report;
	comparison_fields wtc = options.get_what_to_check();
	path real_fs_root(".");

        try
        {

                // sanity checks

	    if(!exploitable)
                throw Elibcall("op_extract", gettext("This archive is not exploitable, check documentation for more"));

	    check_against_isolation(lax_read_mode);
		// this avoid to try extracting archive directly from an isolated catalogue
		// the isolated catalogue can be used to extract data (since format "08") but only
		// associated with the real plain archive, not alone.

		// end of sanity checks

	    if(wtc == comparison_fields::all
	       && capability_CHOWN(get_ui(), options.get_info_details()) == capa_clear
	       && getuid() != 0)
	    {
		wtc = comparison_fields::ignore_owner;
		get_ui().pause(gettext("File ownership will not be restored du to the lack of privilege, you can disable this message by asking not to restore file ownership"));
	    }

	    enable_natural_destruction();

	    if(options.get_in_place())
	    {
		if(!get_cat().get_in_place(real_fs_root))
		    throw Erange("op_extract", gettext("Cannot use in-place restoration as no in-place path is stored in the archive"));
	    }
	    else
		real_fs_root = fs_root;


		/// calculating and setting the " recursive_has_changed" fields of directories to their values
	    if(options.get_empty_dir() == false)
		get_cat().launch_recursive_has_changed_update();
		/// we can now use the cat_directory::get_recursive_has_changed() to avoid recursion in a directory where
		/// no file has been saved.

	    try
	    {
		filtre_restore(get_pointer(),
			       options.get_selection(),
			       options.get_subtree(),
			       get_cat(),
			       tools_relative2absolute_path(real_fs_root, tools_getcwd()),
			       options.get_warn_over(),
			       options.get_info_details(),
			       options.get_display_treated(),
			       options.get_display_treated_only_dir(),
			       options.get_display_skipped(),
			       *st_ptr,
			       options.get_ea_mask(),
			       options.get_flat(),
			       wtc,
			       options.get_warn_remove_no_match(),
			       options.get_empty(),
			       options.get_empty_dir(),
			       options.get_overwriting_rules(),
			       options.get_dirty_behavior(),
			       options.get_only_deleted(),
			       options.get_ignore_deleted(),
			       options.get_fsa_scope(),
			       options.get_ignore_unix_sockets());
	    }
	    catch(Euser_abort & e)
	    {
		disable_natural_destruction();
		throw;
	    }
	    catch(Ethread_cancel & e)
	    {
		disable_natural_destruction();
		throw;
	    }
	    catch(Erange &e)
	    {
		string msg = string(gettext("Error while restoring data: ")) + e.get_message();
		get_ui().message(msg);
		throw Edata(msg);
	    }
        }
        catch(...)
        {
	    if(sequential_read)
		exploitable = false;
            throw;
        }
	if(sequential_read)
	    exploitable = false;

        return *st_ptr;
    }

    void archive::i_archive::summary()
    {
        try
        {
	    archive_summary sum = summary_data();
	    infinint tmp;

	    get_header().display(get_ui());

	    tmp = sum.get_catalog_size();
	    if(!tmp.is_zero())
		get_ui().printf(gettext("Catalogue size in archive            : %i bytes"), &tmp);
	    else
		get_ui().printf(gettext("Catalogue size in archive            : N/A"));
	    get_ui().printf("");

	    tmp = sum.get_slice_number();
	    if(!tmp.is_zero())
	    {
		get_ui().printf(gettext("Archive is composed of %i file(s)"), &tmp);
		if(tmp == 1)
		{
		    tmp = sum.get_last_slice_size();
		    get_ui().printf(gettext("File size: %i bytes"), &tmp);
		}
		else
		{
		    infinint first = sum.get_first_slice_size();
		    infinint slice = sum.get_slice_size();
		    infinint last = sum.get_last_slice_size();
		    infinint total = sum.get_archive_size();
		    if(first != slice)
			get_ui().printf(gettext("First file size       : %i bytes"), &first);
		    get_ui().printf(gettext("File size             : %i bytes"), &slice);
		    get_ui().printf(gettext("Last file size        : %i bytes"), &last);
		    get_ui().printf(gettext("Archive total size is : %i bytes"), &total);
		}
	    }
	    else // not reading from a sar or in sequential-read mode
	    {
		tmp = sum.get_archive_size();
		if(!tmp.is_zero())
		{
		    get_ui().printf(gettext("Archive size is: %i bytes"), &tmp);
		    get_ui().printf(gettext("Previous archive size does not include headers present in each slice"));
		}
		else
		    get_ui().printf(gettext("Archive size is unknown (reading from a pipe or in sequential mode)"));
	    }

	    if(sum.get_data_size() < sum.get_storage_size())
	    {
		infinint delta = sum.get_storage_size() - sum.get_data_size();
		get_ui().printf(gettext("The overall archive size includes %i byte(s) wasted due to bad compression ratio"), &delta);
	    }
	    else
	    {
		if(!sum.get_storage_size().is_zero())
		    get_ui().message(string(gettext("The global data compression ratio is: "))
				     + tools_get_compression_ratio(sum.get_storage_size(),
								   sum.get_data_size(),
								   true));
	    }

	    if(only_contains_an_isolated_catalogue())
	    {
		infinint first = sum.get_ref_first_slice_size();
		infinint other = sum.get_ref_slice_size();

		get_ui().printf(gettext("\nWARNING! This archive only contains the catalogue of another archive, it can only be used as reference for differential backup or as rescue in case of corruption of the original archive's content. You cannot restore any data from this archive alone\n"));
		get_ui().printf("");
		get_ui().printf("Archive of reference slicing:");
		if(other.is_zero())
		    get_ui().printf(gettext("\tUnknown or no slicing"));
		else
		{
		    if(first != other && !first.is_zero())
			get_ui().printf(gettext("\tFirst slice : %i byte(s)"), &first);
		    get_ui().printf(gettext("\tOther slices: %i byte(s)"), &other);
		}
		get_ui().printf("");
	    }

	    string in_place = sum.get_in_place();
	    if(!in_place.empty())
		get_ui().printf(gettext("in-place path: %S"), & in_place);
	    else
		get_ui().message(gettext("no in-place path recorded"));

	    sum.get_contents().listing(get_ui());
	}
	catch(...)
	{
	    if(sequential_read)
		exploitable = false;
	    throw;
	}
	if(sequential_read)
	    exploitable = false;
    }

    archive_summary archive::i_archive::summary_data()
    {
	archive_summary ret;
	infinint sub_slice_size;
	infinint first_slice_size;
	infinint last_slice_size;
	infinint slice_number;
	infinint archive_size;
	slice_layout slyt;

	path tmp(".");

	    // sanity checks

	if(!exploitable)
	    throw Elibcall("summary", gettext("This archive is not exploitable, check the archive class usage in the API documentation"));

	    // end of sanity checks

	try
	{
	    if(!get_catalogue_slice_layout(slyt))
		slyt.clear();

	    init_catalogue();

	    if(get_sar_param(sub_slice_size, first_slice_size, last_slice_size, slice_number))
	    {
		if(slice_number <= 1)
		{
		    sub_slice_size = last_slice_size;
		    first_slice_size = last_slice_size;
		    archive_size = last_slice_size;
		}
		else
		    archive_size = first_slice_size + (slice_number - 2)*sub_slice_size + last_slice_size;
	    }
	    else // not reading from a sar
	    {
		archive_size = get_level2_size();
		(void)archive_size.is_zero();
		sub_slice_size = 0;
		first_slice_size = 0;
		last_slice_size = 0;
		slice_number = 0;
	    }
	}
	catch(Erange & e)
	{
	    get_ui().message(e.get_message());
	    sub_slice_size = 0;
	    first_slice_size = 0;
	    last_slice_size = 0;
	    slice_number = 0;
	    archive_size = 0;
	}

	ret.set_slice_size(sub_slice_size);
	ret.set_first_slice_size(first_slice_size);
	ret.set_last_slice_size(last_slice_size);
	ret.set_ref_slice_size(slyt.other_size);
	ret.set_ref_first_slice_size(slyt.first_size);
	ret.set_slice_number(slice_number);
	ret.set_archive_size(archive_size);

	ret.set_catalog_size(get_cat_size());
	ret.set_edition(get_header().get_edition().display());
	ret.set_compression_algo(compression2string(get_header().get_compression_algo()));
	ret.set_user_comment(get_header().get_command_line());
	ret.set_cipher(get_header().get_sym_crypto_name());
	ret.set_asym(get_header().get_asym_crypto_name());
	ret.set_signed(get_header().is_signed());
	ret.set_tape_marks(get_header().get_tape_marks());

	if(get_cat().get_contenu() == nullptr)
	    throw SRC_BUG;
	ret.set_storage_size(get_cat().get_contenu()->get_storage_size());
	ret.set_data_size(get_cat().get_contenu()->get_size());
	if(get_cat().get_in_place(tmp))
	{
	    if(tmp.is_relative())
		throw SRC_BUG;
	    ret.set_in_place(tmp.display());
	}
	else
	    ret.set_in_place(""); // empty string when in_place is not set
	ret.set_compression_block_size(get_header().get_compression_block_size());
	ret.set_salt(get_header().get_salt());
	ret.set_iteration_count(get_header().get_iteration_count());
	ret.set_kdf_hash(hash_algo_to_string(get_header().get_kdf_hash()));

	ret.set_contents(get_cat().get_stats());

	return ret;
    }


    void archive::i_archive::op_listing(archive_listing_callback callback,
					void *context,
					const archive_options_listing & options) const
    {
	i_archive *me = const_cast<i_archive *>(this);

	if(me == nullptr)
	    throw SRC_BUG;

	if(callback == nullptr)
	    throw Elibcall("archive::op_listing", "null pointer given as callback function for archive listing");

        try
        {
	    slice_layout used_layout;
	    thread_cancellation thr;

	    if(options.get_display_ea() && sequential_read)
		throw Erange("archive::i_archive::get_children_of", gettext("Fetching EA value while listing an archive is not possible in sequential read mode"));

	    if(options.get_slicing_location())  // -Tslice is asked
	    {
		if(!only_contains_an_isolated_catalogue()
		   && sequential_read)
		    throw Erange("archive::i_archive::op_listing", gettext("slicing focused output is not available in sequential-read mode"));

		if(!get_catalogue_slice_layout(used_layout))
		{
		    if(options.get_user_slicing(used_layout.first_size, used_layout.other_size))
		    {
			if(options.get_info_details())
			    get_ui().printf(gettext("Using user provided modified slicing (first slice = %i bytes, other slices = %i bytes)"), &used_layout.first_size, &used_layout.other_size);
		    }
		    else
			throw Erange("archive::i_archive::op_listing", gettext("No slice layout of the archive of reference for the current isolated catalogue is available, cannot provide slicing information, aborting"));
		}
	    }

	    if(options.get_filter_unsaved())
		get_cat().launch_recursive_has_changed_update();

            me->enable_natural_destruction();
            try
            {
		const cat_entree *e = nullptr;
		const cat_nomme *e_nom = nullptr;
		const cat_inode *e_ino = nullptr;
		const cat_directory *e_dir = nullptr;
		const cat_eod *e_eod = nullptr;
		const cat_mirage *e_mir = nullptr;
		const cat_eod tmp_eod;
		thread_cancellation thr;
		defile juillet = FAKE_ROOT;
		list_entry ent;
		bool isolated;

		if(exploitable)
		    isolated = only_contains_an_isolated_catalogue();
		else
		    isolated = false;

		get_cat().reset_read();
		while(get_cat().read(e))
		{
		    e_nom = dynamic_cast<const cat_nomme *>(e);
		    e_ino = dynamic_cast<const cat_inode *>(e);
		    e_dir = dynamic_cast<const cat_directory *>(e);
		    e_eod = dynamic_cast<const cat_eod *>(e);
		    e_mir = dynamic_cast<const cat_mirage *>(e);

		    if(e == nullptr)
			throw SRC_BUG;

		    thr.check_self_cancellation();
		    juillet.enfile(e);

		    if(options.get_subtree().is_covered(juillet.get_path())
		       && (e_dir != nullptr
			   || e_nom == nullptr
			   || options.get_selection().is_covered(e_nom->get_name())))
		    {
			if(e_mir != nullptr)
			    e_ino = e_mir->get_inode();

			if(!options.get_filter_unsaved()        // invoking callback if not filtering unsaved
			   || e_eod != nullptr                  // invoking callback for all eod
			   || e->get_saved_status() == saved_status::saved // invoking callback for file having data saved
			   || e->get_saved_status() == saved_status::delta // invoking callback for file having a delta patch as data
			   || (e_ino != nullptr && e_ino->ea_get_saved_status() == ea_saved_status::full) // invoking callback for files having EA saved
			   || (e_ino != nullptr && e_ino->ea_get_saved_status() == ea_saved_status::fake) // invoking callback for saved in recorded in isolated catalogue
			   || (e_dir != nullptr && e_dir->get_recursive_has_changed()) // invoking callback for directory containing saved files
			    )
			{
			    e->set_list_entry(&used_layout,
					      options.get_display_ea(),
					      ent);
			    if(local_check_dirty_seq(get_cat().get_escape_layer()))
				ent.set_dirtiness(true);
			    if(isolated &&
			       (e->get_saved_status() == saved_status::saved
				|| e->get_saved_status() == saved_status::delta))
				ent.set_saved_status(saved_status::fake);
			    try
			    {
				callback(juillet.get_string_without_root(), ent, context);
			    }
			    catch(Egeneric & e)
			    {
				throw Elibcall("archive::i_archive::op_listing",
					       tools_printf(gettext("Exception caught from archive_listing_callback execution: %s"),
							    e.dump_str().c_str()));
			    }
			    catch(...)
			    {
				throw Elibcall("archive::i_archive::op_listing", gettext("Exception caught from archive_listing_callback execution"));
			    }
			}
			else // not saved, filtered out
			{
			    if(e_dir != nullptr)
			    {
				get_cat().skip_read_to_parent_dir();
				juillet.enfile(&tmp_eod);
			    }
			}
		    }
		    else // filtered out
		    {
			if(e_dir != nullptr)
			{
			    get_cat().skip_read_to_parent_dir();
			    juillet.enfile(&tmp_eod);
			}
		    }
		}
            }
            catch(Euser_abort & e)
            {
                me->disable_natural_destruction();
                throw;
            }
            catch(Erange & e)
            {
                string msg = string(gettext("Error while listing archive contents: ")) + e.get_message();
                throw Edata(msg);
            }
        }
        catch(...)
        {
	    if(sequential_read)
		me->exploitable = false;
            throw;
        }
	if(sequential_read)
	    me->exploitable = false;
    }

    statistics archive::i_archive::op_diff(const path & fs_root,
					   const archive_options_diff & options,
					   statistics * progressive_report)
    {
        statistics st = false;  // false => no lock for this internal object
	statistics *st_ptr = progressive_report == nullptr ? &st : progressive_report;
	bool isolated_mode = false;
	path real_fs_root(".");

        try
        {

                // sanity checks

            if(!exploitable)
                throw Elibcall("op_diff", gettext("This archive is not exploitable, check documentation for more"));

	    try
	    {
		check_against_isolation(lax_read_mode);
	    }
	    catch(Erange & e)
	    {
		get_ui().message("This archive contains an isolated catalogue, only meta data can be used for comparison, CRC will be used to compare data of delta signature if present. Warning: Succeeding this comparison does not mean there is no difference as two different files may have the same CRC or the same delta signature");
		isolated_mode = true;
	    }
                // end of sanity checks

            enable_natural_destruction();

	    if(options.get_in_place())
	    {
		if(!get_cat().get_in_place(real_fs_root))
		    throw Erange("op_extract", gettext("Cannot use in-place restoration as no in-place path is stored in the archive"));
	    }
	    else
		real_fs_root = fs_root;

            try
            {
                filtre_difference(get_pointer(),
				  options.get_selection(),
				  options.get_subtree(),
				  get_cat(),
				  tools_relative2absolute_path(real_fs_root, tools_getcwd()),
				  options.get_info_details(),
				  options.get_display_treated(),
				  options.get_display_treated_only_dir(),
				  options.get_display_skipped(),
				  *st_ptr,
				  options.get_ea_mask(),
				  options.get_alter_atime(),
				  options.get_furtive_read_mode(),
				  options.get_what_to_check(),
				  options.get_hourshift(),
				  options.get_compare_symlink_date(),
				  options.get_fsa_scope(),
				  isolated_mode,
				  sequential_read,
				  options.get_auto_zeroing_neg_dates());
            }
            catch(Euser_abort & e)
            {
                disable_natural_destruction();
                throw;
            }
            catch(Ethread_cancel & e)
            {
                disable_natural_destruction();
                throw;
            }
            catch(Erange & e)
            {
                string msg = string(gettext("Error while comparing archive with filesystem: "))+e.get_message();
                get_ui().message(msg);
                throw Edata(msg);
            }
        }
        catch(...)
        {
	    if(sequential_read)
		exploitable = false;
            throw;
        }
	if(sequential_read)
	    exploitable = false;

        return *st_ptr;
    }

    statistics archive::i_archive::op_test(const archive_options_test & options,
					   bool repairing,
					   statistics * progressive_report)
    {
        statistics st = false;  // false => no lock for this internal object
	statistics *st_ptr = progressive_report == nullptr ? &st : progressive_report;
	bool isolated = false;

        try
        {

                // sanity checks

            if(!exploitable)
                throw Elibcall("op_test", gettext("This archive is not exploitable, check the archive class usage in the API documentation"));

	    try
	    {
		check_against_isolation(lax_read_mode);
	    }
	    catch(Erange & e)
	    {
		    // no data/EA are available in this archive,
		    // we can return normally at this point
		    // as the catalogue has already been read
		    // thus all that could be tested has been tested
		get_ui().message(gettext("WARNING! This is an isolated catalogue, no data or EA is present in this archive, only the catalogue structure can be checked"));
		isolated = true;
	    }

                // end of sanity checks


            enable_natural_destruction();
            try
            {
		try
		{
		    if(isolated)
		    {
			const cat_entree *tmp;
			if(cat == nullptr)
			    throw SRC_BUG;
			cat->read(tmp); // should be enough to have the whole catalogue being read if using sequential read mode
			cat->reset_read();
		    }
		    else
			filtre_test(get_pointer(),
				    options.get_selection(),
				    options.get_subtree(),
				    get_cat(),
				    options.get_info_details(),
				    options.get_display_treated(),
				    options.get_display_treated_only_dir(),
				    options.get_display_skipped(),
				    options.get_empty(),
				    repairing,
				    *st_ptr);
		}
		catch(Erange & e)
		{
		    get_ui().message(gettext("A problem occurred while reading this archive contents: ") + e.get_message());
		}
            }
            catch(Euser_abort & e)
            {
                disable_natural_destruction();
                throw;
            }
            catch(Ethread_cancel & e)
            {
		disable_natural_destruction();
                throw;
            }
            catch(Erange & e)
            {
                string msg = string(gettext("Error while testing archive: "))+e.get_message();
                get_ui().message(msg);
                throw Edata(msg);
            }
        }
        catch(...)
        {
	    if(sequential_read)
		exploitable = false;
            throw;
        }
	if(sequential_read)
	    exploitable = false;

        return *st_ptr;
    }


    void archive::i_archive::op_isolate(const path &sauv_path,
					const string & filename,
					const string & extension,
					const archive_options_isolate & options)
    {
	shared_ptr<entrepot> sauv_path_t = options.get_entrepot();
	if(sauv_path_t == nullptr)
	    throw Ememory("archive::i_archive::archive");

	sauv_path_t->set_user_ownership(options.get_slice_user_ownership());
	sauv_path_t->set_group_ownership(options.get_slice_group_ownership());
	sauv_path_t->set_location(sauv_path);

	tools_avoid_slice_overwriting_regex(get_ui(),
					    *sauv_path_t,
					    filename,
					    extension,
					    options.get_info_details(),
					    options.get_allow_over(),
					    options.get_warn_over(),
					    options.get_empty());

	try
	{
	    pile layers;
	    header_version isol_ver;
	    label isol_data_name;
	    label internal_name;
	    slice_layout isol_slices; // this field is not used here, but necessary to call macro_tools_create_layers()

	    if(!exploitable && options.get_delta_signature())
		throw Erange("archive::i_archive::op_isolate", gettext("Isolation with delta signature is not possible on a just created archive (on-fly isolation)"));

	    do
	    {
		isol_data_name.generate_internal_filename();
	    }
	    while(isol_data_name == cat->get_data_name());
	    internal_name = isol_data_name;


	    macro_tools_create_layers(get_pointer(),
				      layers,
				      isol_ver,
				      isol_slices,
				      &slices, // giving our slice_layout as reference to be stored in the archive header/trailer
				      sauv_path_t,
				      filename,
				      extension,
				      options.get_allow_over(),
				      options.get_warn_over(),
				      options.get_info_details(),
				      options.get_pause(),
				      options.get_compression(),
				      options.get_compression_level(),
				      options.get_compression_block_size(),
				      options.get_slice_size(),
				      options.get_first_slice_size(),
				      options.get_execute(),
				      options.get_crypto_algo(),
				      options.get_crypto_pass(),
				      options.get_crypto_size(),
				      options.get_gnupg_recipients(),
				      options.get_gnupg_signatories(),
				      options.get_empty(),
				      options.get_slice_permission(),
				      options.get_sequential_marks(),
				      options.get_user_comment(),
				      options.get_hash_algo(),
				      options.get_slice_min_digits(),
				      internal_name,
				      isol_data_name,
				      options.get_iteration_count(),
				      options.get_kdf_hash(),
				      options.get_multi_threaded_crypto(),
				      options.get_multi_threaded_crypto()); /* must be changed with dedicated field for compression */

	    if(cat == nullptr)
		throw SRC_BUG;

	    try
	    {

		    // note:
		    // an isolated catalogue keeps the data, EA and FSA pointers of the archive they come from
		    // but have their own copy of the delta signature if present, in order to be able to make
		    // a differential/incremental delta binary based on an isolated catalogue. The drawback is
		    // that reading an archive with the help of an isolated catalogue, the delta signature used
		    // will always be those of the isolated catalogue, not those of the archive the catalogue
		    // has been isolated from.
		    // Using an isolated catalogue as backup of the internal catalogue stays possible but not
		    // for the delta_signature which are ignored by libdar in that situation (archive reading).
		    // However as delta_signature are only used at archive creation time this does not hurt. Delta
		    // signature will be available during a differential backup either from the isolated catalogue
		    // using its own copy of them, or from the original archive which would contain their original
		    // version.

		if(options.get_delta_signature())
		{
		    pile_descriptor pdesc = & layers;
		    cat->transfer_delta_signatures(pdesc,
						   sequential_read,
						   options.get_has_delta_mask_been_set(),
						   options.get_delta_mask(),
						   options.get_delta_sig_min_size(),
						   options.get_sig_block_len());
		}
		else
			// we drop the delta signature as they will never be used
			// because none will be in the isolated catalogue
			// and that an isolated catalogue as backup of an internal
			// catalogue cannot rescue the delta signature (they have
			// to be in the isolated catalogue)
		    cat->drop_delta_signatures();
	    }
	    catch(Ebug & e)
	    {
		throw;
	    }
	    catch(Egeneric & e)
	    {
		    // including Euser_abort in case a slide is missing
		    // and user could not provide it

		if(options.get_repair_mode())
		{
			// dropping the latest read entry (obviously corrupted)
			// but not throwing any exception
		    cat->tail_catalogue_to_current_read(true);
		}
		else
		    throw;
	    }

	    if(isol_data_name == cat->get_data_name())
		throw SRC_BUG;
		// data_name generated just above by slice layer
		// should never equal the data_name of the catalogue
		// when performing isolation

	    macro_tools_close_layers(get_pointer(),
				     layers,
				     isol_ver,
				     *cat,
				     options.get_info_details(),
				     options.get_crypto_algo(),
				     options.get_compression(),
				     options.get_gnupg_recipients(),
				     options.get_gnupg_signatories(),
				     options.get_empty());
	}
	catch(...)
	{
	    sauv_path_t.reset();
	    throw;
	}
	sauv_path_t.reset();
    }

    void archive::i_archive::load_catalogue(bool repairing)
    {
	if(exploitable && sequential_read) // the catalogue is not even yet read, so we must first read it entirely
	{
	    if(cat == nullptr)
		throw SRC_BUG;;

	    if(only_contains_an_isolated_catalogue())
		    // this is easy... asking just an entry
		    //from the catalogue makes its whole being read
	    {
		const cat_entree *tmp;
		cat->read(tmp); // should be enough to have the whole catalogue being read
		cat->reset_read();
	    }
	    else
		    // here we have a plain archive, doing the test operation
		    // is the simplest way to read the whole archive and thus get its contents
		    // (i.e.: the catalogue)
		(void)op_test(archive_options_test(),
			      repairing,
			      nullptr);
	}
    }


    bool archive::i_archive::get_children_of(archive_listing_callback callback,
					     void *context,
					     const string & dir,
					     bool fetch_ea)
    {
	bool ret;

	if(callback == nullptr)
	    throw Erange("archive::i_archive::get_children_of", "nullptr provided as user callback function");

	if(fetch_ea && sequential_read)
	    throw Erange("archive::i_archive::get_children_of", gettext("Fetching EA value while listing an archive is not possible in sequential read mode"));
	if(cat != nullptr && cat->get_early_memory_release())
	    throw Erange("i_archive::load_catalogue", gettext("get_children_of is not possible on a catalogue set with early memory release"));

	load_catalogue(false);
	    // OK, now that we have the whole catalogue available in memory, let's rock!

	vector<list_entry> tmp = get_children_in_table(dir,fetch_ea);
	vector<list_entry>::iterator it = tmp.begin();

	ret = (it != tmp.end());
	while(it != tmp.end())
	{
	    try
	    {
		callback(dir, *it, context);
	    }
	    catch(...)
	    {
		throw Elibcall("archive::i_archive::get_children_of", "user provided callback function should not throw any exception");
	    }
	}

	return ret;
    }


    const vector<list_entry> archive::i_archive::get_children_in_table(const string & dir, bool fetch_ea) const
    {
	vector <list_entry> ret;
	i_archive* me = const_cast<i_archive *>(this);

	if(me == nullptr)
	    throw SRC_BUG;

	if(fetch_ea && sequential_read)
	    throw Erange("archive::i_archive::get_children_of", gettext("Fetching EA value while listing an archive is not possible in sequential read mode"));

	if(cat != nullptr && cat->get_early_memory_release())
	    throw Erange("i_archive::load_catalogue", gettext("get_children_in_table is not possible on a catalogue set with early memory release"));

	me->load_catalogue(false);

	const cat_directory* parent = get_dir_object(dir);
	const cat_nomme* tmp_ptr = nullptr;
	list_entry ent;

	if(parent == nullptr)
	    throw SRC_BUG;
	else
	{
	    try
	    {
		U_I sz = 0;
		infinint i_sz = parent->get_dir_size();
		i_sz.unstack(sz);
		    // if i_sz > 0 the requested size is larger than what U_I can hold
		    // we won't handle this case here as we have not mean to
		    // ask the stdc++ lib to reserver as much memory
		    // though, there is chances that the stdc++ library
		    // will not be able to get as much memory and will
		    // throw a std::bad_alloc exception when calling reserve()
		ret.reserve(sz);
	    }
	    catch(std::bad_alloc &)
	    {
		throw Ememory("std::vector<libdar::list_entry>::reserve()");
	    }
	}

	parent->reset_read_children();
	while(parent->read_children(tmp_ptr))
	{
	    if(tmp_ptr == nullptr)
		throw SRC_BUG;

	    tmp_ptr->set_list_entry(&slices, fetch_ea, ent);

		// fill a new entry in the table
	    ret.push_back(ent);
	}

	return ret;
    }

    bool archive::i_archive::has_subdirectory(const string & dir) const
    {
	bool ret = false;
	const cat_directory *parent = get_dir_object(dir);
	const cat_nomme *tmp_ptr = nullptr;

	parent->reset_read_children();
	while(parent->read_children(tmp_ptr) && !ret)
	{
	    if(dynamic_cast<const cat_directory *>(tmp_ptr) != nullptr)
		ret = true;
	}

	return ret;
    }

    void archive::i_archive::init_catalogue() const
    {
	if(exploitable && sequential_read) // the catalogue is not even yet read, so we must first read it entirely
	{
	    if(only_contains_an_isolated_catalogue())
		    // this is easy... asking just an entry
		    //from the catalogue makes its whole being read
	    {
		const cat_entree *tmp;
		if(cat == nullptr)
		    throw SRC_BUG;
		cat->read(tmp); // should be enough to have the whole catalogue being read
		cat->reset_read();
	    }
	    else
	    {
		if(cat == nullptr)
		    throw SRC_BUG;
		filtre_sequentially_read_all_catalogue(*cat, get_pointer(), lax_read_mode);
	    }
	}

	if(cat == nullptr)
	    throw SRC_BUG;
    }

    const catalogue & archive::i_archive::get_catalogue() const
    {
	if(exploitable && sequential_read)
	    throw Elibcall("archive::i_archive::get_catalogue", "Reading the first time the catalogue of an archive open in sequential read mode needs passing a \"user_interaction\" object to the argument of archive::i_archive::get_catalogue or call init_catalogue() first ");

	if(cat == nullptr)
	    throw SRC_BUG;

	if(cat->get_memory_released())
	    throw Erange("i_archive::load_catalogue", gettext("cannot get catalogue which memory has been (early) released"));

	return *cat;
    }

    void archive::i_archive::drop_all_filedescriptors(bool repairing)
    {
	if(exploitable && sequential_read)
	{
	    if(only_contains_an_isolated_catalogue())
	    {
		const cat_entree *tmp;
		if(cat == nullptr)
		    throw SRC_BUG;
		cat->read(tmp); // should be enough to have the whole catalogue being read
		cat->reset_read();
	    }
	    else
		(void)op_test(archive_options_test(), repairing, nullptr);
	}

	stack.clear();
	exploitable = false;
    }

    bool archive::i_archive::get_catalogue_slice_layout(slice_layout & slicing) const
    {
	slicing = slices;
	    // by default we use the slice layout of the current archive,
	    // this is modified if the current archive is an isolated catalogue
	    // in archive format 9 or greater, then we use the slice layout contained
	    // in the archive header/trailer which is a copy of the one of the archive of reference
	    // a warning is issued in that case

	if(only_contains_an_isolated_catalogue())
	{
	    if(ver.get_slice_layout() != nullptr)
	    {
		slicing = *ver.get_slice_layout();
		return true;
	    }
	    else // no slicing of the archive of reference stored in this isolated catalogue's header/trailer
	    {

		if(ver.get_edition() >= 9)
		    throw SRC_BUG; // starting revision 9 isolated catalogue should always contain
		    // the slicing of the archive of reference, even if that reference is using an archive format
		    // older than version 9.
		return false;
	    }
	}
	else
	    return true;
    }


    U_64 archive::i_archive::get_first_slice_header_size() const
    {
	U_64 ret;
	infinint pre_ret;
	const generic_file *bottom = stack.bottom();
	const trivial_sar *b_triv = dynamic_cast<const trivial_sar *>(bottom);
	const sar *b_sar = dynamic_cast<const sar *>(bottom);
	const zapette *b_zap = dynamic_cast<const zapette *>(bottom);

	if(b_triv != nullptr)
	    pre_ret = b_triv->get_slice_header_size();
	else if(b_sar != nullptr)
	    pre_ret = b_sar->get_first_slice_header_size();
	else if(b_zap != nullptr)
	    pre_ret = b_zap->get_first_slice_header_size();
	else
	    pre_ret = 0; // unknown size

	if(!tools_infinint2U_64(pre_ret, ret))
	    ret = 0;

	return ret;
    }

    U_64 archive::i_archive::get_non_first_slice_header_size() const
    {
	U_64 ret;
	infinint pre_ret;
	const generic_file *bottom = stack.bottom();
	const trivial_sar *b_triv = dynamic_cast<const trivial_sar *>(bottom);
	const sar *b_sar = dynamic_cast<const sar *>(bottom);
	const zapette *b_zap = dynamic_cast<const zapette *>(bottom);

	if(b_triv != nullptr)
	    pre_ret = b_triv->get_slice_header_size();
	else if(b_sar != nullptr)
	    pre_ret = b_sar->get_non_first_slice_header_size();
	else if(b_zap != nullptr)
	    pre_ret = b_zap->get_non_first_slice_header_size();
	else
	    pre_ret = 0; // unknown size

	if(!tools_infinint2U_64(pre_ret, ret))
	    ret = 0;

	return ret;
    }


	////////////////////
	// PRIVATE METHODS FOLLOW
	//

    statistics archive::i_archive::op_create_in(operation op,
						const path & fs_root,
						const shared_ptr<entrepot> & sauv_path_t,
						archive *ref_arch,
						const mask & selection,
						const mask & subtree,
						const string & filename,
						const string & extension,
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
						U_I compression_block_size,
						const infinint & file_size,
						const infinint & first_file_size,
						const mask & ea_mask,
						const string & execute,
						crypto_algo crypto,
						const secu_string & pass,
						U_32 crypto_size,
						const vector<string> & gnupg_recipients,
						const vector<string> & gnupg_signatories,
						const mask & compr_mask,
						const infinint & min_compr_size,
						bool nodump,
						const string & exclude_by_ea,
						const infinint & hourshift,
						bool empty,
						bool alter_atime,
						bool furtive_read_mode,
						const filesystem_ids & same_fs,
						comparison_fields what_to_check,
						bool snapshot,
						bool cache_directory_tagging,
						const infinint & fixed_date,
						const string & slice_permission,
						const infinint & repeat_count,
						const infinint & repeat_byte,
						bool add_marks_for_sequential_reading,
						bool security_check,
						const infinint & sparse_file_min_size,
						const string & user_comment,
						hash_algo hash,
						const infinint & slice_min_digits,
						const string & backup_hook_file_execute,
						const mask & backup_hook_file_mask,
						bool ignore_unknown,
						const fsa_scope & scope,
						U_I multi_threaded_crypto,
						U_I multi_threaded_compress,
						bool delta_signature,
						bool build_delta_sig,
						const mask & delta_mask,
						const infinint & delta_sig_min_size,
						bool delta_diff,
						bool zeroing_neg_date,
						const set<string> & ignored_symlinks,
						modified_data_detection mod_data_detect,
						const infinint & iteration_count,
						hash_algo kdf_hash,
						const delta_sig_block_size & sig_block_len,
						bool never_resave_uncompressed,
						statistics * progressive_report)
    {
        statistics st = false;  // false => no lock for this internal object
	statistics *st_ptr = progressive_report == nullptr ? &st : progressive_report;

            // sanity checks as much as possible to avoid libdar crashing due to bad arguments
            // useless arguments are not reported.

        if((compression_level > 9 && algo != compression::zstd) || compression_level < 1)
            throw Elibcall("op_create_in", gettext("Compression_level must be between 1 and 9 included"));
        if(file_size.is_zero() && !first_file_size.is_zero())
            throw Elibcall("op_create_in", gettext("\"first_file_size\" cannot be different from zero if \"file_size\" is equal to zero"));
        if(crypto_size < 10 && crypto != crypto_algo::none)
            throw Elibcall("op_create_in", gettext("Crypto block size must be greater than 10 bytes"));
#ifndef	LIBDAR_NODUMP_FEATURE
	if(nodump)
	    throw Ecompilation(gettext("nodump flag feature has not been activated at compilation time, it is thus not available"));
#endif

	check_libgcrypt_hash_bug(get_ui(), hash, first_file_size, file_size);

            // end of sanity checks

	catalogue *ref_cat = nullptr;
	bool initial_pause = false;
	path sauv_path_abs = sauv_path_t->get_location();
	path fs_root_abs = fs_root.is_relative() ? tools_relative2absolute_path(fs_root, tools_getcwd()) : fs_root;

	if(sauv_path_abs.is_relative())
	    sauv_path_abs = sauv_path_t->get_root() + sauv_path_abs;

	tools_avoid_slice_overwriting_regex(get_ui(),
					    *sauv_path_t,
					    filename,
					    extension,
					    info_details,
					    allow_over,
					    warn_over,
					    empty);

	if(ref_arch != nullptr && ref_arch->pimpl->sequential_read && (delta_signature || delta_diff))
	    throw Erange("archive::i_archive::op_create_in", gettext("Cannot sequentially read an archive of reference when delta signature or delta patch is requested"));

	local_cat_size = 0; // unknown catalogue size (need to re-open the archive, once creation has completed) [object member variable]

	sauv_path_abs.explode_undisclosed();

	    // warning against saving the archive itself

	const entrepot_local *sauv_path_t_local = dynamic_cast<const entrepot_local *>(sauv_path_t.get());

	if(op == oper_create
	   && sauv_path_t_local != nullptr  // not using a remote storage
	   && sauv_path_abs.is_subdir_of(fs_root_abs, true)
	   && selection.is_covered(filename+".1."+extension)
	   && subtree.is_covered(sauv_path_abs.append(filename+".1."+extension))
	   && filename!= "-")
	{
	    bool cov = true;      // whether the archive is covered by filter (this is saving itself)
	    string drop;          // will carry the removed part of the sauv_path_abs variable

		// checking for exclusion due to different filesystem

	    if(! same_fs.is_covered(sauv_path_abs))
		cov = false;

	    if(snapshot)     // if we do a snapshot we dont create an archive this no risk to save ourselves
		cov = false;

		// checking for directory auto inclusion
	    do
	    {
		cov = cov && subtree.is_covered(sauv_path_abs);
	    }
	    while(cov && sauv_path_abs.pop(drop));

	    if(cov)
		get_ui().pause(tools_printf(gettext("WARNING! The archive is located in the directory to backup, this may create an endless loop when the archive will try to save itself. You can either add -X \"%S.*.%S\" on the command line, or change the location of the archive (see -h for help). Do you really want to continue?"), &filename, &extension));
	}

	    // building the reference catalogue

	if(ref_arch != nullptr) // from a existing archive
	{
	    const shared_ptr<entrepot> ref_where = ref_arch->pimpl->get_entrepot();
	    if(ref_where)
		initial_pause = (*ref_where == *sauv_path_t);
	    ref_cat = const_cast<catalogue *>(& ref_arch->pimpl->get_catalogue());
	}

	op_create_in_sub(op,
			 fs_root,
			 sauv_path_t,
			 ref_cat,
			 nullptr,
			 initial_pause,
			 selection,
			 subtree,
			 filename,
			 extension,
			 allow_over,
			 allow_over ? crit_constant_action(data_overwrite, EA_overwrite) : crit_constant_action(data_preserve, EA_preserve), // we do not have any overwriting policy in this environement (archive creation and isolation), so we create one on-fly
			 warn_over,
			 info_details,
			 display_treated,
			 display_treated_only_dir,
			 display_skipped,
			 display_finished,
			 pause,
			 empty_dir,
			 algo,
			 compression_level,
			 compression_block_size,
			 file_size,
			 first_file_size,
			 ea_mask,
			 execute,
			 crypto,
			 pass,
			 crypto_size,
			 gnupg_recipients,
			 gnupg_signatories,
			 compr_mask,
			 min_compr_size,
			 nodump,
			 exclude_by_ea,
			 hourshift,
			 empty,
			 alter_atime,
			 furtive_read_mode,
			 same_fs,
			 what_to_check,
			 snapshot,
			 cache_directory_tagging,
			 false,   // keep_compressed
			 fixed_date,
			 slice_permission,
			 repeat_count,
			 repeat_byte,
			 false,   // decremental mode
			 add_marks_for_sequential_reading,
			 security_check,
			 sparse_file_min_size,
			 user_comment,
			 hash,
			 slice_min_digits,
			 backup_hook_file_execute,
			 backup_hook_file_mask,
			 ignore_unknown,
			 scope,
			 multi_threaded_crypto,
			 multi_threaded_compress,
			 delta_signature,
			 build_delta_sig,
			 delta_mask,
			 delta_sig_min_size,
			 delta_diff,
			 zeroing_neg_date,
			 ignored_symlinks,
			 mod_data_detect,
			 iteration_count,
			 kdf_hash,
			 sig_block_len,
			 never_resave_uncompressed,
			 st_ptr);

	return *st_ptr;
    }

    void archive::i_archive::op_create_in_sub(operation op,
					      const path & fs_root,
					      const shared_ptr<entrepot> & sauv_path_t,
					      catalogue *ref_cat1,
					      const catalogue *ref_cat2,
					      bool initial_pause,
					      const mask & selection,
					      const mask & subtree,
					      const string & filename,
					      const string & extension,
					      bool allow_over,
					      const crit_action & overwrite,
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
					      U_I compression_block_size,
					      const infinint & file_size,
					      const infinint & first_file_size,
					      const mask & ea_mask,
					      const string & execute,
					      crypto_algo crypto,
					      const secu_string & pass,
					      U_32 crypto_size,
					      const vector<string> & gnupg_recipients,
					      const vector<string> & gnupg_signatories,
					      const mask & compr_mask,
					      const infinint & min_compr_size,
					      bool nodump,
					      const string & exclude_by_ea,
					      const infinint & hourshift,
					      bool empty,
					      bool alter_atime,
					      bool furtive_read_mode,
					      const filesystem_ids & same_fs,
					      comparison_fields what_to_check,
					      bool snapshot,
					      bool cache_directory_tagging,
					      bool keep_compressed,
					      const infinint & fixed_date,
					      const string & slice_permission,
					      const infinint & repeat_count,
					      const infinint & repeat_byte,
					      bool decremental,
					      bool add_marks_for_sequential_reading,
					      bool security_check,
					      const infinint & sparse_file_min_size,
					      const string & user_comment,
					      hash_algo hash,
					      const infinint & slice_min_digits,
					      const string & backup_hook_file_execute,
					      const mask & backup_hook_file_mask,
					      bool ignore_unknown,
					      const fsa_scope & scope,
					      U_I multi_threaded_crypto,
					      U_I multi_threaded_compress,
					      bool delta_signature,
					      bool build_delta_sig,
					      const mask & delta_mask,
					      const infinint & delta_sig_min_size,
					      bool delta_diff,
					      bool zeroing_neg_date,
					      const set<string> & ignored_symlinks,
					      modified_data_detection mod_data_detect,
					      const infinint & iteration_count,
					      hash_algo kdf_hash,
					      const delta_sig_block_size & sig_block_len,
					      bool never_resave_uncompressed,
					      statistics * st_ptr)
    {
	try
	{
	    stack.clear(); // [object member variable]
	    bool aborting = false;
	    infinint aborting_next_etoile = 0;
	    U_64 flag = 0; // carries the sar option flag

	    label internal_name;
	    generic_file *tmp = nullptr;
	    thread_cancellation thr_cancel;

	    if(ref_cat1 == nullptr && op != oper_create)
		throw SRC_BUG;
	    if(st_ptr == nullptr)
		throw SRC_BUG;

	    secu_string real_pass = pass;
	    internal_name.generate_internal_filename();

	    try
	    {
		    // pausing if saving in the same directory where is located the archive of reference

		if(!pause.is_zero() && initial_pause)
		    get_ui().pause(gettext("Ready to start writing down the archive?"));

		macro_tools_create_layers(get_pointer(),
					  stack, // this object field is set!
					  ver,   // this object field is set!
					  slices,// this object field is set!
					  nullptr,  // no slicing reference stored in archive header/trailer
					  sauv_path_t,
					  filename,
					  extension,
					  allow_over,
					  warn_over,
					  info_details,
					  pause,
					  algo,
					  compression_level,
					  compression_block_size,
					  file_size,
					  first_file_size,
					  execute,
					  crypto,
					  pass,
					  crypto_size,
					  gnupg_recipients,
					  gnupg_signatories,
					  empty,
					  slice_permission,
					  add_marks_for_sequential_reading,
					  user_comment,
					  hash,
					  slice_min_digits,
					  internal_name,
					  internal_name, // data_name is equal to internal_name in the current situation
					  iteration_count,
					  kdf_hash,
					  multi_threaded_crypto,
					  multi_threaded_compress);

		    // ********** building the catalogue (empty for now) ************************* //
		datetime root_mtime;
		pile_descriptor pdesc(&stack);
		crit_action* rep_decr = nullptr; // not used, just needed to pass as argumen to filtre_merge_step0()
		const crit_action *rep_over = &overwrite; // not used, just needed to pass as argumen to filtre_merge_step0()


		cat = nullptr; // [object member variable]

		if(info_details)
		    get_ui().message(gettext("Building the catalog object..."));
		try
		{
		    if(fs_root.display() != "<ROOT>")
			root_mtime = tools_get_mtime(get_ui(),
						     fs_root.display(),
						     zeroing_neg_date,
						     false); // not silent
		    else // case of merging operation for example
		    {
			datetime mtime1 = ref_cat1 != nullptr ? ref_cat1->get_root_mtime() : datetime(0);
			datetime mtime2 = ref_cat2 != nullptr ? ref_cat2->get_root_mtime() : datetime(0);
			root_mtime = mtime1 > mtime2 ? mtime1 : mtime2;
		    }
		}
		catch(Erange & e)
		{
		    string tmp = fs_root.display();
		    throw Erange("archive::i_archive::op_create_in_sub", tools_printf(gettext("Error while fetching information for %S: "), &tmp) + e.get_message());
		}

		switch(op)
		{
		case oper_merge:
		case oper_repair:
		    if(add_marks_for_sequential_reading && !empty)
			cat = new (nothrow) escape_catalogue(get_pointer(), pdesc, ref_cat1->get_root_dir_last_modif(), internal_name);
		    else
			cat = new (nothrow) catalogue(get_pointer(), ref_cat1->get_root_dir_last_modif(), internal_name);
		    break;
		case oper_create:
		    if(add_marks_for_sequential_reading && !empty)
			cat = new (nothrow) escape_catalogue(get_pointer(), pdesc, root_mtime, internal_name);
		    else
			cat = new (nothrow) catalogue(get_pointer(), root_mtime, internal_name);
		    break;
		default:
		    throw SRC_BUG;
		}

		if(cat == nullptr)
		    throw Ememory("archive::i_archive::op_create_in_sub");


		    // *********** now we can perform the data filtering operation (adding data to the archive) *************** //

		path ref1_in_place(".");
		path ref2_in_place(".");

		try
		{
		    catalogue *void_cat = nullptr;
		    const catalogue *ref_cat_ptr = ref_cat1;

		    switch(op)
		    {
		    case oper_create:

			    // setting in_place
			cat->set_in_place(fs_root);

			if(ref_cat1 == nullptr)
			{
				// using a empty catalogue as reference if no reference is given

			    label data_name;
			    data_name.clear();
			    void_cat = new (nothrow) catalogue(get_pointer(),
							       datetime(0),
							       data_name);
			    if(void_cat == nullptr)
				throw Ememory("archive::i_archive::op_create_in_sub");
			    ref_cat_ptr = void_cat;
			}
			else
			{
			    if(ref_cat1->get_in_place(ref1_in_place))
			    {
				if(ref1_in_place != fs_root)
				{
				    string ref1 = ref1_in_place.display();
				    string actual = fs_root.display();
				    get_ui().printf(gettext("Warning making a differential/incremental backup with a different root directory: %S <-> %S"),
						    &ref1,
						    &actual);
				}
				    // else we cannot check (old archive format or merged archive
			    }

			}

			try
			{
			    if(info_details)
				get_ui().message(gettext("Processing files for backup..."));
			    filtre_sauvegarde(get_pointer(),
					      selection,
					      subtree,
					      pdesc,
					      *cat,
					      *ref_cat_ptr,
					      fs_root,
					      info_details,
					      display_treated,
					      display_treated_only_dir,
					      display_skipped,
					      display_finished,
					      *st_ptr,
					      empty_dir,
					      ea_mask,
					      compr_mask,
					      min_compr_size,
					      nodump,
					      hourshift,
					      alter_atime,
					      furtive_read_mode,
					      same_fs,
					      what_to_check,
					      snapshot,
					      cache_directory_tagging,
					      security_check,
					      repeat_count,
					      repeat_byte,
					      fixed_date,
					      sparse_file_min_size,
					      backup_hook_file_execute,
					      backup_hook_file_mask,
					      ignore_unknown,
					      scope,
					      exclude_by_ea,
					      delta_signature,
					      delta_sig_min_size,
					      delta_mask,
					      delta_diff,
					      zeroing_neg_date,
					      ignored_symlinks,
					      mod_data_detect,
					      sig_block_len,
					      never_resave_uncompressed,
					      sequential_read);
				// build_delta_sig is not used for archive creation it is always implied when delta_signature is set
			}
			catch(...)
			{
			    if(void_cat != nullptr)
			    {
				delete void_cat;
				void_cat = nullptr;
			    }
			    throw;
			}
			if(void_cat != nullptr)
			{
			    delete void_cat;
			    void_cat = nullptr;
			}
			break;
		    case oper_merge:

			if(ref_cat2 == nullptr || ! ref_cat2->get_in_place(ref2_in_place))
			    if(ref_cat1->get_in_place(ref1_in_place))
				cat->set_in_place(ref1_in_place); // keeping the in-place of ref1
			    else
				cat->clear_in_place();
			else // ref_cat2 exists and has an in_place path stored in ref2_in_place
			    if(ref_cat1->get_in_place(ref1_in_place))
				if(ref1_in_place == ref2_in_place)
				    cat->set_in_place(ref1_in_place); // both the same in_place
				else
				    cat->clear_in_place(); // different in-place paths, merging without it
			    else
				cat->set_in_place(ref2_in_place); // only ref2 has in_place path

			if(ref_cat1->get_in_place(ref1_in_place))

			if(info_details)
			    get_ui().message(gettext("Processing files for merging..."));

			filtre_merge(get_pointer(),
				     selection,
				     subtree,
				     pdesc,
				     *cat,
				     ref_cat1,
				     ref_cat2,
				     info_details,
				     display_treated,
				     display_treated_only_dir,
				     display_skipped,
				     *st_ptr,
				     empty_dir,
				     ea_mask,
				     compr_mask,
				     min_compr_size,
				     keep_compressed,
				     overwrite,
				     warn_over,
				     decremental,
				     sparse_file_min_size,
				     scope,
				     delta_signature,
				     build_delta_sig,
				     delta_sig_min_size,
				     delta_mask,
				     sig_block_len,
				     never_resave_uncompressed);
			break;
		    case oper_repair:
			if(ref_cat2 != nullptr)
			    throw SRC_BUG;
			if(ref_cat1 == nullptr)
			    throw SRC_BUG;

			if(ref_cat1->get_in_place(ref1_in_place))
			    cat->set_in_place(ref1_in_place);
			else
			    cat->clear_in_place();

			if(info_details)
			    get_ui().message(gettext("Processing files for fixing..."));

			try
			{
			    filtre_merge_step0(get_pointer(),
					       ref_cat1,
					       ref_cat2,
					       *st_ptr,
					       false,
					       rep_decr,
					       rep_over,
					       aborting,
					       thr_cancel);
			    if(rep_decr != nullptr)
				throw SRC_BUG;
				// we should be prepared to release decr
				// but we do not need such argument for fixing op.
			    filtre_merge_step2(get_pointer(),
					       pdesc,
					       *ref_cat1,
					       info_details,
					       display_treated,
					       display_treated_only_dir,
					       compr_mask,
					       min_compr_size,
					       keep_compressed,
					       sparse_file_min_size,
					       delta_signature,
					       build_delta_sig,
					       delta_sig_min_size,
					       delta_mask,
					       aborting,
					       thr_cancel,
					       true,
					       delta_sig_block_size(), // we will not recalculate delta signature upon repairing
					       never_resave_uncompressed);

				// at this step, cat (the current archive's catalogue) is still empty
				// we will need to add ref_cat1's content at the end of the archive
				// not our own's content

				// changing the data_name of the catalogue that will be dropped at the
				// end of the archive to match the data_name of the slice header
			    ref_cat1->set_data_name(internal_name);
			}
			catch(Erange & e)
			{
				// changing the data_name of the catalogue that will be dropped at the
				// end of the archive to match the data_name of the slice header
			    ref_cat1->set_data_name(internal_name);
			}
			break;
		    default:
			throw SRC_BUG;
		    }

		    thr_cancel.block_delayed_cancellation(true);
			// we must protect the following code against delayed cancellations
		}
		catch(Ethread_cancel & e)
		{
		    disable_natural_destruction();
		    if(e.immediate_cancel())
			throw;
		    else
		    {
			Ethread_cancel_with_attr *e_attr = dynamic_cast<Ethread_cancel_with_attr *>(&e);

			aborting = true;
			flag = e.get_flag();
			if(e_attr != nullptr)
			    aborting_next_etoile = e_attr->get_attr();
			else
			    aborting_next_etoile = 0;
			thr_cancel.block_delayed_cancellation(true);
			    // we must protect the following code against delayed cancellations
			stack.top()->sync_write(); // flushing only the top of the stack (compressor) must not yet flush the below encryption layer!!!
		    }
		}

		if(ref_cat1 != nullptr && op == oper_create)
		{
		    if(info_details)
			get_ui().message(gettext("Adding reference to files that have been destroyed since reference backup..."));
		    if(aborting)
			cat->update_absent_with(*ref_cat1, aborting_next_etoile);
		    else
			st_ptr->add_to_deleted(cat->update_destroyed_with(*ref_cat1));
		}

		cat->drop_escape_layer();
		    // we need to remove pointer to layers
		    // that are about to be destroyed now

		macro_tools_close_layers(get_pointer(),
					 stack,
					 ver,
					 op != oper_repair ? *cat : *ref_cat1,
					 info_details,
					 crypto,
					 algo,
					 gnupg_recipients,
					 gnupg_signatories,
					 empty);

		thr_cancel.block_delayed_cancellation(false);
		    // release pending delayed cancellation (if any)

		if(aborting)
		    throw Ethread_cancel(false, flag);

	    }
	    catch(...)
	    {
		if(tmp != nullptr)
		{
		    delete tmp;
		    tmp = nullptr;
		}
		if(cat != nullptr)
		{
		    delete cat;
		    cat = nullptr;
		}
		throw;
	    }
	}
	catch(Euser_abort & e)
	{
	    disable_natural_destruction();
	    throw;
	}
	catch(Ebug & e)
	{
	    throw;
	}
	catch(Erange &e)
	{
	    string msg = string(gettext("Error while saving data: ")) + e.get_message();
	    throw Edata(msg);
	}
    }

    void archive::i_archive::free_mem()
    {
	stack.clear();
	gnupg_signed.clear();
	slices.clear();
	ver.clear_crypted_key();
	if(cat != nullptr)
	{
	    delete cat;
	    cat = nullptr;
	}
    }

    void archive::i_archive::check_gnupg_signed() const
    {
	list<signator>::const_iterator it = gnupg_signed.begin();

	while(it != gnupg_signed.end() && it->result == signator::good)
	    ++it;

	if(it != gnupg_signed.end())
	    get_ui().pause(gettext("WARNING! Incorrect signature found for archive, continue anyway?"));
    }

    void archive::i_archive::disable_natural_destruction()
    {
        sar *tmp = nullptr;
	trivial_sar *triv_tmp = nullptr;

	stack.find_first_from_bottom(tmp);
	if(tmp != nullptr)
            tmp->disable_natural_destruction();
	else
	{
	    stack.find_first_from_bottom(triv_tmp);
	    if(triv_tmp != nullptr)
		triv_tmp->disable_natural_destruction();
	}
    }

    void archive::i_archive::enable_natural_destruction()
    {
        sar *tmp = nullptr;
	trivial_sar *triv_tmp = nullptr;

	stack.find_first_from_bottom(tmp);
        if(tmp != nullptr)
            tmp->enable_natural_destruction();
	else
	{
	    stack.find_first_from_bottom(triv_tmp);
	    if(triv_tmp != nullptr)
		triv_tmp->disable_natural_destruction();
	}
    }

    const label & archive::i_archive::get_layer1_data_name() const
    {
	contextual *l1 = nullptr;

	stack.find_first_from_bottom(l1);
	if(l1 != nullptr)
	    return l1->get_data_name();
	else
	    throw Erange("catalogue::get_data_name", gettext("Cannot get data name of the archive, this archive is not completely initialized"));
    }

    const label & archive::i_archive::get_catalogue_data_name() const
    {
	if(cat != nullptr)
	    return cat->get_data_name();
	else
	    throw SRC_BUG;
    }

    bool archive::i_archive::only_contains_an_isolated_catalogue() const
    {
	return get_layer1_data_name() != get_catalogue_data_name() && ver.get_edition() >= 8;
    }

    void archive::i_archive::check_against_isolation(bool lax) const
    {
	if(cat != nullptr)
	{
	    try
	    {
		if(only_contains_an_isolated_catalogue())
		{
		    if(!lax)
			throw Erange("archive::i_archive::check_against_isolation", gettext("This archive contains an isolated catalogue, it cannot be used for this operation. It can only be used as reference for a incremental/differential backup or as backup of the original archive's catalogue"));
			// note1: that old archives do not have any data_name neither in the catalogue nor in the layer1 of the archive
			// both are faked equal to a zeroed label when reading them with recent dar version. Older archives than "08" would
			// thus pass this test successfully if no check was done against the archive version
			// note2: Old isolated catalogue do not carry any data, this is safe to try to restore them because any
			// pointer to data and/or EA has been removed during the isolation.
		    else
			get_ui().pause(gettext("LAX MODE: Archive seems to be only an isolated catalogue (no data in it), Can I assume data corruption occurred and consider the archive as being a real archive?"));
		}
	    }
	    catch(Erange & e)
	    {
		throw Erange("archive::i_archive::check_against_isolation", string(gettext("Error while fetching archive properties: ")) + e.get_message());
	    }
	}
	else
	    throw SRC_BUG;
	    // this method should be called once the archive object has been constructed
	    // and this object should be totally exploitable, thus have an available catalogue
    }

    bool archive::i_archive::get_sar_param(infinint & sub_file_size,
					   infinint & first_file_size,
					   infinint & last_file_size,
					   infinint & total_file_number)
    {
        sar *real_decoupe = nullptr;

	stack.find_first_from_bottom(real_decoupe);
        if(real_decoupe != nullptr)
        {
	    slice_layout tmp = real_decoupe->get_slicing();

            sub_file_size = tmp.other_size;
            first_file_size = tmp.first_size;
            if(! real_decoupe->get_total_file_number(total_file_number))
		total_file_number = 0;
	    if(! real_decoupe->get_last_file_size(last_file_size))
		last_file_size = 0;
	    return true;
        }
        else
            return false;
    }

    shared_ptr<entrepot> archive::i_archive::get_entrepot()
    {
	shared_ptr<entrepot> ret;
	sar *real_decoupe = nullptr;

	stack.find_first_from_bottom(real_decoupe);
	if(real_decoupe != nullptr)
	{
	    ret = real_decoupe->get_entrepot();
	    if(!ret)
		throw SRC_BUG;
	}

	return ret;
    }

    infinint archive::i_archive::get_level2_size()
    {
	generic_file *level1 = stack.get_by_label(LIBDAR_STACK_LABEL_LEVEL1);

	if(dynamic_cast<trivial_sar *>(level1) == nullptr)
	{
	    stack.skip_to_eof();
	    return stack.get_position();
	}
	else
	    return 0;
    }

    const cat_directory *archive::i_archive::get_dir_object(const string & dir) const
    {
	const cat_directory *parent = nullptr;
	const cat_nomme *tmp_ptr = nullptr;

	parent = get_cat().get_contenu();
	if(parent == nullptr)
	    throw SRC_BUG;

	if(dir != "")
	{
	    path search = dir;
	    string tmp;
	    bool loop = true;

	    while(loop)
	    {
		loop = search.pop_front(tmp);
		if(!loop) // failed because now, search is a one level path
		    tmp = search.basename();
		if(parent->search_children(tmp, tmp_ptr))
		    parent = dynamic_cast<const cat_directory *>(tmp_ptr);
		else
		    parent = nullptr;

		if(parent == nullptr)
		    throw Erange("archive::i_archive::get_children_in_table", tools_printf("%S entry does not exist", &dir));
	    }
	}
	    // else returning the root of the archive

	return parent;
    }

    static bool local_check_dirty_seq(escape *ptr)
    {
	bool ret;

	if(ptr != nullptr)
	{
	    bool already_set = ptr->is_unjumpable_mark(escape::seqt_file);

	    if(!already_set)
		ptr->add_unjumpable_mark(escape::seqt_file);
	    ret = ptr != nullptr && ptr->skip_to_next_mark(escape::seqt_dirty, true);
	    if(!already_set)
		ptr->remove_unjumpable_mark(escape::seqt_file);
	}
	else
	    ret = false;

	return ret;
    }

    static void check_libgcrypt_hash_bug(user_interaction & dialog,
					 hash_algo hash,
					 const infinint & first_file_size,
					 const infinint & file_size)
    {
#if CRYPTO_AVAILABLE
	if(hash != hash_algo::none && !crypto_min_ver_libgcrypt_no_bug())
	{
	    const infinint limit = tools_get_extended_size("256G", 1024);
	    if(file_size >= limit || first_file_size >= limit)
		dialog.pause(tools_printf(gettext("libgcrypt version < %s. Ligcrypt used has a bug that leads md5 and sha1 hash results to be erroneous for files larger than 256 Gio (gibioctet), do you really want to spend CPU cycles calculating a useless hash?"), MIN_VERSION_GCRYPT_HASH_BUG));
	}
#endif
    }

} // end of namespace
