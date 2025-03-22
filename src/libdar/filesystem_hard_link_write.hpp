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

    /// \file filesystem_hard_link_write.hpp
    /// \brief class filesystem_hard_link_write keeps trace of already written inode to restore hard links
    /// \ingroup Private

#ifndef FILESYSTEM_HARD_LINK_WRITE_HPP
#define FILESYSTEM_HARD_LINK_WRITE_HPP

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
#include "catalogue.hpp"
#include "infinint.hpp"
#include "fsa_family.hpp"
#include "cat_all_entrees.hpp"

#include <set>

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// keep trace of already written inodes to restore hard links

    class filesystem_hard_link_write : protected mem_ui
    {
            // this class is not to be used directly
            // it only provides routines to its inherited classes

    public:
	filesystem_hard_link_write(const std::shared_ptr<user_interaction> & dialog): mem_ui(dialog) { corres_write.clear(); };
	filesystem_hard_link_write(const filesystem_hard_link_write & ref) = delete;
	filesystem_hard_link_write(filesystem_hard_link_write && ref) = delete;
	filesystem_hard_link_write & operator = (const filesystem_hard_link_write & ref) = delete;
	filesystem_hard_link_write & operator = (filesystem_hard_link_write && ref) = delete;
	~filesystem_hard_link_write() = default;

        void write_hard_linked_target_if_not_set(const cat_mirage *ref, const std::string & chemin);
            // if a hard linked inode has not been restored (no change, or less recent than the one on filesystem)
            // it is necessary to inform filesystem, where to hard link on, any future hard_link
            // that could be necessary to restore.

	    /// \return true if an inode in filesystem has been seen for that hard linked inode
        bool known_etiquette(const infinint & eti);

	    /// forget everything about a hard link if the path used to build subsequent hard links is the one given in argument

	    /// \param[in] ligne is the etiquette number for that hard link
	    /// \param[in] path if the internaly recorded path to build subsequent hard link to that inode is equal to path, forget everything about this hard linked inode
        void clear_corres_if_pointing_to(const infinint & ligne, const std::string & path);

    protected:
        void corres_reset() { corres_write.clear(); };

	    /// generate inode or make a hard link on an already restored or existing inode.

	    /// \param[in] ref object to restore in filesystem
	    /// \param[in] ou  where to restore it
        void make_file(const cat_nomme * ref,
		       const path & ou);

	    /// add the given EA matching the given mask to the file pointed to by "e" and spot

	    /// \param[in] e may be an inode or a hard link to an inode,
	    /// \param[in] list_ea the list of EA to restore
	    /// \param[in] spot the path where to restore these EA (full path required, including the filename of 'e')
	    /// \param[in] ea_mask the EA entry to restore from the list_ea (other entries are ignored)
	    /// \return true if EA could be restored, false if "e" is a hard link to an inode that has its EA already restored previously
	    /// \note the list_ea EA are restored to spot path, the object e is only here to validate that this operation
	    /// has not already been done through another hard linked inode to that same inode
        bool raw_set_ea(const cat_nomme *e,
			const ea_attributs & list_ea,
			const std::string & spot,
			const mask & ea_mask);
            // check whether the inode for which to restore EA is not a hard link to
            // an already restored inode. if not, it calls the proper ea_filesystem call to restore EA

	    /// remove EA set from filesystem's file, allows subsequent raw_set_ea

	    /// \param[in] e this object may be a hard link to or an inode
	    /// \param[in] path the path in the filesystem where reside the object whose EA to clear
	    /// \return true if EA could be cleared, false if "e" is a hard link to an inode that has its  EA already restored previously
	bool raw_clear_ea_set(const cat_nomme *e, const std::string & path);


    private:
        struct corres_ino_ea
        {
            std::string chemin;
            bool ea_restored;
        };

        std::map <infinint, corres_ino_ea> corres_write;
    };

	/// @}

} // end of namespace

#endif
