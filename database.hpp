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
// $Id: database.hpp,v 1.2 2003/02/11 22:01:33 edrusb Rel $
//
/*********************************************************************/


#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "catalogue.hpp"
#include "generic_file.hpp"
#include "data_tree.hpp"
#include <list>


class database
{
public:
    database(); // build an empty database
    database(generic_file & f); // build a database from a file
    ~database();

    void dump(generic_file & f) const; // write a database to a file

	// SETTINGS
	//
    void add_archive(const catalogue & cat, const string & chemin, const string & basename);
    void remove_archive(archive_num num);
    void change_name(archive_num num, const string & basename);
    void set_path(archive_num num, const string & chemin);
    void set_options(const vector<string> &opt) { options_to_dar = opt; };
    void set_dar_path(const string & chemin) { dar_path = chemin; };
    void set_permutation(archive_num src, archive_num dst);

	// "GETTINGS"
	//
    void show_contents() const; // displays all archive information
    vector<string> get_options() const { return options_to_dar; }; // show option passed to dar
    string get_dar_path() const { return dar_path; }; // show path to dar command
    void show_files(archive_num num) const; // 0 displays all files else files related to archive number
    void show_version(path chemin) const; // list archive where the given file is saved
    void show_most_recent_stats() const; // display the most recent version statistics (number of files by archive)

	// ACTION
	//
    void restore(const vector<string> & filename);

private:
    struct archive_data 
    {
	string chemin;
	string basename;
    };

    vector<struct archive_data> coordinate;
    vector<string> options_to_dar;
    string dar_path;
    data_dir *files;
};

#endif
