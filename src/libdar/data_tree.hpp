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

    /// \file data_tree.hpp
    /// \brief two classes used to store tree information in dar_manager databases
    /// \ingroup Private


#ifndef DATA_TREE_HPP
#define DATA_TREE_HPP

#include "../my_config.h"

#include <map>
#include <string>
#include <list>
#include "infinint.hpp"
#include "generic_file.hpp"
#include "infinint.hpp"
#include "user_interaction.hpp"
#include "path.hpp"
#include "on_pool.hpp"
#include "cat_directory.hpp"
#include "cat_inode.hpp"
#include "cat_detruit.hpp"

namespace libdar
{

	/// \ingroup Private
	/// @}

    typedef U_16 archive_num;
#define ARCHIVE_NUM_MAX  65534

	/// the data_tree class stores presence of a given file in a set of archives

	/// the data associated to a given file are the different modification dates
	/// that this file has been found in the archive the database has been feed by
	/// \ingroup Private
    class data_tree : public on_pool
    {
    public:
	enum lookup { found_present, found_removed, not_found, not_restorable };
	enum etat
	{
	    et_saved,    //< data/EA present in the archive
	    et_present,  //< file/EA present in the archive but data not saved (differential backup)
	    et_removed,  //< file/EA stored as deleted since archive of reference of file/EA not present in the archive
	    et_absent    //< file not even mentionned in the archive, This entry is equivalent to et_removed, but is required to be able to properly re-order the archive when user asks to do so. The dates associated to this state are computed from neighbor archives in the database
	};

	data_tree(const std::string &name);
	data_tree(generic_file &f, unsigned char db_version);
	virtual ~data_tree() {};

	virtual void dump(generic_file & f) const;
	std::string get_name() const { return filename; };
	void set_name(const std::string & name) { filename = name; };

	    /// return the archive where to find the data that was defined just before or at the given date
	lookup get_data(archive_num & archive, const datetime & date, bool even_when_removed) const;

	    /// if EA has been saved alone later, returns in which version for the state of the file at the given date.
	lookup get_EA(archive_num & archive, const datetime & date, bool even_when_removed) const;

	    /// return the date of file's last modification date within the give archive and whether the file has been saved or deleted
	bool read_data(archive_num num, datetime & val, etat & present) const;

	    /// return the date of last inode change and whether the EA has been saved or deleted
	bool read_EA(archive_num num, datetime & val, etat & present) const;

	void set_data(const archive_num & archive, const datetime & date, etat present) { status sta = { date, present }; last_mod[archive] = sta; };
	void set_EA(const archive_num & archive, const datetime & date, etat present) { status sta = { date, present }; last_change[archive] = sta; };

	    /// check date order between archives withing the database ; throw Erange if problem found with date order
	virtual bool check_order(user_interaction & dialog, const path & current_path, bool & initial_warn) const { return check_map_order(dialog, last_mod, current_path, "data", initial_warn) && check_map_order(dialog, last_change, current_path, "EA", initial_warn); };

	    /// add deleted entry if no object of the current archive exist and the entry of the previous archive is already present.

	    /// \param[in] archive is the number of the archive to finalize
	    /// \param[in] deleted_date date of deletion to use for inode removal when no
	    /// information can be grabbed from the archive (this date is taken from the
	    /// parent dir last modification date)
	    /// \param[in] ignore_archive_greater_or_equal ignore archives which number
	    /// is greater or equal than "ignore_archive_greater_or_equal" as if they were not
	    /// present in the database. If set to zero, no archive is ignored.
	virtual void finalize(const archive_num & archive,
			      const datetime & deleted_date,
			      const archive_num & ignore_archive_greater_or_equal);

	    /// return true if the corresponding file is no more located in any archive (thus, the object is no more usefull in the base)
	virtual bool remove_all_from(const archive_num & archive_to_remove, const archive_num & last_archive);

	    /// list where is saved this file
	void listing(user_interaction & dialog) const;
	virtual void apply_permutation(archive_num src, archive_num dst);

	    /// decrement archive numbers above num
	virtual void skip_out(archive_num num);
	virtual void compute_most_recent_stats(std::vector<infinint> & data,
					       std::vector<infinint> & ea,
					       std::vector<infinint> & total_data,
					       std::vector<infinint> & total_ea) const;

	virtual char obj_signature() const { return signature(); };
	static char signature() { return 't'; };

	    // fix corruption case that was brought by bug in release 2.4.0 to 2.4.9
	virtual bool fix_corruption(); // return true whether corruption could be fixed (meaning this entry can be safely removed from base)

    private:
	struct status
	{
	    datetime date;                     //< date of the event
	    etat present;                      //< file's status in the archive
	    void dump(generic_file & f) const; //< write the struct to file
	    void read(generic_file &f,         //< set the struct from file
		      unsigned char db_version);
	};


	std::string filename;
	std::map<archive_num, status> last_mod;    //< key is archive number ; value is last_mod time
	std::map<archive_num, status> last_change; //< key is archive number ; value is last_change time


	    // when false is returned, this means that the user wants to ignore subsequent error of the same type
	    // else either no error yet met or user want to continue receiving the same type of error for other files
	    // in that later case initial_warn is set to false (first warning has been shown).
	bool check_map_order(user_interaction & dialog,
			     const std::map<archive_num, status> the_map,
			     const path & current_path,
			     const std::string & field_nature,
			     bool & initial_warn) const;
    };

	/// the data_dir class inherits from data_tree and holds the directory tree's parent relationship

	/// \ingroup Private
    class data_dir : public data_tree
    {
    public:
	data_dir(const std::string &name);
	data_dir(generic_file &f, unsigned char db_version);
	data_dir(const data_dir & ref);
	data_dir(const data_tree & ref);
	~data_dir();

	void dump(generic_file & f) const;

	void add(const cat_inode *entry, const archive_num & archive);
	void add(const cat_detruit *entry, const archive_num & archive);
	const data_tree *read_child(const std::string & name) const;
	void read_all_children(std::vector<std::string> & fils) const;
	virtual void finalize_except_self(const archive_num & archive,
					  const datetime & deleted_date,
					  const archive_num & ignore_archives_greater_or_equal);

	    // inherited methods
	bool check_order(user_interaction & dialog, const path & current_path, bool & initial_warn) const;
	void finalize(const archive_num & archive, const datetime & deleted_date, const archive_num & ignore_archives_greater_or_equal);
	bool remove_all_from(const archive_num & archive_to_remove, const archive_num & last_archive);

	    /// list the most recent files owned by that archive (or by any archive if num == 0)
	void show(user_interaction & dialog, archive_num num, std::string marge = "") const;
	void apply_permutation(archive_num src, archive_num dst);
	void skip_out(archive_num num);
	void compute_most_recent_stats(std::vector<infinint> & data, std::vector<infinint> & ea,
				       std::vector<infinint> & total_data, std::vector<infinint> & total_ea) const;

	char obj_signature() const { return signature(); };
	static char signature() { return 'd'; };

	virtual bool fix_corruption(); // inherited from data_tree


    private:
	std::list<data_tree *> rejetons;          //< subdir and subfiles of the current dir

	void add_child(data_tree *fils);          //< "this" is now responsible of "fils" disalocation
	void remove_child(const std::string & name);
	data_tree *find_or_addition(const std::string & name, bool is_dir, const archive_num & archive);
    };

    extern data_dir *data_tree_read(generic_file & f, unsigned char db_version, memory_pool *pool);

	/// lookup routine to find a pointer to the dat_dir object corresponding to the given path

	/// \param[in] chemin is the path to look for
	/// \param[in] racine is the database to look into
	/// \param[out] ptr is a pointer to the looked node if found
	/// \return true if a node could be found in the database
    extern bool data_tree_find(path chemin, const data_dir & racine, const data_tree *& ptr);
    extern void data_tree_update_with(const cat_directory *dir, archive_num archive, data_dir *racine);
    extern archive_num data_tree_permutation(archive_num src, archive_num dst, archive_num x);

	/// @}

} // end of namespace

#endif
