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
// $Id: libdar_4_4.cpp,v 1.15.2.2 2011/07/21 14:29:01 edrusb Exp $
//
/*********************************************************************/

#include "../my_config.h"

#include "libdar_4_4.hpp"
#include "libdar.hpp"

namespace libdar_4_4
{

    static void fake_version(U_I & major, U_I & medium, U_I & minor);


    void user_interaction::dar_manager_show_version(U_I number,
						    const std::string & data_date,
						    const std::string & ea_date)
    {
	throw libdar::Elibcall("user_interaction::dar_manager_show_version", "Not overwritten dar_manager_show_version() method has been called!");
    }

    libdar::secu_string string2secu_string(const std::string & st)
    {
	return libdar::secu_string(st.c_str(), (libdar::U_I)st.size());
    }


    static libdar::archive_options_read build_options_read(crypto_algo crypto,
							   const std::string &pass,
							   U_32 crypto_size,
							   const std::string & input_pipe,
							   const std::string & output_pipe,
							   const std::string & execute,
							   bool info_details)
    {
	libdar::archive_options_read ret;

	ret.set_crypto_algo(crypto);
	ret.set_crypto_pass(string2secu_string(pass));
	ret.set_crypto_size(crypto_size);
	ret.set_input_pipe(input_pipe);
	ret.set_output_pipe(output_pipe);
	ret.set_execute(execute);
	ret.set_info_details(info_details);

	return ret;
    }

    archive *archive::piggy_convert(libdar::archive * ref)
    {
	return (archive *)(ref);
    }

    archive::archive(user_interaction & dialog,
		     const path & chem,
		     const std::string & basename,
		     const std::string & extension,
		     crypto_algo crypto,
		     const std::string &pass,
		     U_32 crypto_size,
		     const std::string & input_pipe,
		     const std::string & output_pipe,
		     const std::string & execute,
		     bool info_details)
	: libdar::archive(dialog,
			  chem,
			  basename,
			  extension,
			  build_options_read(crypto,
					     pass,
					     crypto_size,
					     input_pipe,
					     output_pipe,
					     execute,
					     info_details)
	    ) {}

    static libdar::archive_options_create build_options_create(archive *ref_arch,
							       const mask & selection,
							       const mask & subtree,
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
							       const std::string & execute,
							       crypto_algo crypto,
							       const std::string & pass,
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
							       const infinint & fixed_date)
    {
	libdar::archive_options_create ret;

	ret.set_reference(ref_arch);
	ret.set_selection(selection);
	ret.set_subtree(subtree);
	ret.set_allow_over(allow_over);
	ret.set_warn_over(warn_over);
	ret.set_info_details(info_details);
	ret.set_pause(pause);
	ret.set_empty_dir(empty_dir);
	ret.set_compression(algo);
	ret.set_compression_level(compression_level);
	ret.set_slicing(file_size, first_file_size);
	ret.set_ea_mask(ea_mask);
	ret.set_execute(execute);
	ret.set_crypto_algo(crypto);
	ret.set_crypto_pass(string2secu_string(pass));
	ret.set_crypto_size(crypto_size);
	ret.set_compr_mask(compr_mask);
	ret.set_min_compr_size(min_compr_size);
	ret.set_nodump(nodump);
	ret.set_what_to_check(what_to_check);
	ret.set_hourshift(hourshift);
	ret.set_empty(empty);
	ret.set_alter_atime(alter_atime);
	ret.set_same_fs(same_fs);
	ret.set_snapshot(snapshot);
	ret.set_cache_directory_tagging(cache_directory_tagging);
	ret.set_display_skipped(display_skipped);
	ret.set_fixed_date(fixed_date);

	return ret;
    }

    archive::archive(user_interaction & dialog,
		     const path & fs_root,
		     const path & sauv_path,
		     archive *ref_arch,
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
		     const infinint &file_size,
		     const infinint &first_file_size,
		     const mask & ea_mask,
		     const std::string & execute,
		     crypto_algo crypto,
		     const std::string & pass,
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
	: libdar::archive(dialog,
			  fs_root,
			  sauv_path,
			  filename,
			  extension,
			  build_options_create(ref_arch,
					       selection,
					       subtree,
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
					       what_to_check,
					       hourshift,
					       empty,
					       alter_atime,
					       same_fs,
					       snapshot,
					       cache_directory_tagging,
					       display_skipped,
					       fixed_date),
			  progressive_report) {}

    static libdar::archive_options_isolate build_options_isolate(bool allow_over,
								 bool warn_over,
								 bool info_details,
								 const infinint & pause,
								 compression algo,
								 U_I compression_level,
								 const infinint &file_size,
								 const infinint &first_file_size,
								 const std::string & execute,
								 crypto_algo crypto,
								 const std::string & pass,
								 U_32 crypto_size,
								 bool empty)
    {
	libdar::archive_options_isolate ret;

	ret.set_allow_over(allow_over);
	ret.set_warn_over(warn_over);
	ret.set_info_details(info_details);
	ret.set_pause(pause);
	ret.set_compression(algo);
	ret.set_compression_level(compression_level);
	ret.set_slicing(file_size, first_file_size);
	ret.set_execute(execute);
	ret.set_crypto_algo(crypto);
	ret.set_crypto_pass(string2secu_string(pass));
	ret.set_crypto_size(crypto_size);
	ret.set_empty(empty);

	return ret;
    }

    archive::archive(user_interaction & dialog,
		     const path &sauv_path,
		     archive *ref_arch,
		     const std::string & filename,
		     const std::string & extension,
		     bool allow_over,
		     bool warn_over,
		     bool info_details,
		     const infinint & pause,
		     compression algo,
		     U_I compression_level,
		     const infinint &file_size,
		     const infinint &first_file_size,
		     const std::string & execute,
		     crypto_algo crypto,
		     const std::string & pass,
		     U_32 crypto_size,
		     bool empty)
	: libdar::archive(dialog,
			  sauv_path,
			  ref_arch,
			  filename,
			  extension,
			  build_options_isolate(allow_over,
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
						empty)
	    ) {}

    static libdar::archive_options_merge build_options_merge(archive *ref_arch2,
							     const mask & selection,
							     const mask & subtree,
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
							     bool keep_compressed)
    {
	libdar::archive_options_merge ret;

	ret.set_auxilliary_ref(ref_arch2);
	ret.set_selection(selection);
	ret.set_subtree(subtree);
	ret.set_allow_over(allow_over);
	ret.set_warn_over(warn_over);
	ret.set_info_details(info_details);
	ret.set_pause(pause);
	ret.set_empty_dir(empty_dir);
	ret.set_compression(algo);
	ret.set_compression_level(compression_level);
	ret.set_slicing(file_size, first_file_size);
	ret.set_ea_mask(ea_mask);
	ret.set_execute(execute);
	ret.set_crypto_algo(crypto);
	ret.set_crypto_pass(string2secu_string(pass));
	ret.set_crypto_size(crypto_size);
	ret.set_compr_mask(compr_mask);
	ret.set_min_compr_size(min_compr_size);
	ret.set_empty(empty);
	ret.set_display_skipped(display_skipped);
	ret.set_keep_compressed(keep_compressed);

	return ret;
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
	: libdar::archive(dialog,
			  sauv_path,
			  ref_arch1,
			  filename,
			  extension,
			  build_options_merge(ref_arch2,
					      selection,
					      subtree,
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
					      empty,
					      display_skipped,
					      keep_compressed
			      ),
			  progressive_report) {}


    statistics archive::op_extract(user_interaction & dialog,
				   const path &fs_root,
				   const mask &selection,
				   const mask &subtree,
				   bool allow_over,
				   bool warn_over,
				   bool info_details,
				   bool detruire,
				   bool only_more_recent,
				   const mask & ea_mask,
				   bool flat,
				   inode::comparison_fields what_to_check,
				   bool warn_remove_no_match,
				   const infinint & hourshift,
				   bool empty,
				   bool ea_erase,
				   bool display_skipped,
				   statistics *progressive_report)
    {
	libdar::archive_options_extract options;
	const libdar::crit_action *overwrite = NULL;
	statistics ret;

	try
	{
	    tools_4_4_build_compatible_overwriting_policy(allow_over, detruire, only_more_recent, hourshift, ea_erase, overwrite);
	    if(overwrite == NULL)
		throw SRC_BUG;
	    options.set_overwriting_rules(*overwrite);
	    options.set_selection(selection);
	    options.set_subtree(subtree);
	    options.set_warn_over(warn_over);
	    options.set_info_details(info_details);
	    options.set_ea_mask(ea_mask);
	    options.set_flat(flat);
	    options.set_what_to_check(what_to_check);
	    options.set_warn_remove_no_match(warn_remove_no_match);
	    options.set_empty(empty);
	    options.set_display_skipped(display_skipped);

	    ret = libdar::archive::op_extract(dialog,
					      fs_root,
					      options,
					      progressive_report);

	}
	catch(...)
	{
	    if(overwrite != NULL)
		delete overwrite;
	    throw;
	}
	if(overwrite != NULL)
	    delete overwrite;

	return ret;
    }

    void archive::op_listing(user_interaction & dialog,
			     bool info_details,
			     archive::listformat list_mode,
			     const mask &selection,
			     bool filter_unsaved)
    {
	libdar::archive_options_listing options;

	if(info_details)
	{
	    libdar::archive::summary(dialog);
	    dialog.pause(libdar::dar_gettext("Continue listing archive contents?"));
	}
	options.set_info_details(info_details);
	options.set_list_mode(list_mode);
	options.set_selection(selection);
	options.set_filter_unsaved(filter_unsaved);

	return libdar::archive::op_listing(dialog, options);
    }

    statistics archive::op_diff(user_interaction & dialog,
				const path & fs_root,
				const mask &selection,
				const mask &subtree,
				bool info_details,
				const mask & ea_mask,
				inode::comparison_fields what_to_check,
				bool alter_atime,
				bool display_skipped,
				statistics * progressive_report,
				const infinint & hourshift)
    {
	libdar::archive_options_diff options;

	options.set_selection(selection);
	options.set_subtree(subtree);
	options.set_info_details(info_details);
	options.set_ea_mask(ea_mask);
	options.set_what_to_check(what_to_check);
	options.set_alter_atime(alter_atime);
	options.set_display_skipped(display_skipped);
	options.set_hourshift(hourshift);

	return libdar::archive::op_diff(dialog, fs_root, options, progressive_report);
    }

    statistics archive::op_test(user_interaction & dialog,
				const mask &selection,
				const mask &subtree,
				bool info_details,
				bool display_skipped,
				statistics * progressive_report)
    {
	libdar::archive_options_test options;

	options.set_selection(selection);
	options.set_subtree(subtree);
	options.set_info_details(info_details);
	options.set_display_skipped(display_skipped);

	return libdar::archive::op_test(dialog, options, progressive_report);
    }

    void get_version(U_I & major, U_I & minor, bool init_libgcrypt)
    {
	U_I medium;
	    // this is needed to initialise the real libdar
	libdar::get_version(major, medium, minor, init_libgcrypt);
	    // but we must fake libdar 4.4.x (OK, here this is 4.6.x but it stays compatible with 4.4.x)
	fake_version(major, medium, minor);
    }

    void get_version_noexcept(U_I & major, U_I & minor, U_16 & exception, std::string & except_msg, bool init_libgcrypt)
    {
	U_I medium;
	libdar::get_version_noexcept(major, medium, minor, exception, except_msg, init_libgcrypt);
	fake_version(major, medium, minor);
    }

    void get_version(U_I & major, U_I & medium, U_I & minor, bool init_libgcrypt)
    {
	libdar::get_version(major, medium, minor, init_libgcrypt);
	fake_version(major, medium, minor);
    }

    void get_version_noexcept(U_I & major, U_I & medium, U_I & minor, U_16 & exception, std::string & except_msg, bool init_libgcrypt)
    {
	libdar::get_version_noexcept(major, medium, minor, exception, except_msg, init_libgcrypt);
	fake_version(major, medium, minor);
    }

    void get_compile_time_features(bool & ea, bool & largefile, bool & nodump, bool & special_alloc, U_I & bits,
				   bool & thread_safe,
				   bool & libz, bool & libbz2, bool & libcrypto,
				   bool & new_blowfish)
    {
	ea = libdar::compile_time::ea();
	largefile = libdar::compile_time::largefile();
	nodump = libdar::compile_time::nodump();
	special_alloc = libdar::compile_time::special_alloc();
	bits = libdar::compile_time::bits();
	thread_safe = libdar::compile_time::thread_safe();
	libz = libdar::compile_time::libz();
	libbz2 = libdar::compile_time::libbz2();
	libcrypto = libdar::compile_time::libgcrypt();
	new_blowfish = libcrypto;
    }

    archive* open_archive_noexcept(user_interaction & dialog,
				   const path & chem, const std::string & basename,
				   const std::string & extension,
				   crypto_algo crypto, const std::string &pass, U_32 crypto_size,
				   const std::string & input_pipe, const std::string & output_pipe,
				   const std::string & execute, bool info_details,
				   U_16 & exception,
				   std::string & except_msg)
    {
	libdar::archive_options_read options;

	options.set_crypto_algo(crypto);
	options.set_crypto_pass(string2secu_string(pass));
	options.set_crypto_size(crypto_size);
	options.set_input_pipe(input_pipe);
	options.set_output_pipe(output_pipe);
	options.set_execute(execute);
	options.set_info_details(info_details);

	return archive::piggy_convert(
	    libdar::open_archive_noexcept(dialog,
					  chem, basename,
					  extension,
					  options,
					  exception,
					  except_msg));
    }


    archive *create_archive_noexcept(user_interaction & dialog,
				     const path & fs_root,
				     const path & sauv_path,
				     archive *ref_arch,
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
				     const infinint &file_size,
				     const infinint &first_file_size,
				     const mask & ea_mask,
				     const std::string & execute,
				     crypto_algo crypto,
				     const std::string & pass,
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
				     statistics * progressive_report,
				     U_16 & exception,
				     std::string & except_msg)
    {
	libdar::archive_options_create options;

	options.set_reference(ref_arch);
	options.set_selection(selection);
	options.set_subtree(subtree);
	options.set_allow_over(allow_over);
	options.set_warn_over(warn_over);
	options.set_info_details(info_details);
	options.set_pause(pause);
	options.set_empty_dir(empty_dir);
	options.set_compression(algo);
	options.set_compression_level(compression_level);
	options.set_slicing(file_size, first_file_size);
	options.set_ea_mask(ea_mask);
	options.set_execute(execute);
	options.set_crypto_algo(crypto);
	options.set_crypto_pass(string2secu_string(pass));
	options.set_crypto_size(crypto_size);
	options.set_compr_mask(compr_mask);
	options.set_min_compr_size(min_compr_size);
	options.set_nodump(nodump);
	options.set_what_to_check(what_to_check);
	options.set_hourshift(hourshift);
	options.set_empty(empty);
	options.set_alter_atime(alter_atime);
	options.set_same_fs(same_fs);
	options.set_snapshot(snapshot);
	options.set_cache_directory_tagging(cache_directory_tagging);
	options.set_display_skipped(display_skipped);
	options.set_fixed_date(fixed_date);

	return archive::piggy_convert(
	    libdar::create_archive_noexcept(dialog,
					    fs_root,
					    sauv_path,
					    filename,
					    extension,
					    options,
					    progressive_report,
					    exception,
					    except_msg));
    }

    archive *isolate_archive_noexcept(user_interaction & dialog,
				      const path &sauv_path,
				      archive *ref_arch,
				      const std::string & filename,
				      const std::string & extension,
				      bool allow_over,
				      bool warn_over,
				      bool info_details,
				      const infinint & pause,
				      compression algo,
				      U_I compression_level,
				      const infinint &file_size,
				      const infinint &first_file_size,
				      const std::string & execute,
				      crypto_algo crypto,
				      const std::string & pass,
				      U_32 crypto_size,
				      bool empty,
				      U_16 & exception,
				      std::string & except_msg)
    {
	libdar::archive_options_isolate options;

	options.set_allow_over(allow_over);
	options.set_warn_over(warn_over);
	options.set_info_details(info_details);
	options.set_pause(pause);
	options.set_compression(algo);
	options.set_compression_level(compression_level);
	options.set_slicing(file_size, first_file_size);
	options.set_execute(execute);
	options.set_crypto_algo(crypto);
	options.set_crypto_pass(string2secu_string(pass));
	options.set_crypto_size(crypto_size);
	options.set_empty(empty);

	return archive::piggy_convert(
	    libdar::isolate_archive_noexcept(dialog,
					     sauv_path,
					     ref_arch,
					     filename,
					     extension,
					     options,
					     exception,
					     except_msg));
    }

    archive *merge_archive_noexcept(user_interaction & dialog,
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
				    statistics * progressive_report,
				    U_16 & exception,
				    std::string & except_msg)
    {
	libdar::archive_options_merge options;

	options.set_auxilliary_ref(ref_arch2);
	options.set_selection(selection);
	options.set_subtree(subtree);
	options.set_allow_over(allow_over);
	options.set_warn_over(warn_over);
	options.set_info_details(info_details);
	options.set_pause(pause);
	options.set_empty_dir(empty_dir);
       	options.set_compression(algo);
	options.set_compression_level(compression_level);
	options.set_slicing(file_size, first_file_size);
	options.set_ea_mask(ea_mask);
	options.set_execute(execute);
	options.set_crypto_algo(crypto);
	options.set_crypto_pass(string2secu_string(pass));
	options.set_crypto_size(crypto_size);
	options.set_compr_mask(compr_mask);
	options.set_min_compr_size(min_compr_size);
	options.set_empty(empty);
	options.set_display_skipped(display_skipped);
	options.set_keep_compressed(keep_compressed);

	return archive::piggy_convert(
	    libdar::merge_archive_noexcept(dialog,
					   sauv_path,
					   ref_arch1,
					   filename,
					   extension,
					   options,
					   progressive_report,
					   exception,
					   except_msg));
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: libdar_4_4.cpp,v 1.15.2.2 2011/07/21 14:29:01 edrusb Exp $";
        dummy_call(id);
    }

    void tools_4_4_build_compatible_overwriting_policy(bool allow_over,
						       bool detruire,
						       bool more_recent,
						       const libdar::infinint & hourshift,
						       bool ea_erase,
						       const libdar::crit_action * & overwrite)
    {
	libdar::crit_action *tmp1 = NULL;
	libdar::crit_action *tmp2 = NULL; // tmp1 and tmp2 are used for construction of the overwriting policy
        overwrite = NULL;

	try
	{
	    if(allow_over)
	    {
		if(ea_erase)
		    overwrite = new libdar::crit_constant_action(libdar::data_overwrite, libdar::EA_overwrite);
		else
		    overwrite = new libdar::crit_constant_action(libdar::data_overwrite, libdar::EA_merge_overwrite);
		if(overwrite == NULL)
		    throw Ememory("tools_build_compatible_overwriting_policy");

		tmp1 = new libdar::crit_constant_action(libdar::data_preserve, libdar::EA_preserve);
		if(tmp1 == NULL)
		    throw Ememory("tools_build_compatible_overwriting_policy");

		if(more_recent)
		{
		    tmp2 = new libdar::testing(libdar::crit_invert(libdar::crit_in_place_data_more_recent(hourshift)), *overwrite, *tmp1);
		    if(tmp2 == NULL)
			throw Ememory("tools_build_compatible_overwriting_policy");

		    delete overwrite;
		    overwrite = tmp2;
		    tmp2 = NULL;
		}

		if(!detruire)
		{
		    tmp2 = new libdar::testing(libdar::crit_invert(libdar::crit_in_place_is_inode()), *overwrite, *tmp1);
		    delete overwrite;
		    overwrite = tmp2;
		    tmp2 = NULL;
		}

		delete tmp1;
		tmp1 = NULL;
	    }
	    else
	    {
		overwrite = new libdar::crit_constant_action(libdar::data_preserve, libdar::EA_preserve);
		if(overwrite == NULL)
		    throw Ememory("tools_build_compatible_overwriting_policy");
	    }

	    if(overwrite == NULL)
		throw SRC_BUG;
	    if(tmp1 != NULL)
		throw SRC_BUG;
	    if(tmp2 != NULL)
		throw SRC_BUG;
	}
	catch(...)
	{
	    if(tmp1 != NULL)
		delete tmp1;
	    if(tmp2 != NULL)
		delete tmp2;
	    if(overwrite != NULL)
	    {
		delete overwrite;
		overwrite = NULL;
	    }
	    throw;
	}
    }

    static void fake_version(U_I & major, U_I & medium, U_I & minor)
    {
	major = LIBDAR_COMPILE_TIME_MAJOR;
	medium = LIBDAR_COMPILE_TIME_MEDIUM;
	minor = LIBDAR_COMPILE_TIME_MINOR;
    }

} // end of namespace
