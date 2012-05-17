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
// $Id: libdar.cpp,v 1.15.2.1 2003/11/29 08:49:58 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"
#include <string>
#include <iostream>
#include "erreurs.hpp"
#include "generic_file.hpp"
#include "user_interaction.hpp"
#include "terminateur.hpp"
#include "compressor.hpp"
#include "catalogue.hpp"
#include "sar.hpp"
#include "filtre.hpp"
#include "tools.hpp"
#include "header.hpp"
#include "header_version.hpp"
#include "defile.hpp"
#include "deci.hpp"
#include "test_memory.hpp"
#include "sar_tools.hpp"
#include "tuyau.hpp"
#include "scrambler.hpp"
#include "macro_tools.hpp"
#include "integers.hpp"
#include "header.hpp"
#include "libdar.hpp"

using namespace std;

namespace libdar
{

    static statistics op_create_in(bool create_not_isolate, const path & fs_root, const path & sauv_path, const path *ref_path,
                                   const mask & selection, const mask & subtree,
                                   const string & filename, const string & extension, const string *ref_filename,
                                   bool allow_over, bool warn_over, bool info_details, bool pause,
                                   bool empty_dir, compression algo, U_I compression_level,
                                   const infinint & file_size,
                                   const infinint & first_file_size,
                                   bool root_ea, bool user_ea,
                                   const string & input_pipe, const string & output_pipe,
                                   const string & execute, const string & execute_ref,
                                   const string & pass, const string & pass_ref,
                                   const mask & compr_mask,
                                   const infinint & min_compr_size,
                                   bool nodump,
                                   const infinint & hourshift);

    void get_version(U_I & major, U_I & minor)
    {
        if(&major == NULL)
            throw Erange("get_version", "argument given to major is NULL pointer");
        if(&minor == NULL)
            throw Erange("get_version", "argument given to minor is NULL pointer");
        major = LIBDAR_COMPILE_TIME_MAJOR;
        minor = LIBDAR_COMPILE_TIME_MINOR;
    }

    statistics op_create(const path & fs_root,
                         const path & sauv_path,
                         const path *ref_path,
                         const mask & selection,
                         const mask & subtree,
                         const string & filename,
                         const string & extension,
                         const string *ref_filename,
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
                         const string & input_pipe,
                         const string & output_pipe,
                         const string & execute,
                         const string & execute_ref,
                         const string & pass,
                         const string & pass_ref,
                         const mask & compr_mask,
                         const infinint & min_compr_size,
                         bool nodump,
                         bool ignore_owner,
                         const infinint & hourshift)
    {
        inode::set_ignore_owner(ignore_owner);
        return op_create_in(true, fs_root, sauv_path, ref_path, selection, subtree, filename, extension, ref_filename,
                            allow_over, warn_over,
                            info_details, pause, empty_dir, algo, compression_level, file_size, first_file_size, root_ea, user_ea,
                            input_pipe, output_pipe, execute, execute_ref, pass, pass_ref, compr_mask, min_compr_size,
                            nodump, hourshift);
    }

    void op_isolate(const path &sauv_path,
                    const path *ref_path,
                    const string & filename,
                    const string & extension,
                    const string *ref_filename,
                    bool allow_over,
                    bool warn_over,
                    bool info_details,
                    bool pause,
                    compression algo,
                    U_I compression_level,
                    const infinint &file_size,
                    const infinint &first_file_size,
                    const string &input_pipe,
                    const string &output_pipe,
                    const string & execute,
                    const string & execute_ref,
                    const string & pass,
                    const string & pass_ref)
    {
        (void)op_create_in(false, path("."), sauv_path, ref_path, bool_mask(false), bool_mask(false),
                           filename, extension, ref_filename, allow_over, warn_over, info_details,
                           pause, false, algo, compression_level, file_size, first_file_size, false,
                           false, input_pipe, output_pipe, execute, execute_ref, pass,
                           pass_ref, bool_mask(false), 0, false, 0);
            // we ignore returned value;
    }


    statistics op_extract(const path & fs_root, const path & sauv_path,
                          const mask & selection, const mask & subtree,
                          const string & filename, const string & extension,
                          bool allow_over, bool warn_over,
                          bool info_details,  bool detruire,
                          bool only_more_recent, bool restore_ea_root,
                          bool restore_ea_user,
                          const string & input_pipe, const string & output_pipe,
                          const string & execute,
                          const string & pass,
                          bool flat, bool ignore_owner,
                          const infinint & hourshift)
    {
        statistics st;

            // sanity checks

        if(&fs_root == NULL)
            throw Elibcall("op_extract", "NULL argument given to fs_root");
        if(&sauv_path == NULL)
            throw Elibcall("op_extract", "NULL argument given to sauv_path");
        if(&selection == NULL)
            throw Elibcall("op_extract", "NULL argument given to selection");
        if(&subtree == NULL)
            throw Elibcall("op_extract", "NULL argument given to subtree");
        if(&filename == NULL)
            throw Elibcall("op_extract", "NULL argument given to filename");
        if(&extension == NULL)
            throw Elibcall("op_extract", "NULL argument given to extension");
        if(&input_pipe == NULL)
            throw Elibcall("op_extract", "NULL argument given to input_pipe");
        if(&output_pipe == NULL)
            throw Elibcall("op_extract", "NULL argument given to output_pipe");
        if(&execute == NULL)
            throw Elibcall("op_extract", "NULL argument given to execute");
        if(&pass == NULL)
            throw Elibcall("op_extract", "NULL argument given to pass");

            // end of sanity checks

        try
        {
            generic_file *decoupe = NULL;
            compressor *zip = NULL;
            scrambler *scram = NULL;
            header_version ver;

            inode::set_ignore_owner(ignore_owner);

            try
            {
                infinint tmp;

                macro_tools_open_archive(sauv_path, filename, extension, SAR_OPT_DEFAULT, pass, decoupe, scram, zip, ver, input_pipe, output_pipe, execute);
                if((ver.flag & VERSION_FLAG_SAVED_EA_ROOT) == 0)
                    restore_ea_root = false; // not restoring something not saved
                if((ver.flag & VERSION_FLAG_SAVED_EA_USER) == 0)
                    restore_ea_user = false; // not restoring something not saved

                catalogue *cat = macro_tools_get_catalogue_from(*decoupe, *zip, info_details, tmp);
                try
                {
                    MEM_IN;
                    filtre_restore(selection, subtree, *cat, detruire,
                                   fs_root, allow_over, warn_over, info_details,
                                   st, only_more_recent, restore_ea_root,
                                   restore_ea_user, flat, ignore_owner, hourshift);
                    MEM_OUT;
                }
                catch(...)
                {
                    delete cat;
                    throw;
                }
                delete cat;
            }
            catch(...)
            {
                if(zip != NULL)
                    delete zip;
                if(scram != NULL)
                    delete scram;
                if(decoupe != NULL)
                    delete decoupe;
                throw;
            }
            if(zip != NULL)
                delete zip;
            if(scram != NULL)
                delete scram;
            if(decoupe != NULL)
                delete decoupe;
        }
        catch(Erange &e)
        {
            string msg = string("error while restoring data: ") + e.get_message();
            user_interaction_warning(msg);
            throw Edata(msg);
        }

        return st;
    }

    void op_listing(const path & sauv_path, const string & filename, const string & extension,
                    bool info_details, bool tar_format,
                    const string & input_pipe, const string & output_pipe,
                    const string & execute,
                    const string & pass,
                    const mask & selection)
    {
            // sanity checks

        if(&sauv_path == NULL)
            throw Elibcall("op_listing", "NULL argument given to sauv_path");
        if(&filename == NULL)
            throw Elibcall("op_listing", "NULL argument given to filenamee");
        if(&extension == NULL)
            throw Elibcall("op_listing", "NULL argument given to extension");
        if(&input_pipe == NULL)
            throw Elibcall("op_listing", "NULL argument given to input_pipe");
        if(&output_pipe == NULL)
            throw Elibcall("op_listing", "NULL argument given to output_pipe");
        if(&execute == NULL)
            throw Elibcall("op_listing", "NULL argument given to execute");
        if(&pass == NULL)
            throw Elibcall("op_listing", "NULL argument given to pass");
        if(&selection == NULL)
            throw Elibcall("op_listing", "NULL argument given to selection");

	    // end of sanity checks

        try
        {
            generic_file *decoupe = NULL;
            compressor *zip = NULL;
            scrambler *scram = NULL;
            header_version ver;

            MEM_IN;
            try
            {
                infinint cat_size;

                macro_tools_open_archive(sauv_path, filename, extension, SAR_OPT_DEFAULT, pass, decoupe, scram, zip, ver, input_pipe, output_pipe, execute);

                catalogue *cat = macro_tools_get_catalogue_from(*decoupe, *zip, info_details, cat_size);
                try
                {
                    if(info_details)
                    {
                        sar *real_decoupe = dynamic_cast<sar *>(decoupe);
                        infinint sub_file_size;
                        infinint first_file_size;
                        infinint last_file_size, file_number;
                        string algo = compression2string(char2compression(ver.algo_zip));

                        ui_printf("Archive version format               : %s\n", ver.edition);
                        ui_printf("Compression algorithm used           : %S\n", &algo);
                        ui_printf("Scrambling                           : %s\n", ((ver.flag & VERSION_FLAG_SCRAMBLED) != 0 ? "yes" : "no"));
                        ui_printf("Catalogue size in archive            : %i bytes\n", &cat_size);
                            // the following field is no more used (lack of pertinence, when included files are used).
                        ui_printf("Command line options used for backup : %S\n", &ver.cmd_line);

                        if(real_decoupe != NULL)
                        {
                            infinint sub_file_size = real_decoupe->get_sub_file_size();
                            infinint first_file_size = real_decoupe->get_first_sub_file_size();
                            if(real_decoupe->get_total_file_number(file_number)
                               && real_decoupe->get_last_file_size(last_file_size))
                            {
                                ui_printf("Archive is composed of %i file\n", &file_number);
                                if(file_number == 1)
                                    ui_printf("File size: %i bytes\n", &last_file_size);
                                else
                                {
                                    if(first_file_size != sub_file_size)
                                        ui_printf("First file size       : %i bytes\n", &first_file_size);
                                    ui_printf("File size             : %i bytes\n", &sub_file_size);
                                    ui_printf("Last file size        : %i bytes\n", &last_file_size);
                                }
                                if(file_number > 1)
                                {
                                    infinint total = first_file_size + (file_number-2)*sub_file_size + last_file_size;
                                    ui_printf("Archive total size is : %i bytes\n", &total);
                                }
                            }
                            else // could not read size parameters
                                ui_printf("Sorry, file size is unknown at this step of the program.\n");
                        }
                        else // not reading from a sar
                        {
                            infinint tmp;

                            decoupe->skip_to_eof();
                            tmp = decoupe->get_position();
                            ui_printf("Archive size is: %i bytes\n", &tmp);
                            ui_printf("Previous archive size does not include headers present in each slice\n");
                        }

                        entree_stats stats = cat->get_stats();
                        stats.listing();
                        user_interaction_pause("Continue listing archive contents?");
                    }
                    if(tar_format)
                        cat->tar_listing(selection);
                    else
                        cat->listing(selection);
                }
                catch(...)
                {
                    delete cat;
                    throw;
                }
                delete cat;
            }
            catch(...)
            {
                if(zip != NULL)
                    delete zip;
                if(scram != NULL)
                    delete scram;
                if(decoupe != NULL)
                    delete decoupe;
                throw;
            }
            if(zip != NULL)
                delete zip;
            if(scram != NULL)
                delete scram;
            if(decoupe != NULL)
                delete decoupe;
            MEM_OUT;
        }
        catch(Erange &e)
        {
            string msg = string("error while listing archive contents: ") + e.get_message();
            user_interaction_warning(msg);
            throw Edata(msg);
        }
    }

    statistics op_diff(const path & fs_root, const path & sauv_path,
                       const mask & selection, const mask & subtree,
                       const string & filename, const string & extension,
                       bool info_details,
                       bool check_ea_root, bool check_ea_user,
                       const string & input_pipe, const string & output_pipe,
                       const string & execute,
                       const string & pass,
                       bool ignore_owner)
    {
        statistics st;

            // sanity checks

        if(&fs_root == NULL)
            throw Elibcall("op_diff", "NULL argument given to fs_root");
        if(&sauv_path == NULL)
            throw Elibcall("op_diff", "NULL argument given to sauv_path");
        if(&selection == NULL)
            throw Elibcall("op_diff", "NULL argument given to selection");
        if(&subtree == NULL)
            throw Elibcall("op_diff", "NULL argument given to subtree");
        if(&filename == NULL)
            throw Elibcall("op_diff", "NULL argument given to filename");
        if(&extension == NULL)
            throw Elibcall("op_diff", "NULL argument given to extension");
        if(&input_pipe == NULL)
            throw Elibcall("op_diff", "NULL argument given to input_pipe");
        if(&output_pipe == NULL)
            throw Elibcall("op_diff", "NULL argument given to output_pipe");
        if(&execute == NULL)
            throw Elibcall("op_diff", "NULL argument given to execute");
        if(&pass == NULL)
            throw Elibcall("op_diff", "NULL argument given to pass");

            // end of sanity checks

        try
        {
            generic_file *decoupe = NULL;
            scrambler *scram = NULL;
            compressor *zip = NULL;
            header_version ver;
            catalogue *cat = NULL;

            inode::set_ignore_owner(ignore_owner);

            try
            {
                infinint cat_size;

                macro_tools_open_archive(sauv_path, filename, extension, SAR_OPT_DEFAULT, pass, decoupe, scram, zip, ver, input_pipe, output_pipe, execute);
                cat = macro_tools_get_catalogue_from(*decoupe, *zip, info_details, cat_size);
                filtre_difference(selection, subtree, *cat, fs_root, info_details, st, check_ea_root, check_ea_user);
            }
            catch(...)
            {
                if(cat != NULL)
                    delete cat;
                if(zip != NULL)
                    delete zip;
                if(scram != NULL)
                    delete scram;
                if(decoupe != NULL)
                    delete decoupe;
                throw;
            }
            if(cat != NULL)
                delete cat;
            if(zip != NULL)
                delete zip;
            if(scram != NULL)
                delete scram;
            if(decoupe != NULL)
                delete decoupe;
        }
        catch(Erange & e)
        {
            string msg = string("error while comparing archive with filesystem: ")+e.get_message();
            user_interaction_warning(msg);
            throw Edata(msg);
        }

        return st;
    }


    statistics op_test(const path & sauv_path, const mask & selection,
                       const mask & subtree, const string & filename, const string & extension,
                       bool info_details,
                       const string & input_pipe, const string & output_pipe,
                       const string & execute,
                       const string & pass)
    {
        statistics st;

            // sanity checks

        if(&sauv_path == NULL)
            throw Elibcall("op_test", "NULL argument given to sauv_path");
        if(&selection == NULL)
            throw Elibcall("op_test", "NULL argument given to selection");
        if(&subtree == NULL)
            throw Elibcall("op_test", "NULL argument given to subtree");
        if(&filename == NULL)
            throw Elibcall("op_test", "NULL argument given to filename");
        if(&extension == NULL)
            throw Elibcall("op_test", "NULL argument given to extension");
        if(&input_pipe == NULL)
            throw Elibcall("op_test", "NULL argument given to input_pipe");
        if(&output_pipe == NULL)
            throw Elibcall("op_test", "NULL argument given to output_pipe");
        if(&execute == NULL)
            throw Elibcall("op_test", "NULL argument given to execute");
        if(&pass == NULL)
            throw Elibcall("op_test", "NULL argument given to pass");

            // end of sanity checks

        try
        {
            generic_file *decoupe = NULL;
            scrambler *scram = NULL;
            compressor *zip = NULL;
            header_version ver;
            catalogue *cat = NULL;

            try
            {
                infinint cat_size;

                macro_tools_open_archive(sauv_path, filename, extension, SAR_OPT_DEFAULT, pass, decoupe, scram, zip, ver, input_pipe, output_pipe, execute);
                cat = macro_tools_get_catalogue_from(*decoupe, *zip, info_details, cat_size);
                filtre_test(selection, subtree, *cat, info_details, st);
            }
            catch(...)
            {
                if(cat != NULL)
                    delete cat;
                if(zip != NULL)
                    delete zip;
                if(scram != NULL)
                    delete scram;
                if(decoupe != NULL)
                    delete decoupe;
                throw;
            }
            if(cat != NULL)
                delete cat;
            if(zip != NULL)
                delete zip;
            if(scram != NULL)
                delete scram;
            if(decoupe != NULL)
                delete decoupe;
        }
        catch(Erange & e)
        {
            string msg = string("error while testing archive: ")+e.get_message();
            user_interaction_warning(msg);
            throw Edata(msg);
        }

        return st;
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: libdar.cpp,v 1.15.2.1 2003/11/29 08:49:58 edrusb Rel $";
        dummy_call(id);
    }

    char *libdar_str2charptr(const string & x)
    {
        char *ret;
        try
        {
            ret = tools_str2charptr(x);
        }
        catch(...)
        {
            ret = NULL;
        }

        return ret;
    }

    void set_op_tar_listing_callback(void (*callback)(const string & flag,
                                                      const string & perm,
                                                      const string & uid,
                                                      const string & gid,
                                                      const string & size,
                                                      const string & date,
                                                      const string & filename))
    {
        directory::set_tar_listing_callback(callback);
    }


    void get_compile_time_features(bool & ea, bool & largefile, bool & nodump, bool & special_alloc, U_I & bits)
    {
#ifdef EA_SUPPORT
        ea = true;
#else
        ea = false;
#endif
#if defined( _FILE_OFFSET_BITS ) ||  defined( _LARGE_FILES )
        largefile = true;
#else
        largefile = sizeof(long) > 4;
#endif
#ifdef NODUMP_FEATURE
        nodump = true;
#else
        nodump = false;
#endif
#ifdef SPECIAL_ALLOC
        special_alloc = true;
#else
        special_alloc = false;
#endif
#ifdef MODE
        bits = MODE;
#else
        bits = 0; // infinint
#endif
    }

    static statistics op_create_in(bool create_not_isolate, const path & fs_root, const path & sauv_path, const path *ref_path,
                                   const mask & selection, const mask & subtree,
                                   const string & filename, const string & extension, const string *ref_filename,
                                   bool allow_over, bool warn_over, bool info_details, bool pause,
                                   bool empty_dir, compression algo, U_I compression_level,
                                   const infinint & file_size,
                                   const infinint & first_file_size,
                                   bool root_ea, bool user_ea,
                                   const string & input_pipe, const string & output_pipe,
                                   const string & execute, const string & execute_ref,
                                   const string & pass, const string & pass_ref,
                                   const mask & compr_mask,
                                   const infinint & min_compr_size,
                                   bool nodump,
                                   const infinint & hourshift)
    {
        statistics st;

            // sanity checks as much as possible to avoid libdar crashing due to bad arguments
            // useless arguments are not reported.

        if(&fs_root == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to fs_root");
        if(&sauv_path == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to sauv_path");
        if((ref_path == NULL) ^ (ref_filename == NULL))
            throw Elibcall("op_create/op_isolate", "ref_filename and ref_path must be both either NULL or not");
        if(&selection == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to selection");
        if(&subtree == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to subtree");
        if(&filename == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to filename");
        if(&extension == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to extension");
        if(compression_level > 9 || compression_level < 1)
            throw Elibcall("op_create/op_isolate", "compression_level must be between 1 and 9 included");
        if(file_size == 0 && first_file_size != 0)
            throw Elibcall("op_create/op_isolate", "first_file_size cannot be different from zero if file_size is not also");
        if(&input_pipe == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to input_pipe");
        if(&output_pipe == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to output_pipe");
        if(&execute == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to execute");
        if(&execute_ref == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to execute_ref");
        if(&pass == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to pass");
        if(&pass_ref == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to pass_ref");
        if(&compr_mask == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to compr_mask");
        if(&min_compr_size == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to min_compr_size");

            // end of sanity checks

        try
        {
            terminateur coord;
            catalogue current;
            catalogue *ref = NULL;
            generic_file *decoupe = NULL;
            compressor *zip = NULL;
            scrambler *scram = NULL;
            S_I sar_opt = 0;
            generic_file *zip_base = NULL;

            if(create_not_isolate && sauv_path.is_subdir_of(fs_root)
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
		    user_interaction_pause(string("WARNING! The archive is located in the directory to backup, this may create an endless loop when the archive will try to save itself. You can either add -X \"")+ filename + ".*." + extension +"\" on the command line, or change the location of the archive (see -h for help). Do you really want to continue?");
	    }

                // building the reference catalogue
            if(ref_filename != NULL) // from a existing archive
            {
                generic_file *tmp = NULL;
                scrambler *scram_ref = NULL;
                header_version v;

                if(ref_path == NULL)
                    throw SRC_BUG; // ref_filename != NULL but ref_path == NULL;
                try
                {
                    infinint tmp2;

                    macro_tools_open_archive(*ref_path, *ref_filename, extension, SAR_OPT_DONT_ERASE, pass_ref, tmp, scram_ref, zip, v, input_pipe, output_pipe, execute_ref);
                    ref = macro_tools_get_catalogue_from(*tmp, *zip, info_details, tmp2);
                }
                catch(...)
                {
                    if(zip != NULL)
                        delete zip;
                    if(scram_ref != NULL)
                        delete scram_ref;
                    if(tmp != NULL)
                        delete tmp;
                    throw;
                }
                if(zip != NULL)
                    delete zip;
                if(scram_ref != NULL)
                    delete scram_ref;
                if(tmp != NULL)
                    delete tmp;
                zip = NULL;
            }
            else // building reference catalogue from scratch (empty catalogue)
                ref = new catalogue();

            if(ref == NULL)
                throw Ememory("op_create");
            try
            {
                header_version ver;

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
                    user_interaction_pause("Ready to start writing the archive? ");

                if(file_size == 0) // one SLICE
                    if(filename == "-") // output to stdout
                        decoupe = sar_tools_open_archive_tuyau(1, gf_write_only); //archive goes to stdout
                    else
                        decoupe = sar_tools_open_archive_fichier((sauv_path + sar_make_filename(filename, 1, extension)).display(), allow_over, warn_over);
                else
                    decoupe = new sar(filename, extension, file_size, first_file_size, sar_opt, sauv_path, execute);

                if(decoupe == NULL)
                    throw Ememory("op_create");

                version_copy(ver.edition, macro_tools_supported_version);
                ver.algo_zip = compression2char(algo);
                ver.cmd_line = "N/A";
                ver.flag = 0;
                if(root_ea)
                    ver.flag |= VERSION_FLAG_SAVED_EA_ROOT;
                if(user_ea)
                    ver.flag |= VERSION_FLAG_SAVED_EA_USER;
                if(pass != "")
                    ver.flag |= VERSION_FLAG_SCRAMBLED;
                ver.write(*decoupe);

                if(pass != "")
                {
                    scram = new scrambler(pass, *decoupe);
                    if(scram == NULL)
                        throw Ememory("op_create");
                    zip_base = scram;
                }
                else
                    zip_base = decoupe;


                zip = new compressor(algo, *zip_base, compression_level);
                if(zip == NULL)
                    throw Ememory("op_create");

                if(create_not_isolate)
                    filtre_sauvegarde(selection, subtree, zip, current, *ref, fs_root, info_details, st, empty_dir, root_ea, user_ea, compr_mask, min_compr_size, nodump, hourshift);
                else
                {
                    st.clear(); // clear st, as filtre_isolate does not use it
                    filtre_isolate(current, *ref, info_details);
                }

                zip->flush_write();
                coord.set_catalogue_start(zip->get_position());
                if(ref_filename != NULL && create_not_isolate)
                {
                    if(info_details)
                        user_interaction_warning("Adding reference to files that have been destroyed since reference backup...");
                    st.deleted = current.update_destroyed_with(*ref);
                }

                    // making some place in memory
                if(ref != NULL)
                {
                    delete ref;
                    ref = NULL;
                }

                if(info_details)
                    user_interaction_warning("Writing archive contents...");
                current.dump(*zip);
                zip->flush_write();
                delete zip;
                zip = NULL;
                if(scram != NULL)
                {
                    delete scram;
                    scram = NULL;
                }
                coord.dump(*decoupe);
                delete decoupe;
                decoupe = NULL;
            }
            catch(...)
            {
                if(ref != NULL)
                    delete ref;
                if(zip != NULL)
                {
                    zip->clean_write();
                    delete zip;
                }
                if(scram != NULL)
                    delete scram;
                if(decoupe != NULL)
                    delete decoupe;
                throw;
            }
        }
        catch(Erange &e)
        {
            string msg = string("error while saving data: ") + e.get_message();
            user_interaction_warning(msg);
            throw Edata(msg);
        }

        return st;
    }

} // end of namespace
