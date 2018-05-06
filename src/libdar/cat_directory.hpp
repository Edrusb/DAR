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

#include "cat_inode.hpp"

#ifdef LIBDAR_FAST_DIR
#include <map>
#endif
#include <list>

namespace libdar
{
    class cat_eod;

	/// \addtogroup Private
	/// @{

	/// the cat_directory inode class
    class cat_directory : public cat_inode
    {
    public :
        cat_directory(const infinint & xuid,
		      const infinint & xgid,
		      U_16 xperm,
		      const datetime & last_access,
		      const datetime & last_modif,
		      const datetime & last_change,
		      const std::string & xname,
		      const infinint & device);
        cat_directory(const std::shared_ptr<user_interaction> & dialog,
		      const smart_pointer<pile_descriptor> & pdesc,
		      const archive_version & reading_ver,
		      saved_status saved,
		      entree_stats & stats,
		      std::map <infinint, cat_etoile *> & corres,
		      compression default_algo,
		      bool lax,
		      bool only_detruit, // objects of other class than detruit and cat_directory are not built in memory
		      bool small);
	cat_directory(const cat_directory &ref); // only the inode part is build, no children is duplicated (empty dir)
	cat_directory(cat_directory && ref) noexcept;
	cat_directory & operator = (const cat_directory & ref); // set the inode part *only* no subdirectories/subfiles are copies or removed.
	cat_directory & operator = (cat_directory && ref) noexcept;
        ~cat_directory() throw(Ebug); // detruit aussi tous les fils et se supprime de son 'parent'

	    /// attention this compares only the directories themselves, not the list of their children
	virtual bool operator == (const cat_entree & ref) const override;

        void add_children(cat_nomme *r); // when r is a cat_directory, 'parent' is set to 'this'
	bool has_children() const { return !ordered_fils.empty(); };
        void reset_read_children() const;
	void end_read() const;
        bool read_children(const cat_nomme * &r) const; // read the direct children of the cat_directory, returns false if no more is available
	    // remove all entry not yet read by read_children
	void tail_to_read_children();


	    /// remove the given entry from the catalogue
	    ///
	    /// \note read_children() is taken into account by this operation,
	    /// no need to call reset_read_children(), if the argument removed was the
	    /// one about to be read by read_children() the one following the removed entry
	    /// will be returned the next time read_children() is invoked.
	void remove(const std::string & name);

        cat_directory * get_parent() const { return parent; };
        bool search_children(const std::string &name, const cat_nomme *&ref) const;

            // using is_more_recent_than() from cat_inode class
            // using method has_changed_since() from cat_inode class
        virtual unsigned char signature() const override { return 'd'; };

	virtual std::string get_description() const override { return "folder"; };


	    /// detemine whether some data has changed since archive of reference in this cat_directory or subdirectories
	bool get_recursive_has_changed() const { return recursive_has_changed; };

	    /// ask recursive update for the recursive_has_changed field
	void recursive_has_changed_update() const;

	    /// get then number of "cat_nomme" entry contained in this cat_directory and subdirectories (recursive call)
	infinint get_tree_size() const;

	    /// get the number of entry having some EA set in the cat_directory tree (recursive call)
	infinint get_tree_ea_num() const;

	    /// get the number of entry that are hard linked inode (aka mirage in dar implementation) (recursive call)
	infinint get_tree_mirage_num() const;

	    // for each mirage found (hard link implementation) in the cat_directory tree, add its etiquette to the returned
	    // list with the number of reference that has been found in the tree. (map[etiquette] = number of occurence)
	    // from outside of class cat_directory, the given argument is expected to be an empty map.
	void get_etiquettes_found_in_tree(std::map<infinint, infinint> & already_found) const;

	    /// whether this cat_directory is empty or not
	bool is_empty() const { return ordered_fils.empty(); };

	    /// recursively remove all mirage entries
	void remove_all_mirages_and_reduce_dirs();

	    /// recursively set all mirage inode_wrote flag
	void set_all_mirage_s_inode_wrote_field_to(bool val) const;

	    /// set the value of inode_dumped for all mirage (recusively)
	void set_all_mirage_s_inode_dumped_field_to(bool val) const;

        virtual cat_entree *clone() const override { return new (std::nothrow) cat_directory(*this); };

	const infinint & get_size() const { recursive_update_sizes(); return x_size; };
	const infinint & get_storage_size() const { recursive_update_sizes(); return x_storage_size; };

	void recursively_set_to_unsaved_data_and_FSA();

	    /// overwrite virtual method of cat_entree to propagate the action to all entries of the directory tree
	virtual void change_location(const smart_pointer<pile_descriptor> & pdesc) override;

    protected:
        virtual void inherited_dump(const pile_descriptor & pdesc, bool small) const override;

    private:
	static const cat_eod fin;

	mutable infinint x_size;
	mutable infinint x_storage_size;
	mutable bool updated_sizes;
        cat_directory *parent;
#ifdef LIBDAR_FAST_DIR
        std::map<std::string, cat_nomme *> fils; // used for fast lookup
#endif
	std::deque<cat_nomme *> ordered_fils;
        mutable std::deque<cat_nomme *>::const_iterator it; //< next entry to be returned by read_children
	mutable bool recursive_has_changed;

	void init() noexcept;
	void clear();
	void recursive_update_sizes() const;
	void recursive_flag_size_to_update() const;
	void erase_ordered_fils(std::deque<cat_nomme *>::const_iterator debut,
				std::deque<cat_nomme *>::const_iterator fin);
    };

	/// @}

} // end of namespace

#endif
