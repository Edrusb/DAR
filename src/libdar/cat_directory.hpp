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

    /// \file cat_directory.hpp
    /// \brief class used to organize objects in tree in catalogue as like directories in a filesystem
    /// \ingroup Private

#ifndef CAT_DIRECTORY_HPP
#define CAT_DIRECTORY_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#ifdef LIBDAR_FAST_DIR
#include <map>
#endif

#include <list>
#include "cat_inode.hpp"

namespace libdar
{
    class eod;

	/// \addtogroup Private
	/// @{

	/// the directory inode class
    class directory : public inode
    {
    public :
        directory(const infinint & xuid,
		  const infinint & xgid,
		  U_16 xperm,
                  const datetime & last_access,
                  const datetime & last_modif,
		  const datetime & last_change,
                  const std::string & xname,
		  const infinint & device);
        directory(const directory &ref); // only the inode part is build, no children is duplicated (empty dir)
	const directory & operator = (const directory & ref); // set the inode part *only* no subdirectories/subfiles are copies or removed.
        directory(user_interaction & dialog,
		  generic_file & f,
		  const archive_version & reading_ver,
		  saved_status saved,
		  entree_stats & stats,
		  std::map <infinint, etoile *> & corres,
		  compression default_algo,
		  generic_file *data_loc,
		  compressor *efsa_loc,
		  bool lax,
		  bool only_detruit, // objects of other class than detruit and directory are not built in memory
		  escape *ptr);
        ~directory(); // detruit aussi tous les fils et se supprime de son 'parent'

        void add_children(nomme *r); // when r is a directory, 'parent' is set to 'this'
	bool has_children() const { return !ordered_fils.empty(); };
        void reset_read_children() const;
	void end_read() const;
        bool read_children(const nomme * &r) const; // read the direct children of the directory, returns false if no more is available
	    // remove all entry not yet read by read_children
	void tail_to_read_children();

	void remove(const std::string & name); // remove the given entry from the catalogue
	    // as side effect the reset_read_children() method must be called.

        directory * get_parent() const { return parent; };
        bool search_children(const std::string &name, const nomme *&ref) const;
	bool callback_for_children_of(user_interaction & dialog, const std::string & sdir, bool isolated = false) const;

            // using is_more_recent_than() from inode class
            // using method has_changed_since() from inode class
        unsigned char signature() const { return mk_signature('d', get_saved_status()); };

	    /// detemine whether some data has changed since archive of reference in this directory or subdirectories
	bool get_recursive_has_changed() const { return recursive_has_changed; };

	    /// ask recursive update for the recursive_has_changed field
	void recursive_has_changed_update() const;

	    /// get then number of "nomme" entry contained in this directory and subdirectories (recursive call)
	infinint get_tree_size() const;

	    /// get the number of entry having some EA set in the directory tree (recursive call)
	infinint get_tree_ea_num() const;

	    /// get the number of entry that are hard linked inode (aka mirage in dar implementation) (recursive call)
	infinint get_tree_mirage_num() const;

	    // for each mirage found (hard link implementation) in the directory tree, add its etiquette to the returned
	    // list with the number of reference that has been found in the tree. (map[etiquette] = number of occurence)
	    // from outside of class directory, the given argument is expected to be an empty map.
	void get_etiquettes_found_in_tree(std::map<infinint, infinint> & already_found) const;

	    /// whether this directory is empty or not
	bool is_empty() const { return ordered_fils.empty(); };

	    /// recursively remove all mirage entries
	void remove_all_mirages_and_reduce_dirs();

	    /// set the value of inode_dumped for all mirage (recusively)
	void set_all_mirage_s_inode_dumped_field_to(bool val);

        cat_entree *clone() const { return new (get_pool()) directory(*this); };

	const infinint & get_size() const { recursive_update_sizes(); return x_size; };
	const infinint & get_storage_size() const { recursive_update_sizes(); return x_storage_size; };

	void recursively_set_to_unsaved_data_and_FSA();

    protected:
        void inherited_dump(generic_file & f, bool small) const;

    private:
	static const eod fin;

	infinint x_size;
	infinint x_storage_size;
	bool updated_sizes;
        directory *parent;
#ifdef LIBDAR_FAST_DIR
        std::map<std::string, nomme *> fils; // used for fast lookup
#endif
	std::list<nomme *> ordered_fils;
        std::list<nomme *>::iterator it;
	bool recursive_has_changed;

	void clear();
	void recursive_update_sizes() const;
	void recursive_flag_size_to_update() const;
    };

	/// @}

} // end of namespace

#endif
