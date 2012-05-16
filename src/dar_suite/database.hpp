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
// $Id: database.hpp,v 1.11 2004/06/30 15:09:44 edrusb Rel $
//
/*********************************************************************/


#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "../my_config.h"

#include <list>

#include "catalogue.hpp"
#include "generic_file.hpp"
#include "data_tree.hpp"
#include "storage.hpp"

using namespace libdar;

class database
{
public:
    database(); // build an empty database
    database(generic_file & f, bool partial) { build(f, partial); };
        // not all methods available if partially built from file (see bellow)
    ~database();

    void dump(generic_file & f) const; // write the database to a file

        // SETTINGS
        //
        // theses 3 following methods are not available with partially extracted database
    void add_archive(const catalogue & cat, const string & chemin, const string & basename);
    void remove_archive(archive_num min, archive_num max);
    void set_permutation(archive_num src, archive_num dst);
        //
    void change_name(archive_num num, const string & basename);
    void set_path(archive_num num, const string & chemin);
    void set_options(const vector<string> &opt) { options_to_dar = opt; };
    void set_dar_path(const string & chemin) { dar_path = chemin; };


        // "GETTINGS"
        //
    void show_contents(user_interaction & dialog) const; // displays all archive information
    vector<string> get_options() const { return options_to_dar; }; // show option passed to dar
    string get_dar_path() const { return dar_path; }; // show path to dar command
        //
        // theses 3 following are not available with partially extracted databases
        //
    void show_files(user_interaction & dialog, archive_num num) const; // 0 displays all files else files related to archive number
    void show_version(user_interaction & dialog, path chemin) const; // list archive where the given file is saved
    void show_most_recent_stats(user_interaction & dialog) const; // display the most recent version statistics (number of files by archive)

        // ACTION (not available with partially extracted databases)
        //
    void restore(user_interaction & dialog, const vector<string> & filename, bool early_release);
        // if early_release is true, free quite all memory allocated by the database before calling dar
        // withdraw, is that no more action is possible after this call (except destruction)

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
    storage *data_files;

    void build(generic_file & f, bool partial);
};

#endif
