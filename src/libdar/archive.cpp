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
#include "compressor.hpp"
#include "nls_swap.hpp"
#include "thread_cancellation.hpp"
#include "erreurs_ext.hpp"
#include "cache.hpp"
#include "entrepot.hpp"
#include "entrepot_local.hpp"
#include "crypto_sym.hpp"
#include "cat_all_entrees.hpp"
#include "zapette.hpp"

#define ARCHIVE_NOT_EXPLOITABLE "Archive of reference given is not exploitable"

using namespace std;

namespace libdar
{

    static void check_libgcrypt_hash_bug(user_interaction & dialog, hash_algo hash, const infinint & first_file_size, const infinint & file_size);

	// opens an already existing archive

    archive::archive(user_interaction & dialog,
                     const path & chem,
		     const string & basename,
		     const string & extension,
		     const archive_options_read & options)
    {
        NLS_SWAP_IN;
        try
        {
	    entrepot *where = options.get_entrepot().clone();
	    bool info_details = options.get_info_details();

	    if(where == nullptr)
		throw Ememory("archive::archive");

	    pool = nullptr;
	    cat = nullptr;
	    freed_and_checked = false;

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

		init_pool();

		try
		{
		    if(info_details)
			dialog.printf(gettext("Opening archive %s ..."), basename.c_str());

			// we open the main archive to get the different layers (level1, scram and level2).
		    macro_tools_open_archive(dialog,
					     pool,
					     *where,
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
					     options.get_multi_threaded(),
					     options.get_header_only());

		    if(options.get_header_only())
		    {
			ver.display(dialog);
			throw Erange("archive::achive",
				     gettext("header only mode asked"));
		    }

		    pdesc = pile_descriptor(&stack);

		    if(options.is_external_catalogue_set())
		    {
			pile ref_stack;
			entrepot *ref_where = options.get_ref_entrepot().clone();
			if(ref_where == nullptr)
			    throw Ememory("archive::archive");

			if(info_details)
			    dialog.printf(gettext("Opening the archive of reference %s to retreive the isolated catalog ... "), options.get_ref_basename().c_str());


			try
			{
			    ref_where->set_location(options.get_ref_path());
			    try
			    {
				slice_layout ignored;

				if(options.get_ref_basename() == "-")
				    throw Erange("archive::archive", gettext("Reading the archive of reference from pipe or standard input is not possible"));
				if(options.get_ref_basename() == "+")
				    throw Erange("archive::archive", gettext("The basename '+' is reserved for special a purpose that has no meaning in this context"));

				    // we open the archive of reference also to get its different layers (ref_stack)
				macro_tools_open_archive(dialog,
							 pool,
							 *ref_where,
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
							 options.get_multi_threaded(),
							 false);
				    // we do not comparing the signatories of the archive of reference with the current archive
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
				throw Erange("archive::archive", string(gettext("Error while opening the archive of reference: ")) + e.get_message());
			    }
			}
			catch(...)
			{
			    if(ref_where != nullptr)
				delete ref_where;
			    throw;
			}
			if(ref_where != nullptr)
			    delete ref_where;

			    // fetching the catalogue in the archive of reference, making it point on the main archive layers.

			ref_ver.set_compression_algo(ver.get_compression_algo()); // set the default encryption to use to the one of the main archive

			if(info_details)
			    dialog.warning(gettext("Loading isolated catalogue in memory..."));

			cat = macro_tools_get_derivated_catalogue_from(dialog,
								       pool,
								       stack,
								       ref_stack,
								       ref_ver,
								       options.get_info_details(),
								       local_cat_size,
								       ref_second_terminateur_offset,
								       tmp2_signatories,
								       false); // never relaxed checking for external catalogue
			if(!same_signatories(tmp1_signatories, tmp2_signatories))
			    dialog.pause(gettext("Archive of reference is not signed properly (no the same signatories for the archive and the internal catalogue), do we continue?"));
			if(cat == nullptr)
			    throw SRC_BUG;

			    // checking for compatibility of the archive of reference with this archive data_name

			if(get_layer1_data_name() != get_catalogue_data_name())
			    throw Erange("archive::archive", gettext("The archive and the isolated catalogue do not correspond to the same data, they are thus incompatible between them"));

			    // we drop delta signature as they refer to the isolated catalogue container/archive
			    // to avoid having to fetch them at wrong offset from the original archive we created
			    // this isolated catalogue from. Anyway we do not need them to read an archive, we
			    // only need delta signatures for archive differential backup, in which case we use the
			    // original archive *or* the isolated catalogue *alone*
			cat->drop_delta_signatures();
		    }
		    else // no isolated archive to fetch the catalogue from
		    {
			try
			{
			    if(!options.get_sequential_read())
			    {
				if(info_details)
				    dialog.warning(gettext("Loading catalogue into memory..."));
				cat = macro_tools_get_catalogue_from(dialog,
								     pool,
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
					dialog.pause(msg);
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
					    dialog.warning(gettext("No data found in that archive, sequentially reading the catalogue found at the end of the archive..."));
					pdesc.stack->flush_read_above(pdesc.esc);

					contextual *layer1 = nullptr;
					label lab = label_zero;
					stack.find_first_from_bottom(layer1);
					if(layer1 != nullptr)
					    lab = layer1->get_data_name();

					cat = macro_tools_read_catalogue(dialog,
									 pool,
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
						dialog.pause(msg);
					    else
						throw Edata(msg);
					}
				    }
				    else
				    {
					if(info_details)
					    dialog.warning(gettext("The catalogue will be filled while sequentially reading the archive, preparing the data structure..."));
					cat = new (pool) escape_catalogue(dialog,
									  pdesc,
									  ver,
									  gnupg_signed,
									  options.get_lax());
				    }
				    if(cat == nullptr)
					throw Ememory("archive::archive");
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
				    dialog.printf(gettext("LAX MODE: The end of the archive is corrupted, cannot get the archive contents (the \"catalogue\")"));
				    dialog.pause(gettext("LAX MODE: Do you want to bypass some sanity checks and try again reading the archive contents (this may take some time, this may also fail)?"));
				    try
				    {
					label tmp;
					tmp.clear(); // this way we do not modify the catalogue data name even in lax mode
					cat = macro_tools_lax_search_catalogue(dialog,
									       pool,
									       stack,
									       ver.get_edition(),
									       ver.get_compression_algo(),
									       options.get_info_details(),
									       false, // even partial
									       tmp);
				    }
				    catch(Erange & e)
				    {
					dialog.printf(gettext("LAX MODE: Could not find a whole catalogue in the archive. If you have an isolated catalogue, stop here and use it as backup of the internal catalogue, else continue but be advised that all data will not be able to be retrieved..."));
					dialog.pause(gettext("LAX MODE: Do you want to try finding portions of the original catalogue if some remain (this may take even more time and in any case, it will only permit to recover some files, at most)?"));
					cat = macro_tools_lax_search_catalogue(dialog,
									       pool,
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
			check_gnupg_signed(dialog);
		    exploitable = true;
		}
		catch(...)
		{
		    if(where != nullptr)
			delete where;
		    throw;
		}
		if(where != nullptr)
		    delete where;
	    }
	    catch(...)
	    {
		free_all();
		throw;
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

	// creates a new archive

    archive::archive(user_interaction & dialog,
                     const path & fs_root,
                     const path & sauv_path,
                     const string & filename,
                     const string & extension,
		     const archive_options_create & options,
                     statistics * progressive_report)
    {
        NLS_SWAP_IN;
        try
        {
	    pool = nullptr;
	    cat = nullptr;
	    freed_and_checked = false;
	    init_pool();

	    try
	    {
		entrepot *sauv_path_t = options.get_entrepot().clone();
		if(sauv_path_t == nullptr)
		    throw Ememory("archive::archive");
		sauv_path_t->set_user_ownership(options.get_slice_user_ownership());
		sauv_path_t->set_group_ownership(options.get_slice_group_ownership());
		sauv_path_t->set_location(sauv_path);

		try
		{
		    sequential_read = false; // updating the archive field
		    (void)op_create_in(dialog,
				       oper_create,
				       tools_relative2absolute_path(fs_root, tools_getcwd()),
				       *sauv_path_t,
				       options.get_reference(),
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
				       options.get_same_fs(),
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
				       options.get_multi_threaded(),
				       options.get_delta_signature(),
				       options.get_has_delta_mask_been_set(),
				       options.get_delta_mask(),
				       options.get_delta_sig_min_size(),
				       options.get_delta_diff(),
				       options.get_ignored_as_symlink(),
				       progressive_report);
		    exploitable = false;
		    stack.terminate();
		}
		catch(...)
		{
		    if(sauv_path_t != nullptr)
			delete sauv_path_t;
		    throw;
		}
		if(sauv_path_t != nullptr)
		    delete sauv_path_t;
	    }
	    catch(...)
	    {
		free_all();
		throw;
	    }
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }


	// DEPRECATED way to isolate an archive DO NOT USE IT unless you know what you are doing.
	// RATHER use op_isolate() on an existing archive object
    archive::archive(user_interaction & dialog,
		     const path & sauv_path,
		     archive *ref_arch,
		     const string & filename,
		     const string & extension,
		     const archive_options_isolate & options)
    {
	pool = nullptr;
	cat = nullptr;
	local_cat_size = 0;
	exploitable = false;
	lax_read_mode = false;
	sequential_read = false;
	freed_and_checked = true;

	if(ref_arch == nullptr)
	    throw Elibcall("deprecated isolate constructor", "nullptr argument given to \"ref_arch\"");

	ref_arch->op_isolate(dialog,
			     sauv_path,
			     filename,
			     extension,
			     options);
    }


	// merge constructor

    archive::archive(user_interaction & dialog,
		     const path & sauv_path,
		     archive *ref_arch1,
		     const string & filename,
		     const string & extension,
		     const archive_options_merge & options,
		     statistics * progressive_report)
    {
	statistics st = false;
	statistics *st_ptr = progressive_report == nullptr ? &st : progressive_report;
	catalogue *ref_cat1 = nullptr;
	catalogue *ref_cat2 = nullptr;
	archive *ref_arch2 = options.get_auxilliary_ref();
	compression algo_kept = none;
	entrepot *sauv_path_t = options.get_entrepot().clone();
	entrepot_local *sauv_path_t_local = dynamic_cast<entrepot_local *>(sauv_path_t);
	freed_and_checked = false;

	NLS_SWAP_IN;
	try
	{
	    pool = nullptr;
	    cat = nullptr;
	    init_pool();

	    try
	    {
		if(sauv_path_t == nullptr)
		    throw Ememory("archive::archive(merge)");
		sauv_path_t->set_user_ownership(options.get_slice_user_ownership());
		sauv_path_t->set_group_ownership(options.get_slice_group_ownership());
		sauv_path_t->set_location(sauv_path);

		try
		{
		    exploitable = false;
		    sequential_read = false; // updating the archive field

			// sanity checks as much as possible to avoid libdar crashing due to bad arguments
			// useless arguments are not reported.

		    if(options.get_compression_level() > 9 || options.get_compression_level() < 1)
			throw Elibcall("op_merge", gettext("Compression_level must be between 1 and 9 included"));
		    if(options.get_slice_size().is_zero() && !options.get_first_slice_size().is_zero())
			throw Elibcall("op_merge", gettext("\"first_file_size\" cannot be different from zero if \"file_size\" is equal to zero"));
		    if(options.get_crypto_size() < 10 && options.get_crypto_algo() != crypto_none)
			throw Elibcall("op_merge", gettext("Crypto block size must be greater than 10 bytes"));

		    check_libgcrypt_hash_bug(dialog, options.get_hash_algo(), options.get_first_slice_size(), options.get_slice_size());

		    if(ref_arch1 != nullptr)
			if(ref_arch1->only_contains_an_isolated_catalogue())
				// convert all data to unsaved
			    ref_arch1->set_to_unsaved_data_and_FSA();

		    if(ref_arch2 != nullptr)
			if(ref_arch2->only_contains_an_isolated_catalogue())
			    ref_arch2->set_to_unsaved_data_and_FSA();

			// end of sanity checks

		    sauv_path_t->set_location(sauv_path);

		    if(!options.get_empty() && sauv_path_t_local != nullptr)
			tools_avoid_slice_overwriting_regex(dialog,
							    *sauv_path_t,
							    filename,
							    extension,
							    options.get_info_details(),
							    options.get_allow_over(),
							    options.get_warn_over(),
							    options.get_empty());

		    if(ref_arch1 == nullptr)
			if(ref_arch2 == nullptr)
			    throw Elibcall("archive::archive[merge]", string(gettext("Both reference archive are nullptr, cannot merge archive from nothing")));
			else
			    if(ref_arch2->cat == nullptr)
				throw SRC_BUG; // an archive should always have a catalogue available
			    else
				if(ref_arch2->exploitable)
				    ref_cat1 = ref_arch2->cat;
				else
				    throw Elibcall("archive::archive[merge]", gettext(ARCHIVE_NOT_EXPLOITABLE));
		    else
			if(ref_arch2 == nullptr)
			    if(ref_arch1->cat == nullptr)
				throw SRC_BUG; // an archive should always have a catalogue available
			    else
				if(ref_arch1->exploitable)
				    ref_cat1 = ref_arch1->cat;
				else
				    throw Elibcall("archive::archive[merge]", gettext(ARCHIVE_NOT_EXPLOITABLE));
			else // both catalogues available
			{
			    if(!ref_arch1->exploitable || !ref_arch2->exploitable)
				throw Elibcall("archive::archive[merge]", gettext(ARCHIVE_NOT_EXPLOITABLE));
			    if(ref_arch1->cat == nullptr)
				throw SRC_BUG;
			    if(ref_arch2->cat == nullptr)
				throw SRC_BUG;
			    ref_cat1 = ref_arch1->cat;
			    ref_cat2 = ref_arch2->cat;
			    if(ref_arch1->ver.get_compression_algo() != ref_arch2->ver.get_compression_algo() && ref_arch1->ver.get_compression_algo() != none && ref_arch2->ver.get_compression_algo() != none && options.get_keep_compressed())
				throw Efeature(gettext("the \"Keep file compressed\" feature is not possible when merging two archives using different compression algorithms (This is for a future version of dar). You can still merge these two archives but without keeping file compressed (thus you will probably like to use compression (-z or -y options) for the resulting archive"));
			}

		    if(options.get_keep_compressed())
		    {
			if(ref_arch1 == nullptr)
			    throw SRC_BUG;

			algo_kept = ref_arch1->ver.get_compression_algo();
			if(algo_kept == none && ref_cat2 != nullptr)
			{
			    if(ref_arch2 == nullptr)
				throw SRC_BUG;
			    else
				algo_kept = ref_arch2->ver.get_compression_algo();
			}
		    }

		    if(ref_cat1 == nullptr)
			throw SRC_BUG;

		    if(options.get_delta_signature())
		    {
			if(options.get_keep_compressed() && options.get_has_delta_mask_been_set())
			    throw Elibcall("op_merge", gettext("Cannot calculate delta signature when merging if keep compressed is asked"));
			if(options.get_sparse_file_min_size().is_zero() && options.get_has_delta_mask_been_set())
			    dialog.warning(gettext("To calculate delta signatures of files saved as sparse files, you need to activate sparse file detection mechanism with merging operation"));
		    }

			// then we call op_create_in_sub which will call filter_merge operation to build the archive described by the catalogue
		    op_create_in_sub(dialog,
				     oper_merge,
				     path(FAKE_ROOT),
				     *sauv_path_t,
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
				     false,   // same_fs
				     cat_inode::cf_all,        // what_to_check
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
				     options.get_multi_threaded(),
				     options.get_delta_signature(),
				     options.get_has_delta_mask_been_set(), // build delta sig
				     options.get_delta_mask(), // delta_mask
				     options.get_delta_sig_min_size(),
				     false,   // delta diff
				     set<string>(),            // empty list
				     st_ptr);
		    exploitable = false;
		    stack.terminate();
		}
		catch(...)
		{
		    if(sauv_path_t != nullptr)
			delete sauv_path_t;
		    throw;
		}

		if(sauv_path_t != nullptr)
		    delete sauv_path_t;
		sauv_path_t = nullptr;
	    }
	    catch(...)
	    {
		free_all();
		throw;
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    archive::archive(user_interaction & dialog,
		     const path & chem_src,
		     const string & basename_src,
		     const string & extension_src,
		     const archive_options_read & options_read,
		     const path & chem_dst,
		     const string & basename_dst,
		     const string & extension_dst,
		     const archive_options_repair & options_repair)
    {
	archive_options_read my_options_read = options_read;
	bool initial_pause = (options_read.get_entrepot() == options_repair.get_entrepot() && chem_src == chem_dst);
	statistics not_filled;

	    ////
	    // initializing object fields

	    // stack will be set by op_create_in_sub()
	    // ver will be set by op_create_in_sub()
	init_pool(); // initializes pool
	cat = nullptr; // will be set by op_create_in_sub()
	exploitable = false;
	lax_read_mode = false;
	sequential_read = false;
	freed_and_checked = false;
	    // gnupg_signed is not used while creating an archive
	    // slices will be set by op_create_in_sub()
	    // local_cat_size not used while creating an archive

	    ////
	    // initial parameter setup

	entrepot *sauv_path_t = options_repair.get_entrepot().clone();
	if(sauv_path_t == nullptr)
	    throw Ememory("archive::archive(repair)");

	try
	{
	    sauv_path_t->set_user_ownership(options_repair.get_slice_user_ownership());
	    sauv_path_t->set_group_ownership(options_repair.get_slice_group_ownership());
	    sauv_path_t->set_location(chem_dst);


		/////
		// opening the source archive in sequential read mode

	    my_options_read.set_sequential_read(true);

	    archive src = archive(dialog,
				  chem_src,
				  basename_src,
				  extension_src,
				  my_options_read);

	    if(src.cat == nullptr)
		throw SRC_BUG;

	    op_create_in_sub(dialog,
			     oper_repair,
			     chem_dst,
			     *sauv_path_t,
			     src.cat,             // ref1
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
			     src.ver.get_compression_algo(),
			     9,                   // we keep the data compressed this parameter has no importance
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
			     false,               // same_fs
			     cat_inode::cf_all,   // comparison_fields
			     false,               // snapshot
			     false,               // cache_directory_tagging,
			     true,                // keep_compressed, always
			     0,                   // fixed_date
			     options_repair.get_slice_permission(),
			     0,                   // repeat_count
			     0,                   // repeat_byte
			     false,               // decremental
			     options_repair.get_sequential_marks(),
			     false,               // security_check
			     0,                   // sparse_file_min_size (disabling hole detection)
			     options_repair.get_user_comment(),
			     options_repair.get_hash_algo(),
			     options_repair.get_slice_min_digits(),
			     "",                  // backup_hook_file_execute
			     bool_mask(true),     // backup_hook_file_mask
			     false,               // ignore_unknown
			     all_fsa_families(),  // fsa_scope
			     options_repair.get_multi_threaded(),
			     true,                // delta_signature
			     false,               // build_delta_signature
			     bool_mask(true),     // delta_mask
			     0,                   // delta_sig_min_size
			     false,               // delta_diff
			     set<string>(),       // ignored_symlinks
			     &not_filled);        // statistics

		// stealing src's catalogue, our's is still empty at this step
	    catalogue *tmp = cat;
	    cat = src.cat;
	    src.cat = tmp;

	    dialog.printf(gettext("Archive repairing completed. WARNING! it is strongly advised to test the resulting archive before removing the damaged one"));
	}
	catch(...)
	{
	    if(sauv_path_t != nullptr)
	    {
		delete sauv_path_t;
		sauv_path_t = nullptr;
	    }
	    throw;
	}
	if(sauv_path_t != nullptr)
	{
	    delete sauv_path_t;
	    sauv_path_t = nullptr;
	}
    }


    statistics archive::op_extract(user_interaction & dialog,
                                   const path & fs_root,
				   const archive_options_extract & options,
				   statistics * progressive_report)
    {
        statistics st = false;  // false => no lock for this internal object
	statistics *st_ptr = progressive_report == nullptr ? &st : progressive_report;

        NLS_SWAP_IN;
        try
        {
                // sanity checks

	    if(freed_and_checked)
		throw Erange("catalogue::op_extract", "catalogue::free_and_check_memory() method has been called, this object is no more usable");
            if(!exploitable)
                throw Elibcall("op_extract", gettext("This archive is not exploitable, check documentation for more"));

	    check_against_isolation(dialog, lax_read_mode);
		// this avoid to try extracting archive directly from an isolated catalogue
		// the isolated catalogue can be used to extract data (since format "08") but only
		// associated with the real plain archive, not alone.

		// end of sanity checks

	    fs_root.explode_undisclosed();
	    enable_natural_destruction();

		/// calculating and setting the " recursive_has_changed" fields of directories to their values
	    if(options.get_empty_dir() == false)
		get_cat().launch_recursive_has_changed_update();
		/// we can now use the cat_directory::get_recursive_has_changed() to avoid recursion in a directory where
		/// no file has been saved.

	    try
	    {
		filtre_restore(dialog,
			       pool,
			       options.get_selection(),
			       options.get_subtree(),
			       get_cat(),
			       tools_relative2absolute_path(fs_root, tools_getcwd()),
			       options.get_warn_over(),
			       options.get_info_details(),
			       options.get_display_treated(),
			       options.get_display_treated_only_dir(),
			       options.get_display_skipped(),
			       *st_ptr,
			       options.get_ea_mask(),
			       options.get_flat(),
			       options.get_what_to_check(),
			       options.get_warn_remove_no_match(),
			       options.get_empty(),
			       options.get_empty_dir(),
			       options.get_overwriting_rules(),
			       options.get_dirty_behavior(),
			       options.get_only_deleted(),
			       options.get_ignore_deleted(),
			       options.get_fsa_scope());
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
		dialog.warning(msg);
		throw Edata(msg);
	    }
        }
        catch(...)
        {
            NLS_SWAP_OUT;
	    if(sequential_read)
		exploitable = false;
            throw;
        }
        NLS_SWAP_OUT;
	if(sequential_read)
	    exploitable = false;

        return *st_ptr;
    }

    void archive::summary(user_interaction & dialog)
    {
        NLS_SWAP_IN;

        try
        {

		// sanity checks

	    if(freed_and_checked)
		throw Erange("catalogue::summary", "catalogue::free_and_check_memory() method has been called, this object is no more usable");
	    if(!exploitable)
		throw Elibcall("summary", gettext("This archive is not exploitable, check the archive class usage in the API documentation"));

		// end of sanity checks

	    infinint sub_file_size;
	    infinint first_file_size;
	    infinint last_file_size, file_number;
	    infinint cat_size = get_cat_size();
	    get_header().display(dialog);
	    if(!cat_size.is_zero())
		dialog.printf(gettext("Catalogue size in archive            : %i bytes\n"), &cat_size);
	    else
		dialog.printf(gettext("Catalogue size in archive            : N/A\n"));
	    dialog.printf("\n");

	    try
	    {
		if(get_sar_param(sub_file_size, first_file_size, last_file_size, file_number))
		{
		    dialog.printf(gettext("Archive is composed of %i file(s)\n"), &file_number);
		    if(file_number == 1)
			dialog.printf(gettext("File size: %i bytes\n"), &last_file_size);
		    else
		    {
			if(first_file_size != sub_file_size)
			    dialog.printf(gettext("First file size       : %i bytes\n"), &first_file_size);
			dialog.printf(gettext("File size             : %i bytes\n"), &sub_file_size);
			dialog.printf(gettext("Last file size        : %i bytes\n"), &last_file_size);
		    }
		    if(file_number > 1)
		    {
			infinint total = first_file_size + (file_number-2)*sub_file_size + last_file_size;
			dialog.printf(gettext("Archive total size is : %i bytes\n"), &total);
		    }
		}
		else // not reading from a sar
		{
		    infinint arch_size = get_level2_size();
		    if(!arch_size.is_zero())
		    {
			dialog.printf(gettext("Archive size is: %i bytes\n"), &arch_size);
			dialog.printf(gettext("Previous archive size does not include headers present in each slice\n"));
		    }
		    else
			dialog.printf(gettext("Archive size is unknown (reading from a pipe)"));
		}
	    }
	    catch(Erange & e)
	    {
		string msg = e.get_message();
		dialog.printf("%S\n", &msg);
	    }

	    entree_stats stats = get_cat().get_stats();

	    if(get_cat().get_contenu() == nullptr)
		throw SRC_BUG;

	    infinint g_storage_size = get_cat().get_contenu()->get_storage_size();
	    infinint g_size =  get_cat().get_contenu()->get_size();

	    if(g_size < g_storage_size)
	    {
		infinint delta = g_storage_size - g_size;
		dialog.printf(gettext("The overall archive size includes %i byte(s) wasted due to bad compression ratio"), &delta);
	    }
	    else
		dialog.warning(string(gettext("The global data compression ratio is: "))
			       + tools_get_compression_ratio(g_storage_size,
							     g_size,
							     true));

	    if(only_contains_an_isolated_catalogue())
		dialog.printf(gettext("\nWARNING! This archive only contains the contents of another archive, it can only be used as reference for differential backup or as rescue in case of corruption of the original archive's content. You cannot restore any data from this archive alone\n"));

	    stats.listing(dialog);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    if(sequential_read)
		exploitable = false;
	    throw;
	}
	NLS_SWAP_OUT;
	if(sequential_read)
	    exploitable = false;
    }

    void archive::op_listing(user_interaction & dialog,
			     const archive_options_listing & options)
    {
        NLS_SWAP_IN;

        try
        {
	    slice_layout used_layout = slices;
		// by default we use the slice layout of the current archive,
		// this is modified if the current archive is an isolated catalogue
		// in archive format 9 or greater, then we use the slice layout contained
		// in the archive header/trailer which is a copy of the one of the archive of reference
		// a warning is issued in that case


	    if(freed_and_checked)
		throw Erange("catalogue::op_listing", "catalogue::free_and_check_memory() method has been called, this object is no more usable");
            enable_natural_destruction();
            try
            {
                switch(options.get_list_mode())
		{
		case archive_options_listing::normal:
		    get_cat().tar_listing(only_contains_an_isolated_catalogue(),
					  options.get_selection(),
					  options.get_subtree(),
					  options.get_filter_unsaved(),
					  options.get_display_ea(),
					  options.get_sizes_in_bytes(),
					  "");
		    break;
		case archive_options_listing::tree:
		    get_cat().listing(only_contains_an_isolated_catalogue(),
				      options.get_selection(),
				      options.get_subtree(),
				      options.get_filter_unsaved(),
				      options.get_display_ea(),
				      options.get_sizes_in_bytes(),
				      "");
		    break;
		case archive_options_listing::xml:
		    get_cat().xml_listing(only_contains_an_isolated_catalogue(),
					  options.get_selection(),
					  options.get_subtree(),
					  options.get_filter_unsaved(),
					  options.get_display_ea(),
					  options.get_sizes_in_bytes(),
					  "");
		    break;
		case archive_options_listing::slicing:
		    if(only_contains_an_isolated_catalogue())
		    {
			if(ver.get_slice_layout() != nullptr)
			{
			    used_layout = *ver.get_slice_layout();
			    if(options.get_user_slicing(used_layout.first_size, used_layout.other_size))
			    {
				if(options.get_info_details())
				    dialog.printf(gettext("Using user provided modified slicing (first slice = %i bytes, other slices = %i bytes)"), &used_layout.first_size, &used_layout.other_size);
			    }
			    else
				dialog.warning(gettext("Using the slice layout of the archive of reference recorded at the time this isolated catalogue was done\n Note: if this reference has been resliced this isolated catalogue has been created, the resulting slicing information given here will be wrong and will probably lead to an error. Check documentation to know hos to manually specify the slicing to use"));
			}
			else // no slicing of the archive of reference stored in this isolated catalogue's header/trailer
			{
			    if(ver.get_edition() >= 9)
				throw SRC_BUG; // starting revision 9 isolated catalogue should always contain
				// the slicing of the archive of reference, even if that reference is using an archive format
				// older than version 9.

			    if(options.get_user_slicing(used_layout.first_size, used_layout.other_size))
				dialog.warning(gettext("Warning: No slice layout of the archive of reference has been recorded in this isolated catalogue. The additional slicing information you provided may still lead the operation to fail because the archive has an _unsupported_ (too old) format for this feature"));
			    else
				throw Erange("archive::op_listing", gettext("No slice layout of the archive of reference for the current isolated catalogue is available, cannot provide slicing information, aborting"));
			}
		    }
		    get_cat().slice_listing(only_contains_an_isolated_catalogue(), options.get_selection(), options.get_subtree(), used_layout);
		    break;
		default:
		    throw SRC_BUG;
                }
            }
            catch(Euser_abort & e)
            {
                disable_natural_destruction();
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
            NLS_SWAP_OUT;
	    if(sequential_read)
		exploitable = false;
            throw;
        }
        NLS_SWAP_OUT;
	if(sequential_read)
	    exploitable = false;
    }

    statistics archive::op_diff(user_interaction & dialog,
                                const path & fs_root,
				const archive_options_diff & options,
				statistics * progressive_report)
    {
        statistics st = false;  // false => no lock for this internal object
	statistics *st_ptr = progressive_report == nullptr ? &st : progressive_report;
	bool isolated_mode = false;

        NLS_SWAP_IN;
        try
        {

                // sanity checks

	    if(freed_and_checked)
		throw Erange("catalogue::op_diff", "catalogue::free_and_check_memory() method has been called, this object is no more usable");
            if(!exploitable)
                throw Elibcall("op_diff", gettext("This archive is not exploitable, check documentation for more"));

	    try
	    {
		check_against_isolation(dialog, lax_read_mode);
	    }
	    catch(Erange & e)
	    {
		dialog.warning("This archive contains an isolated catalogue, only meta data can be used for comparison, CRC will be used to compare data of delta signature if present. Warning: Succeeding this comparison does not mean there is no difference as two different files may have the same CRC or the same delta signature");
		isolated_mode = true;
	    }
                // end of sanity checks

	    fs_root.explode_undisclosed();
            enable_natural_destruction();
            try
            {
                filtre_difference(dialog,
				  pool,
				  options.get_selection(),
				  options.get_subtree(),
				  get_cat(),
				  tools_relative2absolute_path(fs_root, tools_getcwd()),
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
				  isolated_mode);
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
                dialog.warning(msg);
                throw Edata(msg);
            }
        }
        catch(...)
        {
            NLS_SWAP_OUT;
	    if(sequential_read)
		exploitable = false;
            throw;
        }

        NLS_SWAP_OUT;
	if(sequential_read)
	    exploitable = false;

        return *st_ptr;
    }

    statistics archive::op_test(user_interaction & dialog,
				const archive_options_test & options,
				statistics * progressive_report)
    {
        statistics st = false;  // false => no lock for this internal object
	statistics *st_ptr = progressive_report == nullptr ? &st : progressive_report;
	bool isolated = false;

        NLS_SWAP_IN;
        try
        {

                // sanity checks

	    if(freed_and_checked)
		throw Erange("catalogue::op_test", "catalogue::free_and_check_memory() method has been called, this object is no more usable");
            if(!exploitable)
                throw Elibcall("op_test", gettext("This archive is not exploitable, check the archive class usage in the API documentation"));

	    try
	    {
		check_against_isolation(dialog, lax_read_mode);
	    }
	    catch(Erange & e)
	    {
		    // no data/EA are available in this archive,
		    // we can return normally at this point
		    // as the catalogue has already been read
		    // thus all that could be tested has been tested
		dialog.warning(gettext("WARNING! This is an isolated catalogue, no data or EA is present in this archive, only the catalogue structure can be checked"));
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
			filtre_test(dialog,
				    pool,
				    options.get_selection(),
				    options.get_subtree(),
				    get_cat(),
				    options.get_info_details(),
				    options.get_display_treated(),
				    options.get_display_treated_only_dir(),
				    options.get_display_skipped(),
				    options.get_empty(),
				    *st_ptr);
		}
		catch(Erange & e)
		{
		    dialog.warning(gettext("A problem occurred while reading this archive contents: ") + e.get_message());
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
                dialog.warning(msg);
                throw Edata(msg);
            }
        }
        catch(...)
        {
            NLS_SWAP_OUT;
	    if(sequential_read)
		exploitable = false;
            throw;
        }
        NLS_SWAP_OUT;
	if(sequential_read)
	    exploitable = false;

        return *st_ptr;
    }


    void archive::op_isolate(user_interaction & dialog,
			     const path &sauv_path,
			     const string & filename,
			     const string & extension,
			     const archive_options_isolate & options)
    {
        NLS_SWAP_IN;
        try
        {
	    entrepot *sauv_path_t = options.get_entrepot().clone();
	    if(sauv_path_t == nullptr)
		throw Ememory("archive::archive");
	    sauv_path_t->set_user_ownership(options.get_slice_user_ownership());
	    sauv_path_t->set_group_ownership(options.get_slice_group_ownership());
	    sauv_path_t->set_location(sauv_path);

	    try
	    {
		pile layers;
		header_version isol_ver;
		label isol_data_name;
		label internal_name;
		slice_layout isol_slices;

		if(!exploitable && options.get_delta_signature())
		    throw Erange("archive::op_isolate", gettext("Isolation with delta signature is not possible on a just created archive (on-fly isolation)"));

		do
		{
		    isol_data_name.generate_internal_filename();
		}
		while(isol_data_name == cat->get_data_name());
		internal_name = isol_data_name;


		macro_tools_create_layers(dialog,
					  layers,
					  isol_ver,
					  isol_slices,
					  &slices, // giving our slice_layout as reference to be stored in the archive header/trailer
					  get_pool(),
					  *sauv_path_t,
					  filename,
					  extension,
					  options.get_allow_over(),
					  options.get_warn_over(),
					  options.get_info_details(),
					  options.get_pause(),
					  options.get_compression(),
					  options.get_compression_level(),
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
					  options.get_multi_threaded());

		if(cat == nullptr)
		    throw SRC_BUG;

		    // note:
		    // an isolated catalogue keep the data, EA and FSA pointers of the archive they come from
		    // but if they have delta signature their offset is those of the isolated catalogue archive
		    // not/no more those of the archive the catalogue has been isolated from.
		    // Using an isolated catalogue as backup of the internal catalogue stays possible but not
		    // for the delta_signature that are ignored by libdar in that situation. However delta_signature
		    // are only used at archive creation, thus reading an other archive as reference, either the
		    // original archive with all its contents, or an isolated catalogue with metadata and eventually
		    // delta_signatures.

		if(options.get_delta_signature())
		{
		    pile_descriptor pdesc = & layers;
		    cat->transfer_delta_signatures(pdesc,
						   sequential_read,
						   options.get_has_delta_mask_been_set(),
						   options.get_delta_mask(),
						   options.get_delta_sig_min_size());
		}
		else
			// we drop the delta signature as they will never be used
			// because none will be in the isolated catalogue
			// and that an isolated catalogue as backup of an internal
			// catalogue cannot rescue the delta signature (they have
			// to be in the isolated catalogue)
		    cat->drop_delta_signatures();



		if(isol_data_name == cat->get_data_name())
		    throw SRC_BUG;
		    // data_name generated just above by slice layer
		    // should never equal the data_name of the catalogue
		    // when performing isolation

		macro_tools_close_layers(dialog,
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
		if(sauv_path_t != nullptr)
		    delete sauv_path_t;
		throw;
	    }

	    if(sauv_path_t != nullptr)
		delete sauv_path_t;
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }


    bool archive::get_children_of(user_interaction & dialog,
                                  const string & dir)
    {
	bool ret;
        NLS_SWAP_IN;
        try
        {
	    if(freed_and_checked)
		throw Erange("catalogue::get_children_of", "catalogue::free_and_check_memory() method has been called, this object is no more usable");
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
			// here we have a plain archive, doing the test operation
			// is the simplest way to read the whole archive and thus get its contents
			// (i.e.: the catalogue)
		    (void)op_test(dialog, archive_options_test(), nullptr);
	    }

		// OK, now that we have the whole catalogue available in memory, let's rock!

            ret = get_cat().get_contenu()->callback_for_children_of(dialog, dir);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

	return ret;
    }


    const vector<list_entry> archive::get_children_in_table(const string & dir) const
    {
	vector <list_entry> ret;

	NLS_SWAP_IN;
        try
        {
	    const cat_directory * parent = get_dir_object(dir);
	    const cat_nomme *tmp_ptr = nullptr;

	    if(freed_and_checked)
		throw Erange("catalogue::get_children_in_table", "catalogue::free_and_check_memory() method has been called, this object is no more usable");
            if(parent == nullptr)
		throw SRC_BUG;

	    parent->reset_read_children();
	    while(parent->read_children(tmp_ptr))
	    {
		if(tmp_ptr == nullptr)
		    throw SRC_BUG;

		list_entry ent;

		const cat_inode *tmp_inode = dynamic_cast<const cat_inode *>(tmp_ptr);
		const cat_file *tmp_file = dynamic_cast<const cat_file *>(tmp_ptr);
		const cat_lien *tmp_lien = dynamic_cast<const cat_lien *>(tmp_ptr);
		const cat_device *tmp_device = dynamic_cast<const cat_device *>(tmp_ptr);
		const cat_mirage *tmp_mir = dynamic_cast<const cat_mirage *>(tmp_ptr);

		ent.set_name(tmp_ptr->get_name());

		if(tmp_mir == nullptr)
		{
		    ent.set_hard_link(false);
		    ent.set_type(get_base_signature(tmp_ptr->signature()));
		}
		else
		{
		    ent.set_hard_link(true);
		    ent.set_type(get_base_signature(tmp_mir->get_inode()->signature()));
		    tmp_inode = tmp_mir->get_inode();
		    tmp_file = dynamic_cast<const cat_file *>(tmp_inode);
		    tmp_lien = dynamic_cast<const cat_lien *>(tmp_inode);
		    tmp_device = dynamic_cast<const cat_device *>(tmp_inode);
		}

		if(tmp_inode != nullptr)
		{
		    ent.set_uid(tmp_inode->get_uid());
		    ent.set_gid(tmp_inode->get_gid());
		    ent.set_perm(tmp_inode->get_perm());
		    ent.set_last_access(tmp_inode->get_last_access());
		    ent.set_last_modif(tmp_inode->get_last_modif());
		    ent.set_saved_status(tmp_inode->get_saved_status());
		    ent.set_ea_status(tmp_inode->ea_get_saved_status());
		    if(tmp_inode->has_last_change())
			ent.set_last_change(tmp_inode->get_last_change());
		    if(tmp_inode->ea_get_saved_status() == cat_inode::ea_full)
		    {
			infinint tmp;

			if(tmp_inode->ea_get_offset(tmp))
			    ent.set_archive_offset_for_EA(tmp);
			ent.set_storage_size_for_EA(tmp_inode->ea_get_size());
		    }
		    ent.set_fsa_status(tmp_inode->fsa_get_saved_status());
		    if(tmp_inode->fsa_get_saved_status() == cat_inode::fsa_full)
		    {
			infinint tmp;

			if(tmp_inode->fsa_get_offset(tmp))
			    ent.set_archive_offset_for_FSA(tmp);
			ent.set_storage_size_for_FSA(tmp_inode->fsa_get_size());
		    }
		}

		if(tmp_file != nullptr)
		{
		    ent.set_file_size(tmp_file->get_size());
		    ent.set_storage_size(tmp_file->get_storage_size());
		    ent.set_is_sparse_file(tmp_file->get_sparse_file_detection_read());
		    ent.set_compression_algo(tmp_file->get_compression_algo_read());
		    ent.set_dirtiness(tmp_file->is_dirty());
		    ent.set_delta_sig(tmp_file->has_delta_signature_available());
		    if(tmp_file->get_saved_status() == s_saved)
		    {
			ent.set_archive_offset_for_data(tmp_file->get_offset());
			ent.set_storage_size_for_data(tmp_file->get_storage_size());
		    }
		}

		if(tmp_lien != nullptr && tmp_lien->get_saved_status() == s_saved)
		    ent.set_link_target(tmp_lien->get_target());

		if(tmp_device != nullptr)
		{
		    ent.set_major(tmp_device->get_major());
		    ent.set_minor(tmp_device->get_minor());
		}

		ent.set_slices(macro_tools_get_slices(tmp_ptr, slices));

		    // fill a new entry in the table
		ret.push_back(ent);
	    }

	}
        catch(...)
        {
	    NLS_SWAP_OUT;
            throw;
	}
        NLS_SWAP_OUT;

	return ret;
    }

    bool archive::has_subdirectory(const string & dir) const
    {
	bool ret = false;

	NLS_SWAP_IN;
        try
        {
	    const cat_directory *parent = get_dir_object(dir);
	    const cat_nomme *tmp_ptr = nullptr;

	    if(freed_and_checked)
		throw Erange("catalogue::has_subdirectory", "catalogue::free_and_check_memory() method has been called, this object is no more usable");
	    parent->reset_read_children();
	    while(parent->read_children(tmp_ptr) && !ret)
	    {
		if(dynamic_cast<const cat_directory *>(tmp_ptr) != nullptr)
		    ret = true;
	    }
	}
        catch(...)
        {
	    NLS_SWAP_OUT;
            throw;
	}
        NLS_SWAP_OUT;

	return ret;
    }

    void archive::init_catalogue(user_interaction & dialog) const
    {
	NLS_SWAP_IN;
        try
        {
	    if(freed_and_checked)
		throw Erange("catalogue::init_catalogue", "catalogue::free_and_check_memory() method has been called, this object is no more usable");
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
		    filtre_sequentially_read_all_catalogue(*cat, dialog, lax_read_mode);
		}
	    }

	    if(cat == nullptr)
		throw SRC_BUG;
	}
        catch(...)
        {
	    NLS_SWAP_OUT;
            throw;
	}
        NLS_SWAP_OUT;
    }

    const catalogue & archive::get_catalogue() const
    {
	NLS_SWAP_IN;

	try
	{
	    if(freed_and_checked)
		throw Erange("catalogue::get_catalogue", "catalogue::free_and_check_memory() method has been called, this object is no more usable");
	    if(exploitable && sequential_read)
		throw Elibcall("archive::get_catalogue", "Reading the first time the catalogue of an archive open in sequential read mode needs passing a \"user_interaction\" object to the argument of archive::get_catalogue or call init_catalogue() first ");

	    if(cat == nullptr)
		throw SRC_BUG;
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}

	NLS_SWAP_OUT;

	return *cat;
    }


    const catalogue & archive::get_catalogue(user_interaction & dialog) const
    {
	init_catalogue(dialog);
	return get_catalogue();
    }

    void archive::drop_all_filedescriptors()
    {
	NLS_SWAP_IN;

	try
	{
	    if(freed_and_checked)
		throw Erange("catalogue::drop_all_filedescriptors", "catalogue::free_and_check_memory() method has been called, this object is no more usable");
	    if(exploitable && sequential_read)
		throw Elibcall("archive::drop_all_filedescriptiors", "Dropping all filedescriptors for an archive in sequential read mode that has not yet been read need passing a \"user_interaction\" object to the argument of archive::drop_all_filedescriptors");

	    stack.clear();
	    exploitable = false;
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}

	NLS_SWAP_OUT;
    }


    void archive::drop_all_filedescriptors(user_interaction & dialog)
    {
	NLS_SWAP_IN;

	try
	{
	    if(freed_and_checked)
		throw Erange("catalogue::drop_all_filedescriptors(user_interaction)", "catalogue::free_and_check_memory() method has been called, this object is no more usable");
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
		    (void)op_test(dialog, archive_options_test(), nullptr);
	    }

	    stack.clear();
	    exploitable = false;
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}

	NLS_SWAP_OUT;
    }

    string archive::free_and_check_memory() const
    {
	string ret = "";
	archive *me = const_cast<archive *>(this);

	if(freed_and_checked)
	    throw Erange("catalogue::free_and_check_memory", "catalogue::free_and_check_memory() method has been called, this object is no more usable");

	if(me == nullptr)
	    throw SRC_BUG;
	me->freed_and_checked = true;
	me->free_except_memory_pool();

	if(pool != nullptr)
	{
#ifdef LIBDAR_DEBUG_MEMORY
	    ret += pool->max_percent_full();
#endif
	    pool->garbage_collect();
	    if(!pool->is_empty())
		ret += pool->dump();
	}

	return ret;
    }

    U_64 archive::get_first_slice_header_size() const
    {
	U_64 ret;
	infinint pre_ret;
	archive *me = const_cast<archive *>(this);
	const generic_file *bottom = me->stack.bottom();
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

    U_64 archive::get_non_first_slice_header_size() const
    {
	U_64 ret;
	infinint pre_ret;
	archive *me = const_cast<archive *>(this);
	const generic_file *bottom = me->stack.bottom();
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

    statistics archive::op_create_in(user_interaction & dialog,
                                     operation op,
				     const path & fs_root,
                                     const entrepot & sauv_path_t,
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
                                     bool same_fs,
				     cat_inode::comparison_fields what_to_check,
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
				     bool multi_threaded,
				     bool delta_signature,
				     bool build_delta_sig,
				     const mask & delta_mask,
				     const infinint & delta_sig_min_size,
				     bool delta_diff,
				     const set<string> & ignored_symlinks,
				     statistics * progressive_report)
    {
        statistics st = false;  // false => no lock for this internal object
	statistics *st_ptr = progressive_report == nullptr ? &st : progressive_report;

            // sanity checks as much as possible to avoid libdar crashing due to bad arguments
            // useless arguments are not reported.

        if(compression_level > 9 || compression_level < 1)
            throw Elibcall("op_create_in", gettext("Compression_level must be between 1 and 9 included"));
        if(file_size.is_zero() && !first_file_size.is_zero())
            throw Elibcall("op_create_in", gettext("\"first_file_size\" cannot be different from zero if \"file_size\" is equal to zero"));
        if(crypto_size < 10 && crypto != crypto_none)
            throw Elibcall("op_create_in", gettext("Crypto block size must be greater than 10 bytes"));
#ifndef	LIBDAR_NODUMP_FEATURE
	if(nodump)
	    throw Ecompilation(gettext("nodump flag feature has not been activated at compilation time, it is thus not available"));
#endif

	check_libgcrypt_hash_bug(dialog, hash, first_file_size, file_size);

            // end of sanity checks

	fs_root.explode_undisclosed();

	const catalogue *ref_cat = nullptr;
	bool initial_pause = false;
	path sauv_path_abs = sauv_path_t.get_location();
	const entrepot_local *sauv_path_t_local = dynamic_cast<const entrepot_local *>(&sauv_path_t);
	path fs_root_abs = fs_root.is_relative() ? tools_relative2absolute_path(fs_root, tools_getcwd()) : fs_root;

	if(sauv_path_abs.is_relative())
	    sauv_path_abs = sauv_path_t.get_root() + sauv_path_abs;

	tools_avoid_slice_overwriting_regex(dialog,
					    sauv_path_t,
					    filename,
					    extension,
					    info_details,
					    allow_over,
					    warn_over,
					    empty);

	if(ref_arch != nullptr && ref_arch->sequential_read && (delta_signature || delta_diff))
	    throw Erange("archive::op_create_in", gettext("Cannot sequentially read an archive of reference when delta signature or delta patch is requested"));

	local_cat_size = 0; // unknown catalogue size (need to re-open the archive, once creation has completed) [object member variable]

	sauv_path_abs.explode_undisclosed();

	    // warning against saving the archive itself

	if(op == oper_create
	   && sauv_path_t_local != nullptr  // not using a remote storage
	   && sauv_path_abs.is_subdir_of(fs_root_abs, true)
	   && selection.is_covered(filename+".1."+extension)
	   && subtree.is_covered(sauv_path_abs + string(filename+".1."+extension))
	   && filename!= "-")
	{
	    bool cov = true;      // whether the archive is covered by filter (this is saving itself)
	    string drop;          // will carry the removed part of the sauv_path_abs variable

		// checking for exclusion due to different filesystem

	    if(same_fs && !tools_are_on_same_filesystem(sauv_path_abs.display(), fs_root.display()))
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
		dialog.pause(tools_printf(gettext("WARNING! The archive is located in the directory to backup, this may create an endless loop when the archive will try to save itself. You can either add -X \"%S.*.%S\" on the command line, or change the location of the archive (see -h for help). Do you really want to continue?"), &filename, &extension));
	}

	    // building the reference catalogue

	if(ref_arch != nullptr) // from a existing archive
	{
	    const entrepot *ref_where = ref_arch->get_entrepot();
	    if(ref_where != nullptr)
		initial_pause = (*ref_where == sauv_path_t);
	    ref_cat = & ref_arch->get_catalogue(dialog);
	}

	op_create_in_sub(dialog,
			 op,
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
			 multi_threaded,
			 delta_signature,
			 build_delta_sig,
			 delta_mask,
			 delta_sig_min_size,
			 delta_diff,
			 ignored_symlinks,
			 st_ptr);

	return *st_ptr;
    }

    void archive::op_create_in_sub(user_interaction & dialog,
				   operation op,
				   const path & fs_root,
				   const entrepot & sauv_path_t,
				   const catalogue *ref_cat1,
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
				   bool same_fs,
				   cat_inode::comparison_fields what_to_check,
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
				   bool multi_threaded,
				   bool delta_signature,
				   bool build_delta_sig,
				   const mask & delta_mask,
				   const infinint & delta_sig_min_size,
				   bool delta_diff,
				   const set<string> & ignored_symlinks,
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
		SRC_BUG;
	    if(st_ptr == nullptr)
		throw SRC_BUG;

	    secu_string real_pass = pass;
	    internal_name.generate_internal_filename();

	    try
	    {
		    // pausing if saving in the same directory where is located the archive of reference

		if(!pause.is_zero() && initial_pause)
		    dialog.pause(gettext("Ready to start writing down the archive?"));

		macro_tools_create_layers(dialog,
					  stack, // this object field is set!
					  ver,   // this object field is set!
					  slices,// this object field is set!
					  nullptr,  // no slicing reference stored in archive header/trailer
					  pool,  // this object field
					  sauv_path_t,
					  filename,
					  extension,
					  allow_over,
					  warn_over,
					  info_details,
					  pause,
					  algo,
					  compression_level,
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
					  multi_threaded);

		    // ********** building the catalogue (empty for now) ************************* //
		datetime root_mtime;
		pile_descriptor pdesc(&stack);
		crit_action* rep_decr = nullptr; // not used, just needed to pass as argumen to filtre_merge_step0()
		const crit_action *rep_over = &overwrite; // not used, just needed to pass as argumen to filtre_merge_step0()


		cat = nullptr; // [object member variable]

		if(info_details)
		    dialog.warning(gettext("Building the catalog object..."));
		try
		{
		    if(fs_root.display() != "<ROOT>")
			root_mtime = tools_get_mtime(fs_root.display());
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
		    throw Erange("archive::op_create_in_sub", tools_printf(gettext("Error while fetching information for %S: "), &tmp) + e.get_message());
		}

		switch(op)
		{
		case oper_merge:
		case oper_repair:
		    if(add_marks_for_sequential_reading && !empty)
			cat = new (pool) escape_catalogue(dialog, pdesc, ref_cat1->get_root_dir_last_modif(), internal_name);
		    else
			cat = new (pool) catalogue(dialog, ref_cat1->get_root_dir_last_modif(), internal_name);
		    break;
		case oper_create:
		    if(add_marks_for_sequential_reading && !empty)
			cat = new (pool) escape_catalogue(dialog, pdesc, root_mtime, internal_name);
		    else
			cat = new (pool) catalogue(dialog, root_mtime, internal_name);
		default:
		    throw SRC_BUG;
		}

		if(cat == nullptr)
		    throw Ememory("archive::op_create_in_sub");


		    // *********** now we can perform the data filtering operation (adding data to the archive) *************** //

		try
		{
		    catalogue *void_cat = nullptr;
		    const catalogue *ref_cat_ptr = ref_cat1;

		    switch(op)
		    {
		    case oper_create:
			if(ref_cat1 == nullptr)
			{
				// using a empty catalogue as reference if no reference is given

			    label data_name;
			    data_name.clear();
			    void_cat = new (pool) catalogue(dialog,
							    datetime(0),
							    data_name);
			    if(void_cat == nullptr)
				throw Ememory("archive::op_create_in_sub");
			    ref_cat_ptr = void_cat;
			}

			try
			{
			    if(info_details)
				dialog.warning(gettext("Processing files for backup..."));
			    filtre_sauvegarde(dialog,
					      pool,
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
					      ignored_symlinks);
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
			if(info_details)
			    dialog.warning(gettext("Processing files for merging..."));

			filtre_merge(dialog,
				     pool,
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
				     delta_mask);
			break;
		    case oper_repair:
			if(info_details)
			    dialog.warning(gettext("Processing files for fixing..."));

			try
			{
			    filtre_merge_step0(dialog,
				pool,
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
			    filtre_merge_step2(dialog,
				pool,
				pdesc,
				*(const_cast<catalogue *>(ref_cat1)),
				info_details,
				display_treated,
				false,    // display_trated_only_dir
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
				true);

				// at this step, cat (the current archive's catalogue) is still empty
				// we will need to add ref_cat1's content at the end of the archive
				// not our own's content

				// changing the data_name of the catalogue that will be dropped at the
				// end of the archive to match the data_name of the slice header
			    const_cast<catalogue *>(ref_cat1)->set_data_name(internal_name);
			}
			catch(Erange & e)
			{
				// do nothing, this may be an incoherence due to the fact
				// the archive has could not be finished, thing we tend to fix here
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
			dialog.warning(gettext("Adding reference to files that have been destroyed since reference backup..."));
		    if(aborting)
			cat->update_absent_with(*ref_cat1, aborting_next_etoile);
		    else
			st_ptr->add_to_deleted(cat->update_destroyed_with(*ref_cat1));
		}

		macro_tools_close_layers(dialog,
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

    void archive::free_except_memory_pool()
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

    void archive::free_all()
    {
	free_except_memory_pool();

	if(pool != nullptr)
	{
	    if(get_pool() == nullptr)
	    {
		delete pool;
		pool = nullptr;
	    }
	    else
	    {
		if(pool != get_pool())
		    throw SRC_BUG;
		pool = nullptr;
	    }
	}
    }

    void archive::init_pool()
    {
	pool = nullptr;

#ifdef LIBDAR_SPECIAL_ALLOC
	if(get_pool() == nullptr)
	{
	    pool = new (nothrow) memory_pool();
	    if(pool == nullptr)
		throw Ememory("archive::archive (read) for memory_pool");
	}
	else
	    pool = get_pool();
#endif
    }

    void archive::check_gnupg_signed(user_interaction & dialog) const
    {
	list<signator>::const_iterator it = gnupg_signed.begin();

	while(it != gnupg_signed.end() && it->result == signator::good)
	    ++it;

	if(it != gnupg_signed.end())
	    dialog.pause(gettext("WARNING! Incorrect signature found for archive, continue anyway?"));
    }

    void archive::disable_natural_destruction()
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

    void archive::enable_natural_destruction()
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

    const label & archive::get_layer1_data_name() const
    {
	contextual *l1 = nullptr;
	archive *ceci = const_cast<archive *>(this);

	ceci->stack.find_first_from_bottom(l1);
	if(l1 != nullptr)
	    return l1->get_data_name();
	else
	    throw Erange("catalogue::get_data_name", gettext("Cannot get data name of the archive, this archive is not completely initialized"));
    }

    const label & archive::get_catalogue_data_name() const
    {
	if(cat != nullptr)
	    return cat->get_data_name();
	else
	    throw SRC_BUG;
    }

    bool archive::only_contains_an_isolated_catalogue() const
    {
	return get_layer1_data_name() != get_catalogue_data_name() && ver.get_edition() >= 8;
    }

    void archive::check_against_isolation(user_interaction & dialog, bool lax) const
    {
	if(cat != nullptr)
	{
	    try
	    {
		if(only_contains_an_isolated_catalogue())
		{
		    if(!lax)
			throw Erange("archive::check_against_isolation", gettext("This archive contains an isolated catalogue, it cannot be used for this operation. It can only be used as reference for a incremental/differential backup or as backup of the original archive's catalogue"));
			// note1: that old archives do not have any data_name neither in the catalogue nor in the layer1 of the archive
			// both are faked equal to a zeroed label when reading them with recent dar version. Older archives than "08" would
			// thus pass this test successfully if no check was done against the archive version
			// note2: Old isolated catalogue do not carry any data, this is safe to try to restore them because any
			// pointer to data and/or EA has been removed during the isolation.
		    else
			dialog.pause(gettext("LAX MODE: Archive seems to be only an isolated catalogue (no data in it), Can I assume data corruption occurred and consider the archive as being a real archive?"));
		}
	    }
	    catch(Erange & e)
	    {
		throw Erange("archive::check_against_isolation", string(gettext("Error while fetching archive properties: ")) + e.get_message());
	    }
	}
	else
	    throw SRC_BUG;
	    // this method should be called once the archive object has been constructed
	    // and this object should be totally exploitable, thus have an available catalogue
    }

    bool archive::get_sar_param(infinint & sub_file_size, infinint & first_file_size, infinint & last_file_size,
                                infinint & total_file_number)
    {
        sar *real_decoupe = nullptr;

	stack.find_first_from_bottom(real_decoupe);
        if(real_decoupe != nullptr)
        {
	    slice_layout tmp = real_decoupe->get_slicing();

            sub_file_size = tmp.other_size;
            first_file_size = tmp.first_size;
            if(real_decoupe->get_total_file_number(total_file_number)
               && real_decoupe->get_last_file_size(last_file_size))
                return true;
            else // could not read size parameters
                throw Erange("archive::get_sar_param", gettext("Sorry, file size is unknown at this step of the program.\n"));
        }
        else
            return false;
    }

    const entrepot *archive::get_entrepot()
    {
	const entrepot *ret = nullptr;
	sar *real_decoupe = nullptr;

	stack.find_first_from_bottom(real_decoupe);
	if(real_decoupe != nullptr)
	{
	    ret = real_decoupe->get_entrepot();
	    if(ret == nullptr)
		throw SRC_BUG;
	}

	return ret;
    }


    infinint archive::get_level2_size()
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

    const cat_directory *archive::get_dir_object(const string & dir) const
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
		    throw Erange("archive::get_children_in_table", tools_printf("%S entry does not exist", &dir));
	    }
	}
	    // else returning the root of the archive

	return parent;
    }


    static void check_libgcrypt_hash_bug(user_interaction & dialog, hash_algo hash, const infinint & first_file_size, const infinint & file_size)
    {
#if CRYPTO_AVAILABLE
	if(hash != hash_none && !crypto_min_ver_libgcrypt_no_bug())
	{
	    const infinint limit = tools_get_extended_size("256G", 1024);
	    if(file_size >= limit || first_file_size >= limit)
		dialog.pause(tools_printf(gettext("libgcrypt version < %s. Ligcrypt used has a bug that leads md5 and sha1 hash results to be erroneous for files larger than 256 Gio (gibioctet), do you really want to spend CPU cycles calculating a useless hash?"), MIN_VERSION_GCRYPT_HASH_BUG));
	}
#endif
    }

} // end of namespace
