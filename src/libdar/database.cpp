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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: database.cpp,v 1.6.2.1 2007/07/22 16:34:59 edrusb Rel $
//
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
	files = new data_dir("root"); // "root" or whaterver else
	if(files == NULL)
	    throw Ememory("database::database");
	data_files = NULL;

    }

    database::database(user_interaction & dialog, const string & base, bool partial)
    {
	generic_file *f = database_header_open(dialog, base);
	if(f == NULL)
	    throw Ememory("database::database");
	try
	{
	    build(*f, partial);
	}
	catch(...)
	{
	    delete f;
	    throw;
	}
	delete f;
    }

    void database::build(generic_file & f, bool partial)
    {
	struct archive_data dat;

	if(&f == NULL)
	    throw SRC_BUG;
	coordinate.clear();
	infinint tmp = infinint(f.get_gf_ui(), NULL, &f); // number of archive to read
	while(tmp > 0)
	{
	    tools_read_string(f, dat.chemin);
	    tools_read_string(f, dat.basename);
	    coordinate.push_back(dat);
	    --tmp;
	}
	if(coordinate.empty())
	    throw Erange("database::database", gettext("Badly formatted database"));
	tools_read_vector(f, options_to_dar);
	tools_read_string(f, dar_path);
	if(!partial)
	{
	    files = data_tree_read(f);
	    if(files == NULL)
		throw Ememory("database::database");
	    data_files = NULL;
	}
	else
	{
	    files = NULL;
	    data_files = file2storage(f);
	}
    }

    database::~database()
    {
	if(files != NULL)
	    delete files;
	if(data_files != NULL)
	    delete data_files;
    }

    void database::dump(user_interaction & dialog, const std::string & filename, bool overwrite) const
    {
	generic_file *f = database_header_create(dialog, filename, overwrite);
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

    void database::add_archive(const archive & arch, const string & chemin, const string & basename)
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
	coordinate.push_back(dat);
	data_tree_update_with(arch.get_catalogue().get_contenu(), number, files);
    }

    void database::remove_archive(archive_num min, archive_num max)
    {
	if(min > max)
	    throw Erange("database::remove_archive", gettext("Incorrect archive range in database"));
	if(min == 0 || max >= coordinate.size())
	    throw Erange("database::remove_archive", gettext("Incorrect archive range in database"));
	for(U_I i = max ; i >= min ; --i)
	{
	    coordinate.erase(coordinate.begin() + i);
	    if(files == NULL)
		throw SRC_BUG;
	    files->remove_all_from(i);
	    files->skip_out(i);
	}
    }

    void database::change_name(archive_num num, const string & basename)
    {
	if(num < coordinate.size() && num != 0)
	    coordinate[num].basename = basename;
	else
	    throw Erange("database::change_name", gettext("Non existent archive in database"));
    }

    void database::set_path(archive_num num, const string & chemin)
    {
	if(num < coordinate.size() && coordinate[num].basename != "")
	    coordinate[num].chemin = chemin;
	else
	    throw Erange("database::change_name", gettext("Non existent archive in database"));
    }

    void database::set_permutation(archive_num src, archive_num dst)
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
    }

    static void dummy_call(char *x)
    {
	static char id[]="$Id: database.cpp,v 1.6.2.1 2007/07/22 16:34:59 edrusb Rel $";
	dummy_call(id);
    }

    void database::show_contents(user_interaction & dialog) const
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

    void database::show_files(user_interaction & dialog, archive_num num) const
    {
	if(files == NULL)
	    throw SRC_BUG;
	if(num < coordinate.size())
	{
	    if(files == NULL)
		throw SRC_BUG;

	    files->show(dialog, num);
	}
	else
	    throw Erange("database::show_files", gettext("Non existent archive in database"));
    }


    void database::show_version(user_interaction & dialog, path chemin) const
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

    void database::show_most_recent_stats(user_interaction & dialog) const
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
	    dialog.printf("--------------+-------------------------+-----------------------\n");
	}
	for(archive_num i = 1; i < coordinate.size(); ++i)
	    if(dialog.get_use_dar_manager_statistics())
		dialog.dar_manager_statistics(i, stats_data[i], total_data[i], stats_ea[i], total_ea[i]);
	    else
		dialog.printf("\t%u %i/%i \t\t\t %i/%i\n", i, &stats_data[i], &total_data[i], &stats_ea[i], &total_ea[i]);
    }


    void database::restore(user_interaction & dialog,
			   const vector<string> & filename,
			   bool early_release,
			   const vector<string> & extra_options_for_dar,
			   const infinint & date)
    {
	map<archive_num, vector<string> > command_line;
	deque<string> anneau;
	const data_tree *ptr;

	anneau.assign(filename.begin(), filename.end());
	if(files == NULL)
	    throw SRC_BUG;

	    // determination of the archive to restore and files to restore for each selected archive
	while(!anneau.empty())
	{
	    if(data_tree_find(anneau.front(), *files, ptr))
	    {
		const data_dir *ptr_dir = dynamic_cast<const data_dir *>(ptr);
		archive_num num_data = 0;
		archive_num num_ea = 0;

		ptr->get_data(num_data, date);
		ptr->get_EA(num_ea, date);

		if(num_data == num_ea)
		{
		    if(num_data != 0)
		    {
			command_line[num_data].push_back("-g");
			command_line[num_data].push_back(anneau.front());
		    }
		}
		else
		{
		    if(num_data != 0)
		    {
			command_line[num_data].push_back("-g");
			command_line[num_data].push_back(anneau.front());
		    }
		    if(num_ea != 0)
		    {
			command_line[num_data].push_back("-g");
			command_line[num_ea].push_back(anneau.front());
		    }
		    if(num_data != 0 && num_ea != 0)
			if(num_data > num_ea) // will restore "EA only" then "data + old EA"
			{
			    string fic = anneau.front();
			    dialog.printf(gettext("Concerning file %S  : archive #%d contains the most recent data and some old EA while archive #%d contains the most recent EA only."), &fic, num_data, num_ea);
			    dialog.printf(gettext("Dar manager will always restore files from the archives in the order they have been added in database, thus, for this file, last EA version, will be overwritten by the older version associated with saved data."));
			    dialog.printf(gettext("To avoid such type of problem, reorder the archive in the database to have the most recent ones having the higher index number"));
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

	if(early_release)
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
		    vector<string> argvector = argvector_init;

		    if(coordinate[ut->first].chemin != "")
			archive_name = coordinate[ut->first].chemin + "/";
		    else
			archive_name = "";
		    archive_name += coordinate[ut->first].basename;
		    argvector.push_back("-x");
		    argvector.push_back(archive_name);
		    argvector += options_to_dar;
		    argvector += extra_options_for_dar;
		    argvector += ut->second;

		    cout << gettext("CALLING DAR: ") << tools_concat_vector(" ", argvector) << endl;
		    tools_system(dialog, argvector);
		}
		catch(Erange & e)
		{
		    dialog.warning(string(gettext("Error while restoring the following files : "))
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
