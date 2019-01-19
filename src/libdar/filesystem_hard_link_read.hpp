/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

    /// \file filesystem_hard_link_read.hpp
    /// \brief filesystem_hard_link_read classes manages hardlinked inode read from filesystem
    /// \ingroup Private

#ifndef FILESYSTEM_HARD_LINK_READ_HPP
#define FILESYSTEM_HARD_LINK_READ_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
} // end extern "C"

#include <map>
#include "infinint.hpp"
#include "fsa_family.hpp"
#include "cat_all_entrees.hpp"
#include "mem_ui.hpp"

#include <set>

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// keep trace of hard links when reading the filesystem

    class filesystem_hard_link_read : protected mem_ui
    {
            // this class is not to be used directly
            // it only provides some routine for the inherited classes

    public:
	filesystem_hard_link_read(const std::shared_ptr<user_interaction> & dialog,
				  bool x_furtive_read_mode,
				  const fsa_scope & scope) : mem_ui(dialog)
	{ furtive_read_mode = x_furtive_read_mode; sc = scope; ask_before_zeroing_neg_dates = true; };

	    // the copy of the current object would make copy of addresses in
	    // corres_read that could be released twice ... thus, copy constructor and
	    // assignement are forbidden for this class:

	filesystem_hard_link_read(const filesystem_hard_link_read & ref) = delete;
	filesystem_hard_link_read(filesystem_hard_link_read && ref) = delete;
	filesystem_hard_link_read & operator = (const filesystem_hard_link_read & ref) = delete;
	filesystem_hard_link_read & operator = (filesystem_hard_link_read && ref) = delete;

	virtual ~filesystem_hard_link_read() = default;

	    /// get the last assigned number for a hard linked inode
	const infinint & get_last_etoile_ref() const { return etiquette_counter; };

	    /// provide the FSA scope used by the object
	const fsa_scope get_fsa_scope() const { return sc; };

	    /// don't ask before zeroing negative date just warn user
	void zeroing_negative_dates_without_asking() { ask_before_zeroing_neg_dates = false; };

	    /// let the user define which file to not consider as symlinks
	    ///
	    /// \note argument is a list of full paths. No mask or special character is taken into account
	void set_ignored_symlinks_list(const std::set<std::string> & x_ignored_symlinks)
	{ ignored_symlinks = x_ignored_symlinks; };

    protected:

	    /// reset the whole list of hard linked inodes (hard linked inode stay alive but are no more referenced by the current object)
        void corres_reset() { corres_read.clear(); etiquette_counter = 0; };

	    /// create and return a libdar object corresponding to one pointed to by its path
	    /// during this operation, hard linked inode are recorded in a list to be easily pointed
	    /// to by a new reference to it.
        cat_nomme *make_read_entree(const path & lieu,         ///< path of the file to read
				    const std::string & name,  ///< name of the file to read
				    bool see_hard_link,        ///< whether we want to detect hard_link and eventually return a cat_mirage object (not necessary when diffing an archive with filesystem)
				    const mask & ea_mask       ///< which EA to consider when creating the object
	    );

	bool get_ask_before_zeroing_neg_dates() const { return ask_before_zeroing_neg_dates; };
    private:

	    // private datastructure

        struct couple
        {
            nlink_t count;       ///< counts the number of hard link on that inode that have not yet been found in filesystem, once this count reaches zero, the "couple" structure can be dropped and the "holder" too (no more expected hard links to be found)
            cat_etoile *obj;     ///< the address of the corresponding cat_etoile object for that inode
	    cat_mirage holder;   ///< it increments by one the obj internal counters, thus, while this object is alive, the obj will not be destroyed

	    couple(cat_etoile *ptr, nlink_t ino_count) : holder("FAKE", ptr) { count = ino_count; obj = ptr; };
        };

	struct node
	{
	    node(ino_t num, dev_t dev) { numnode = num; device = dev; };

		// this operator is required to use the type node in a std::map
	    bool operator < (const node & ref) const { return numnode < ref.numnode || (numnode == ref.numnode && device < ref.device); };
	    ino_t numnode;
	    dev_t device;
	};

	    // private variable

        std::map <node, couple> corres_read;
	infinint etiquette_counter;
	bool furtive_read_mode;
	fsa_scope sc;
	bool ask_before_zeroing_neg_dates;
	std::set<std::string> ignored_symlinks;

	    // private methods

	bool ignore_if_symlink(const std::string & full_path)
	{ return ignored_symlinks.find(full_path) != ignored_symlinks.end(); };
    };

	/// @}

} // end of namespace

#endif
