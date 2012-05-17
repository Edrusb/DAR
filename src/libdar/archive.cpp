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
// $Id: archive.cpp,v 1.40.2.9 2009/04/07 08:45:29 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"

#include "infinint.hpp"
#include "generic_file.hpp"
#include "archive.hpp"
#include "sar.hpp"
#include "macro_tools.hpp"
#include "test_memory.hpp"
#include "filtre.hpp"
#include "sar.hpp"
#include "tools.hpp"
#include "header.hpp"
#include "header_version.hpp"
#include "sar_tools.hpp"
#include "tuyau.hpp"
#include "scrambler.hpp"
#include "null_file.hpp"
#include "crypto.hpp"
#include "elastic.hpp"
#include "terminateur.hpp"
#include "compressor.hpp"
#include "nls_swap.hpp"
#include "thread_cancellation.hpp"

#define GLOBAL_ELASTIC_BUFFER_SIZE 10240

#define ARCHIVE_NOT_EXPLOITABLE "Archive of reference given is not exploitable"

using namespace std;

namespace libdar
{
    archive::archive(user_interaction & dialog,
                     const path & chem, const std::string & basename, const std::string & extension,
                     crypto_algo crypto, const std::string &pass, U_32 crypto_size,
                     const std::string & input_pipe, const std::string & output_pipe,
                     const std::string & execute, bool info_details)
    {
        level1 = NULL;
        scram = NULL;
        level2 = NULL;
        cat = NULL;
        local_path = NULL;

        NLS_SWAP_IN;
        try
        {
            macro_tools_open_archive(dialog, chem, basename, extension, crypto, pass, crypto_size, level1, scram, level2, ver,
                                     input_pipe, output_pipe, execute);
            if((ver.flag
                && ! VERSION_FLAG_SAVED_EA_ROOT // since archive "05" we don't care this flag
                && ! VERSION_FLAG_SAVED_EA_USER // since archive "05" we don't care this flag
                && ! VERSION_FLAG_SCRAMBLED) != 0)
                throw Erange("archive::archive", gettext("Not supported flag or archive corruption"));

            cat = macro_tools_get_catalogue_from(dialog, *level1, ver, *level2, info_details, local_cat_size, scram != NULL ? scram : level1);
            local_path = new path(chem);
            if(local_path == NULL)
                throw Ememory("archive::archive");
            exploitable = true;
        }
        catch(...)
        {
            free();
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    archive::archive(user_interaction & dialog,
                     const path & fs_root,
                     const path & sauv_path,
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
                     const infinint &file_size,
                     const infinint &first_file_size,
		     const mask & ea_mask,
                     const string & execute,
                     crypto_algo crypto,
                     const string & pass,
                     U_32 crypto_size,
                     const mask & compr_mask,
                     const infinint & min_compr_size,
                     bool nodump,
                     inode::comparison_fields what_to_check,
                     const infinint & hourshift,
                     bool empty,
                     bool alter_atime,
                     bool same_fs,
                     bool snapshot,
                     bool cache_directory_tagging,
		     bool display_skipped,
		     const infinint & fixed_date,
                     statistics * progressive_report)
    {
        NLS_SWAP_IN;
        try
        {
            (void)op_create_in(dialog, oper_create,
			       tools_relative2absolute_path(fs_root, tools_getcwd()),
			       sauv_path, ref_arch, selection,
			       subtree, filename, extension,
			       allow_over, warn_over,
			       info_details, pause, empty_dir, algo, compression_level, file_size, first_file_size, ea_mask,
			       execute, crypto, pass, crypto_size, compr_mask, min_compr_size,
			       nodump, hourshift, empty, alter_atime, same_fs, what_to_check, snapshot, cache_directory_tagging,
			       display_skipped, fixed_date, progressive_report);
            exploitable = false;
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    archive::archive(user_interaction & dialog,
                     const path &sauv_path,
                     archive *ref_arch,
                     const string & filename,
                     const string & extension,
                     bool allow_over,
                     bool warn_over,
                     bool info_details,
                     const infinint & pause,
                     compression algo,
                     U_I compression_level,
                     const infinint &file_size,
                     const infinint &first_file_size,
                     const string & execute,
                     crypto_algo crypto,
                     const string & pass,
                     U_32 crypto_size,
                     bool empty)
    {
        NLS_SWAP_IN;
        try
        {
            (void)op_create_in(dialog, oper_isolate, path("."), sauv_path,
                               ref_arch, bool_mask(false), bool_mask(false),
                               filename, extension, allow_over, warn_over, info_details,
                               pause, false, algo, compression_level, file_size, first_file_size, bool_mask(true),
                               execute, crypto, pass, crypto_size,
                               bool_mask(false), 0, false, 0, empty, false, false, inode::cf_all, false, false, false, 0, NULL);
                // we ignore returned value;
            exploitable = false;
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }


    archive::archive(user_interaction & dialog,
		     const path & sauv_path,
		     archive *ref_arch1,
		     archive *ref_arch2,
		     const mask & selection,
		     const mask & subtree,
		     const std::string & filename,
		     const std::string & extension,
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
		     const std::string & execute,
		     crypto_algo crypto,
		     const std::string & pass,
		     U_32 crypto_size,
		     const mask & compr_mask,
		     const infinint & min_compr_size,
		     bool empty,
		     bool display_skipped,
		     bool keep_compressed,
		     statistics * progressive_report)
    {
	statistics st = false;
	statistics *st_ptr = progressive_report == NULL ? &st : progressive_report;
	catalogue *ref_cat1 = NULL;
	catalogue *ref_cat2 = NULL;


	NLS_SWAP_IN;
	try
	{
	    level1 = NULL;
	    scram = NULL;
	    level2 = NULL;
	    exploitable = false;

		// sanity checks as much as possible to avoid libdar crashing due to bad arguments
		// useless arguments are not reported.

	    if(&sauv_path == NULL)
		throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"sauv_path\""));
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
	    if(&pass == NULL)
		throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"pass\""));
	    if(&compr_mask == NULL)
		throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"compr_mask\""));
	    if(&min_compr_size == NULL)
		throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"min_compr_size\""));
	    if(crypto_size < 10 && crypto != crypto_none)
		throw Elibcall("op_create/op_isolate", gettext("Crypto block size must be greater than 10 bytes"));
	    if(&ea_mask == NULL)
		throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"ea_mask\""));

	    local_path = new path(sauv_path);
	    if(local_path == NULL)
		throw Ememory("archive::archive[merge]");

	    if(!empty)
		tools_avoid_slice_overwriting(dialog,
					      sauv_path,
					      filename+".*."+extension,
					      info_details,
					      allow_over,
					      warn_over,
					      empty);

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
		    if(ref_arch1->ver.algo_zip != ref_arch2->ver.algo_zip && char2compression(ref_arch1->ver.algo_zip) != none && char2compression(ref_arch2->ver.algo_zip) != none && keep_compressed)
			throw Efeature(gettext("the \"Keep file compressed\" feature is not possible when merging two archives using different compression algorithms (This is for a future version of dar). You can still merge these two archives but without keeping file compressed (thus you will probably like to use compression (-z or -y options) for the resulting archive"));
		}

	    if(keep_compressed)
	    {
		if(ref_arch1 == NULL)
		    throw SRC_BUG;
		algo = char2compression(ref_arch1->ver.algo_zip);
		if(algo == none && ref_cat2 != NULL)
		{
		    if(ref_arch2 == NULL)
			throw SRC_BUG;
		    else
			algo = char2compression(ref_arch2->ver.algo_zip);
		}
	    }

		// then we call op_create_in_sub which will call filter_merge operation to build the archive described by the catalogue
	    op_create_in_sub(dialog,
			     oper_merge,
			     path(FAKE_ROOT),
			     sauv_path,
			     ref_cat1,
			     ref_cat2,
			     NULL,  // ref_path
			     selection,
			     subtree,
			     filename,
			     extension,
			     allow_over,
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
			     false,   // nodump
			     0,       // hourshift
			     empty,   // dry-run
			     true,    // alter_atime
			     false,   // same_fs
			     inode::cf_all,   // what_to_check
			     false,   // snapshot
			     false,   // cache_directory_tagging
			     display_skipped,
			     keep_compressed,
			     0,       // fixed_date
			     st_ptr);
	    exploitable = false;
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    bool archive::get_sar_param(infinint & sub_file_size, infinint & first_file_size, infinint & last_file_size,
                                infinint & total_file_number)
    {
        sar *real_decoupe = dynamic_cast<sar *>(level1);
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


    infinint archive::get_level2_size()
    {
        level2->skip_to_eof();
        return level2->get_position();
    }


    static void dummy_call(char *x)
    {
        static char id[]="$Id: archive.cpp,v 1.40.2.9 2009/04/07 08:45:29 edrusb Rel $";
        dummy_call(id);
    }

    statistics archive::op_extract(user_interaction & dialog,
                                   const path & fs_root,
                                   const mask & selection, const mask & subtree,
                                   bool allow_over, bool warn_over,
                                   bool info_details,  bool detruire,
                                   bool only_more_recent,
				   const mask & ea_mask,
                                   bool flat,
				   inode::comparison_fields what_to_check,
                                   bool warn_remove_no_match,
                                   const infinint & hourshift,
                                   bool empty,
				   bool ea_erase,
				   bool display_skipped,
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
            if(&selection == NULL)
                throw Elibcall("op_extract", gettext("NULL argument given to \"selection\""));
            if(&subtree == NULL)
                throw Elibcall("op_extract", gettext("NULL argument given to \"subtree\""));
            if(&hourshift == NULL)
                throw Elibcall("op_extract", gettext("NULL argument given to \"hourshift\""));
	    if(&ea_mask == NULL)
                throw Elibcall("op_extract", gettext("NULL argument given to \"ea_mask\""));

                // end of sanity checks

            enable_natural_destruction();

            try
            {
                MEM_IN;

                filtre_restore(dialog, selection, subtree, get_cat(), detruire,
			       tools_relative2absolute_path(fs_root, tools_getcwd()),
                               allow_over, warn_over, info_details,
                               *st_ptr, only_more_recent,
			       ea_mask,
                               flat,
			       what_to_check,
			       warn_remove_no_match,
                               hourshift, empty, ea_erase, display_skipped);
                MEM_OUT;
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
            throw;
        }
        NLS_SWAP_OUT;

        return *st_ptr;
    }


    void archive::op_listing(user_interaction & dialog,
                             bool info_details,
			     listformat list_mode,
                             const mask & selection,
			     bool filter_unsaved)
    {
        NLS_SWAP_IN;
        try
        {
                // sanity checks

            if(!exploitable)
                throw Elibcall("op_listing", gettext("This archive is not exploitable, check the archive class usage in the API documentation"));
            if(&selection == NULL)
                throw Elibcall("op_listing", gettext("NULL argument given to \"selection\""));

                // end of sanity checks

            enable_natural_destruction();
            try
            {
                MEM_IN;

                if(info_details)
                {
                    infinint sub_file_size;
                    infinint first_file_size;
                    infinint last_file_size, file_number;
                    string algo = compression2string(char2compression(get_header().algo_zip));
                    infinint cat_size = get_cat_size();

                    dialog.printf(gettext("Archive version format               : %s\n"), get_header().edition);
                    dialog.printf(gettext("Compression algorithm used           : %S\n"), &algo);
                    dialog.printf(gettext("Scrambling or strong encryption used : %s\n"), ((get_header().flag & VERSION_FLAG_SCRAMBLED) != 0 ? gettext("yes") : gettext("no")));
                    dialog.printf(gettext("Archive contents size in archive     : %i bytes\n"), &cat_size);
                        // the following field is no more used (lack of pertinence, when included files are used).
                        // dialog.printf(gettext("Command line options used for backup : %S\n"), &(get_header().cmd_line));

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
                    stats.listing(dialog);
                    dialog.pause(gettext("Continue listing archive contents?"));
                }

                switch(list_mode)
		{
		case normal:
		    get_cat().tar_listing(selection, filter_unsaved);
		    break;
		case tree:
		    get_cat().listing(selection, filter_unsaved);
		    break;
		case xml:
		    get_cat().xml_listing(selection, filter_unsaved);
		    break;
		default:
		    throw SRC_BUG;
                }

                MEM_OUT;
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
            throw;
        }
        NLS_SWAP_OUT;
    }

    statistics archive::op_diff(user_interaction & dialog,
                                const path & fs_root,
                                const mask & selection, const mask & subtree,
                                bool info_details,
				const mask & ea_mask,
                                inode::comparison_fields what_to_check,
                                bool alter_atime,
				bool display_skipped,
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
            if(&selection == NULL)
                throw Elibcall("op_diff", gettext("NULL argument given to \"selection\""));
            if(&subtree == NULL)
                throw Elibcall("op_diff", gettext("NULL argument given to \"subtree\""));
	    if(&ea_mask == NULL)
		throw Elibcall("op_diff", gettext("NULL argument given to \"ea_mask\""));

                // end of sanity checks
            enable_natural_destruction();
            try
            {
                filtre_difference(dialog,
				  selection,
				  subtree,
				  get_cat(),
				  tools_relative2absolute_path(fs_root, tools_getcwd()),
				  info_details,
				  *st_ptr,
				  ea_mask,
				  alter_atime, what_to_check,
				  display_skipped);
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
            throw;
        }

        NLS_SWAP_OUT;
        return *st_ptr;
    }

    statistics archive::op_test(user_interaction & dialog,
                                const mask & selection,
                                const mask & subtree,
                                bool info_details,
				bool display_skipped,
				statistics * progressive_report)
    {
        statistics st = false;  // false => no lock for this internal object
	statistics *st_ptr = progressive_report == NULL ? &st : progressive_report;

        NLS_SWAP_IN;
        try
        {

                // sanity checks

            if(!exploitable)
                throw Elibcall("op_test", gettext("This archive is not exploitable, check the archive class usage in the API documentation"));
            if(&selection == NULL)
                throw Elibcall("op_test", gettext("NULL argument given to \"selection\""));
            if(&subtree == NULL)
                throw Elibcall("op_test", gettext("NULL argument given to \"subtree\""));

                // end of sanity checks
            enable_natural_destruction();
            try
            {
                filtre_test(dialog, selection, subtree, get_cat(), info_details, *st_ptr, display_skipped);
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
            throw;
        }
        NLS_SWAP_OUT;

        return *st_ptr;
    }

    bool archive::get_children_of(user_interaction & dialog,
                                  const string & dir)
    {
	bool ret;
        NLS_SWAP_IN;
        try
        {
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


    statistics archive::op_create_in(user_interaction & dialog,
                                     operation op,
				     const path & fs_root,
                                     const path & sauv_path,
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
                                     const string & pass,
                                     U_32 crypto_size,
                                     const mask & compr_mask,
                                     const infinint & min_compr_size,
                                     bool nodump,
                                     const infinint & hourshift,
                                     bool empty,
                                     bool alter_atime,
                                     bool same_fs,
				     inode::comparison_fields what_to_check,
                                     bool snapshot,
                                     bool cache_directory_tagging,
				     bool display_skipped,
				     const infinint & fixed_date,
				     statistics * progressive_report)
    {
        statistics st = false;  // false => no lock for this internal object
	statistics *st_ptr = progressive_report == NULL ? &st : progressive_report;

            // sanity checks as much as possible to avoid libdar crashing due to bad arguments
            // useless arguments are not reported.

        if(&fs_root == NULL)
            throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"fs_root\""));
        if(&sauv_path == NULL)
            throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"sauv_path\""));
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
        if(&pass == NULL)
            throw Elibcall("op_create/op_isolate", gettext("NULL argument given to \"pass\""));
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

	tools_avoid_slice_overwriting(dialog,
				      sauv_path,
				      filename+".*."+extension,
				      info_details,
				      allow_over,
				      warn_over,
				      empty);

	catalogue *ref_cat = NULL;
	const path *ref_path = NULL;

	local_cat_size = 0; // unknown catalogue size (need to re-open the archive, once creation has completed) [object member variable]
	local_path = new path(sauv_path); // [object member variable]
	if(local_path == NULL)
	    throw Ememory("archive::op_create_in");

	try
	{

		// warning against saving the archive itself

	    if(op == oper_create &&
	       sauv_path.is_subdir_of(fs_root, true)
	       && selection.is_covered(filename+".1."+extension)
	       && filename!= "-")
	    {
		path tmp = sauv_path; // to be able to step by step cut the path, we need to work on a copy
		bool cov = true;      // whether the archive is covered by filter (this is saving itself)
		string drop;          // will carry the removed part of the tmp variable

		    // checking for exclusion due to different filesystem

		if(same_fs && !tools_are_on_same_filesystem(tmp.display(), fs_root.display()))
		    cov = false;

		if(snapshot)     // if we do a snapshot we dont create an archive this no risk to save ourselves
		    cov = false;

		    // checking for directory auto inclusion
		do
		{
		    cov = cov && subtree.is_covered(tmp.display());
		}
		while(cov && tmp.pop(drop));

		if(cov)
		    dialog.pause(tools_printf(gettext("WARNING! The archive is located in the directory to backup, this may create an endless loop when the archive will try to save itself. You can either add -X \"%S.*.%S\" on the command line, or change the location of the archive (see -h for help). Do you really want to continue?"), &filename, &extension));
	    }

		// building the reference catalogue

	    if(ref_arch != NULL) // from a existing archive
	    {
		ref_cat = &(ref_arch->get_cat());    // this is the copy of a reference not of the object [we don't have to delete this address]
		ref_path = & ref_arch->get_path();   // path to the archive of reference (here too this is the copy of a reference)
	    }
	    else // building reference catalogue from scratch (empty catalogue)
	    {
		ref_cat = new catalogue(dialog);    // here this is a new object we will have to release at the end of this method
		ref_path = NULL;                    // should already be set to NULL
	    }

	    try
	    {
		op_create_in_sub(dialog,
				 op,
				 fs_root,
				 sauv_path,
				 ref_cat,
				 NULL,
				 ref_path,
				 selection,
				 subtree,
				 filename,
				 extension,
				 allow_over,
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
				 same_fs,
				 what_to_check,
				 snapshot,
				 cache_directory_tagging,
				 display_skipped,
				 false,   // keep_compressed
				 fixed_date,
				 st_ptr);
	    }
	    catch(...)
	    {
		if(ref_cat != NULL && ref_arch == NULL)
		    delete ref_cat;
		throw;
	    }

	    if(ref_cat != NULL && ref_arch == NULL)
		delete ref_cat;
	}
	catch(...)
	{
	    if(local_path != NULL)
	    {
		delete local_path;
		local_path = NULL;
	    }
	    throw;
	}

	return *st_ptr;
    }


    void archive::op_create_in_sub(user_interaction & dialog,
				   operation op,
				   const path & fs_root,
				   const path & sauv_path_t,
				   catalogue  *ref_cat1,
				   catalogue  *ref_cat2,
				   const path * ref_path,
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
				   const string & pass,
				   U_32 crypto_size,
				   const mask & compr_mask,
				   const infinint & min_compr_size,
				   bool nodump,
				   const infinint & hourshift,
				   bool empty,
				   bool alter_atime,
				   bool same_fs,
				   inode::comparison_fields what_to_check,
				   bool snapshot,
				   bool cache_directory_tagging,
				   bool display_skipped,
				   bool keep_compressed,
				   const infinint & fixed_date,
				   statistics * st_ptr)
    {
	try
	{
	    level1 = NULL; // [object member variable]
	    scram = NULL;  // [object member variable]
	    level2 = NULL; // [object member variable]
	    cat = NULL;    // [object member variable]
	    bool aborting = false;
	    U_64 flag;     // carries the sar option flag
	    terminateur coord;

	    path sauv_path = tools_relative2absolute_path(sauv_path_t, tools_getcwd());

	    if(ref_cat1 == NULL)
		SRC_BUG; // ref_cat1 must never be NULL
	    if(st_ptr == NULL)
		throw SRC_BUG;

	    string real_pass = pass;

	    try
	    {
		thread_cancellation thr_cancel;
		generic_file *zip_base = NULL;

		    // pausing if saving in the same directory where is located the archive of reference

		if(pause != 0 && ref_path != NULL && *ref_path == sauv_path)
		    dialog.pause(gettext("Ready to start writing down the archive?"));

		    // building the level 1 generic_file layer

		if(empty)
		    level1 = new null_file(dialog, gf_write_only);
		else
		    if(file_size == 0) // one SLICE
			if(filename == "-") // output to stdout
			    level1 = sar_tools_open_archive_tuyau(dialog, 1, gf_write_only); //archive goes to stdout
			else
			    level1 = sar_tools_open_archive_fichier(dialog, (sauv_path + sar_make_filename(filename, 1, extension)).display(), allow_over, warn_over);
		    else
			level1 = new sar(dialog, filename, extension, file_size, first_file_size, warn_over, allow_over, pause, sauv_path, execute);

		if(level1 == NULL)
		    throw Ememory("op_create_in_sub");

		    // creating the archive header in the level 1 layer

		version_copy(ver.edition, macro_tools_supported_version);
		ver.algo_zip = compression2char(algo);
		ver.cmd_line = "N/A";
		ver.flag = 0;

		    // optaining a password on-fly if necessary

		if(crypto != crypto_none && real_pass == "")
		{
		    string t1 = dialog.get_string(tools_printf(gettext("Archive %S requires a password: "), &filename), false);
		    string t2 = dialog.get_string(gettext("Please confirm your password: "), false);
		    if(t1 == t2)
			real_pass = t1;
		    else
			throw Erange("op_create_in_sub", gettext("The two passwords are not identical. Aborting"));
		}

		switch(crypto)
		{
		case crypto_scrambling:
		case crypto_blowfish:
		case crypto_blowfish_weak:
		    ver.flag |= VERSION_FLAG_SCRAMBLED;
		    break;
		case crypto_none:
		    break; // no bit to set;
		default:
		    throw Erange("libdar:op_create_in_sub",gettext("Unknown crypto algorithm"));
		}
		ver.write(*level1);

		    // the archive header has just been written to level 1 layer,
		    // now creating the next layer

		    // building the encryption layer if necessary

		if(!empty)
		{
		    switch(crypto)
		    {
		    case crypto_scrambling:
			scram = new scrambler(dialog, real_pass, *level1);
			break;
		    case crypto_blowfish:
			scram = new blowfish(dialog, crypto_size, real_pass, *level1, macro_tools_supported_version, false);
			break;
		    case crypto_blowfish_weak:
			scram = new blowfish(dialog, crypto_size, real_pass, *level1, macro_tools_supported_version, true);
			break;
		    case crypto_none:
			zip_base = level1;
			break;
		    default:
			throw SRC_BUG; // cryto value should have been checked before
		    }

		    if(crypto != crypto_none)
		    {
			if(scram == NULL)
			    throw Ememory("op_create_in_sub");
			zip_base = scram;
			tools_add_elastic_buffer(*zip_base, GLOBAL_ELASTIC_BUFFER_SIZE);
			    // initial elastic buffer
		    }
		}
		else
		    zip_base = level1;

		    // building the level2 layer (compression)

		level2 = new compressor(dialog, empty ? none : algo, *zip_base, compression_level);
		if(level2 == NULL)
		    throw Ememory("op_create_in_sub");

		    // building the catalogue (empty for now)

		cat = new catalogue(dialog);
		if(cat == NULL)
		    throw Ememory("archive::op_create_in_sub");

		    // now we can perform the data filtering operation

		try
		{
		    switch(op)
		    {
		    case oper_create:
			filtre_sauvegarde(dialog,
					  selection,
					  subtree,
					  level2,
					  *cat,
					  *ref_cat1,
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
					  same_fs,
					  what_to_check,
					  snapshot,
					  cache_directory_tagging,
					  display_skipped,
					  fixed_date);
			break;
		    case oper_isolate:
			st_ptr->clear(); // clear st, as filtre_isolate does not use it
			filtre_isolate(dialog, *cat, *ref_cat1, info_details);
			break;
		    case oper_merge:
			filtre_merge(dialog,
				     selection,
				     subtree,
				     level2,
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
				     keep_compressed);
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
			aborting = true;
			flag = e.get_flag();
			thr_cancel.block_delayed_cancellation(true);
			    // we must protect the following code against delayed cancellations
		    }
		}

		    // flushing caches

		level2->flush_write();

		    // writing down the catalogue of the archive

		coord.set_catalogue_start(level2->get_position());
		if(ref_path != NULL && op == oper_create)
		{
		    if(info_details)
			dialog.warning(gettext("Adding reference to files that have been destroyed since reference backup..."));
		    if(aborting)
			cat->update_absent_with(*ref_cat1);
		    else
			st_ptr->add_to_deleted(cat->update_destroyed_with(*ref_cat1));
		}

		if(info_details)
		    dialog.warning(gettext("Writing archive contents..."));
		cat->dump(*level2);
		level2->flush_write();
		delete level2;
		level2 = NULL;

		    // writing down the terminator at the end of the archive

		coord.dump(*zip_base); // since format "04" the terminateur is encrypted
		if(crypto != crypto_none)
		    tools_add_elastic_buffer(*zip_base, GLOBAL_ELASTIC_BUFFER_SIZE);
		    // terminal elastic buffer (after terminateur to protect against
		    // plain text attack on the terminator string)


		    // releasing memory which calls destructors and releases file descriptors

		if(scram != NULL)
		{
		    tronconneuse *ptr = dynamic_cast<tronconneuse *>(scram);

		    if(ptr != NULL)
			ptr->write_end_of_file();
		    delete scram;
		    scram = NULL;
		}
		delete level1;
		level1 = NULL;

		thr_cancel.block_delayed_cancellation(false);
		    // release pending delayed cancellation (if any)

		if(aborting)
		    throw Ethread_cancel(false, flag);
	    }
	    catch(...)
	    {
		if(level2 != NULL)
		{
		    level2->clean_write();
		    delete level2;
		    level2 = NULL;
		}
		if(scram != NULL)
		{
		    delete scram;
		    scram = NULL;
		}
		if(level1 != NULL)
		{
		    disable_natural_destruction();
		    delete level1;
		    level1 = NULL;
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
	    dialog.warning(msg);
	    throw Edata(msg);
	}
    }


    void archive::free()
    {
        if(cat != NULL)
            delete cat;
        if(level2 != NULL)
            delete level2;
        if(scram != NULL)
            delete scram;
        if(level1 != NULL)
            delete level1;
        if(local_path != NULL)
            delete local_path;
    }

    void archive::disable_natural_destruction()
    {
        sar *tmp = dynamic_cast<sar *>(level1);
        if(tmp != NULL)
            tmp->disable_natural_destruction();
    }

    void archive::enable_natural_destruction()
    {
        sar *tmp = dynamic_cast<sar *>(level1);
        if(tmp != NULL)
            tmp->enable_natural_destruction();
    }

} // end of namespace
