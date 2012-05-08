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
// $Id: database.cpp,v 1.5 2003/02/19 22:29:36 edrusb Rel $
//
/*********************************************************************/

#include <stdlib.h>
#include <errno.h>
#include <iomanip>
#include <iostream>
#include "database.hpp"
#include "user_interaction.hpp"
#include "deci.hpp"
#include "tools.hpp"

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
}

database::database(generic_file & f)
{
  infinint tmp;
  struct archive_data dat;

  coordinate.clear();
  tmp.read_from_file(f); // number of archive to read
  while(tmp > 0)
    {
      tools_read_string(f, dat.chemin);
      tools_read_string(f, dat.basename);
      coordinate.push_back(dat);
      tmp--;
    }
  if(coordinate.size() < 1)
    throw Erange("database::database", "badly formated database");
  tools_read_vector(f, options_to_dar);
  tools_read_string(f, dar_path);
  files = data_tree_read(f);
    
  if(files == NULL)
    throw Ememory("database::database");
}

database::~database()
{
  if(files != NULL)
    delete files;
}

void database::dump(generic_file & f) const
{
  archive_num tmp = coordinate.size();
    
  infinint(tmp).dump(f);
  for(archive_num i = 0; i < tmp; i++)
    {
      tools_write_string(f, coordinate[i].chemin);
      tools_write_string(f, coordinate[i].basename);
    }
  tools_write_vector(f, options_to_dar);
  tools_write_string(f, dar_path);
  files->dump(f);
}

void database::add_archive(const catalogue & cat, const string & chemin, const string & basename)
{
  struct archive_data dat;
  archive_num number = coordinate.size();
    
  if(basename == "")
    throw Erange("database::add_archive", "empty string is an invalid archive basename");
  if(number >= ARCHIVE_NUM_MAX)
    throw Erange("database::add_archive", "cannot add another archive, database is full");
 
  dat.chemin = chemin;
  dat.basename = basename;
  coordinate.push_back(dat);
  data_tree_update_with(cat.get_contenu(), number, files);
}

void database::remove_archive(archive_num num)
{
  vector<struct archive_data>::iterator it = coordinate.begin() + num;

  if(num == 0 || num >= coordinate.size())
    throw Erange("database::remove_archive", "non-existant archive in database");
  coordinate.erase(it);
  if(files == NULL)
    throw SRC_BUG;
  files->remove_all_from(num);
  files->skip_out(num);
}

void database::change_name(archive_num num, const string & basename)
{
  if(num < coordinate.size() && num != 0)
    coordinate[num].basename = basename;
  else
    throw Erange("database::change_name", "non-existant archive in database");
}

void database::set_path(archive_num num, const string & chemin)
{
  if(num < coordinate.size() && coordinate[num].basename != "")
    coordinate[num].chemin = chemin;
  else
    throw Erange("database::change_name", "non-existant archive in database");
}

void database::set_permutation(archive_num src, archive_num dst)
{
  struct archive_data moved;

  if(files == NULL)
    throw SRC_BUG;
  if(src > coordinate.size() || src <= 0)
    throw Erange("database::set_permutation", string("Invalid archive number: ") + tools_int2str(src));
  if(dst > coordinate.size() || dst <= 0)
    throw Erange("database::set_permutation", string("Invalid archive number: ") + tools_int2str(dst));

  moved = coordinate[src];
  coordinate.erase(coordinate.begin()+src);
  coordinate.insert(coordinate.begin()+dst, moved);
  files->apply_permutation(src, dst);
}

static void dummy_call(char *x)
{
  static char id[]="$Id: database.cpp,v 1.5 2003/02/19 22:29:36 edrusb Rel $";
  dummy_call(id);
}

void database::show_contents() const
{
  user_interaction_stream() << endl;
  user_interaction_stream() << "dar path    : " << dar_path << endl;
  user_interaction_stream() << "dar options : " << tools_concat_vector(" ", options_to_dar) << endl;
  user_interaction_stream() << endl;
        
  user_interaction_stream() << "archive #   |    path      |    basename " <<endl;
  user_interaction_stream() << "------------+--------------+---------------" <<endl;
  for(archive_num i = 1; i < coordinate.size(); i++)
    user_interaction_stream() << setw(10) << i << "\t" << ((coordinate[i].chemin == "") ? "<empty>" : coordinate[i].chemin) << "\t" << coordinate[i].basename << endl;
}

void database::show_files(archive_num num) const
{
  if(num < coordinate.size())
    {
      if(files == NULL)
	throw SRC_BUG;

      files->show(num);
    }
  else
    throw Erange("database::show_files", "non existant archive in database");
}


void database::show_version(path chemin) const
{
  const data_tree *ptr = NULL;
  const data_dir *ptr_dir = files;
  string tmp;

  if(!chemin.is_relative())
    throw Erange("database::show_version", "invalid path, path must be relative");

  while(chemin.pop_front(tmp) && ptr_dir != NULL)
    {
      ptr = ptr_dir->read_child(tmp);
      if(ptr == NULL)
	throw Erange("database::show_version", "non existant file in database");
      ptr_dir = dynamic_cast<const data_dir *>(ptr);
    }

  if(ptr_dir == NULL)
    throw Erange("database::show_version", "non existant file in database");

  ptr = ptr_dir->read_child(chemin.display());
  if(ptr == NULL)
    throw Erange("database::show_version", "non existant file in database");
  else
    ptr->listing();
}

void database::show_most_recent_stats() const
{
  vector<infinint> stats_data(coordinate.size(), 0);
  vector<infinint> stats_ea(coordinate.size(), 0);
  vector<infinint> total_data(coordinate.size(), 0);
  vector<infinint> total_ea(coordinate.size(), 0);
  if(files == NULL)
    throw SRC_BUG;
  files->compute_most_recent_stats(stats_data, stats_ea, total_data, total_ea);

  user_interaction_stream() << "  archive #   |  most recent/total data |  most recent/total EA" << endl;
  user_interaction_stream() << "--------------+-------------------------+-----------------------" << endl;
  for(archive_num i = 1; i < coordinate.size(); i++)
    user_interaction_stream() << setw(10) << i << " " << stats_data[i]
			      << "/" << total_data[i] 
			      << "\t\t\t" << stats_ea[i] 
			      << "/" << total_ea[i] 
			      << endl;
}


void database::restore(const vector<string> & filename)
{
  map<archive_num, vector<string> > command_line;
  vector<string>::iterator it = const_cast<vector<string> *>(&filename)->begin();
  vector<string>::iterator fin = const_cast<vector<string> *>(&filename)->end();
  const data_tree *ptr;
    
  // determination of the archive to restore and files to restore for each selected archive
  while(it != fin)
    {
      if(data_tree_find(*it, *files, ptr))
	{
	  archive_num num_data = 0;
	  archive_num num_ea = 0;
	    
	  ptr->get_data(num_data);
	  ptr->get_EA(num_ea);
	    
	  if(num_data == num_ea)
	    {
	      if(num_data != 0)
		command_line[num_data].push_back(*it);
	    }
	  else 
	    {
	      if(num_data != 0)
		command_line[num_data].push_back(*it);
	      if(num_ea != 0)
		command_line[num_ea].push_back(*it);
	      if(num_data != 0 && num_ea != 0)
		if(num_data > num_ea) // will restore "EA only" then "data + old EA"
		  user_interaction_warning(string("concerning file ") + *it 
					   + " : archive #" + tools_int2str(num_data) 
					   + " contains the most recent data and some old EA while archive #"
					   + tools_int2str(num_ea)
					   + " contains a delta with the most recent EA only. Dar manager "
					   + "will always restore archive in the order they have been added "
					   + "in database, thus, for this file, last EA version, will not be "
					   + "overwritten by the older version, saved with the data. "
					   + " Either rebuid the database adding archive following chronological order "
					   + " or restore EA manually from archive#" + tools_int2str(num_ea) + ".");
	    }
	}
      else
	user_interaction_warning(string("cannot restore file ") + *it + " : non existant file in database");
      it++;
    }

  // calling dar for each archive
  if(command_line.size() > 0) 
    {
      string dar_cmd = dar_path != "" ? dar_path : "dar";
      map<archive_num, vector<string> >::iterator ut = command_line.begin();
      vector<string> argvector_init = vector<string>(1, dar_cmd) + options_to_dar;
	
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
	      argvector += ut->second;
		
	      cout << "CALLING DAR: " << tools_concat_vector(" ", argvector) << endl;
	      tools_system(argvector);
	    }
	  catch(Erange & e)
	    {
	      user_interaction_warning(string("Error while restoring the following files : ") 
				       + tools_concat_vector( " ", ut->second) 
				       + "   : " 
				       + e.get_message());
	    }
	  ut++;
	}
    }
  else
    user_interaction_warning("Cannot restore any file, nothing to do");
}
