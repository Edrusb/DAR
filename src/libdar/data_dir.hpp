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

    /// \file data_dir.hpp
    /// \brief classes used to store directory tree information in dar_manager databases
    /// \ingroup Private


#ifndef DATA_DIR_HPP
#define DATA_DIR_HPP

#include "../my_config.h"

#include <map>
#include <string>
#include <deque>
#include <set>
#include "infinint.hpp"
#include "generic_file.hpp"
#include "infinint.hpp"
#include "user_interaction.hpp"
#include "path.hpp"
#include "cat_directory.hpp"
#include "cat_inode.hpp"
#include "cat_detruit.hpp"
#include "database_listing_callback.hpp"
#include "database_aux.hpp"
#include "data_tree.hpp"

namespace libdar
{

	/// \ingroup Private
	/// @{

	/// the data_dir class inherits from data_tree and holds the directory tree's parent relationship

    class data_dir : public data_tree
    {
    public:
	data_dir(const std::string &name);
	data_dir(generic_file &f, unsigned char db_version); //< does not read signature
	data_dir(const data_tree & ref);
	data_dir(const data_dir & ref);
	data_dir(data_dir && ref) noexcept = default;
	data_dir & operator = (const data_dir & ref) { rejetons.clear(); return *this; };
	data_dir & operator = (data_dir && ref) noexcept = default;
	~data_dir();

	virtual void dump(generic_file & f) const override; //< write signature followed by data constructor will read

	void add(const cat_inode *entry, const archive_num & archive);
	void add(const cat_detruit *entry, const archive_num & archive);
	const data_tree *read_child(const std::string & name) const;
	void read_all_children(std::vector<std::string> & fils) const;
	virtual void finalize_except_self(const archive_num & archive,
					  const datetime & deleted_date,
					  const archive_num & ignore_archives_greater_or_equal);

	    // inherited methods
	virtual bool check_order(user_interaction & dialog, const path & current_path, bool & initial_warn) const override;
	virtual void finalize(const archive_num & archive, const datetime & deleted_date, const archive_num & ignore_archives_greater_or_equal) override;
	virtual bool remove_all_from(const archive_num & archive_to_remove, const archive_num & last_archive) override;

	    /// list the most recent files owned by that archive (or by any archive if num == 0)
	void show(database_listing_show_files_callback callback,
		  void *tag,
		  archive_num num,
		  std::string marge = "") const;
	virtual void apply_permutation(archive_num src, archive_num dst) override;
	virtual void skip_out(archive_num num) override;
	void compute_most_recent_stats(std::vector<infinint> & data,
				       std::vector<infinint> & ea,
				       std::vector<infinint> & total_data,
				       std::vector<infinint> & total_ea) const;

	virtual char obj_signature() const override { return signature(); };
	static char signature() { return 'd'; };

	virtual bool fix_corruption() override; // inherited from data_tree

	    /// lookup routine to find a pointer to the dat_tree object corresponding to the given path

	    /// \param[in] chemin is the path to look for
	    /// \param[out] ptr is a pointer to the looked node, if found
	    /// \return true if a node could be found in the database
	bool data_tree_find(path chemin, const data_tree *& ptr) const;

	    /// add a directory to the dat_dir
	void data_tree_update_with(const cat_directory *dir, archive_num archive);


	    /// read a signature and then run the data_dir constructor but aborts if not a data_dir

	    /// \note the constructors of data_tree and data_dir do not read the signature
	    /// because its purpose it to know whether the following is a data_dir or data_tree
	    /// dump() result.
	static data_dir *data_tree_read(generic_file & f, unsigned char db_version);

    private:
	std::deque<data_tree *> rejetons;          //< subdir and subfiles of the current dir

	void add_child(data_tree *fils);          //< "this" is now responsible of "fils" disalocation
	void remove_child(const std::string & name);
	data_tree *find_or_addition(const std::string & name, bool is_dir, const archive_num & archive);

	    /// read signature and depening on it run data_tree or data_dir constructor
	static data_tree *read_next_in_list_from_file(generic_file & f, unsigned char db_version);
    };




	/// @}

} // end of namespace

#endif
