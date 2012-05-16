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
// $Id: libdar.cpp,v 1.25.2.6 2004/09/10 22:34:15 edrusb Exp $
//
/*********************************************************************/
//

#include "../my_config.h"

extern "C"
{
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"

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
#include "null_file.hpp"

using namespace std;

#define WRAPPER_IN try {
#define WRAPPER_OUT(code,msg)                      \
                       code = LIBDAR_NOEXCEPT;     \
                    }                              \
                    catch(Ememory & e)             \
                    {                              \
			code = LIBDAR_EMEMORY;     \
                        msg = e.get_message();     \
                    }                              \
                    catch(Ebug & e)                \
                    {                              \
			code = LIBDAR_EBUG;        \
                        msg = e.get_message();     \
                    }                              \
                    catch(Einfinint & e)           \
                    {                              \
			code = LIBDAR_EINFININT;   \
                        msg = e.get_message();     \
                    }                              \
                    catch(Elimitint & e)           \
                    {                              \
			code = LIBDAR_ELIMITINT;   \
                        msg = e.get_message();     \
                    }                              \
                    catch(Erange & e)              \
                    {                              \
			code = LIBDAR_ERANGE;      \
                        msg = e.get_message();     \
                    }                              \
                    catch(Edeci & e)               \
                    {                              \
			code = LIBDAR_EDECI;       \
                        msg = e.get_message();     \
                    }                              \
                    catch(Efeature & e)            \
                    {                              \
			code = LIBDAR_EFEATURE;    \
                        msg = e.get_message();     \
                    }                              \
                    catch(Ehardware & e)           \
                    {                              \
			code = LIBDAR_EHARDWARE;   \
                        msg = e.get_message();     \
                    }                              \
                    catch(Euser_abort & e)         \
                    {                              \
			code = LIBDAR_EUSER_ABORT; \
                        msg = e.get_message();     \
                    }                              \
                    catch(Edata & e)               \
                    {                              \
			code = LIBDAR_EDATA;       \
                        msg = e.get_message();     \
                    }                              \
                    catch(Escript & e)             \
                    {                              \
			code = LIBDAR_ESCRIPT;     \
                        msg = e.get_message();     \
                    }                              \
                    catch(Elibcall & e)            \
                    {                              \
			code = LIBDAR_ELIBCALL;    \
                        msg = e.get_message();     \
                    }                              \
                    catch(Egeneric & e)            \
                    {/*unknown Egeneric exception*/\
			code = LIBDAR_EBUG;        \
                        msg = string("unknown Egeneric exception") + e.get_message();  \
                    }                              \
                    catch(...)                     \
                    {/* unknown Egeneric exception*/\
			code = LIBDAR_UNKNOWN;     \
                        msg = "not a libdar exception caught";  \
                    }                              \


namespace libdar
{
    archive* open_archive_noexcept(const path & chem, const std::string & basename,
				   const std::string & extension, crypto_algo crypto, const std::string &pass,
				   const std::string & input_pipe, const std::string & output_pipe,
				   const std::string & execute, bool info_details,
				   U_16 & exception,
				   std::string & except_msg)
    {
	archive *ret = NULL;
	WRAPPER_IN
	ret = new archive(chem,  basename,  extension,
			  crypto, pass,
			  input_pipe, output_pipe,
			  execute, info_details);
	WRAPPER_OUT(exception, except_msg)
	return ret;
    }

    void close_archive_noexcept(archive *ptr,
				U_16 & exception,
				std::string & except_msg)
    {
	WRAPPER_IN
	if(ptr == NULL)
	    throw Erange("close_archive_noexcept", "invalid NULL pointer given to close_archive");
	else
	    delete ptr;
	WRAPPER_OUT(exception, except_msg)
    }

    static statistics op_create_in(bool create_not_isolate, const path & fs_root, const path & sauv_path, archive *ref_arch,
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
                                   const mask & compr_mask,
                                   const infinint & min_compr_size,
                                   bool nodump,
                                   const infinint & hourshift,
				   bool empty);

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
                         const mask & compr_mask,
                         const infinint & min_compr_size,
                         bool nodump,
                         bool ignore_owner,
                         const infinint & hourshift,
			 bool empty)
    {
        inode::set_ignore_owner(ignore_owner);
        return op_create_in(true, fs_root, sauv_path, ref_arch, selection, subtree, filename, extension,
                            allow_over, warn_over,
                            info_details, pause, empty_dir, algo, compression_level, file_size, first_file_size, root_ea, user_ea,
                            execute, crypto, pass, compr_mask, min_compr_size,
                            nodump, hourshift, empty);
    }

    statistics op_create_noexcept(const path &fs_root,
				  const path &sauv_path,
				  archive *ref_arch,
				  const mask &selection,
				  const mask &subtree,
				  const std::string &filename,
				  const std::string & extension,
				  bool allow_over,
				  bool warn_over,
				  bool info_details,
				  bool pause,
				  bool empty_dir,
				  compression algo,
				  U_I compression_level,
				  const infinint & file_size,
				  const infinint & first_file_size,
				  bool root_ea,
				  bool user_ea,
				  const std::string & execute,
				  crypto_algo crypto,
				  const std::string & pass,
				  const mask &compr_mask,
				  const infinint & min_compr_size,
				  bool nodump,
				  bool ignore_owner,
				  const infinint & hourshift,
				  bool empty,
				  U_16 & exception,
				  std::string & except_msg)
    {
	statistics ret;
	WRAPPER_IN
	ret = op_create(fs_root,
			sauv_path,
			ref_arch,
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
			root_ea,
			user_ea,
			execute,
			crypto,
			pass,
			compr_mask,
			min_compr_size,
			nodump,
			ignore_owner,
			hourshift,
			empty);
	WRAPPER_OUT(exception, except_msg)
	return ret;
    }

    void op_isolate(const path &sauv_path,
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
                    const string & pass)
    {
        (void)op_create_in(false, path("."), sauv_path, ref_arch, bool_mask(false), bool_mask(false),
                           filename, extension, allow_over, warn_over, info_details,
                           pause, false, algo, compression_level, file_size, first_file_size, false,
                           false, execute, crypto, pass,
                           bool_mask(false), 0, false, 0, false);
            // we ignore returned value;
    }

    void op_isolate_noexcept(const path &sauv_path,
			     archive *ref_arch,
			     const std::string & filename,
			     const std::string & extension,
			     bool allow_over,
			     bool warn_over,
			     bool info_details,
			     bool pause,
			     compression algo,
			     U_I compression_level,
			     const infinint &file_size,
			     const infinint &first_file_size,
			     const std::string & execute,
			     crypto_algo crypto,
			     const std::string & pass,
			     U_16 & exception,
			     std::string & except_msg)
    {
	WRAPPER_IN
	op_isolate(sauv_path,
		   ref_arch,
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
		   pass);
	WRAPPER_OUT(exception, except_msg)
    }

    statistics op_extract(archive *arch, const path & fs_root,
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

	if(arch == NULL)
	    throw Elibcall("op_extract", "NULL argument given to arch");
        if(&fs_root == NULL)
            throw Elibcall("op_extract", "NULL argument given to fs_root");
        if(&selection == NULL)
            throw Elibcall("op_extract", "NULL argument given to selection");
        if(&subtree == NULL)
            throw Elibcall("op_extract", "NULL argument given to subtree");
	if(&hourshift == NULL)
	    throw Elibcall("op_extract", "NULL argument given to hourshift");

            // end of sanity checks

        try
        {
	    MEM_IN;
            inode::set_ignore_owner(ignore_owner);

	    filtre_restore(selection, subtree, arch->get_cat(), detruire,
			   fs_root, allow_over, warn_over, info_details,
			   st, only_more_recent, restore_ea_root,
			   restore_ea_user, flat, ignore_owner, warn_remove_no_match,
			   hourshift, empty);
	    MEM_OUT;
        }
        catch(Erange &e)
        {
            string msg = string("error while restoring data: ") + e.get_message();
            user_interaction_warning(msg);
            throw Edata(msg);
        }

        return st;
    }

    void op_listing(archive *arch,
                    bool info_details, bool tar_format,
                    const mask & selection, bool filter_unsaved)
    {
            // sanity checks

	if(arch == NULL)
	    throw Elibcall("op_listing", "NULL argument given to arch");
        if(&selection == NULL)
            throw Elibcall("op_listing", "NULL argument given to selection");

	    // end of sanity checks

        try
        {
            MEM_IN;

	    if(info_details)
	    {
		infinint sub_file_size;
		infinint first_file_size;
		infinint last_file_size, file_number;
		string algo = compression2string(char2compression(arch->get_header().algo_zip));
		infinint cat_size = arch->get_cat_size();

		ui_printf("Archive version format               : %s\n", arch->get_header().edition);
		ui_printf("Compression algorithm used           : %S\n", &algo);
		ui_printf("Scrambling                           : %s\n", ((arch->get_header().flag & VERSION_FLAG_SCRAMBLED) != 0 ? "yes" : "no"));
		ui_printf("Catalogue size in archive            : %i bytes\n", &cat_size);
		    // the following field is no more used (lack of pertinence, when included files are used).
		ui_printf("Command line options used for backup : %S\n", &(arch->get_header().cmd_line));

		try
		{
		    if(arch->get_sar_param(sub_file_size, first_file_size, last_file_size, file_number))
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
			else // not reading from a sar
			{
			    infinint arch_size = arch->get_level2_size();
			    ui_printf("Archive size is: %i bytes\n", &arch_size);
			    ui_printf("Previous archive size does not include headers present in each slice\n");
			}
		    }
		}
		catch(Erange & e)
		{
		    string msg = e.get_message();
		    ui_printf("%S\n", &msg);
		}

		entree_stats stats = arch->get_cat().get_stats();
		stats.listing();
		user_interaction_pause("Continue listing archive contents?");
	    }

	    if(tar_format)
		arch->get_cat().tar_listing(selection, filter_unsaved);
	    else
		arch->get_cat().listing(selection, filter_unsaved);

	    MEM_OUT;
        }
        catch(Erange &e)
        {
            string msg = string("error while listing archive contents: ") + e.get_message();
            user_interaction_warning(msg);
            throw Edata(msg);
        }
    }

    void op_listing_noexcept(archive  *arch,
			     bool info_details,
			     bool tar_format,
			     const mask &selection,
			     bool filter_unsaved,
			     U_16 & exception,
			     std::string & except_msg)
    {
	WRAPPER_IN
	op_listing(arch,
		   info_details,
		   tar_format,
		   selection,
		   filter_unsaved);
	WRAPPER_OUT(exception, except_msg)
    }


    statistics op_extract_noexcept(archive *arch,
				   const path &fs_root,
				   const mask &selection,
				   const mask &subtree,
				   bool allow_over,
				   bool warn_over,
				   bool info_details,
				   bool detruire,
				   bool only_more_recent,
				   bool restore_ea_root,
				   bool restore_ea_user,
				   bool flat,
				   bool ignore_owner,
				   bool warn_remove_no_match,
				   const infinint & hourshift,
				   bool empty,
				   U_16 & exception,
				   std::string & except_msg)
    {
	statistics ret;
	WRAPPER_IN
	ret = op_extract(arch,
			 fs_root,
			 selection,
			 subtree,
			 allow_over,
			 warn_over,
			 info_details,
			 detruire,
			 only_more_recent,
			 restore_ea_root,
			 restore_ea_user,
			 flat,
			 ignore_owner,
			 warn_remove_no_match,
			 hourshift,
			 empty);
	WRAPPER_OUT(exception,  except_msg)
	return ret;
    }


    statistics op_diff(archive *arch, const path & fs_root,
                       const mask & selection, const mask & subtree,
                       bool info_details,
                       bool check_ea_root, bool check_ea_user,
                       bool ignore_owner)
    {
        statistics st;

            // sanity checks

	if(arch == NULL)
	    throw Elibcall("op_diff", "NULL argument given to arch");
        if(&fs_root == NULL)
            throw Elibcall("op_diff", "NULL argument given to fs_root");
        if(&selection == NULL)
            throw Elibcall("op_diff", "NULL argument given to selection");
        if(&subtree == NULL)
            throw Elibcall("op_diff", "NULL argument given to subtree");

            // end of sanity checks

        try
        {
            inode::set_ignore_owner(ignore_owner);

	    filtre_difference(selection, subtree, arch->get_cat(), fs_root, info_details, st, check_ea_root, check_ea_user);
        }
        catch(Erange & e)
        {
            string msg = string("error while comparing archive with filesystem: ")+e.get_message();
            user_interaction_warning(msg);
            throw Edata(msg);
        }

        return st;
    }

    statistics op_diff_noexcept(archive *arch,
				const path & fs_root,
				const mask &selection,
				const mask &subtree,
				bool info_details,
				bool check_ea_root,
				bool check_ea_user,
				bool ignore_owner,
				U_16 & exception,
				std::string & except_msg)
    {
	statistics ret;
	WRAPPER_IN
	ret = op_diff(arch,
		      fs_root,
		      selection,
		      subtree,
		      info_details,
		      check_ea_root,
		      check_ea_user,
		      ignore_owner);
	WRAPPER_OUT(exception, except_msg)
	return ret;
    }


    statistics op_test(archive *arch,
		       const mask & selection,
                       const mask & subtree,
                       bool info_details)
    {
        statistics st;

            // sanity checks

	if(arch == NULL)
	    throw Elibcall("op_test", "NULL argument given to arch");
        if(&selection == NULL)
            throw Elibcall("op_test", "NULL argument given to selection");
        if(&subtree == NULL)
            throw Elibcall("op_test", "NULL argument given to subtree");

            // end of sanity checks

        try
        {
	    filtre_test(selection, subtree, arch->get_cat(), info_details, st);
        }
        catch(Erange & e)
        {
            string msg = string("error while testing archive: ")+e.get_message();
            user_interaction_warning(msg);
            throw Edata(msg);
        }

        return st;
    }

    statistics op_test_noexcept(archive *arch,
				const mask &selection,
				const mask &subtree,
				bool info_details,
				U_16 & exception,
				std::string & except_msg)
    {
	statistics ret;
	WRAPPER_IN
	ret = op_test(arch,
		      selection,
		      subtree,
		      info_details);
	WRAPPER_OUT(exception, except_msg)
	return ret;
    }



    static void dummy_call(char *x)
    {
        static char id[]="$Id: libdar.cpp,v 1.25.2.6 2004/09/10 22:34:15 edrusb Exp $";
        dummy_call(id);
    }

    char *libdar_str2charptr_noexcept(const std::string & x, U_16 & exception, std::string & except_msg)
    {
        char *ret = NULL;
        WRAPPER_IN
	ret = tools_str2charptr(x);
	WRAPPER_OUT(exception, except_msg)
        return ret;
    }

    bool get_children_of(archive *arch, const string & dir)
    {
	if(arch == NULL)
	    throw Erange("libdar:get_children_of", "invalid NULL argument given as archive pointer");

	return arch->get_cat().get_contenu()->callback_for_children_of(dir);
    }

    bool get_children_of_noexcept(archive *arch, const std::string & dir,
				  U_16 & exception,
				  std::string & except_msg)
    {
	bool ret = false;
	WRAPPER_IN
	ret = get_children_of(arch, dir);
	WRAPPER_OUT(exception, except_msg)
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
        largefile = sizeof(off_t) > 4;
#endif
#ifdef LIBDAR_NODUMP_FEATURE
        nodump = true;
#else
        nodump = false;
#endif
#ifdef LIBDAR_SPECIAL_ALLOC
        special_alloc = true;
#else
        special_alloc = false;
#endif
#ifdef LIBDAR_MODE
        bits = LIBDAR_MODE;
#else
        bits = 0; // infinint
#endif
    }

    static statistics op_create_in(bool create_not_isolate, const path & fs_root, const path & sauv_path, archive *ref_arch,
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
                                   const mask & compr_mask,
                                   const infinint & min_compr_size,
                                   bool nodump,
                                   const infinint & hourshift,
				   bool empty)
    {
        statistics st;

            // sanity checks as much as possible to avoid libdar crashing due to bad arguments
            // useless arguments are not reported.

        if(&fs_root == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to fs_root");
        if(&sauv_path == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to sauv_path");
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
        if(&execute == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to execute");
        if(&pass == NULL)
            throw Elibcall("op_create/op_isolate", "NULL argument given to pass");
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
	    const path *ref_path = NULL;
            generic_file *decoupe = NULL;
            compressor *zip = NULL;
            scrambler *scram = NULL;
            S_I sar_opt = 0;
            generic_file *zip_base = NULL;

		// warning against saving the archive itself

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

            if(ref_arch != NULL) // from a existing archive
	    {
		ref = &(ref_arch->get_cat());
		ref_path = & ref_arch->get_path();
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
                    user_interaction_pause("Ready to start writing the archive?");

		if(empty)
		    decoupe = new null_file(gf_write_only);
		else
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

		switch(crypto)
		{
		case crypto_scrambling:
                    ver.flag |= VERSION_FLAG_SCRAMBLED;
		    break;
		case crypto_none:
		    break; // no bit to set;
		default:
		    throw Erange("libdar:op_create_in","unknown crypto algorithm");
		}
                ver.write(*decoupe);

                if(!empty)
                {
		    switch(crypto)
		    {
		    case crypto_scrambling:
			scram = new scrambler(pass, *decoupe);
			if(scram == NULL)
			    throw Ememory("op_create");
			zip_base = scram;
			break;
		    case crypto_none:
			zip_base = decoupe;
			break;
		    default:
			throw SRC_BUG; // cryto value should have been checked before
		    }
		}
		else
		    zip_base = decoupe;

		zip = new compressor(empty ? none : algo, *zip_base, compression_level);
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
                if(ref_arch != NULL && create_not_isolate)
                {
                    if(info_details)
                        user_interaction_warning("Adding reference to files that have been destroyed since reference backup...");
                    st.deleted = current.update_destroyed_with(*ref);
                }

                    // making some place in memory
                if(ref != NULL && ref_arch == NULL)
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
