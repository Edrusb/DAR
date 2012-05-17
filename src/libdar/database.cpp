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

#include "../my_config.h"

extern "C"
{
#if STDC_HEADERS
#include <stdlib.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif
} // end extern "C"

#include <iomanip>
#include <iostream>
#include <deque>
#include "database.hpp"
#include "user_interaction.hpp"
#include "deci.hpp"
#include "tools.hpp"
#include "storage.hpp"
#include "database_header.hpp"

using namespace libdar;
using namespace std;

static storage *file2storage(generic_file &f);
static void memory2file(storage &s, generic_file &f);

namespace libdar
{

    database::database()
    {
	archive_data dat;

	dat.chemin = "";
	dat.basename = "";
	coordinate.clear();
	coordinate.push_back(dat); // coordinate[0] is never used, but must exist
	options_to_dar.clear();
	dar_path = "";
	files = new data_dir("."); // "." or whaterver else (this name is not used)
	if(files == NULL)
	    throw Ememory("database::database");
	data_files = NULL;
	check_order_asked = true;
    }

    database::database(user_interaction & dialog, const string & base, const database_open_options & opt)
    {
	unsigned char db_version;
	generic_file *f = database_header_open(dialog, base, db_version);

	if(f == NULL)
	    throw Ememory("database::database");
	try
	{
	    check_order_asked = opt.get_warn_order();
	    build(dialog, *f, opt.get_partial(), db_version);
	}
	catch(...)
	{
	    delete f;
	    throw;
	}
	delete f;
    }

    void database::build(user_interaction & dialog, generic_file & f, bool partial, const unsigned char db_version)
    {
	NLS_SWAP_IN;
	try
	{
	    struct archive_data dat;

	    if(db_version > database_header_get_supported_version())
		throw SRC_BUG; // we should not get there if the database is more recent than what that software can handle. this is necessary if we do not want to destroy the database or loose data.
	    if(&f == NULL)
		throw SRC_BUG;
	    coordinate.clear();
	    infinint tmp = infinint(f); // number of archive to read
	    while(tmp > 0)
	    {
		tools_read_string(f, dat.chemin);
		tools_read_string(f, dat.basename);
		if(db_version >= 3)
		    dat.root_last_mod.read(f);
		else
		    dat.root_last_mod = 0;
		coordinate.push_back(dat);
		--tmp;
	    }
	    if(coordinate.empty())
		throw Erange("database::database", gettext("Badly formatted database"));
	    tools_read_vector(f, options_to_dar);
	    tools_read_string(f, dar_path);
	    if(db_version < database_header_get_supported_version())
		partial = false;

	    if(!partial)
	    {
		files = data_tree_read(f, db_version);
		if(files == NULL)
		    throw Ememory("database::database");
		if(files->get_name() != ".")
		    files->set_name(".");
		data_files = NULL;
	    }
	    else
	    {
		files = NULL;
		data_files = file2storage(f);
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    database::~database()
    {
	if(files != NULL)
	    delete files;
	if(data_files != NULL)
	    delete data_files;
    }

    void database::dump(user_interaction & dialog, const std::string & filename, const database_dump_options & opt) const
    {
	generic_file *f = database_header_create(dialog, filename, opt.get_overwrite());
	if(f == NULL)
	    throw Ememory("database::dump");

	try
	{
	    archive_num tmp = coordinate.size();

 	    infinint(tmp).dump(*f);
	    for(archive_num i = 0; i < tmp; ++i)
	    {
		tools_write_string(*f, coordinate[i].chemin);
		tools_write_string(*f, coordinate[i].basename);
		coordinate[i].root_last_mod.dump(*f);
	    }
	    tools_write_vector(*f, options_to_dar);
	    tools_write_string(*f, dar_path);
	    if(files != NULL)
		files->dump(*f);
	    else
		if(data_files != NULL)
		    memory2file(*data_files, *f);
		else
		    throw SRC_BUG;
	}
	catch(...)
	{
	    if(f != NULL)
		delete f;
	    throw;
	}
	if(f != NULL)
	    delete f;
    }

    void database::add_archive(const archive & arch, const string & chemin, const string & basename, const database_add_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    struct archive_data dat;
	    archive_num number = coordinate.size();

	    if(files == NULL)
		throw SRC_BUG;
	    if(basename == "")
		throw Erange("database::add_archive", gettext("Empty string is an invalid archive basename"));
	    if(number >= ARCHIVE_NUM_MAX)
		throw Erange("database::add_archive", gettext("Cannot add another archive, database is full"));

	    dat.chemin = chemin;
	    dat.basename = basename;
	    dat.root_last_mod = arch.get_catalogue().get_root_dir_last_modif();
	    coordinate.push_back(dat);
	    data_tree_update_with(arch.get_catalogue().get_contenu(), number, files);
	    if(number > 1)
		files->finalize_except_self(number, get_root_last_mod(number), 0);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::remove_archive(archive_num min, archive_num max, const database_remove_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    min = get_real_archive_num(min, opt.get_revert_archive_numbering());
	    max = get_real_archive_num(max, opt.get_revert_archive_numbering());
	    if(min > max)
		throw Erange("database::remove_archive", gettext("Incorrect archive range in database"));
	    if(min == 0 || max >= coordinate.size())
		throw Erange("database::remove_archive", gettext("Incorrect archive range in database"));
	    for(U_I i = max ; i >= min ; --i)
	    {
		if(files == NULL)
		    throw SRC_BUG;
		files->remove_all_from(i, coordinate.size() - 1);
		files->skip_out(i);
		coordinate.erase(coordinate.begin() + i);
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::change_name(archive_num num, const string & basename, const database_change_basename_options &opt)
    {
	NLS_SWAP_IN;
	try
	{
	    num = get_real_archive_num(num, opt.get_revert_archive_numbering());
	    if(num < coordinate.size() && num != 0)
		coordinate[num].basename = basename;
	    else
		throw Erange("database::change_name", gettext("Non existent archive in database"));
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::set_path(archive_num num, const string & chemin, const database_change_path_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    num = get_real_archive_num(num, opt.get_revert_archive_numbering());
	    if(num < coordinate.size() && coordinate[num].basename != "")
		coordinate[num].chemin = chemin;
	    else
		throw Erange("database::change_name", gettext("Non existent archive in database"));
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::set_permutation(archive_num src, archive_num dst)
    {
	NLS_SWAP_IN;
	try
	{
	    struct archive_data moved;

	    if(files == NULL)
		throw SRC_BUG;
	    if(src >= coordinate.size() || src <= 0)
		throw Erange("database::set_permutation", string(gettext("Invalid archive number: ")) + tools_int2str(src));
	    if(dst >= coordinate.size() || dst <= 0)
		throw Erange("database::set_permutation", string(gettext("Invalid archive number: ")) + tools_int2str(dst));

	    moved = coordinate[src];
	    coordinate.erase(coordinate.begin()+src);
	    coordinate.insert(coordinate.begin()+dst, moved);
	    files->apply_permutation(src, dst);

		// update et_absent dates

	    set<archive_num> re_finalize;
	    set<archive_num>::iterator re_it;

	    if(src < dst)
	    {
		re_finalize.insert(src);
		re_finalize.insert(dst);
		re_finalize.insert(dst+1);
	    }
	    else // src >= dst
	    {
		re_finalize.insert(src+1);
		re_finalize.insert(dst);
		re_finalize.insert(dst+1);

		    // if src == dst the set still contains on entry (src or dst).
		    // this is intended to let the user have the possibility
		    // to ask dates recompilation, even if in theory this is useless
	    }

	    re_it = re_finalize.begin();
	    while(re_it != re_finalize.end())
	    {
		files->finalize_except_self(*re_it, get_root_last_mod(*re_it), *re_it + 1);
		++re_it;
	    }

	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void database::show_contents(user_interaction & dialog) const
    {
	NLS_SWAP_IN;
	try
	{
	    string opt = tools_concat_vector(" ", options_to_dar);

	    if(!dialog.get_use_dar_manager_contents())
	    {
		dialog.printf(gettext("\ndar path    : %S\n"), &dar_path);
		dialog.printf(gettext("dar options : %S\n\n"), &opt);

		dialog.printf(gettext("archive #   |    path      |    basename\n"));
		dialog.printf("------------+--------------+---------------\n");
	    }

	    for(archive_num i = 1; i < coordinate.size(); ++i)
	    {
		if(dialog.get_use_dar_manager_contents())
		    dialog.dar_manager_contents(i, coordinate[i].chemin, coordinate[i].basename);
		else
		{
		    opt = (coordinate[i].chemin == "") ? gettext("<empty>") : coordinate[i].chemin;
		    dialog.printf(" \t%u\t%S\t%S\n", i, &opt, &coordinate[i].basename);
		}
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::show_files(user_interaction & dialog, archive_num num, const database_used_options & opt) const
    {
	NLS_SWAP_IN;
	try
	{
	    if(num != 0)
		num = get_real_archive_num(num, opt.get_revert_archive_numbering());
	    if(files == NULL)
		throw SRC_BUG;

	    if(num < coordinate.size())
		files->show(dialog, num);
	    else
		throw Erange("database::show_files", gettext("Non existent archive in database"));
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    void database::show_version(user_interaction & dialog, path chemin) const
    {
	NLS_SWAP_IN;
	try
	{
	    const data_tree *ptr = NULL;
	    const data_dir *ptr_dir = files;
	    string tmp;

	    if(files == NULL)
		throw SRC_BUG;

	    if(!chemin.is_relative())
		throw Erange("database::show_version", gettext("Invalid path, path must be relative"));

	    while(chemin.pop_front(tmp) && ptr_dir != NULL)
	    {
		ptr = ptr_dir->read_child(tmp);
		if(ptr == NULL)
		    throw Erange("database::show_version", gettext("Non existent file in database"));
		ptr_dir = dynamic_cast<const data_dir *>(ptr);
	    }

	    if(ptr_dir == NULL)
		throw Erange("database::show_version", gettext("Non existent file in database"));

	    ptr = ptr_dir->read_child(chemin.display());
	    if(ptr == NULL)
		throw Erange("database::show_version", gettext("Non existent file in database"));
	    else
		ptr->listing(dialog);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::show_most_recent_stats(user_interaction & dialog) const
    {
	NLS_SWAP_IN;
	try
	{
	    vector<infinint> stats_data(coordinate.size(), 0);
	    vector<infinint> stats_ea(coordinate.size(), 0);
	    vector<infinint> total_data(coordinate.size(), 0);
	    vector<infinint> total_ea(coordinate.size(), 0);
	    if(files == NULL)
		throw SRC_BUG;
	    files->compute_most_recent_stats(stats_data, stats_ea, total_data, total_ea);

	    if(!dialog.get_use_dar_manager_statistics())
	    {
		dialog.printf(gettext("  archive #   |  most recent/total data |  most recent/total EA\n"));
		dialog.printf(gettext("--------------+-------------------------+-----------------------\n")); // having it with gettext let the translater adjust columns width
	    }
	    for(archive_num i = 1; i < coordinate.size(); ++i)
		if(dialog.get_use_dar_manager_statistics())
		    dialog.dar_manager_statistics(i, stats_data[i], total_data[i], stats_ea[i], total_ea[i]);
		else
		    dialog.printf("\t%u %i/%i \t\t\t %i/%i\n", i, &stats_data[i], &total_data[i], &stats_ea[i], &total_ea[i]);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    void database::restore(user_interaction & dialog,
			   const vector<string> & filename,
			   const database_restore_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    map<archive_num, vector<string> > command_line;
	    deque<string> anneau;
	    const data_tree *ptr;

	    anneau.assign(filename.begin(), filename.end());
	    if(files == NULL)
		throw SRC_BUG;

	    if(opt.get_info_details())
		dialog.warning(gettext("Checking chronological ordering of files between the archives..."));
	    check_order(dialog);

		// determination of the archive to restore and files to restore for each selected archive
	    while(!anneau.empty())
	    {
		if(data_tree_find(anneau.front(), *files, ptr))
		{
		    const data_dir *ptr_dir = dynamic_cast<const data_dir *>(ptr);
		    archive_num num_data = 0;
		    archive_num num_ea = 0;
		    data_tree::lookup look_ea, look_data;

		    look_data = ptr->get_data(num_data, opt.get_date(), opt.get_even_when_removed());
		    look_ea = ptr->get_EA(num_ea, opt.get_date(), opt.get_even_when_removed());

		    switch(look_data)
		    {
		    case data_tree::found_present:
			break;
		    case data_tree::found_removed:
			num_data = 0; // we do not restore it
			if(opt.get_info_details())
			    dialog.warning(string(gettext("File recorded as removed at this date in database: ")) + anneau.front());
			break;
		    case data_tree::not_found:
			num_data = 0;
			dialog.warning(string(gettext("File not found in database: ")) + anneau.front());
			break;
		    case data_tree::not_restorable:
			num_data = 0;
			dialog.warning(string(gettext("File found in database but impossible to restore (only found \"unchanged\" in differential backups)")) + anneau.front());
			break;
		    default:
			throw SRC_BUG;
		    }

		    switch(look_ea)
		    {
		    case data_tree::found_present:
			if(opt.get_even_when_removed()
			   && look_data == data_tree::found_present
			   && num_data > num_ea)
			    num_ea = num_data;
			break;
		    case data_tree::found_removed:
			num_ea = 0; // we do not restore it
			break;
		    case data_tree::not_found:
			num_ea = 0;
			break;
		    case data_tree::not_restorable:
			num_ea = 0;
			dialog.warning(string(gettext("Extended Attribute of file found in database but impossible to restore (only found \"unchanged\" in differential backups)")) + anneau.front());
			break;
		    default:
			throw SRC_BUG;
		    }

			// if there is something to restore for that file

		    if(look_ea == data_tree::found_present || look_data == data_tree::found_present)
		    {
			if(num_data == num_ea) // both EA and data are located in the same archive
			{
			    if(num_data != 0) // archive is not zero (so it is a real archive)
			    {
				command_line[num_data].push_back("-g");
				command_line[num_data].push_back(anneau.front());
			    }
			    else // archive number is zero (not a valid archive number)
				if(opt.get_date() != 0) // a date was specified
				{
				    string fic = anneau.front();
				    if(opt.get_info_details())
					dialog.printf(gettext("%S did not exist before specified date and cannot be restored"), &fic);
				}
				else
				    throw SRC_BUG; // no date limitation, the file's data should have been found
			}
			else // num_data != num_ea
			{
			    if(num_data != 0)
			    {
				command_line[num_data].push_back("-g");
				command_line[num_data].push_back(anneau.front());
			    }
			    if(num_ea != 0)
			    {
				command_line[num_ea].push_back("-g");
				command_line[num_ea].push_back(anneau.front());
			    }
			    if(num_data != 0 && num_ea != 0)
				if(num_data > num_ea) // will restore "EA only" then "data + old EA"
				{
				    string fic = anneau.front();
				    if(!opt.get_even_when_removed())
					dialog.printf(gettext("Either archives in database are not properly tidied, or file last modification date has been artificially set to an more ancient date. This may lead improper Extended Attribute restoration for inode %S"), &fic);
				}
			}
		    }

		    if(ptr_dir != NULL)
		    {  // adding current directory children in the list of files
			vector<string> fils;
			vector<string>::iterator fit;
			path base = anneau.front();
			ptr_dir->read_all_children(fils);

			fit = fils.begin();
			while(fit != fils.end())
			    anneau.push_back((base + *(fit++)).display());
		    }
		}
		else
		{
		    string fic = anneau.front();
		    dialog.printf(gettext("Cannot restore file %S : non existent file in database"), &fic);
		}
		anneau.pop_front();
	    }

		//freeing memory if early_release is set

	    if(opt.get_early_release())
	    {
		if(files != NULL)
		{
		    delete files;
		    files = NULL;
		}
	    }

		// calling dar for each archive

	    if(!command_line.empty())
	    {
		string dar_cmd = dar_path != "" ? dar_path : "dar";
		map<archive_num, vector<string> >::iterator ut = command_line.begin();
		vector<string> argvector_init = vector<string>(1, dar_cmd);

		while(ut != command_line.end())
		{
		    try
		    {
			string archive_name;
			vector<string> argvpipe;

			    // building the argument list sent through anonymous pipe

			if(coordinate[ut->first].chemin != "")
			    archive_name = coordinate[ut->first].chemin + "/";
			else
			    archive_name = "";
			archive_name += coordinate[ut->first].basename;
			argvpipe.push_back(dar_cmd); // just to fill the argv[0] by the command even when transmitted through anonymous pipe
			argvpipe.push_back("-x");
			argvpipe.push_back(archive_name);
			if(!opt.get_ignore_dar_options_in_database())
			    argvpipe += options_to_dar;
			argvpipe += opt.get_extra_options_for_dar();
			argvpipe += ut->second;

			dialog.printf("CALLING DAR: restoring %d files from archive %S using anonymous pipe to transmit configuration to the dar process", ut->second.size()/2, &archive_name);
			if(opt.get_info_details())
			{
			    dialog.printf("Arguments sent through anonymous pipe are:");
			    dialog.warning(tools_concat_vector(" ", argvpipe));
			}
			tools_system_with_pipe(dialog, dar_cmd, argvpipe);
		    }
		    catch(Erange & e)
		    {
			dialog.warning(string(gettext("Error while restoring the following files: "))
				       + tools_concat_vector( " ", ut->second)
				       + "   : "
				       + e.get_message());
		    }
		    ut++;
		}
	    }
	    else
		dialog.warning(gettext("Cannot restore any file, nothing done"));
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    archive_num database::get_real_archive_num(archive_num num, bool revert) const
    {
	if(num == 0)
	    throw Erange("database::get_real_archive_num", tools_printf(dar_gettext("Invalid archive number: %d"), num));

	if(revert)
	{
	    U_I size = coordinate.size(); // size is +1 because of record zero that is never used but must exist
	    if(size > num)
		return size - num;
	    else
		throw Erange("database::get_real_archive_num", tools_printf(dar_gettext("Invalid archive number: %d"), -num));
	}
	else
	    return num;
    }

    const infinint & database::get_root_last_mod(const archive_num & num) const
    {
	if(num >= coordinate.size())
	    throw SRC_BUG;

	return coordinate[num].root_last_mod;
    }

} // end of namespace

static storage *file2storage(generic_file &f)
{
    storage *st = new storage(0);
    const U_I taille = 102400;
    unsigned char buffer[taille];
    S_I lu;

    if(st == NULL)
        throw Ememory("dar_manager:file2storage");

    do
    {
        lu = f.read((char *)buffer, taille);
        if(lu > 0)
            st->insert_bytes_at_iterator(st->end(), buffer, lu);
    }
    while(lu > 0);

    return st;
}

static void memory2file(storage &s, generic_file &f)
{
    s.dump(f);
}
