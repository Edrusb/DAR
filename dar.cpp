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
// $Id: dar.cpp,v 1.18 2003/03/21 21:52:27 edrusb Rel $
//
/*********************************************************************/
//
#include <string>
#include <iostream>
#include "erreurs.hpp"
#include "generic_file.hpp"
#include "infinint.hpp"
#include "path.hpp"
#include "mask.hpp"
#include "user_interaction.hpp"
#include "terminateur.hpp"
#include "compressor.hpp"
#include "command_line.hpp"
#include "catalogue.hpp"
#include "sar.hpp"
#include "filtre.hpp"
#include "tools.hpp"
#include "header.hpp"
#include "header_version.hpp"
#include "defile.hpp"
#include "deci.hpp"
#include "test_memory.hpp"
#include "dar.hpp"
#include "sar_tools.hpp"
#include "dar_suite.hpp"
#include "zapette.hpp"
#include "tuyau.hpp"
#include "scrambler.hpp"
#include "macro_tools.hpp"
#include "integers.hpp"

using namespace std;

static void op_create(const operation op, const path &fs_root, const path &sauv_path, 
		      const path *ref_path,
		      const mask &selection, const mask &subtree, 
		      const string &filename, const string *ref_filename,
		      bool allow_over, bool warn_over, bool info_details, 
		      bool pause, bool empty_dir, compression algo, U_I compression_level,
		      const infinint &file_size, 
		      const infinint &first_file_size,
		      S_I argc, const char *argv[],
		      bool root_ea, bool user_ea,
		      const string &input_pipe, const string &output_pipe,
		      const string & execute, const string & execute_ref,
		      const string & pass, const string & pass_ref,
		      const mask &compr_mask,
		      const infinint & min_compr_size,
		      bool nodump);
static void op_extract(const path &fs_root, const path &sauv_path,
		       const mask &selection, const mask &subtree, 
		       const string &filename, bool allow_over, bool warn_over, 
		       bool info_details, bool detruire,
		       bool only_more_recent, bool restore_ea_root, 
		       bool restore_ea_user,
		       const string &input_pipe, const string &output_pipe,
		       const string & execute,
		       const string & pass,
		       bool flat,
		       bool ignore_owner);
static void op_diff(const path & fs_root, const path &sauv_path,
		    const mask &selection, const mask &subtree,
		    const string &filename, bool info_details,
		    bool check_ea_root, bool check_ea_user,
		    const string &input_pipe, const string &output_pipe,
		    const string & execute,
		    const string & pass);
static void op_listing(const path &sauv_path, const string &filename, 
		       bool info_details, bool tar_format,
		       const string &input_pipe, const string &output_pipe,
		       const string & execute,
		       const string & pass,
		       const mask &selection);
static void op_test(const path &sauv_path, const mask &selection, 
		    const mask &subtree, const string &filename, 
		    bool info_details,
		    const string &input_pipe, const string &output_pipe,
		    const string & execute,
		    const string & pass);
static string make_string_from(S_I argc, const char *argv[]);
static void display_sauv_stat(const statistics & st);
static void display_rest_stat(const statistics & st);
static void display_diff_stat(const statistics &st);
static void display_test_stat(const statistics & st);
static S_I little_main(S_I argc, char *argv[], const char **env);

S_I main(S_I argc, char *argv[], const char **env)
{
    return dar_suite_global(argc, argv, env, &little_main);
}

static S_I little_main(S_I argc, char *argv[], const char **env)
{
    operation op;
    path *fs_root;
    path *sauv_root;
    path *ref_root;
    infinint file_size;
    infinint first_file_size;
    mask *selection, *subtree, *compr_mask;
    string filename, *ref_filename;
    bool allow_over, warn_over, info_details, detruire, pause, beep, empty_dir, only_more_recent, ea_user, ea_root;
    compression algo;
    U_I compression_level;
    string input_pipe, output_pipe;
    bool ignore_owner;
    string execute, execute_ref;
    string pass;
    string pass_ref;
    bool flat;
    infinint min_compr_size;
    const char *home = tools_get_from_env(env, "HOME");
    bool nodump;

    if(home == NULL)
	home = "/";
    if(! get_args(home, argc, argv, op, fs_root, sauv_root, ref_root,
		  file_size, first_file_size, selection, 
		  subtree, filename, ref_filename, 
		  allow_over, warn_over, info_details, algo, 
		  compression_level, detruire, 
		  pause, beep, empty_dir, only_more_recent, 
		  ea_root, ea_user,
		  input_pipe, output_pipe, 
		  ignore_owner,
		  execute, execute_ref,
		  pass, pass_ref,
		  compr_mask,
		  flat, min_compr_size, nodump))
	return EXIT_SYNTAX;
    else
    {
	MEM_IN;

	user_interaction_set_beep(beep);
	inode::set_ignore_owner(ignore_owner);

	if(filename != "-"  || (output_pipe != "" && op != create && op != isolate))
	    user_interaction_change_non_interactive_output(&cout);
	    // standart output can be used to send non interactive
	    // messages
	
	try
	{
            MEM_IN;
	    switch(op)
	    {
	    case create:
	    case isolate:
		op_create(op, *fs_root, *sauv_root, ref_root, *selection, *subtree, filename, ref_filename, 
			  allow_over, warn_over, info_details, pause, empty_dir, algo, compression_level, file_size, 
			  first_file_size, argc, (const char **)argv, ea_root, ea_user, input_pipe, output_pipe,
			  execute, execute_ref, pass, pass_ref, *compr_mask,
			  min_compr_size, nodump);
		break;
	    case extract:
		op_extract(*fs_root, *sauv_root, *selection, *subtree, filename, allow_over, warn_over,
			   info_details, detruire, only_more_recent, ea_root, ea_user, input_pipe, output_pipe,
			   execute, pass, flat, ignore_owner);
		break;
	    case diff:
		op_diff(*fs_root, *sauv_root, *selection, *subtree, filename, info_details, ea_root, ea_user, 
			input_pipe, output_pipe, execute, pass);
		break;
	    case test:
		op_test(*sauv_root, *selection, *subtree, filename, info_details, input_pipe, output_pipe,
			execute, pass);
		break;
	    case listing:
		op_listing(*sauv_root, filename, info_details, only_more_recent, input_pipe, output_pipe,
			   execute, pass, *selection);
		    // only_more_recent is set to true when listing and -T (--tar-format) is asked
		break;
	    default:
		throw SRC_BUG;
	    }
	    MEM_OUT;
	}
	catch(...)
	{
	    if(fs_root != NULL)
		delete fs_root;
	    if(sauv_root != NULL)
		delete sauv_root;
	    if(selection != NULL)
		delete selection;
	    if(subtree != NULL)
		delete subtree;
	    if(ref_root != NULL)
		delete ref_root;
	    if(ref_filename != NULL)
		delete ref_filename;
	    if(compr_mask != NULL)
		delete compr_mask;
	    throw;
	}

	MEM_OUT;

	if(fs_root != NULL)
	    delete fs_root;
	if(sauv_root != NULL)
	    delete sauv_root;
	if(selection != NULL)
	    delete selection;
	if(subtree != NULL)
	    delete subtree;
	if(ref_root != NULL)
	    delete ref_root;
	if(ref_filename != NULL)
	    delete ref_filename;
	if(compr_mask != NULL)
	    delete compr_mask;

	return EXIT_OK;
    }
}

static void op_create(const operation op, const path &fs_root, const path &sauv_path, const path *ref_path,
		      const mask &selection, const mask &subtree, 
		      const string &filename, const string *ref_filename,
		      bool allow_over, bool warn_over, bool info_details, bool pause,
		      bool empty_dir, compression algo, U_I compression_level,
		      const infinint &file_size, 
		      const infinint &first_file_size, 
		      S_I argc, const char *argv[], bool root_ea, bool user_ea,
		      const string &input_pipe, const string &output_pipe,
		      const string & execute, const string & execute_ref,
		      const string & pass, const string & pass_ref,
		      const mask &compr_mask,
		      const infinint & min_compr_size,
		      bool nodump)
{
    MEM_IN;

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

/*

	if(op == create && sauv_path.is_subdir_of(fs_root)
	   && selection.is_covered(filename+".1."+EXTENSION)
	   && subtree.is_covered((sauv_path + (filename+".1."+EXTENSION)).display()))
	    user_interaction_pause(string("WARNING! The archive is located in the directory to backup, this may create an endless loop when the archive will try to save itself. You can either add -X \"")+ filename + ".*." + EXTENSION +"\" on the command line, or change the location of the archive (see -h for help). Do you really want to continue?");
*/
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

		macro_tools_open_archive(*ref_path, *ref_filename, EXTENSION, SAR_OPT_DONT_ERASE, pass_ref, tmp, scram_ref, zip, v, input_pipe, output_pipe, execute_ref);
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
	    statistics st;
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
		    decoupe = sar_tools_open_archive_fichier((sauv_path + sar_make_filename(filename, 1, EXTENSION)).display(), allow_over, warn_over);
	    else
		decoupe = new sar(filename, EXTENSION, file_size, first_file_size, sar_opt, sauv_path, execute);
		
	    if(decoupe == NULL)
		throw Ememory("op_create");

	    version_copy(ver.edition, macro_tools_supported_version);
	    ver.algo_zip = compression2char(algo);
	    ver.cmd_line = make_string_from(argc-1, argv+1); // argv[0] is the command itself, we don't need it (where from `+1')
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

	    switch(op)
	    {
	    case create:
		filtre_sauvegarde(selection, subtree, zip, current, *ref, fs_root, info_details, st, empty_dir, root_ea, user_ea, compr_mask, min_compr_size, nodump);
		break;
	    case isolate:
		st.clear(); // used for not to have Edata exception thrown at the final checkpoint
		filtre_isolate(current, *ref, info_details);
		break;
	    default:
		throw SRC_BUG; // op_create must only be called for creation or isolation
	    }

	    zip->flush_write();
	    coord.set_catalogue_start(zip->get_position());
	    if(ref_filename != NULL && op == create)
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
	    if(op == create)
		display_sauv_stat(st);

		// final checkpoint:
	    if(st.errored > 0) // st is not used for isolation
		throw Edata("some file could not be saved");
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

    MEM_OUT;
}

static void op_extract(const path &fs_root, const path &sauv_path,
		       const mask &selection, const mask &subtree, 
		       const string &filename, bool allow_over, bool warn_over, 
		       bool info_details,  bool detruire, 
		       bool only_more_recent, bool restore_ea_root, 
		       bool restore_ea_user,
		       const string &input_pipe, const string &output_pipe,
		       const string & execute,
		       const string & pass,
		       bool flat, bool ignore_owner)
{
    try
    {
	generic_file *decoupe = NULL;
	compressor *zip = NULL;
	scrambler *scram = NULL;
	header_version ver;

	try
	{
	    infinint tmp;

	    macro_tools_open_archive(sauv_path, filename, EXTENSION, SAR_OPT_DEFAULT, pass, decoupe, scram, zip, ver, input_pipe, output_pipe, execute);
	    if((ver.flag & VERSION_FLAG_SAVED_EA_ROOT) == 0)
		restore_ea_root = false; // not restoring something not saved
	    if((ver.flag & VERSION_FLAG_SAVED_EA_USER) == 0)
		restore_ea_user = false; // not restoring something not saved
	    
	    catalogue *cat = macro_tools_get_catalogue_from(*decoupe, *zip, info_details, tmp);
	    try
	    {
		statistics st;
		MEM_IN;
		filtre_restore(selection, subtree, *cat, detruire, 
			       fs_root, allow_over, warn_over, info_details,
			       st, only_more_recent, restore_ea_root,
			       restore_ea_user, flat, ignore_owner);
		MEM_OUT;
		display_rest_stat(st);
		if(st.errored > 0)
		    throw Edata("all file asked could not be restored");
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
}

static void op_listing(const path &sauv_path, const string &filename, 
		       bool info_details, bool tar_format,  
		       const string &input_pipe, const string &output_pipe,
		       const string & execute,
		       const string & pass,
		       const mask &selection)
{
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

	    macro_tools_open_archive(sauv_path, filename, EXTENSION, SAR_OPT_DEFAULT, pass, decoupe, scram, zip, ver, input_pipe, output_pipe, execute);
 
	    catalogue *cat = macro_tools_get_catalogue_from(*decoupe, *zip, info_details, cat_size);
	    try
	    {
		if(info_details)
		{
		    sar *real_decoupe = dynamic_cast<sar *>(decoupe);
		    infinint sub_file_size;
		    infinint first_file_size;
		    infinint last_file_size, file_number;
		    
		    user_interaction_stream() << "Archive version format               : " << ver.edition << endl;
		    user_interaction_stream() << "Compression algorithm used           : " << compression2string(char2compression(ver.algo_zip)) << endl;
		    user_interaction_stream() << "Scrambling                           : " << ((ver.flag & VERSION_FLAG_SCRAMBLED) != 0 ? "yes" : "no") << endl;
		    user_interaction_stream() << "Catalogue size in archive            : " << deci(cat_size).human() << " bytes" << endl;
		    user_interaction_stream() << "Command line options used for backup : " << ver.cmd_line << endl;
		    
		    if(real_decoupe != NULL)
		    {
			infinint sub_file_size = real_decoupe->get_sub_file_size();
			infinint first_file_size = real_decoupe->get_first_sub_file_size();
			if(real_decoupe->get_total_file_number(file_number) 
			   && real_decoupe->get_last_file_size(last_file_size))
			{
			    user_interaction_stream() << "Archive is composed of " << file_number << " file" << endl;
			    if(file_number == 1)
				user_interaction_stream() << "File size: " <<  last_file_size << " bytes" << endl;
			    else
			    {
				if(first_file_size != sub_file_size)
				    user_interaction_stream() << "First file size       : " << first_file_size << " bytes" << endl;
				user_interaction_stream() << "File size             : " << sub_file_size << " bytes" << endl;
				user_interaction_stream() << "Last file size        : " << last_file_size << " bytes" << endl;
			    }
			    if(file_number > 1)
				user_interaction_stream() << "Archive total size is : " << (first_file_size + (file_number-2)*sub_file_size + last_file_size ) << " bytes" << endl;
			}
			else // could not read size parameters
			    user_interaction_stream() << "Sorry, file size is unknown at this step of the program." << endl;
		    }
		    else // not reading from a sar
		    {
			decoupe->skip_to_eof();
			user_interaction_stream() << "Archive size is: " << decoupe->get_position() << " bytes" << endl;
			user_interaction_stream() << "Previous archive size does not include headers present in each slice" << endl;
		    }
		    
		    entree_stats stats = cat->get_stats();
		    stats.listing(user_interaction_stream());
		    user_interaction_pause("Continue listing archive contents?");
		}
		if(tar_format)
		    cat->tar_listing(user_interaction_stream(), selection);
		else
		    cat->listing(user_interaction_stream(), selection);
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

static void op_diff(const path & fs_root, const path &sauv_path,
		    const mask &selection, const mask &subtree,
		    const string &filename, bool info_details,
		    bool check_ea_root, bool check_ea_user,
		    const string &input_pipe, const string &output_pipe,
		    const string & execute,
		    const string & pass)
{
    MEM_IN;
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
	    statistics st;
	    
	    macro_tools_open_archive(sauv_path, filename, EXTENSION, SAR_OPT_DEFAULT, pass, decoupe, scram, zip, ver, input_pipe, output_pipe, execute);
	    cat = macro_tools_get_catalogue_from(*decoupe, *zip, info_details, cat_size);
	    filtre_difference(selection, subtree, *cat, fs_root, info_details, st, check_ea_root, check_ea_user);
	    display_diff_stat(st);
	    if(st.errored > 0 || st.deleted > 0)
		throw Edata("some file comparisons failed");
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
    MEM_OUT;
}


static void op_test(const path &sauv_path, const mask &selection, 
		    const mask &subtree, const string &filename, 
		    bool info_details,
		    const string &input_pipe, const string &output_pipe,
		    const string & execute,
		    const string & pass)
{
    MEM_IN;
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
	    statistics st;
	    
	    macro_tools_open_archive(sauv_path, filename, EXTENSION, SAR_OPT_DEFAULT, pass, decoupe, scram, zip, ver, input_pipe, output_pipe, execute);
	    cat = macro_tools_get_catalogue_from(*decoupe, *zip, info_details, cat_size);
	    filtre_test(selection, subtree, *cat, info_details, st);
	    display_test_stat(st);
	    if(st.errored > 0)
		throw Edata("some files are corrupted in the archive and it will not be possible to restore them");
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
    MEM_OUT;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: dar.cpp,v 1.18 2003/03/21 21:52:27 edrusb Rel $";
    dummy_call(id);
}


static void display_sauv_stat(const statistics & st)
{
    user_interaction_stream() << endl << endl << " -------------------------------------------- " << endl;
    user_interaction_stream() << " " << st.treated << " inode(s) saved" << endl;
    user_interaction_stream() << " with " << st.hard_links << " hard link(s) recorded" << endl;
    user_interaction_stream() << " " << st.skipped << " inode(s) not saved (no file change)" << endl;
    user_interaction_stream() << " " << st.errored << " inode(s) failed to save (filesystem error)" << endl;
    user_interaction_stream() << " " << st.ignored << " files(s) ignored (excluded by filters)" << endl;
    user_interaction_stream() << " " << st.deleted << " files(s) recorded as deleted from reference backup" << endl;
    user_interaction_stream() << " -------------------------------------------- " << endl;
    user_interaction_stream() << " Total number of file considered: " << st.total() << endl;
#ifdef EA_SUPPORT
    user_interaction_stream() << " -------------------------------------------- " << endl;
    user_interaction_stream() << " EA saved for " << st.ea_treated << " file(s)" << endl;
    user_interaction_stream() << " -------------------------------------------- " << endl;
#endif
}

static void display_rest_stat(const statistics & st)
{

    user_interaction_stream() << endl << endl << " -------------------------------------------- " << endl;
    user_interaction_stream() << " " << st.treated << " file(s) restored" << endl;
    user_interaction_stream() << " " << st.skipped << " file(s) not restored (not saved in archive)" << endl;
    user_interaction_stream() << " " << st.ignored << " file(s) ignored (excluded by filters)" << endl;
    user_interaction_stream() << " " << st.tooold  << " file(s) less recent than the one on filesystem" << endl;
    user_interaction_stream() << " " << st.errored << " file(s) failed to restore (filesystem error)" << endl;
    user_interaction_stream() << " " << st.deleted << " file(s) deleted" << endl;
    user_interaction_stream() << " -------------------------------------------- " << endl;
    user_interaction_stream() << " Total number of file considered: " << st.total() << endl;
#ifdef EA_SUPPORT
    user_interaction_stream() << " -------------------------------------------- " << endl;
    user_interaction_stream() << " EA restored for " << st.ea_treated << " file(s)" << endl;
    user_interaction_stream() << " -------------------------------------------- " << endl;
#endif
}

static void display_diff_stat(const statistics &st)
{
    user_interaction_stream() << endl;
    user_interaction_stream() << " " << st.errored << " file(s) do not match those on filesystem" << endl;
    user_interaction_stream() << " " << st.deleted << " file(s) failed to be read" << endl;
    user_interaction_stream() << " " << st.ignored << " file(s) ignored (excluded by filters)" << endl;
    user_interaction_stream() << " -------------------------------------------- " << endl;
    user_interaction_stream() << " Total number of file considered: " << st.total() << endl;
}

static void display_test_stat(const statistics & st)
{
    user_interaction_stream() << endl;
    user_interaction_stream() << " " << st.errored << " file(s) with error" << endl;
    user_interaction_stream() << " " << st.ignored << " file(s) ignored (excluded by filters)" << endl;
    user_interaction_stream() << " -------------------------------------------- " << endl;
    user_interaction_stream() << " Total number of file considered: " << st.total() << endl;
}

static string make_string_from(S_I argc, const char *argv[])
{
    string ret = "";
    S_I i;
    bool hide = false;

    for(i = 0; i < argc; i++)
    {
	if(!hide)
	    ret += argv[i];
	else
	{
	    ret += "...";
	    hide = false;
	}
	if(strcmp(argv[i], "-K") == 0 || strcmp(argv[i], "-J") == 0)
	    hide = true;
	ret += " ";
    }

    return ret;
}
