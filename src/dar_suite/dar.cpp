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
// $Id: dar.cpp,v 1.28 2004/12/01 22:10:44 edrusb Rel $
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
static S_I little_main(user_interaction & dialog, S_I argc, char *argv[], const char **env);

int main(S_I argc, char *argv[], const char **env)
{
    return dar_suite_global(argc, argv, env, &little_main);
}

static S_I little_main(user_interaction & dialog, S_I argc, char *argv[], const char **env)
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
    infinint hourshift;
    bool warn_remove_no_match;
    string alteration;
    bool empty;
    path *on_fly_root;
    string *on_fly_filename;
    bool alter_atime;
    bool same_fs;
    U_32 crypto_size;
    U_32 crypto_size_ref;

    if(home == NULL)
        home = "/";
    if(! get_args(dialog,
		  home, argc, argv, op, fs_root, sauv_root, ref_root,
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
                  flat, min_compr_size, nodump,
                  hourshift, warn_remove_no_match,
		  alteration, empty,
		  on_fly_root,
		  on_fly_filename,
		  alter_atime,
		  same_fs,
		  crypto_size,
		  crypto_size_ref))
        return EXIT_SYNTAX;
    else
    {
        MEM_IN;
	archive *arch = NULL;
	archive *cur = NULL;
	bool on_fly_action = on_fly_filename != NULL && on_fly_root != NULL;
        shell_interaction_set_beep(beep);

        if(filename != "-"  || (output_pipe != "" && op != create && op != isolate))
            shell_interaction_change_non_interactive_output(&cout);
            // standart output can be used to send non interactive
            // messages

        try
        {
            statistics st;
	    string tmp_pass;
	    crypto_algo crypto;

            switch(op)
            {
            case create:
		if(ref_filename != NULL && ref_root != NULL)
		{
		    crypto_split_algo_pass(pass_ref, crypto, tmp_pass);
		    arch = new archive(dialog, *ref_root, *ref_filename, EXTENSION, crypto, tmp_pass, crypto_size_ref,
				       input_pipe, output_pipe, execute_ref, info_details);
		}
		crypto_split_algo_pass(pass, crypto, tmp_pass);
                cur = new archive(dialog, *fs_root, *sauv_root, arch, *selection, *subtree, filename, EXTENSION,
				  allow_over, warn_over, info_details, pause, empty_dir,
				  algo, compression_level, file_size,
				  first_file_size, ea_root, ea_user,
				  execute, crypto, tmp_pass, crypto_size,
				  *compr_mask,
				  min_compr_size, nodump, ignore_owner, hourshift, empty,
				  alter_atime, same_fs, st);
                display_sauv_stat(dialog, st);
		if(arch != NULL) // making some room in memory
		{
		    delete arch;
		    arch = NULL;
		}
		if(st.errored > 0) // cur is not used for isolation
                    throw Edata(gettext("Some file could not be saved"));
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
				      info_details, detruire, only_more_recent, ea_root, ea_user,
				      flat, ignore_owner, warn_remove_no_match, hourshift, empty);
                display_rest_stat(dialog, st);
                if(st.errored > 0)
                    throw Edata(gettext("All files asked could not be restored"));
                break;
            case diff:
		crypto_split_algo_pass(pass, crypto, tmp_pass);
		arch = new archive(dialog, *sauv_root, filename, EXTENSION, crypto, tmp_pass, crypto_size,
				   input_pipe, output_pipe, execute, info_details);
                st = arch->op_diff(dialog, *fs_root, *selection, *subtree, info_details, ea_root, ea_user,
				   ignore_owner, alter_atime);
                display_diff_stat(dialog, st);
                if(st.errored > 0 || st.deleted > 0)
                    throw Edata(gettext("Some file comparisons failed"));
                break;
            case test:
		crypto_split_algo_pass(pass, crypto, tmp_pass);
		arch = new archive(dialog, *sauv_root, filename, EXTENSION, crypto, tmp_pass, crypto_size,
				   input_pipe, output_pipe, execute, info_details);
                st = arch->op_test(dialog, *selection, *subtree, info_details);
                display_test_stat(dialog, st);
                if(st.errored > 0)
                    throw Edata(gettext("Some files are corrupted in the archive and it will not be possible to restore them"));
                break;
            case listing:
		crypto_split_algo_pass(pass, crypto, tmp_pass);
                    // only_more_recent is used to carry --tar-format flag  while listing is asked
		arch = new archive(dialog, *sauv_root, filename, EXTENSION, crypto, tmp_pass, crypto_size,
				   input_pipe, output_pipe, execute, info_details);
                arch->op_listing(dialog, info_details, only_more_recent, *selection, alteration != "");
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

        MEM_OUT;

        return EXIT_OK;
    }
}

static void dummy_call(char *x)
{
    static char id[]="$Id: dar.cpp,v 1.28 2004/12/01 22:10:44 edrusb Rel $";
    dummy_call(id);
}


static void display_sauv_stat(user_interaction & dialog, const statistics & st)
{
    infinint total = st.total();

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i inode(s) saved\n"), &st.treated);
    dialog.printf(gettext(" with %i hard link(s) recorded\n"), &st.hard_links);
    dialog.printf(gettext(" %i inode(s) not saved (no file change)\n"), &st.skipped);
    dialog.printf(gettext(" %i inode(s) failed to save (filesystem error)\n"), &st.errored);
    dialog.printf(gettext(" %i files(s) ignored (excluded by filters)\n"), &st.ignored);
    dialog.printf(gettext(" %i files(s) recorded as deleted from reference backup\n"), &st.deleted);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of file considered: %i\n"), &total);
#ifdef EA_SUPPORT
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" EA saved for %i file(s)\n"), &st.ea_treated);
#endif
    dialog.printf(" --------------------------------------------\n");

}

static void display_rest_stat(user_interaction & dialog, const statistics & st)
{
    infinint total = st.total();

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i file(s) restored\n"), &st.treated);
    dialog.printf(gettext(" %i file(s) not restored (not saved in archive)\n"), &st.skipped);
    dialog.printf(gettext(" %i file(s) ignored (excluded by filters)\n"), &st.ignored);
    dialog.printf(gettext(" %i file(s) less recent than the one on filesystem\n"), &st.tooold);
    dialog.printf(gettext(" %i file(s) failed to restore (filesystem error)\n"), &st.errored);
    dialog.printf(gettext(" %i file(s) deleted\n"), &st.deleted);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of file considered: %i\n"), &total);
#ifdef EA_SUPPORT
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" EA restored for %i file(s)\n"), &st.ea_treated);
#endif
    dialog.printf(" --------------------------------------------\n");
}

static void display_diff_stat(user_interaction & dialog, const statistics &st)
{
    infinint total = st.total();

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i file(s) treated\n"), &st.treated);
    dialog.printf(gettext(" %i file(s) do not match those on filesystem\n"), &st.errored);
    dialog.printf(gettext(" %i file(s) ignored (excluded by filters)\n"), &st.ignored);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of file considered: %i\n"), &total);
    dialog.printf(" --------------------------------------------\n");
}

static void display_test_stat(user_interaction & dialog, const statistics & st)
{
    infinint total = st.total();

    dialog.printf("\n\n --------------------------------------------\n");
    dialog.printf(gettext(" %i file(s) treated\n"), &st.treated);
    dialog.printf(gettext(" %i file(s) with error\n"), &st.errored);
    dialog.printf(gettext(" %i file(s) ignored (excluded by filters)\n"), &st.skipped);
    dialog.printf(" --------------------------------------------\n");
    dialog.printf(gettext(" Total number of file considered: %i\n"), &total);
    dialog.printf(" --------------------------------------------\n");
}

const char *dar_version()
{
    return DAR_VERSION;
}
