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
// $Id: archive.cpp,v 1.137 2012/04/27 11:24:30 edrusb Exp $
//
/*********************************************************************/
//

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
#include "tools.hpp"
#include "header.hpp"
#include "header_version.hpp"
#include "sar_tools.hpp"
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

#define GLOBAL_ELASTIC_BUFFER_SIZE 10240

#define ARCHIVE_NOT_EXPLOITABLE "Archive of reference given is not exploitable"

using namespace std;

namespace libdar
{

	// opens an already existing archive

    archive::archive(user_interaction & dialog,
                     const path & chem,
		     const std::string & basename,
		     const std::string & extension,
		     const archive_options_read & options)
    {
        NLS_SWAP_IN;
        try
        {
	    entrepot *where = options.get_entrepot().clone();

	    if(where == NULL)
		throw Ememory("archive::archive");

	    try
	    {
		where->set_location(chem.display());

		cat = NULL;
		infinint second_terminateur_offset = 0;
		infinint ref_second_terminateur_offset = 0;
		header_version ref_ver;
		escape *esc = NULL;
		lax_read_mode = options.get_lax();
		sequential_read = options.get_sequential_read(); // updating the archive object's field

		try
		{
			// we open the main archive to get the different layers (level1, scram and level2).
		    macro_tools_open_archive(dialog,
					     *where,
					     basename,
					     options.get_slice_min_digits(),
					     extension,
					     options.get_crypto_algo(),
					     options.get_crypto_pass(),
					     options.get_crypto_size(),
					     stack, ver,
					     options.get_input_pipe(),
					     options.get_output_pipe(),
					     options.get_execute(),
					     second_terminateur_offset,
					     options.get_lax(),
					     options.get_sequential_read(),
					     options.get_info_details());
		    try
		    {
			check_header_version();
		    }
		    catch(...)
		    {
			if(!options.get_lax())
			    throw;
			    // ignore error in lax mode
		    }

		    stack.find_first_from_top(esc);
			// esc may be NULL

		    if(options.is_external_catalogue_set())
		    {
			pile ref_stack;
			entrepot *ref_where = options.get_ref_entrepot().clone();
			if(ref_where == NULL)
			    throw Ememory("archive::archive");

			try
			{
			    ref_where->set_location(options.get_ref_path().display());
			    try
			    {
				if(options.get_ref_basename() == "-")
				    throw Erange("archive::archive", gettext("Reading the archive of reference from pipe or standard input is not possible"));
				if(options.get_ref_basename() == "+")
				    throw Erange("archive::archive", gettext("The basename '+' is reserved for special a purpose that has no meaning in this context"));

				    // we open the archive of reference also to get its different layers (ref_stack)
				macro_tools_open_archive(dialog,
							 *ref_where,
							 options.get_ref_basename(),
							 options.get_ref_slice_min_digits(),
							 extension,
							 options.get_ref_crypto_algo(),
							 options.get_ref_crypto_pass(),
							 options.get_ref_crypto_size(),
							 ref_stack, ref_ver,
							 "", "",
							 options.get_ref_execute(),
							 ref_second_terminateur_offset,
							 options.get_lax(),
							 false, // sequential_read is never used to retreive the isolated catalogue (well, that's possible and easy to add this feature), see later ...
							 options.get_info_details());
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
			    if(ref_where != NULL)
				delete ref_where;
			    throw;
			}
			if(ref_where != NULL)
			    delete ref_where;

			    // fetching the catalogue in the archive of reference, making it point on the main archive layers.

			ref_ver.algo_zip = ver.algo_zip; // set the default encryption to use to the one of the main archive

			cat = macro_tools_get_derivated_catalogue_from(dialog,
								       stack,
								       ref_stack,
								       ref_ver,
								       options.get_info_details(),
								       local_cat_size,
								       ref_second_terminateur_offset,
								       false); // never relaxed checking for external catalogue
			if(cat == NULL)
			    throw SRC_BUG;

			    // checking for compatibility of the archive of reference with this archive data_name

			if(get_layer1_data_name() != get_catalogue_data_name())
			    throw Erange("archive::archive", gettext("The archive and the isolated catalogue do not correspond to the same data, they are thus incompatible between them"));
		    }
		    else // no isolated archive to fetch the catalogue from
		    {
			try
			{
			    if(!options.get_sequential_read())
				cat = macro_tools_get_catalogue_from(dialog,
								     stack,
								     ver,
								     options.get_info_details(),
								     local_cat_size,
								     second_terminateur_offset,
								     options.get_lax());
			    else
			    {
				if(esc != NULL)
				{
				    generic_file *ea_loc = stack.get_by_label(LIBDAR_STACK_LABEL_UNCOMPRESSED);
				    generic_file *data_loc = stack.get_by_label(LIBDAR_STACK_LABEL_CLEAR);

				    cat = new escape_catalogue(dialog,
							       ver.edition,
							       char2compression(ver.algo_zip),
							       data_loc,
							       ea_loc,
							       esc,
							       options.get_lax());
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
									       stack,
									       ver.edition,
									       char2compression(ver.algo_zip),
									       options.get_info_details(),
									       false, // even partial
									       tmp);
				    }
				    catch(Erange & e)
				    {
					dialog.printf(gettext("LAX MODE: Could not find a whole catalogue in the archive. If you have an isolated catalogue, stop here and use it as backup of the internal catalogue, else continue but be advised that all data will not be able to be retrieved..."));
					dialog.pause(gettext("LAX MODE: Do you want to try finding portions of the original catalogue if some remain (this may take even more time and in any case, it will only permit to recover some files, at most)?"));
					cat = macro_tools_lax_search_catalogue(dialog,
									       stack,
									       ver.edition,
									       char2compression(ver.algo_zip),
									       options.get_info_details(),
									       true,                     // even partial
									       get_layer1_data_name());
				    }
				}
			    }
			}
		    }
		    exploitable = true;
		}
		catch(...)
		{
		    if(where != NULL)
			delete where;
		    throw;
		}
		if(where != NULL)
		    delete where;
	    }
	    catch(...)
	    {
		free();
		NLS_SWAP_OUT;
		throw;
	    }
	}
	catch(...)
	{
	    free();
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
	    entrepot *sauv_path_t = options.get_entrepot().clone();
	    if(sauv_path_t == NULL)
		throw Ememory("archive::archive");
	    sauv_path_t->set_location(sauv_path.display());

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
				   options.get_compr_mask(),
				   options.get_min_compr_size(),
				   options.get_nodump(),
				   options.get_hourshift(),
				   options.get_empty(),
				   options.get_alter_atime(),
				   options.get_furtive_read_mode(),
				   options.get_same_fs(),
				   options.get_comparison_fields(),
				   options.get_snapshot(),
				   options.get_cache_directory_tagging(),
				   options.get_display_skipped(),
				   options.get_fixed_date(),
				   options.get_slice_permission(),
				   options.get_slice_user_ownership(),
				   options.get_slice_group_ownership(),
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
				   progressive_report);
		exploitable = false;
		stack.terminate();
	    }
	    catch(...)
	    {
		if(sauv_path_t != NULL)
		    delete sauv_path_t;
		throw;
	    }
	    if(sauv_path_t != NULL)
		delete sauv_path_t;
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

	// creates an isolated catalogue

    archive::archive(user_interaction & dialog,
                     const path &sauv_path,
                     archive *ref_arch,
                     const string & filename,
                     const string & extension,
		     const archive_options_isolate & options)
    {
        NLS_SWAP_IN;
        try
        {
	    entrepot *sauv_path_t = options.get_entrepot().clone();
	    if(sauv_path == NULL)
		throw Ememory("archive::archive");
	    sauv_path_t->set_location(sauv_path.display());

	    try
	    {
		sequential_read = false; // updating the archive field

		(void)op_create_in(dialog,
				   oper_isolate,
				   path("."),
				   *sauv_path_t,
				   ref_arch,
				   bool_mask(false),
				   bool_mask(false),
				   filename,
				   extension,
				   options.get_allow_over(),
				   options.get_warn_over(),
				   options.get_info_details(),
				   options.get_pause(),
				   false,
				   options.get_compression(),
				   options.get_compression_level(),
				   options.get_slice_size(),
				   options.get_first_slice_size(),
				   bool_mask(true),
				   options.get_execute(),
				   options.get_crypto_algo(),
				   options.get_crypto_pass(),
				   options.get_crypto_size(),
				   bool_mask(false),
				   0,
				   false,
				   0,
				   options.get_empty(),
				   false,
				   false,
				   false,
				   inode::cf_all,
				   false,
				   false,
				   false,
				   0,
				   options.get_slice_permission(),
				   options.get_slice_user_ownership(),
				   options.get_slice_group_ownership(),
				   0, // repeat count
				   0, // repeat byte
				   options.get_sequential_marks(),
				   false, // security check
				   0, // sparse file min size (0 = no detection)
				   options.get_user_comment(),
				   options.get_hash_algo(),
				   options.get_slice_min_digits(),
				   "",                 // backup_hook_file_execute
				   bool_mask(false),   // backup_hook_file_mask
				   false,
				   NULL);
		    // we ignore returned value;
		exploitable = false;
		stack.terminate();
	    }
	    catch(...)
	    {
		if(sauv_path_t != NULL)
		    delete sauv_path_t;
		throw;
	    }
	    if(sauv_path_t != NULL)
		delete sauv_path_t;
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

	// merge constructor

    archive::archive(user_interaction & dialog,
		     const path & sauv_path,
		     archive *ref_arch1,
		     const std::string & filename,
		     const std::string & extension,
		     const archive_options_merge & options,
		     statistics * progressive_report)
    {
	statistics st = false;
	statistics *st_ptr = progressive_report == NULL ? &st : progressive_report;
	catalogue *ref_cat1 = NULL;
	catalogue *ref_cat2 = NULL;
	archive *ref_arch2 = options.get_auxilliary_ref();
	compression algo_kept = none;
	entrepot *sauv_path_t = options.get_entrepot().clone();
	entrepot_local *sauv_path_t_local = dynamic_cast<entrepot_local *>(sauv_path_t);

	NLS_SWAP_IN;
	try
	{
	    exploitable = false;
	    sequential_read = false; // updating the archive field

		// sanity checks as much as possible to avoid libdar crashing due to bad arguments
		// useless arguments are not reported.

	    if(&sauv_path == NULL)
		throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"sauv_path\""));
	    if(sauv_path_t == NULL)
		throw SRC_BUG; // options.get_entrepot() should have allocated object or else thrown an exception
	    if(&filename == NULL)
		throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"filename\""));
	    if(&extension == NULL)
		throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"extension\""));
	    if(options.get_compression_level() > 9 || options.get_compression_level() < 1)
		throw Elibcall("op_create/op_isolate", gettext("Compression_level must be between 1 and 9 included"));
	    if(options.get_slice_size() == 0 && options.get_first_slice_size() != 0)
		throw Elibcall("op_create/op_isolate", gettext("\"first_file_size\" cannot be different from zero if \"file_size\" is equal to zero"));
	    if(options.get_crypto_size() < 10 && options.get_crypto_algo() != crypto_none)
		throw Elibcall("op_create/op_isolate", gettext("Crypto block size must be greater than 10 bytes"));

	    if(ref_arch1 != NULL)
		ref_arch1->check_against_isolation(dialog, false);
		// this avoid to merge an archive from an isolated catalogue
		// there is no way to know whether the corresponding merging of the real archive exists, nor to know the data_name of this
		// hypothetical merged archive. Thus we forbid the use of isolated catalogue for merging.


	    if(ref_arch2 != NULL)
		ref_arch2->check_against_isolation(dialog, false);
		// same remark as above for ref_arch1


		// end of sanity checks

	    sauv_path_t->set_location(sauv_path.display());

	    if(!options.get_empty() && sauv_path_t_local != NULL)
		tools_avoid_slice_overwriting_regex(dialog,
						    sauv_path,
						    string("^")+filename+"\\.[0-9]+\\."+extension+"(\\.(md5|sha1))?$",
						    options.get_info_details(),
						    options.get_allow_over(),
						    options.get_warn_over(),
						    options.get_empty());

	    if(ref_arch1 == NULL)
		if(ref_arch2 == NULL)
		    throw Elibcall("archive::archive[merge]", string(gettext("Both reference archive are NULL, cannot merge archive from nothing")));
		else
		    if(ref_arch2->cat == NULL)
			throw SRC_BUG; // an archive should always have a catalogue available
		    else
			if(ref_arch2->exploitable)
			    ref_cat1 = ref_arch2->cat;
			else
			    throw Elibcall("archive::archive[merge]", gettext(ARCHIVE_NOT_EXPLOITABLE));
	    else
		if(ref_arch2 == NULL)
		    if(ref_arch1->cat == NULL)
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
		    if(ref_arch1->cat == NULL)
			throw SRC_BUG;
		    if(ref_arch2->cat == NULL)
			throw SRC_BUG;
		    ref_cat1 = ref_arch1->cat;
		    ref_cat2 = ref_arch2->cat;
		    if(ref_arch1->ver.algo_zip != ref_arch2->ver.algo_zip && char2compression(ref_arch1->ver.algo_zip) != none && char2compression(ref_arch2->ver.algo_zip) != none && options.get_keep_compressed())
			throw Efeature(gettext("the \"Keep file compressed\" feature is not possible when merging two archives using different compression algorithms (This is for a future version of dar). You can still merge these two archives but without keeping file compressed (thus you will probably like to use compression (-z or -y options) for the resulting archive"));
		}

	    if(options.get_keep_compressed())
	    {
		if(ref_arch1 == NULL)
		    throw SRC_BUG;

		algo_kept = char2compression(ref_arch1->ver.algo_zip);
		if(algo_kept == none && ref_cat2 != NULL)
		{
		    if(ref_arch2 == NULL)
			throw SRC_BUG;
		    else
			algo_kept = char2compression(ref_arch2->ver.algo_zip);
		}
	    }
	    if(ref_cat1 == NULL)
		throw SRC_BUG;

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
			     options.get_compr_mask(),
			     options.get_min_compr_size(),
			     false,   // nodump
			     0,       // hourshift
			     options.get_empty(),
			     true,    // alter_atime
			     false,   // furtive_read_mode
			     false,   // same_fs
			     inode::cf_all,   // what_to_check
			     false,   // snapshot
			     false,   // cache_directory_tagging
			     options.get_display_skipped(),
			     options.get_keep_compressed(),
			     0,       // fixed_date
			     options.get_slice_permission(),
			     options.get_slice_user_ownership(),
			     options.get_slice_group_ownership(),
			     0,  // repeat_count
			     0,  // repeat_byte
			     options.get_decremental_mode(),
			     options.get_sequential_marks(),
			     false,  // security_check
			     options.get_sparse_file_min_size(),
			     options.get_user_comment(),
			     options.get_hash_algo(),
			     options.get_slice_min_digits(),
			     "",      // backup_hook_file_execute
			     bool_mask(false), //backup_hook_file_mask
			     false,
			     st_ptr);
	    exploitable = false;
	    stack.terminate();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    static void dummy_call(char *x)
    {
        static char id[]="$Id: archive.cpp,v 1.137 2012/04/27 11:24:30 edrusb Exp $";
        dummy_call(id);
    }

    statistics archive::op_extract(user_interaction & dialog,
                                   const path & fs_root,
				   const archive_options_extract & options,
				   statistics * progressive_report)
    {
        statistics st = false;  // false => no lock for this internal object
	statistics *st_ptr = progressive_report == NULL ? &st : progressive_report;

        NLS_SWAP_IN;
        try
        {
                // sanity checks

            if(!exploitable)
                throw Elibcall("op_extract", gettext("This archive is not exploitable, check documentation for more"));
            if(&fs_root == NULL)
                throw Elibcall("op_extract", gettext("NULL argument given to \"fs_root\""));

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
		/// we can now use the directory::get_recursive_has_changed() to avoid recursion in a directory where
		/// no file has been saved.

            try
            {
                filtre_restore(dialog,
			       options.get_selection(),
			       options.get_subtree(),
			       get_cat(),
			       tools_relative2absolute_path(fs_root, tools_getcwd()),
			       options.get_warn_over(),
			       options.get_info_details(),
                               *st_ptr,
			       options.get_ea_mask(),
                               options.get_flat(),
			       options.get_what_to_check(),
			       options.get_warn_remove_no_match(),
			       options.get_empty(),
			       options.get_display_skipped(),
			       options.get_empty_dir(),
			       options.get_overwriting_rules(),
			       options.get_dirty_behavior(),
			       options.get_only_deleted(),
			       options.get_ignore_deleted());
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

	    if(!exploitable)
		throw Elibcall("summary", gettext("This archive is not exploitable, check the archive class usage in the API documentation"));

		// end of sanity checks

	    infinint sub_file_size;
	    infinint first_file_size;
	    infinint last_file_size, file_number;
	    string algo = compression2string(char2compression(get_header().algo_zip));
	    infinint cat_size = get_cat_size();
	    const header_version ver = get_header();

	    dialog.printf(gettext("Archive version format               : %s\n"), get_header().edition.display().c_str());
	    dialog.printf(gettext("Compression algorithm used           : %S\n"), &algo);
	    dialog.printf(gettext("Scrambling or strong encryption used : %s\n"), ((ver.flag & VERSION_FLAG_SCRAMBLED) != 0 ? gettext("yes") : gettext("no")));
	    dialog.printf(gettext("Sequential reading marks             : %s\n"), ((ver.flag & VERSION_FLAG_SEQUENCE_MARK) != 0 ? gettext("present") : gettext("absent")));
	    dialog.printf(gettext("Catalogue size in archive            : %i bytes\n"), &cat_size);

	    dialog.printf(gettext("User comment                         : %S\n\n"), &(get_header().cmd_line));

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
		    dialog.printf(gettext("Archive size is: %i bytes\n"), &arch_size);
		    dialog.printf(gettext("Previous archive size does not include headers present in each slice\n"));
		}
	    }
	    catch(Erange & e)
	    {
		string msg = e.get_message();
		dialog.printf("%S\n", &msg);
	    }

	    entree_stats stats = get_cat().get_stats();

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
            enable_natural_destruction();
            try
            {
                switch(options.get_list_mode())
		{
		case archive_options_listing::normal:
		    get_cat().tar_listing(only_contains_an_isolated_catalogue(), options.get_selection(), options.get_subtree(), options.get_filter_unsaved(), options.get_display_ea(), "");
		    break;
		case archive_options_listing::tree:
		    get_cat().listing(only_contains_an_isolated_catalogue(), options.get_selection(), options.get_subtree(), options.get_filter_unsaved(), options.get_display_ea(), "");
		    break;
		case archive_options_listing::xml:
		    get_cat().xml_listing(only_contains_an_isolated_catalogue(), options.get_selection(), options.get_subtree(), options.get_filter_unsaved(), options.get_display_ea(), "");
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
            catch(Erange &e)
            {
                string msg = string(gettext("Error while listing archive contents: ")) + e.get_message();
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
    }

    statistics archive::op_diff(user_interaction & dialog,
                                const path & fs_root,
				const archive_options_diff & options,
				statistics * progressive_report)
    {
        statistics st = false;  // false => no lock for this internal object
	statistics *st_ptr = progressive_report == NULL ? &st : progressive_report;

        NLS_SWAP_IN;
        try
        {

                // sanity checks

            if(!exploitable)
                throw Elibcall("op_diff", gettext("This archive is not exploitable, check documentation for more"));
            if(&fs_root == NULL)
                throw Elibcall("op_diff", gettext("NULL argument given to \"fs_root\""));

	    check_against_isolation(dialog, lax_read_mode);
		// this avoid to try diffing archive directly from an isolated catalogue
		// the isolated catalogue can be used to compaire data (since format "08") but only
		// associated with the real plain archive, not alone.

                // end of sanity checks

	    fs_root.explode_undisclosed();
            enable_natural_destruction();
            try
            {
                filtre_difference(dialog,
				  options.get_selection(),
				  options.get_subtree(),
				  get_cat(),
				  tools_relative2absolute_path(fs_root, tools_getcwd()),
				  options.get_info_details(),
				  *st_ptr,
				  options.get_ea_mask(),
				  options.get_alter_atime(),
				  options.get_furtive_read_mode(),
				  options.get_what_to_check(),
				  options.get_display_skipped(),
				  options.get_hourshift(),
				  options.get_compare_symlink_date());
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
	statistics *st_ptr = progressive_report == NULL ? &st : progressive_report;
	bool isolated = false;

        NLS_SWAP_IN;
        try
        {

                // sanity checks

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
			const entree *tmp;
			if(cat == NULL)
			    throw SRC_BUG;
			cat->read(tmp); // should be enough to have the whole catalogue being read if using sequential read mode
			cat->reset_read();
		    }
		    else
			filtre_test(dialog,
				    options.get_selection(),
				    options.get_subtree(),
				    get_cat(),
				    options.get_info_details(),
				    options.get_empty(),
				    *st_ptr,
				    options.get_display_skipped());
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

    bool archive::get_children_of(user_interaction & dialog,
                                  const string & dir)
    {
	bool ret;
        NLS_SWAP_IN;
        try
        {
	    if(exploitable && sequential_read) // the catalogue is not even yet read, so we must first read it entirely
	    {
		if(only_contains_an_isolated_catalogue())
			// this is easy... asking just an entry
			//from the catalogue makes its whole being read
		{
		    const entree *tmp;
		    if(cat == NULL)
			throw SRC_BUG;
		    cat->read(tmp); // should be enough to have the whole catalogue being read
		    cat->reset_read();
		}
		else
			// here we have a plain archive, doing the test operation
			// is the simplest way to read the whole archive and thus get its contents
			// (i.e.: the catalogue)
		    (void)op_test(dialog, archive_options_test(), NULL);
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

    const catalogue & archive::get_catalogue() const
    {
	NLS_SWAP_IN;

	try
	{
	    if(exploitable && sequential_read)
		throw Elibcall("archive::get_catalogue", "Dropping all filedescriptors for an archive in sequential read mode that has not yet been read need passing a \"user_interaction\" object to the argument of archive::get_catalogue");

	    if(cat == NULL)
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
	NLS_SWAP_IN;

	try
	{
	    if(exploitable && sequential_read)
	    {
		if(only_contains_an_isolated_catalogue())
		{
		    const entree *tmp;
		    if(cat == NULL)
			throw SRC_BUG;
		    cat->read(tmp); // should be enough to have the whole catalogue being read
		    cat->reset_read();
		}
		else
		    (void)const_cast<archive *>(this)->op_test(dialog, archive_options_test(), NULL);
	    }

	    if(cat == NULL)
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

    void archive::drop_all_filedescriptors()
    {
	NLS_SWAP_IN;

	try
	{
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
	    if(exploitable && sequential_read)
	    {
		if(only_contains_an_isolated_catalogue())
		{
		    const entree *tmp;
		    if(cat == NULL)
			throw SRC_BUG;
		    cat->read(tmp); // should be enough to have the whole catalogue being read
		    cat->reset_read();
		}
		else
		    (void)op_test(dialog, archive_options_test(), NULL);
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
                                     const mask & compr_mask,
                                     const infinint & min_compr_size,
                                     bool nodump,
                                     const infinint & hourshift,
                                     bool empty,
                                     bool alter_atime,
				     bool furtive_read_mode,
                                     bool same_fs,
				     inode::comparison_fields what_to_check,
                                     bool snapshot,
                                     bool cache_directory_tagging,
				     bool display_skipped,
				     const infinint & fixed_date,
				     const string & slice_permission,
				     const string & slice_user_ownership,
				     const string & slice_group_ownership,
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
				     statistics * progressive_report)
    {
        statistics st = false;  // false => no lock for this internal object
	statistics *st_ptr = progressive_report == NULL ? &st : progressive_report;

            // sanity checks as much as possible to avoid libdar crashing due to bad arguments
            // useless arguments are not reported.

        if(&fs_root == NULL)
            throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"fs_root\""));
        if(&sauv_path_t == NULL)
            throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"sauv_path_t\""));
        if(&selection == NULL)
            throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"selection\""));
        if(&subtree == NULL)
            throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"subtree\""));
        if(&filename == NULL)
            throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"filename\""));
        if(&extension == NULL)
            throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"extension\""));
        if(compression_level > 9 || compression_level < 1)
            throw Elibcall("op_create/op_isolate", gettext("Compression_level must be between 1 and 9 included"));
        if(file_size == 0 && first_file_size != 0)
            throw Elibcall("op_create/op_isolate", gettext("\"first_file_size\" cannot be different from zero if \"file_size\" is equal to zero"));
        if(&execute == NULL)
            throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"execute\""));
        if(&compr_mask == NULL)
            throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"compr_mask\""));
        if(&min_compr_size == NULL)
            throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"min_compr_size\""));
        if(crypto_size < 10 && crypto != crypto_none)
            throw Elibcall("op_create/op_isolate", gettext("Crypto block size must be greater than 10 bytes"));
	if(&ea_mask == NULL)
	    throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"ea_mask\""));
#ifndef	LIBDAR_NODUMP_FEATURE
	if(nodump)
	    throw Ecompilation(gettext("nodump flag feature has not been activated at compilation time, it is thus not available"));
#endif

            // end of sanity checks

	fs_root.explode_undisclosed();

	const catalogue *ref_cat = NULL;
	bool initial_pause = false;
	path sauv_path_abs = sauv_path_t.get_location();
	const entrepot_local *sauv_path_t_local = dynamic_cast<const entrepot_local *>(&sauv_path_t);
	path fs_root_abs = fs_root.is_relative() ? tools_relative2absolute_path(fs_root, tools_getcwd()) : fs_root;

	if(sauv_path_abs.is_relative())
	    sauv_path_abs = sauv_path_t.get_root() + sauv_path_abs;

	if(!empty && sauv_path_t_local != NULL)
	    tools_avoid_slice_overwriting_regex(dialog,
						sauv_path_abs,
						string("^")+filename+"\\.[0-9]+\\."+extension+"(\\.(md5|sha1))?$",
						info_details,
						allow_over,
						warn_over,
						empty);

	local_cat_size = 0; // unknown catalogue size (need to re-open the archive, once creation has completed) [object member variable]

	sauv_path_abs.explode_undisclosed();

	    // warning against saving the archive itself

	if(op == oper_create
	   && sauv_path_t_local != NULL  // not using a remote storage
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

	if(ref_arch != NULL) // from a existing archive
	{
	    const entrepot *ref_where = ref_arch->get_entrepot();
	    if(ref_where != NULL)
		initial_pause = (*ref_where == sauv_path_t);
	    ref_cat = & ref_arch->get_catalogue();
	}

	op_create_in_sub(dialog,
			 op,
			 fs_root,
			 sauv_path_t,
			 ref_cat,
			 NULL,
			 initial_pause,
			 selection,
			 subtree,
			 filename,
			 extension,
			 allow_over,
			 allow_over ? crit_constant_action(data_overwrite, EA_overwrite) : crit_constant_action(data_preserve, EA_preserve), // we do not have any overwriting policy in this environement (archive creation and isolation), so we create one on-fly
			 warn_over,
			 info_details,
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
			 compr_mask,
			 min_compr_size,
			 nodump,
			 hourshift,
			 empty,
			 alter_atime,
			 furtive_read_mode,
			 same_fs,
			 what_to_check,
			 snapshot,
			 cache_directory_tagging,
			 display_skipped,
			 false,   // keep_compressed
			 fixed_date,
			 slice_permission,
			 slice_user_ownership,
			 slice_group_ownership,
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
				   const mask & compr_mask,
				   const infinint & min_compr_size,
				   bool nodump,
				   const infinint & hourshift,
				   bool empty,
				   bool alter_atime,
				   bool furtive_read_mode,
				   bool same_fs,
				   inode::comparison_fields what_to_check,
				   bool snapshot,
				   bool cache_directory_tagging,
				   bool display_skipped,
				   bool keep_compressed,
				   const infinint & fixed_date,
				   const string & slice_permission,
				   const string & slice_user_ownership,
				   const string & slice_group_ownership,
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
				   statistics * st_ptr)
    {
	try
	{
	    stack.clear();
	    cat = NULL;    // [object member variable]
	    bool aborting = false;
	    infinint aborting_next_etoile = 0;
	    U_64 flag = 0;     // carries the sar option flag
	    terminateur coord;
	    label data_name;
	    generic_file *tmp = NULL;
	    escape *esc = NULL;
	    compressor *compr_ptr = NULL;
	    tronconneuse *tronco_ptr = NULL;
	    thread_cancellation thr_cancel;

	    if(op == oper_isolate)
	    {
		    // sleeping one second is mandatory to be sure the label that will soon be generated for
		    // layer1 is not equal to the label of the archive of reference. Else, for short archives
		    // that are on-fly isolated, the on-fly extracted catalogue may end with the same data_name
		    // as its own layer1 leading dar to thing that it is a normal archive with data.
		sleep(1);
	    }

	    data_name.clear();

	    if(ref_cat1 == NULL && op != oper_create)
		SRC_BUG;
	    if(st_ptr == NULL)
		throw SRC_BUG;

	    secu_string real_pass = pass;

	    try
	    {
		    // pausing if saving in the same directory where is located the archive of reference

		if(pause != 0 && initial_pause)
		    dialog.pause(gettext("Ready to start writing down the archive?"));

		    // **********  building the level 1 generic_file layer *********** //

		if(empty)
		    tmp = new null_file(gf_write_only);
		    // data_name is unchanged because all the archive goes to a black hole.
		else
		    if(file_size == 0) // one SLICE
			if(filename == "-") // output to stdout
			{
			    trivial_sar *tvs = sar_tools_open_archive_tuyau(dialog, 1, gf_write_only, data_name, execute); //archive goes to stdout
			    tmp = tvs;
			    if(tvs != NULL)
				data_name = tvs->get_data_name();
				// if tvs == NULL, then tmp == NULL is true, this case is handled below
			}
			else
			{
			    trivial_sar *tvs = new trivial_sar(dialog,
							       filename,
							       extension,
							       sauv_path_t, // entrepot !!
							       data_name,
							       execute,
							       allow_over,
							       warn_over,
							       hash,
							       slice_min_digits);
			    tmp = tvs;
			    if(tvs != NULL)
				data_name = tvs->get_data_name();
				// if tvs == NULL, then level1 == NULL is true, this case is handled below
			}
		    else
		    {
			sar *rsr = new sar(dialog,
					   filename,
					   extension,
					   file_size,
					   first_file_size,
					   warn_over,
					   allow_over,
					   pause,
					   sauv_path_t, // entrepot !!
					   data_name,
					   hash,
					   slice_min_digits,
					   execute);
			tmp = rsr;
			if(rsr != NULL)
			    data_name = rsr->get_data_name();
			    // if rsr == NULL, then level1 == NULL is true, this case is handled below
		    }


		if(tmp == NULL)
		    throw Ememory("op_create_in_sub");
		else
		{
		    stack.push(tmp);
		    tmp = NULL;
		}

		    // ******** creating and writing the archive header ******************* //

		ver.edition = macro_tools_supported_version;
		ver.algo_zip = compression2char(algo);
		ver.cmd_line = user_comment;
		ver.flag = 0;

		    // optaining a password on-fly if necessary

		if(crypto != crypto_none && real_pass.size() == 0)
		{
		    if(!secu_string::is_string_secured())
			dialog.warning(gettext("WARNING: support for secure memory was not available at compilation time, in case of heavy memory load, this may lead the password you are about to provide to be wrote to disk (swap space) in clear. You have been warned!"));
		    secu_string t1 = dialog.get_secu_string(tools_printf(gettext("Archive %S requires a password: "), &filename), false);
		    secu_string t2 = dialog.get_secu_string(gettext("Please confirm your password: "), false);
		    if(t1 == t2)
			real_pass = t1;
		    else
			throw Erange("op_create_in_sub", gettext("The two passwords are not identical. Aborting"));
		}

		switch(crypto)
		{
		case crypto_scrambling:
		case crypto_blowfish:
		case crypto_aes256:
		case crypto_twofish256:
		case crypto_serpent256:
		case crypto_camellia256:
		    ver.flag |= VERSION_FLAG_SCRAMBLED;
		    break;
		case crypto_none:
		    break; // no bit to set;
		default:
		    throw Erange("libdar:op_create_in_sub",gettext("Current implementation does not support this (new) crypto algorithm"));
		}

		if(add_marks_for_sequential_reading)
		    ver.flag |= VERSION_FLAG_SEQUENCE_MARK;

		    // we drop the header at the beginning of the archive in any case (to be able to
		    // know whether sequential reading is possible or not, and if sequential reading
		    // is asked, be able to get the required parameter to read the archive.
		    // It also servers of backup copy for normal reading if the end of the archive
		    // is corrupted.

		ver.write(stack);

		    // now we can add the initial offset in the archive_header datastructure, which will be written
		    // a second time, but at the end of the archive. If we start reading the archive from the end
		    // we must know where ended the initial archive header.

		ver.initial_offset = stack.get_position();

		    // ************ building the encryption layer if required ****** //

		if(!empty)
		{
		    switch(crypto)
		    {
		    case crypto_scrambling:
			tmp = new scrambler(real_pass, *(stack.top()));
			break;
		    case crypto_blowfish:
		    case crypto_aes256:
		    case crypto_twofish256:
		    case crypto_serpent256:
		    case crypto_camellia256:
			tmp = new crypto_sym(crypto_size, real_pass, *(stack.top()), false, macro_tools_supported_version, crypto);
			break;
		    case crypto_none:
			tmp = new cache(*(stack.top()), false);
			break;
		    default:
			throw SRC_BUG; // cryto value should have been checked before
		    }

		    if(tmp == NULL)
			throw Ememory("op_create_in_sub");
		    else
		    {
			stack.push(tmp);
			tmp = NULL;
		    }

		    if(crypto != crypto_none) // initial elastic buffer
			tools_add_elastic_buffer(stack, GLOBAL_ELASTIC_BUFFER_SIZE);
		}

		    // ********** if required building the escape layer  ***** //

		if(add_marks_for_sequential_reading && ! empty)
		{
		    set<escape::sequence_type> unjump;

		    unjump.insert(escape::seqt_catalogue);
		    tmp = esc = new escape(stack.top(), unjump);
		    if(tmp == NULL)
			throw Ememory("op_create_in_sub");
		    else
		    {
			stack.push(tmp);
			tmp = NULL;
		    }
		}

		    // ********** building the level2 layer (compression) ************************ //

		tmp = new compressor(empty ? none : algo, *(stack.top()), compression_level);
		if(tmp == NULL)
		    throw Ememory("op_create_in_sub");
		else
		{
		    stack.push(tmp);
		    tmp = NULL;
		}

		    // ********** building the catalogue (empty for now) ************************* //

		infinint root_mtime;

		try
		{
		    if(fs_root.display() != "<ROOT>")
			root_mtime = tools_get_mtime(fs_root.display());
		    else
		    {
			infinint mtime1 = ref_cat1 != NULL ? ref_cat1->get_root_mtime() : 0;
			infinint mtime2 = ref_cat2 != NULL ? ref_cat2->get_root_mtime() : 0;
			root_mtime = mtime1 > mtime2 ? mtime1 : mtime2;
		    }
		}
		catch(Erange & e)
		{
		    string tmp = fs_root.display();
		    throw Erange("archive::op_create_in_sub", tools_printf(gettext("Error while fetching information for %S: "), &tmp) + e.get_message());
		}

		if(op == oper_isolate)
		    if(add_marks_for_sequential_reading && !empty)
			cat = new escape_catalogue(dialog, ref_cat1->get_root_dir_last_modif(), ref_cat1->get_data_name(), esc);
		    else
			cat = new catalogue(dialog, ref_cat1->get_root_dir_last_modif(), ref_cat1->get_data_name());
		else
		    if(op == oper_merge)
			if(add_marks_for_sequential_reading && !empty)
			    cat = new escape_catalogue(dialog, ref_cat1->get_root_dir_last_modif(), data_name, esc);
			else
			    cat = new catalogue(dialog, ref_cat1->get_root_dir_last_modif(), data_name);
		    else // op == oper_create
			if(add_marks_for_sequential_reading && !empty)
			    cat = new escape_catalogue(dialog, root_mtime, data_name, esc);
			else
			    cat = new catalogue(dialog, root_mtime, data_name);

		if(cat == NULL)
		    throw Ememory("archive::op_create_in_sub");

		    // *********** now we can perform the data filtering operation (adding data to the archive) *************** //

		try
		{
		    catalogue *void_cat = NULL;
		    const catalogue *ref_cat_ptr = ref_cat1;

		    switch(op)
		    {
		    case oper_create:
			if(ref_cat1 == NULL)
			{
			    label data_name;
			    data_name.clear();
			    void_cat = new catalogue(dialog,
						     0,
						     data_name);
			    if(void_cat == NULL)
				throw Ememory("archive::op_create_in_sub");
			    ref_cat_ptr = void_cat;
			}

			try
			{
			    filtre_sauvegarde(dialog,
					      selection,
					      subtree,
					      stack,
					      *cat,
					      *ref_cat_ptr,
					      fs_root,
					      info_details,
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
					      display_skipped,
					      security_check,
					      repeat_count,
					      repeat_byte,
					      fixed_date,
					      sparse_file_min_size,
					      backup_hook_file_execute,
					      backup_hook_file_mask,
					      ignore_unknown);
			}
			catch(...)
			{
			    if(void_cat != NULL)
			    {
				delete void_cat;
				void_cat = NULL;
			    }
			    throw;
			}
			if(void_cat != NULL)
			{
			    delete void_cat;
			    void_cat = NULL;
			}
			break;
		    case oper_isolate:
			st_ptr->clear(); // clear st, as filtre_isolate does not use it
			filtre_isolate(dialog, *cat, *ref_cat1, info_details);
			break;
		    case oper_merge:
			filtre_merge(dialog,
				     selection,
				     subtree,
				     stack,
				     *cat,
				     ref_cat1,
				     ref_cat2,
				     info_details,
				     *st_ptr,
				     empty_dir,
				     ea_mask,
				     compr_mask,
				     min_compr_size,
				     display_skipped,
				     keep_compressed,
				     overwrite,
				     warn_over,
				     decremental,
				     sparse_file_min_size);
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
			if(e_attr != NULL)
			    aborting_next_etoile = e_attr->get_attr();
			else
			    aborting_next_etoile = 0;
			thr_cancel.block_delayed_cancellation(true);
			    // we must protect the following code against delayed cancellations
			stack.top()->sync_write(); // flushing only the top of the stack (compressor) must not yet flush the below encryption layer!!!
		    }
		}

		    // *********** writing down the catalogue of the archive *************** //

		if(add_marks_for_sequential_reading && !empty)
		{
		    if(esc != NULL)
			esc->add_mark_at_current_position(escape::seqt_catalogue);
		    else
			throw SRC_BUG;
		}

		coord.set_catalogue_start(stack.get_position());
		if(ref_cat1 != NULL && op == oper_create)
		{
		    if(info_details)
			dialog.warning(gettext("Adding reference to files that have been destroyed since reference backup..."));
		    if(aborting)
			cat->update_absent_with(*ref_cat1, aborting_next_etoile);
		    else
			st_ptr->add_to_deleted(cat->update_destroyed_with(*ref_cat1));
		}

		if(info_details)
		    dialog.warning(gettext("Writing archive contents..."));
		cat->dump(stack);
		stack.top()->sync_write();

		    // releasing the compression layer

		if(!stack.pop_and_close_if_type_is(compr_ptr))
		    throw SRC_BUG;

		    // releasing the escape layer

		if(esc != NULL)
		{
		    if(!add_marks_for_sequential_reading)
			throw SRC_BUG;
		    esc = NULL; // intentionnally set to NULL here, only the pointer type is used by pop_and_close
		    if(!stack.pop_and_close_if_type_is(esc))
			throw SRC_BUG;
		}

		    // *********** writing down the first terminator at the end of the archive  *************** //

		coord.dump(stack); // since format "04" the terminateur is encrypted
		if(crypto != crypto_none)
		    tools_add_elastic_buffer(stack, GLOBAL_ELASTIC_BUFFER_SIZE);
		    // terminal elastic buffer (after terminateur to protect against
		    // plain text attack on the terminator string)


		    // releasing memory by calling destructors and releases file descriptors

		tronco_ptr = dynamic_cast<tronconneuse *>(stack.top());
		if(tronco_ptr != NULL)
		    tronco_ptr->write_end_of_file();

		if(!stack.pop_and_close_if_type_is(tronco_ptr))
		{
		    scrambler *ptr = NULL;
		    (void)stack.pop_and_close_if_type_is(ptr);
		}

		    // *********** writing down the trailier_version with the second terminateur *************** //

		coord.set_catalogue_start(stack.get_position());
		ver.write(stack);
		coord.dump(stack);
		stack.sync_write();


		    // *********** closing the archive ******************** //

		stack.clear(); // closing all generic_files remaining in the stack

		thr_cancel.block_delayed_cancellation(false);
		    // release pending delayed cancellation (if any)

		if(aborting)
		    throw Ethread_cancel(false, flag);
	    }
	    catch(...)
	    {
		if(tmp != NULL)
		{
		    delete tmp;
		    tmp = NULL;
		}
		if(cat != NULL)
		{
		    delete cat;
		    cat = NULL;
		}
		throw;
	    }
	}
	catch(Euser_abort & e)
	{
	    disable_natural_destruction();
	    throw;
	}
	catch(Erange &e)
	{
	    string msg = string(gettext("Error while saving data: ")) + e.get_message();
	    throw Edata(msg);
	}
    }

    void archive::free()
    {
        if(cat != NULL)
	{
            delete cat;
	    cat = NULL;
	}
    }

    void archive::disable_natural_destruction()
    {
        sar *tmp = NULL;

	stack.find_first_from_bottom(tmp);
	if(tmp != NULL)
            tmp->disable_natural_destruction();
    }

    void archive::enable_natural_destruction()
    {
        sar *tmp = NULL;

	stack.find_first_from_bottom(tmp);
        if(tmp != NULL)
            tmp->enable_natural_destruction();
    }

    const label & archive::get_layer1_data_name() const
    {
	contextual *l1 = NULL;
	archive *ceci = const_cast<archive *>(this);

	ceci->stack.find_first_from_bottom(l1);
	if(l1 != NULL)
	    return l1->get_data_name();
	else
	    throw Erange("catalogue::get_data_name", gettext("Cannot get data name of the archive, this archive is not completely initialized"));
    }

    const label & archive::get_catalogue_data_name() const
    {
	if(cat != NULL)
	    return cat->get_data_name();
	else
	    throw SRC_BUG;
    }

    bool archive::only_contains_an_isolated_catalogue() const
    {
	return get_layer1_data_name() != get_catalogue_data_name() && ver.edition >= 8;
    }

    void archive::check_against_isolation(user_interaction & dialog, bool lax) const
    {
	if(cat != NULL)
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

    void archive::check_header_version() const
    {
	if((ver.flag
	    & ! VERSION_FLAG_SAVED_EA_ROOT // since archive "05" we don't care this flag
	    & ! VERSION_FLAG_SAVED_EA_USER // since archive "05" we don't care this flag
	    & ! VERSION_FLAG_SCRAMBLED) != 0)
	    throw Erange("archive::archive", gettext("Not supported flag or archive corruption"));
    }

    bool archive::get_sar_param(infinint & sub_file_size, infinint & first_file_size, infinint & last_file_size,
                                infinint & total_file_number)
    {
        sar *real_decoupe = NULL;

	stack.find_first_from_bottom(real_decoupe);
        if(real_decoupe != NULL)
        {
            sub_file_size = real_decoupe->get_sub_file_size();
            first_file_size = real_decoupe->get_first_sub_file_size();
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
	const entrepot *ret = NULL;
	sar *real_decoupe = NULL;

	stack.find_first_from_bottom(real_decoupe);
	if(real_decoupe != NULL)
	{
	    ret = real_decoupe->get_entrepot();
	    if(ret == NULL)
		throw SRC_BUG;
	}

	return ret;
    }


    infinint archive::get_level2_size()
    {
	compressor *level2 = NULL;
	stack.find_first_from_top(level2);
	if(level2 == NULL)
	    throw SRC_BUG;
        level2->skip_to_eof();
        return level2->get_position();
    }



} // end of namespace
