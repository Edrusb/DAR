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
// $Id: archive.cpp,v 1.21 2004/12/29 13:30:39 edrusb Rel $
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


#define GLOBAL_ELASTIC_BUFFER_SIZE 10240

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

	try
	{
	    macro_tools_open_archive(dialog, chem, basename, extension, SAR_OPT_DEFAULT, crypto, pass, crypto_size, level1, scram, level2, ver,
				     input_pipe, output_pipe, execute);
	    if((ver.flag & VERSION_FLAG_SAVED_EA_ROOT) == 0)
		restore_ea_root = false; // not restoring something not saved
	    if((ver.flag & VERSION_FLAG_SAVED_EA_USER) == 0)
		restore_ea_user = false; // not restoring something not saved
	    if((ver.flag
		&& ! VERSION_FLAG_SAVED_EA_ROOT
		&& ! VERSION_FLAG_SAVED_EA_USER
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
	    throw;
	}

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
		     bool pause,
		     bool empty_dir,
		     compression algo,
		     U_I compression_level,
		     const infinint &file_size,
		     const infinint &first_file_size,
		     bool root_ea,
		     bool user_ea,
		     const string & execute,
		     crypto_algo crypto,
		     const string & pass,
		     U_32 crypto_size,
		     const mask & compr_mask,
		     const infinint & min_compr_size,
		     bool nodump,
		     bool ignore_owner,
		     const infinint & hourshift,
		     bool empty,
		     bool alter_atime,
		     bool same_fs,
		     statistics & ret)
    {
        ret = op_create_in(dialog, true, fs_root, sauv_path, ref_arch, selection,
			   subtree, filename, extension,
			   allow_over, warn_over,
			   info_details, pause, empty_dir, algo, compression_level, file_size, first_file_size, root_ea, user_ea,
			   execute, crypto, pass, crypto_size, compr_mask, min_compr_size,
			   nodump, hourshift, empty, alter_atime, same_fs, ignore_owner);
	exploitable = false;
    }

    archive::archive(user_interaction & dialog,
		     const path &sauv_path,
		     archive *ref_arch,
		     const string & filename,
		     const string & extension,
		     bool allow_over,
		     bool warn_over,
		     bool info_details,
		     bool pause,
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
        (void)op_create_in(dialog, false, path("."), sauv_path,
			   ref_arch, bool_mask(false), bool_mask(false),
                           filename, extension, allow_over, warn_over, info_details,
                           pause, false, algo, compression_level, file_size, first_file_size, false,
                           false, execute, crypto, pass, crypto_size,
                           bool_mask(false), 0, false, 0, empty, false, false, false);
            // we ignore returned value;
	exploitable = false;
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
        static char id[]="$Id: archive.cpp,v 1.21 2004/12/29 13:30:39 edrusb Rel $";
        dummy_call(id);
    }

    statistics archive::op_extract(user_interaction & dialog,
				   const path & fs_root,
				   const mask & selection, const mask & subtree,
				   bool allow_over, bool warn_over,
				   bool info_details,  bool detruire,
				   bool only_more_recent, bool restore_ea_root,
				   bool restore_ea_user,
				   bool flat, bool ignore_owner,
				   bool warn_remove_no_match,
				   const infinint & hourshift,
				   bool empty)
    {
        statistics st;

            // sanity checks

	if(!exploitable)
	    throw Elibcall("op_test", gettext("This archive is not exploitable, check documentation for more"));
        if(&fs_root == NULL)
            throw Elibcall("op_extract", gettext("NULL argument given to \"fs_root\""));
        if(&selection == NULL)
            throw Elibcall("op_extract", gettext("NULL argument given to \"selection\""));
        if(&subtree == NULL)
            throw Elibcall("op_extract", gettext("NULL argument given to \"subtree\""));
	if(&hourshift == NULL)
	    throw Elibcall("op_extract", gettext("NULL argument given to \"hourshift\""));

            // end of sanity checks

	enable_natural_destruction();
        try
        {
	    MEM_IN;

	    filtre_restore(dialog, selection, subtree, get_cat(), detruire,
			   fs_root, allow_over, warn_over, info_details,
			   st, only_more_recent, restore_ea_root,
			   restore_ea_user, flat, ignore_owner, warn_remove_no_match,
 			   hourshift, empty);
	    MEM_OUT;
        }
	catch(Euser_abort & e)
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

        return st;
    }


    void archive::op_listing(user_interaction & dialog,
			     bool info_details, bool tar_format,
			     const mask & selection, bool filter_unsaved)
    {
            // sanity checks

	if(!exploitable)
	    throw Elibcall("op_test", gettext("This archive is not exploitable, check the archive class usage in the API documentation"));
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
		dialog.printf(gettext("Catalogue size in archive            : %i bytes\n"), &cat_size);
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

	    if(tar_format)
		get_cat().tar_listing(selection, filter_unsaved);
	    else
		get_cat().listing(selection, filter_unsaved);

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

    statistics archive::op_diff(user_interaction & dialog,
				const path & fs_root,
				const mask & selection, const mask & subtree,
				bool info_details,
				bool check_ea_root, bool check_ea_user,
				bool ignore_owner,
				bool alter_atime)
    {
        statistics st;

            // sanity checks

	if(!exploitable)
	    throw Elibcall("op_test", gettext("This archive is not exploitable, check documentation for more"));
        if(&fs_root == NULL)
            throw Elibcall("op_diff", gettext("NULL argument given to \"fs_root\""));
        if(&selection == NULL)
            throw Elibcall("op_diff", gettext("NULL argument given to \"selection\""));
        if(&subtree == NULL)
            throw Elibcall("op_diff", gettext("NULL argument given to \"subtree\""));

            // end of sanity checks
	enable_natural_destruction();
        try
        {
	    filtre_difference(dialog, selection, subtree, get_cat(), fs_root, info_details, st, check_ea_root, check_ea_user, alter_atime, ignore_owner);
        }
	catch(Euser_abort & e)
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

        return st;
    }

    statistics archive::op_test(user_interaction & dialog,
				const mask & selection,
				const mask & subtree,
				bool info_details)
    {
        statistics st;

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
	    filtre_test(dialog, selection, subtree, get_cat(), info_details, st);
        }
	catch(Euser_abort & e)
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

        return st;
    }

    bool archive::get_children_of(user_interaction & dialog,
				  const string & dir)
    {
	return get_cat().get_contenu()->callback_for_children_of(dialog, dir);
    }


    statistics archive::op_create_in(user_interaction & dialog,
				     bool create_not_isolate, const path & fs_root,
				     const path & sauv_path, archive *ref_arch,
				     const mask & selection, const mask & subtree,
				     const string & filename, const string & extension,
				     bool allow_over, bool warn_over, bool info_details, bool pause,
				     bool empty_dir, compression algo, U_I compression_level,
				     const infinint & file_size,
				     const infinint & first_file_size,
				     bool root_ea, bool user_ea,
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
				     bool ignore_owner)
    {
        statistics st;
	string real_pass;

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

            // end of sanity checks

	tools_avoid_slice_overwriting(dialog, sauv_path.display(), filename+".*."+extension, info_details, allow_over, warn_over);
	real_pass = pass;

        try
        {
            terminateur coord;
            catalogue *ref = NULL;
	    const path *ref_path = NULL;
            S_I sar_opt = 0;
            generic_file *zip_base = NULL;

            cat = new catalogue(dialog);
	    level1 = NULL;
	    scram = NULL;
	    level2 = NULL;
	    restore_ea_root = root_ea;
	    restore_ea_user = user_ea;
	    local_cat_size = 0; // unknown catalogue size (need to re-open the archive, once creation has completed)
	    local_path = new path(sauv_path);

		// warning against saving the archive itself

            if(create_not_isolate && sauv_path.is_subdir_of(fs_root, true)
               && selection.is_covered(filename+".1."+extension)
               && filename!= "-")
	    {
		path tmp = sauv_path == path(".") ? path(tools_getcwd()) : sauv_path;
		bool cov = true;
		string drop;

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
		ref = &(ref_arch->get_cat());
		ref_path = & ref_arch->get_path();
	    }
            else // building reference catalogue from scratch (empty catalogue)
                ref = new catalogue(dialog);

            if(ref == NULL)
                throw Ememory("archive::op_create / op_isolate");
            try
            {
                if(allow_over)
                    sar_opt &= ~SAR_OPT_DONT_ERASE;
                else
                    sar_opt |= SAR_OPT_DONT_ERASE;
                if(warn_over)
                    sar_opt |= SAR_OPT_WARN_OVERWRITE;
                else
                    sar_opt &= ~SAR_OPT_WARN_OVERWRITE;
                if(pause)
                    sar_opt |= SAR_OPT_PAUSE;
                else
                    sar_opt &= ~SAR_OPT_PAUSE;
                if(pause && ref_path != NULL && *ref_path == sauv_path)
                    dialog.pause(gettext("Ready to start writing down the archive?"));

		if(empty)
		    level1 = new null_file(dialog, gf_write_only);
		else
		    if(file_size == 0) // one SLICE
			if(filename == "-") // output to stdout
			    level1 = sar_tools_open_archive_tuyau(dialog, 1, gf_write_only); //archive goes to stdout
			else
			    level1 = sar_tools_open_archive_fichier(dialog, (sauv_path + sar_make_filename(filename, 1, extension)).display(), allow_over, warn_over);
		    else
			level1 = new sar(dialog, filename, extension, file_size, first_file_size, sar_opt, sauv_path, execute);

                if(level1 == NULL)
                    throw Ememory("op_create");

                version_copy(ver.edition, macro_tools_supported_version);
                ver.algo_zip = compression2char(algo);
                ver.cmd_line = "N/A";
                ver.flag = 0;
                if(restore_ea_root)
                    ver.flag |= VERSION_FLAG_SAVED_EA_ROOT;
                if(restore_ea_user)
                    ver.flag |= VERSION_FLAG_SAVED_EA_USER;

		if(crypto != crypto_none && real_pass == "")
		{
		    string t1 = dialog.get_string(tools_printf(gettext("Archive %S requires a password: "), &filename), false);
		    string t2 = dialog.get_string(gettext("Please confirm your password: "), false);
		    if(t1 == t2)
			real_pass = t1;
		    else
			throw Erange("op_create_in", gettext("The two passwords are not identical. Aborting"));
		}
		switch(crypto)
		{
		case crypto_scrambling:
		case crypto_blowfish:
                    ver.flag |= VERSION_FLAG_SCRAMBLED;
		    break;
		case crypto_none:
		    break; // no bit to set;
		default:
		    throw Erange("libdar:op_create_in",gettext("Unknown crypto algorithm"));
		}
                ver.write(*level1);

                if(!empty)
                {
		    switch(crypto)
		    {
		    case crypto_scrambling:
			scram = new scrambler(dialog, real_pass, *level1);
			break;
		    case crypto_blowfish:
			scram = new blowfish(dialog, crypto_size, real_pass, *level1);
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
			    throw Ememory("op_create");
			zip_base = scram;
			tools_add_elastic_buffer(*zip_base, GLOBAL_ELASTIC_BUFFER_SIZE);
			    // initial elastic buffer
		    }
		}
		else
		    zip_base = level1;

		level2 = new compressor(dialog, empty ? none : algo, *zip_base, compression_level);
                if(level2 == NULL)
                    throw Ememory("op_create");

                if(create_not_isolate)
                    filtre_sauvegarde(dialog, selection, subtree, level2, *cat, *ref, fs_root, info_details, st, empty_dir, restore_ea_root, restore_ea_user, compr_mask, min_compr_size, nodump, hourshift, alter_atime, same_fs, ignore_owner);
                else
                {
                    st.clear(); // clear st, as filtre_isolate does not use it
                    filtre_isolate(dialog, *cat, *ref, info_details);
                }

                level2->flush_write();
                coord.set_catalogue_start(level2->get_position());
                if(ref_arch != NULL && create_not_isolate)
                {
                    if(info_details)
                        dialog.warning(gettext("Adding reference to files that have been destroyed since reference backup..."));
                    st.deleted = cat->update_destroyed_with(*ref);
                }

                    // making some place in memory
                if(ref != NULL && ref_arch == NULL)
                {
                    delete ref;
                    ref = NULL;
                }

                if(info_details)
                    dialog.warning(gettext("Writing archive contents..."));
                cat->dump(*level2);
                level2->flush_write();
                delete level2;
                level2 = NULL;

                coord.dump(*zip_base); // since format "04" the terminateur is encrypted
		if(crypto != crypto_none)
		    tools_add_elastic_buffer(*zip_base, GLOBAL_ELASTIC_BUFFER_SIZE);
		    // terminal elastic buffer (after terminateur to protect against
		    // plain text attack on the terminator string)

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
            }
            catch(...)
            {
		if(level2 != NULL)
                {
                    level2->clean_write();
                    delete level2;
                }
                if(scram != NULL)
                    delete scram;
                if(level1 != NULL)
		{
		    disable_natural_destruction();
                    delete level1;
		}
		if(cat != NULL)
		    delete cat;
                throw;
            }
        }
        catch(Erange &e)
        {
            string msg = string(gettext("Error while saving data: ")) + e.get_message();
	    dialog.warning(msg);
            throw Edata(msg);
        }

        return st;
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
