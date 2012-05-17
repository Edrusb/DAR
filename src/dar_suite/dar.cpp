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
// $Id: dar.cpp,v 1.43 2005/12/29 02:32:41 edrusb Rel $
//
/*********************************************************************/
//
#include "../my_config.h"
#include <string>
#include <iostream>
#include "erreurs.hpp"
#include "user_interaction.hpp"
#include "command_line.hpp"
#include "tools.hpp"
#include "test_memory.hpp"
#include "dar.hpp"
#include "sar_tools.hpp"
#include "dar_suite.hpp"
#include "integers.hpp"
#include "deci.hpp"
#include "libdar.hpp"
#include "shell_interaction.hpp"

#ifndef DAR_VERSION
#define DAR_VERSION "unknown"
#endif

using namespace std;
using namespace libdar;

static void display_sauv_stat(user_interaction & dialog, const statistics & st);
static void display_rest_stat(user_interaction & dialog, const statistics & st);
static void display_diff_stat(user_interaction & dialog, const statistics &st);
static void display_test_stat(user_interaction & dialog, const statistics & st);
static void display_merge_stat(user_interaction & dialog, const statistics & st);
static S_I little_main(user_interaction & dialog, S_I argc, char *argv[], const char **env);

int main(S_I argc, char *argv[], const char **env)
{
    return dar_suite_global(argc, argv, env, &little_main);
}

static S_I little_main(user_interaction & dialog, S_I argc, char *argv[], const char **env)
{
    S_I ret = EXIT_OK;
    operation op;
    path *fs_root;
    path *sauv_root;
    path *ref_root;
    infinint file_size;
    infinint first_file_size;
    mask *selection, *subtree, *compr_mask, *ea_mask;
    string filename, *ref_filename;
    bool allow_over, warn_over, info_details, detruire, beep, empty_dir, only_more_recent, ea_erase;
    infinint pause;
    compression algo;
    U_I compression_level;
    string input_pipe, output_pipe;
    inode::comparison_fields what_to_check;
    string execute, execute_ref;
    string pass;
    string pass_ref;
    bool flat;
    infinint min_compr_size;
    const char *home = tools_get_from_env(env, "HOME");
    bool nodump;
    infinint hourshift;
    bool warn_remove_no_match;
    string alteration;
    bool empty;
    path *on_fly_root;
    string *on_fly_filename;
    bool alter_atime;
    bool same_fs;
    bool snapshot;
    bool cache_directory_tagging;
    U_32 crypto_size;
    U_32 crypto_size_ref;
    bool display_skipped;
    archive::listformat list_mode;
    path *aux_root;
    string *aux_filename;
    string aux_pass;
    string aux_execute;
    U_32 aux_crypto_size;
    bool keep_compressed;
    infinint fixed_date;

    if(home == NULL)
        home = "/";
    if(! get_args(dialog,
		  home,
		  argc,
		  argv,
		  op,
		  fs_root,
		  sauv_root,
		  ref_root,
                  file_size,
		  first_file_size,
		  selection,
                  subtree,
		  filename,
		  ref_filename,
                  allow_over,
		  warn_over,
		  info_details,
		  algo,
                  compression_level,
		  detruire,
                  pause,
		  beep,
		  empty_dir,
		  only_more_recent,
                  ea_mask,
                  input_pipe, output_pipe,
                  what_to_check,
                  execute,
		  execute_ref,
                  pass,
		  pass_ref,
                  compr_mask,
                  flat,
		  min_compr_size,
		  nodump,
                  hourshift,
		  warn_remove_no_match,
		  alteration,
		  empty,
		  on_fly_root,
		  on_fly_filename,
		  alter_atime,
		  same_fs,
		  snapshot,
		  cache_directory_tagging,
		  crypto_size,
		  crypto_size_ref,
		  ea_erase,
		  display_skipped,
		  list_mode,
		  aux_root,
		  aux_filename,
		  aux_pass,
		  aux_execute,
		  aux_crypto_size,
		  keep_compressed,
		  fixed_date))
        return EXIT_SYNTAX;
    else
    {
        MEM_IN;
	archive *arch = NULL;
	archive *aux = NULL;
	archive *cur = NULL;
	bool on_fly_action = on_fly_filename != NULL && on_fly_root != NULL;
        shell_interaction_set_beep(beep);

        if(filename != "-"  || (output_pipe != "" && op != create && op != isolate && op != merging))
            shell_interaction_change_non_interactive_output(&cout);
            // standart output can be used to send non interactive
            // messages

        try
        {
            statistics st = false;
	    string tmp_pass;
	    crypto_algo crypto, aux_crypto;

            switch(op)
            {
            case create:
	    case merging:
		if(ref_filename != NULL && ref_root != NULL)
		{
		    crypto_split_algo_pass(pass_ref, crypto, tmp_pass);
		    if(op == merging && aux_root != NULL && info_details)
			dialog.warning(gettext("Considering the (first) archive of reference:"));
		    arch = new archive(dialog, *ref_root, *ref_filename, EXTENSION, crypto, tmp_pass, crypto_size_ref,
				       input_pipe, output_pipe, execute_ref, info_details);
		}

		if(aux_root != NULL && aux_filename != NULL)
		{
		    if(op != merging)
			throw SRC_BUG;
		    if(info_details)
			dialog.warning(gettext("Considering the (second alias auxiliary) archive of reference:"));
		    crypto_split_algo_pass(aux_pass, aux_crypto, tmp_pass);
		    aux = new archive(dialog, *aux_root, *aux_filename, EXTENSION, aux_crypto, tmp_pass, aux_crypto_size,
				      "", "", aux_execute, info_details);
		}

		crypto_split_algo_pass(pass, crypto, tmp_pass);

		switch(op)
		{
		case create:
		    cur = new archive(dialog, *fs_root, *sauv_root, arch, *selection, *subtree, filename, EXTENSION,
				      allow_over, warn_over, info_details, pause, empty_dir,
				      algo, compression_level,
				      file_size, first_file_size,
				      *ea_mask,
				      execute, crypto, tmp_pass, crypto_size,
				      *compr_mask,
				      min_compr_size, nodump,
				      what_to_check,
				      hourshift, empty,
				      alter_atime, same_fs,
				      snapshot,
				      cache_directory_tagging,
				      display_skipped,
				      fixed_date,
				      &st);
		    display_sauv_stat(dialog, st);
		    break;
		case merging:
		    cur = new archive(dialog,    // user_interaction &
				      *sauv_root,  //const path &
				      arch,       // archive *
				      aux,        // archive *
				      *selection, // const mask &
				      *subtree,   // const mask &
				      filename,   // const string &
				      EXTENSION,  // const string &
				      allow_over, // bool
				      warn_over,  // bool
				      info_details, // bool
				      pause,         // const infinint &
				      empty_dir,   // bool
				      algo,         // compression
				      compression_level,  // U_I
				      file_size,     // const infinint &
				      first_file_size, // const infinint &
				      *ea_mask,      // const mask &
				      execute,       // const string &
				      crypto,        // crypto_algo
				      tmp_pass,      // const string &
				      crypto_size,   // U_32
				      *compr_mask,    // const mask &
				      min_compr_size,  // const infinint &
				      empty,           // bool
				      display_skipped,  // bool
				      keep_compressed,
				      &st);            // statistics*
		    display_merge_stat(dialog, st);
		    break;
		default:
		    throw SRC_BUG;
		}

		    // making some room in memory

		if(arch != NULL)
		{
		    delete arch;
		    arch = NULL;
		}
		if(aux != NULL)
		{
		    delete aux;
		    aux = NULL;
		}

		    // checking for onfly isolation

		if(st.get_errored() > 0) // cur is not used for isolation
		    throw Edata(gettext("Some file could not be saved"));
		if(st.get_tooold() != 0)
		    ret = EXIT_SAVED_MODIFIED;

		if(on_fly_action)
		{
		    if(info_details)
			dialog.warning(gettext("Now performing on-fly isolation..."));
		    if(cur == NULL)
			throw SRC_BUG;
		    arch = new archive(dialog, *on_fly_root, cur, *on_fly_filename, EXTENSION,
				       allow_over, warn_over, info_details,
				       false, bzip2, 9, 0, 0,
				       "", crypto_none, "", 0, empty);
		}
		break;
            case isolate:
		crypto_split_algo_pass(pass_ref, crypto, tmp_pass);
		arch = new archive(dialog, *ref_root, *ref_filename, EXTENSION, crypto, tmp_pass, crypto_size_ref,
				   input_pipe, output_pipe, execute_ref, info_details);
		crypto_split_algo_pass(pass, crypto, tmp_pass);
                cur = new archive(dialog, *sauv_root, arch, filename, EXTENSION, allow_over, warn_over, info_details,
				  pause, algo, compression_level, file_size, first_file_size,
				  execute, crypto, tmp_pass, crypto_size, empty);
                break;
            case extract:
		crypto_split_algo_pass(pass, crypto, tmp_pass);
		arch = new archive(dialog, *sauv_root, filename, EXTENSION, crypto, tmp_pass, crypto_size,
				   input_pipe, output_pipe, execute, info_details);
                st = arch->op_extract(dialog, *fs_root, *selection, *subtree, allow_over, warn_over,
				      info_details, detruire, only_more_recent, *ea_mask,
				      flat,
				      what_to_check,
				      warn_remove_no_match, hourshift, empty, ea_erase,
				      display_skipped, NULL);
                display_rest_stat(dialog, st);
                if(st.get_errored() > 0)
                    throw Edata(gettext("All files asked could not be restored"));
                break;
            case diff:
		crypto_split_algo_pass(pass, crypto, tmp_pass);
		arch = new archive(dialog, *sauv_root, filename, EXTENSION, crypto, tmp_pass, crypto_size,
				   input_pipe, output_pipe, execute, info_details);
                st = arch->op_diff(dialog, *fs_root, *selection, *subtree, info_details, *ea_mask,
				   what_to_check,
				   alter_atime, display_skipped, NULL);
                display_diff_stat(dialog, st);
                if(st.get_errored() > 0 || st.get_deleted() > 0)
                    throw Edata(gettext("Some file comparisons failed"));
                break;
            case test:
		crypto_split_algo_pass(pass, crypto, tmp_pass);
		arch = new archive(dialog, *sauv_root, filename, EXTENSION, crypto, tmp_pass, crypto_size,
				   input_pipe, output_pipe, execute, info_details);
                st = arch->op_test(dialog, *selection, *subtree, info_details, display_skipped, NULL);
                display_test_stat(dialog, st);
                if(st.get_errored() > 0)
                    throw Edata(gettext("Some files are corrupted in the archive and it will not be possible to restore them"));
                break;
            case listing:
		crypto_split_algo_pass(pass, crypto, tmp_pass);
		arch = new archive(dialog, *sauv_root, filename, EXTENSION, crypto, tmp_pass, crypto_size,
				   input_pipe, output_pipe, execute, info_details);
                arch->op_listing(dialog, info_details, list_mode, *selection, alteration != "");
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
	    if(arch != NULL)
		delete arch;
	    if(cur != NULL)
		delete cur;
	    if(ea_mask != NULL)
		delete ea_mask;
	    if(aux != NULL)
		delete aux;
            throw;
        }

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
	if(arch != NULL)
	    delete arch;
	if(cur != NULL)
	    delete cur;
	if(ea_mask != NULL)
	    delete ea_mask;
	if(aux != NULL)
	    delete aux;
        MEM_OUT;

        return ret;
    }
}

static void dummy_call(char *x)
{
    static char id[]="$Id: dar.cpp,v 1.43 2005/12/29 02:32:41 edrusb Rel $";
    dummy_call(id);
}


static void display_sauv_stat(user_interaction & dialog, const statistics & st)
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

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i inode(s) saved\n"), &treated);
    dialog.printf(gettext(" with %i hard link(s) recorded\n"), &hard_links);
    dialog.printf(gettext(" %i inode(s) changed at the moment of the backup\n"), &tooold);
    dialog.printf(gettext(" %i inode(s) not saved (no file change)\n"), &skipped);
    dialog.printf(gettext(" %i inode(s) failed to save (filesystem error)\n"), &errored);
    dialog.printf(gettext(" %i files(s) ignored (excluded by filters)\n"), &ignored);
    dialog.printf(gettext(" %i files(s) recorded as deleted from reference backup\n"), &deleted);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of file considered: %i\n"), &total);
#ifdef EA_SUPPORT
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" EA saved for %i file(s)\n"), &ea_treated);
#endif
    dialog.printf(" --------------------------------------------\n");
}

static void display_rest_stat(user_interaction & dialog, const statistics & st)
{
    infinint total = st.total();
    infinint treated = st.get_treated();
    infinint tooold = st.get_tooold();
    infinint skipped = st.get_skipped();
    infinint ignored = st.get_ignored();
    infinint errored = st.get_errored();
    infinint deleted = st.get_deleted();
    infinint ea_treated = st.get_ea_treated();

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i file(s) restored\n"), &treated);
    dialog.printf(gettext(" %i file(s) not restored (not saved in archive)\n"), &skipped);
    dialog.printf(gettext(" %i file(s) ignored (excluded by filters)\n"), &ignored);
    dialog.printf(gettext(" %i file(s) less recent than the one on filesystem\n"), &tooold);
    dialog.printf(gettext(" %i file(s) failed to restore (filesystem error)\n"), &errored);
    dialog.printf(gettext(" %i file(s) deleted\n"), &deleted);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of file considered: %i\n"), &total);
#ifdef EA_SUPPORT
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" EA restored for %i file(s)\n"), &ea_treated);
#endif
    dialog.printf(" --------------------------------------------\n");
}

static void display_diff_stat(user_interaction & dialog, const statistics &st)
{
    infinint total = st.total();
    infinint treated = st.get_treated();
    infinint ignored = st.get_ignored();
    infinint errored = st.get_errored();

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i file(s) treated\n"), &treated);
    dialog.printf(gettext(" %i file(s) do not match those on filesystem\n"), &errored);
    dialog.printf(gettext(" %i file(s) ignored (excluded by filters)\n"), &ignored);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of file considered: %i\n"), &total);
    dialog.printf(" --------------------------------------------\n");
}

static void display_test_stat(user_interaction & dialog, const statistics & st)
{
    infinint total = st.total();

    infinint treated = st.get_treated();
    infinint errored = st.get_errored();
    infinint skipped = st.get_skipped();

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i file(s) treated\n"), &treated);
    dialog.printf(gettext(" %i file(s) with error\n"), &errored);
    dialog.printf(gettext(" %i file(s) ignored (excluded by filters)\n"), &skipped);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of file considered: %i\n"), &total);
    dialog.printf(" --------------------------------------------\n");
}

static void display_merge_stat(user_interaction & dialog, const statistics & st)
{
    infinint total = st.total();

    infinint treated = st.get_treated();
    infinint hard_links = st.get_hard_links();
    infinint deleted = st.get_deleted();
    infinint ignored = st.get_ignored();

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i file(s) added to archive\n"), &treated);
    dialog.printf(gettext(" with %i hard link(s) recorded\n"), &hard_links);
    dialog.printf(gettext(" %i file(s) ignored (excluded by filters)\n"), &ignored);
    dialog.printf(gettext(" %i file(s) recorded as deleted\n"), &deleted);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of file considered: %i\n"), &total);
    dialog.printf(" --------------------------------------------\n");
}

const char *dar_version()
{
    return DAR_VERSION;
}
