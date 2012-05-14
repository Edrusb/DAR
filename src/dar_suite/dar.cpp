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
// $Id: dar.cpp,v 1.9 2003/10/18 14:43:07 edrusb Rel $
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

static void display_sauv_stat(const statistics & st);
static void display_rest_stat(const statistics & st);
static void display_diff_stat(const statistics &st);
static void display_test_stat(const statistics & st);
static S_I little_main(S_I argc, char *argv[], const char **env);

int main(S_I argc, char *argv[], const char **env)
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
    infinint hourshift;

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
                  flat, min_compr_size, nodump,
                  hourshift))
        return EXIT_SYNTAX;
    else
    {
        MEM_IN;

        shell_interaction_set_beep(beep);

        if(filename != "-"  || (output_pipe != "" && op != create && op != isolate))
            shell_interaction_change_non_interactive_output(&cout);
            // standart output can be used to send non interactive
            // messages
        
        try
        {
            statistics st;

            switch(op)
            {
            case create:
                st = op_create(*fs_root, *sauv_root, ref_root, *selection, *subtree, filename, EXTENSION,
                               ref_filename, allow_over, warn_over, info_details, pause, empty_dir,
                               algo, compression_level, file_size,
                               first_file_size, ea_root, ea_user, input_pipe, output_pipe,
                               execute, execute_ref, pass, pass_ref, *compr_mask,
                               min_compr_size, nodump, ignore_owner, hourshift);
                display_sauv_stat(st);
                if(st.errored > 0) // st is not used for isolation
                    throw Edata("some file could not be saved");
                break;
            case isolate:
                op_isolate(*sauv_root, ref_root, filename, EXTENSION, ref_filename, allow_over, warn_over, info_details,
                           pause, algo, compression_level, file_size, first_file_size, input_pipe, output_pipe,
                           execute, execute_ref, pass, pass_ref);
                break;
            case extract:
                st = op_extract(*fs_root, *sauv_root, *selection, *subtree, filename, EXTENSION, allow_over, warn_over,
                                info_details, detruire, only_more_recent, ea_root, ea_user, input_pipe, output_pipe,
                                execute, pass, flat, ignore_owner, hourshift);
                display_rest_stat(st);
                if(st.errored > 0)
                    throw Edata("all file asked could not be restored");        
                break;
            case diff:
                st = op_diff(*fs_root, *sauv_root, *selection, *subtree, filename, EXTENSION, info_details, ea_root, ea_user,
                             input_pipe, output_pipe, execute, pass, ignore_owner);
                display_diff_stat(st);
                if(st.errored > 0 || st.deleted > 0)
                    throw Edata("some file comparisons failed");
                break;
            case test:
                st = op_test(*sauv_root, *selection, *subtree, filename, EXTENSION, info_details, input_pipe, output_pipe,
                             execute, pass);
                display_test_stat(st);
                if(st.errored > 0)
                    throw Edata("some files are corrupted in the archive and it will not be possible to restore them");
                break;
            case listing:
                    // only_more_recent is used to carry --tar-format flag  while listing is asked
                op_listing(*sauv_root, filename, EXTENSION, info_details, only_more_recent, input_pipe, output_pipe,
                           execute, pass, *selection);
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

static void dummy_call(char *x)
{
    static char id[]="$Id: dar.cpp,v 1.9 2003/10/18 14:43:07 edrusb Rel $";
    dummy_call(id);
}


static void display_sauv_stat(const statistics & st)
{
    infinint total = st.total();

    ui_printf("\n\n --------------------------------------------\n");
    ui_printf(" %i inode(s) saved\n", &st.treated);
    ui_printf(" with %i hard link(s) recorded\n", &st.hard_links);
    ui_printf(" %i inode(s) not saved (no file change)\n", &st.skipped);
    ui_printf(" %i inode(s) failed to save (filesystem error)\n", &st.errored);
    ui_printf(" %i files(s) ignored (excluded by filters)\n", &st.ignored);
    ui_printf(" %i files(s) recorded as deleted from reference backup\n", &st.deleted);
    ui_printf(" --------------------------------------------\n");
    ui_printf(" --------------------------------------------\n");
    ui_printf(" Total number of file considered: %i\n", &total);
#ifdef EA_SUPPORT
    ui_printf(" --------------------------------------------\n");
    ui_printf(" EA saved for %i file(s)\n", &st.ea_treated);
    ui_printf(" --------------------------------------------\n");
#endif
}

static void display_rest_stat(const statistics & st)
{
    infinint total = st.total();
    
    ui_printf("\n\n --------------------------------------------\n");
    ui_printf(" %i file(s) restored\n", &st.treated);
    ui_printf(" %i file(s) not restored (not saved in archive)\n", &st.skipped);
    ui_printf(" %i file(s) ignored (excluded by filters)\n", &st.ignored);
    ui_printf(" %i file(s) less recent than the one on filesystem\n", &st.tooold);
    ui_printf(" %i file(s) failed to restore (filesystem error)\n", &st.errored);
    ui_printf(" %i file(s) deleted\n", &st.deleted);
    ui_printf(" --------------------------------------------\n");
    ui_printf(" Total number of file considered: %i\n", &total);
#ifdef EA_SUPPORT
    ui_printf(" --------------------------------------------\n");
    ui_printf(" EA restored for %i file(s)\n", &st.ea_treated);
    ui_printf(" --------------------------------------------\n");
#endif
}

static void display_diff_stat(const statistics &st)
{
    infinint total = st.total();

    ui_printf("\n %i file(s) do not match those on filesystem\n", &st.errored);
    ui_printf(" %i file(s) failed to be read\n", &st.deleted);
    ui_printf(" %i file(s) ignored (excluded by filters)\n", &st.ignored);
    ui_printf(" --------------------------------------------\n");
    ui_printf(" Total number of file considered: %i\n", &total);
}

static void display_test_stat(const statistics & st)
{
    infinint total = st.total();

    ui_printf("\n %i file(s) with error\n", &st.errored);
    ui_printf(" %i file(s) ignored (excluded by filters)\n", &st.ignored);
    ui_printf(" --------------------------------------------\n");
    ui_printf(" Total number of file considered: %i\n", &total);
}

const char *dar_version()
{
    return DAR_VERSION;
}
