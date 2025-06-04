/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif
} // end extern "C"

#include <iomanip>
#include <iostream>
#include <deque>
#include "tools.hpp"
#include "storage.hpp"
#include "nls_swap.hpp"
#include "database_header.hpp"
#include "i_archive.hpp"
#include "i_database.hpp"


using namespace libdar;
using namespace std;

static storage *file2storage(generic_file &f);
static void memory2file(storage &s, generic_file &f);

namespace libdar
{

    database::i_database::i_database(const shared_ptr<user_interaction> & dialog): mem_ui(dialog)
    {
	archive_data dat;

	dat.chemin = "";
	dat.basename = "";
	coordinate.clear();
	coordinate.push_back(dat); // coordinate[0] is never used, but must exist
	options_to_dar.clear();
	dar_path = "";
	files = new (nothrow) data_dir("."); // "." or whaterver else (this name is not used)
	if(files == nullptr)
	    throw Ememory("database::i_database::database");
	data_files = nullptr;
	check_order_asked = true;
	cur_db_version = database_header_get_supported_version();
	algo = compression::gzip;   // stays the default algorithm for new databases
	compr_level = 9; // stays the default compression level for new databases
    }

    database::i_database::i_database(const shared_ptr<user_interaction> & dialog, const string & base, const database_open_options & opt): mem_ui(dialog)
    {
	generic_file *f = database_header_open(dialog,
					       base,
					       cur_db_version,
					       algo,
					       compr_level);

	if(f == nullptr)
	    throw Ememory("database::i_database::database");
	try
	{
	    check_order_asked = opt.get_warn_order();
	    build(*f, opt.get_partial(), opt.get_partial_read_only(), cur_db_version);
	}
	catch(...)
	{
	    delete f;
	    throw;
	}
	delete f;
    }

    void database::i_database::build(generic_file & f,
			 bool partial,
			 bool read_only,
			 const unsigned char db_version)
    {
	NLS_SWAP_IN;
	try
	{
	    struct archive_data dat;

	    if(db_version > database_header_get_supported_version())
		throw SRC_BUG; // we should not get there if the database is more recent than what that software can handle. this is necessary if we do not want to destroy the database or loose data.
	    coordinate.clear();
	    infinint tmp = infinint(f); // number of archive to read
	    while(!tmp.is_zero())
	    {
		tools_read_string(f, dat.chemin);
		tools_read_string(f, dat.basename);
		if(db_version >= 3)
		    dat.root_last_mod.read(f, db2archive_version(db_version));
		else
		    dat.root_last_mod = datetime(0);
		coordinate.push_back(dat);
		--tmp;
	    }
	    if(coordinate.empty())
		throw Erange("database::i_database::database", gettext("Badly formatted database"));
	    tools_read_vector(f, options_to_dar);
	    tools_read_string(f, dar_path);
	    if(db_version < database_header_get_supported_version())
		partial = false;

	    if(!partial)
	    {
		files = data_dir::data_tree_read(f, db_version);
		if(files == nullptr)
		    throw Ememory("database::i_database::database");
		if(files->get_name() != ".")
		    files->set_name(".");
		data_files = nullptr;
	    }
	    else
	    {
		if(!read_only)
		{
		    files = nullptr;
		    data_files = file2storage(f);
		}
		else
		{
		    files = nullptr;
		    data_files = nullptr;
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

    database::i_database::~i_database()
    {
	if(files != nullptr)
	    delete files;
	if(data_files != nullptr)
	    delete data_files;
    }

    void database::i_database::dump(const std::string & filename,
				    const database_dump_options & opt) const
    {
	if(files == nullptr && data_files == nullptr)
	    throw Erange("database::i_database::dump", gettext("Cannot write down a read-only database"));

	generic_file *f = database_header_create(get_pointer(),
						 filename,
						 opt.get_overwrite(),
						 algo,
						 compr_level);
	if(f == nullptr)
	    throw Ememory("database::i_database::dump");

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
	    if(files != nullptr)
		files->dump(*f);
	    else
		if(data_files != nullptr)
		    memory2file(*data_files, *f);
		else
		    throw SRC_BUG;
	}
	catch(...)
	{
	    if(f != nullptr)
		delete f;
	    throw;
	}
	if(f != nullptr)
	    delete f;
    }

    void database::i_database::add_archive(const archive & arch, const string & chemin, const string & basename, const database_add_options & opt)
    {
	    // note: this methode could now leverage the use of
	    // list_entry provided by op_listing to a callback function, but
	    // it does not worth the effort as it brings no value to the code
	    // and would probably cost the introduction of some bugs
	    //
	    // so we need accessing the internal catalogue object of a given archive
	    // to keep this implementation as is

	NLS_SWAP_IN;
	try
	{
	    struct archive_data dat;
	    archive_num number = coordinate.size();

	    if(files == nullptr)
		throw SRC_BUG;

	    if(basename == "")
		throw Erange("database::i_database::add_archive", gettext("Empty string is an invalid archive basename"));

	    dat.chemin = chemin;
	    dat.basename = basename;
	    dat.root_last_mod = arch.pimpl->get_catalogue().get_root_dir_last_modif();
	    coordinate.push_back(dat);
	    files->data_tree_update_with(arch.pimpl->get_catalogue().get_contenu(), number);
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

    void database::i_database::remove_archive(archive_num min, archive_num max, const database_remove_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    min = get_real_archive_num(min, opt.get_revert_archive_numbering());
	    max = get_real_archive_num(max, opt.get_revert_archive_numbering());
	    if(min > max)
		throw Erange("database::i_database::remove_archive", gettext("Incorrect archive range in database"));
	    if(min == 0 || max >= coordinate.size())
		throw Erange("database::i_database::remove_archive", gettext("Incorrect archive range in database"));
	    for(U_I i = max ; i >= min ; --i)
	    {
		if(files == nullptr)
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

    void database::i_database::change_name(archive_num num, const string & basename, const database_change_basename_options &opt)
    {
	NLS_SWAP_IN;
	try
	{
	    num = get_real_archive_num(num, opt.get_revert_archive_numbering());
	    if(num < coordinate.size() && num != 0)
		coordinate[num].basename = basename;
	    else
		throw Erange("database::i_database::change_name", gettext("Non existent archive in database"));
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::i_database::set_path(archive_num num, const string & chemin, const database_change_path_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    num = get_real_archive_num(num, opt.get_revert_archive_numbering());
	    if(num < coordinate.size() && coordinate[num].basename != "")
		coordinate[num].chemin = chemin;
	    else
		throw Erange("database::i_database::change_name", gettext("Non existent archive in database"));
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::i_database::set_permutation(archive_num src, archive_num dst)
    {
	NLS_SWAP_IN;
	try
	{
	    struct archive_data moved;

	    if(files == nullptr)
		throw SRC_BUG;
	    if(src >= coordinate.size() || src <= 0)
		throw Erange("database::i_database::set_permutation", string(gettext("Invalid archive number: ")) + tools_int2str(src));
	    if(dst >= coordinate.size() || dst <= 0)
		throw Erange("database::i_database::set_permutation", string(gettext("Invalid archive number: ")) + tools_int2str(dst));

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
		if(dst+1 < (archive_num)coordinate.size())
		    re_finalize.insert(dst+1);
	    }
	    else // src >= dst
	    {
		if(src+1 < (archive_num)coordinate.size())
		    re_finalize.insert(src+1);
		re_finalize.insert(dst);
		if(dst+1 < (archive_num)coordinate.size())
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

    database_archives_list database::i_database::get_contents() const
    {
	database_archives_list ret;
	database_archives tmp;

	ret.push_back(tmp); // index 0 is not used

	for(archive_num i = 1; i < coordinate.size(); ++i)
	{
	    tmp.set_path(coordinate[i].chemin);
	    tmp.set_basename(coordinate[i].basename);
	    ret.push_back(tmp);
	}

	return ret;
    }

    void database::i_database::get_files(database_listing_show_files_callback callback,
			     void *context,
			     archive_num num,
			     const database_used_options & opt) const
    {
	NLS_SWAP_IN;
	try
	{
	    if(num != 0)
		num = get_real_archive_num(num, opt.get_revert_archive_numbering());
	    if(files == nullptr)
		throw SRC_BUG;

	    if(num < coordinate.size())
		files->show(callback, context, num);
	    else
		throw Erange("database::i_database::show_files", gettext("Non existent archive in database"));
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    void database::i_database::get_version(database_listing_get_version_callback callback,
			       void *context,
			       path chemin) const
    {
	NLS_SWAP_IN;
	try
	{
	    const data_tree *ptr = nullptr;
	    const data_dir *ptr_dir = files;
	    string tmp;

	    if(files == nullptr)
		throw SRC_BUG;

	    if(!chemin.is_relative())
		throw Erange("database::i_database::show_version", gettext("Invalid path, path must be relative"));

	    while(chemin.pop_front(tmp) && ptr_dir != nullptr)
	    {
		ptr = ptr_dir->read_child(tmp);
		if(ptr == nullptr)
		    throw Erange("database::i_database::show_version", gettext("Non existent file in database"));
		ptr_dir = dynamic_cast<const data_dir *>(ptr);
	    }

	    if(ptr_dir == nullptr)
		throw Erange("database::i_database::show_version", gettext("Non existent file in database"));

	    ptr = ptr_dir->read_child(chemin.display());
	    if(ptr == nullptr)
		throw Erange("database::i_database::show_version", gettext("Non existent file in database"));
	    else
		ptr->listing(callback, context);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::i_database::show_most_recent_stats(database_listing_statistics_callback callback,
					  void *context) const
    {
	NLS_SWAP_IN;
	try
	{
	    deque<infinint> stats_data(coordinate.size(), 0);
	    deque<infinint> stats_ea(coordinate.size(), 0);
	    deque<infinint> total_data(coordinate.size(), 0);
	    deque<infinint> total_ea(coordinate.size(), 0);

	    if(files == nullptr)
		throw SRC_BUG;

	    if(callback == nullptr)
		throw Erange("database::i_database::show_most_recent_stats", "nullptr provided as user callback function");

	    files->compute_most_recent_stats(stats_data, stats_ea, total_data, total_ea);
	    try
	    {
		for(archive_num i = 1; i < coordinate.size(); ++i)
		    callback(context, i, stats_data[i], total_data[i], stats_ea[i], total_ea[i]);
	    }
	    catch(...)
	    {
		throw Elibcall("database::i_database::show_most_recent_stats", "user provided callback should not throw any exception");
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    void database::i_database::restore(const vector<string> & filename,
			   const database_restore_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    map<archive_num, vector<string> > command_line;
	    deque<string> anneau;
	    const data_tree *ptr;

	    anneau.assign(filename.begin(), filename.end());
	    if(files == nullptr)
		throw SRC_BUG;

	    if(opt.get_info_details())
		get_ui().message(gettext("Checking chronological ordering of files between the archives..."));
	    check_order();

		// determination of the archives to restore and files to restore for each selected file
	    while(!anneau.empty())
	    {
		if(files == nullptr)
		    throw SRC_BUG;
		if(files->data_tree_find(anneau.front(), ptr))
		{
		    const data_dir *ptr_dir = dynamic_cast<const data_dir *>(ptr);
		    set<archive_num> num_data;
		    archive_num max_data = 0;
		    archive_num num_ea = 0;
		    db_lookup look_ea, look_data;

		    look_data = ptr->get_data(num_data, opt.get_date(), opt.get_even_when_removed());
		    look_ea = ptr->get_EA(num_ea, opt.get_date(), opt.get_even_when_removed());

		    switch(look_data)
		    {
		    case db_lookup::found_present:
			break;
		    case db_lookup::found_removed:
			num_data.clear(); // we do not restore any data
			if(opt.get_info_details())
			    get_ui().message(string(gettext("File recorded as removed at this date in database: ")) + anneau.front());
			break;
		    case db_lookup::not_found:
			num_data.clear();
			get_ui().message(string(gettext("File not found in database: ")) + anneau.front());
			break;
		    case db_lookup::not_restorable:
			num_data.clear();
			get_ui().message(string(gettext("File found in database but impossible to restore (only found \"unchanged\" in differential backups, or delta patch without reference to base it on in any previous archive of the base): ")) + anneau.front());
			break;
		    default:
			throw SRC_BUG;
		    }

		    switch(look_ea)
		    {
		    case db_lookup::found_present:
			num_ea = 0; // we cannot restore it
			break;
		    case db_lookup::found_removed:
			num_ea = 0; // we cannot restore it
			break;
		    case db_lookup::not_found:
			num_ea = 0; // we cannot restore it
			break;
		    case db_lookup::not_restorable:
			num_ea = 0; // we cannot restore it
			get_ui().message(string(gettext("Extended Attribute of file found in database but impossible to restore (only found \"unchanged\" in differential backups): ")) + anneau.front());
			break;
		    default:
			throw SRC_BUG;
		    }

			// if no data nor EA could be found
		    if(num_ea == 0 && num_data.empty())
		    {
			if(!opt.get_date().is_zero()) // a date was specified
			{
			    string fic = anneau.front();
			    if(opt.get_info_details())
				get_ui().printf(gettext("%S did not exist before specified date and cannot be restored"), &fic);
			}
		    }
		    else // there is something to restore for that file
		    {

			    // if latest EA are not to be found in a archive where
			    // data has also to be restored from, adding a specific command-line for EA
			if(num_ea != 0 && num_data.find(num_ea) != num_data.end())
			{
			    command_line[num_ea].push_back("-g");
			    command_line[num_ea].push_back(anneau.front());
			}

			    // adding command-line for archives where to find data (and possibly EA)
			for(set<archive_num>::iterator it = num_data.begin(); it != num_data.end(); ++it)
			{
			    command_line[*it].push_back("-g");
			    command_line[*it].push_back(anneau.front());
			    if(*it > max_data)
				max_data = *it;
			}

			    // warning if latest EA is not found with the same archive as the most recent Data
			if(max_data != 0 && num_ea != 0)
			    if(max_data > num_ea) // will restore "EA only" then "data + old EA"
			    {
				string fic = anneau.front();
				if(!opt.get_even_when_removed())
				    get_ui().printf(gettext("Either archives in database are not properly tidied, or file last modification date has been artificially set to an more ancient date. This may lead improper Extended Attribute restoration for inode %S"), &fic);
			    }
		    }

		    if(ptr_dir != nullptr)
		    {  // adding current directory children in the list of files
			vector<string> fils;
			vector<string>::iterator fit;
			path base = anneau.front();
			ptr_dir->read_all_children(fils);

			fit = fils.begin();
			while(fit != fils.end())
			    anneau.push_back((base.append(*(fit++))).display());
		    }
		}
		else
		{
		    string fic = anneau.front();
		    get_ui().printf(gettext("Cannot restore file %S : non existent file in database"), &fic);
		}
		anneau.pop_front();
	    }

		//freeing memory if early_release is set

	    if(opt.get_early_release())
	    {
		if(files != nullptr)
		{
		    delete files;
		    files = nullptr;
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

			get_ui().printf("CALLING DAR: restoring %d files from archive %S using anonymous pipe to transmit configuration to the dar process", ut->second.size()/2, &archive_name);
			if(opt.get_info_details())
			{
			    get_ui().printf("Arguments sent through anonymous pipe are:");
			    get_ui().message(tools_concat_vector(" ", argvpipe));
			}
			tools_system_with_pipe(get_pointer(), dar_cmd, argvpipe);
		    }
		    catch(Erange & e)
		    {
			get_ui().message(string(gettext("Error while restoring the following files: "))
					 + tools_concat_vector( " ", ut->second)
					 + "   : "
					 + e.get_message());
		    }
		    ut++;
		}
	    }
	    else
		get_ui().message(gettext("Cannot restore any file, nothing done"));
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::i_database::restore(const archive_options_read & read_options,
				       const path & fs_root,
				       const archive_options_extract & extract_options,
				       const database_restore_options & opt)
    {
	    // indentify the archive_num(s) to run extraction on

	    // for each archive_num

	    // - define a mask_database and modify the archive_options_extract to AND path with it

	    // - open the archive with the provided read options

	    // - extract the archive with the provided/modified extraction options

	    // - close the archive (delete the archive object)
    }


    archive_num database::i_database::get_real_archive_num(archive_num num, bool revert) const
    {
	if(num == 0)
	    throw Erange("database::i_database::get_real_archive_num", tools_printf(dar_gettext("Invalid archive number: %d"), num));

	if(revert)
	{
	    U_I size = coordinate.size(); // size is +1 because of record zero that is never used but must exist
	    if(size > num)
		return size - num;
	    else
		throw Erange("database::i_database::get_real_archive_num", tools_printf(dar_gettext("Invalid archive number: %d"), -num));
	}
	else
	    return num;
    }

    const datetime & database::i_database::get_root_last_mod(const archive_num & num) const
    {
	if(num >= coordinate.size())
	    throw SRC_BUG;

	return coordinate[num].root_last_mod;
    }

} // end of namespace

static storage *file2storage(generic_file &f)
{
    storage *st = new (nothrow) storage(0);
    const U_I taille = 102400;
    unsigned char buffer[taille];
    S_I lu;

    if(st == nullptr)
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
