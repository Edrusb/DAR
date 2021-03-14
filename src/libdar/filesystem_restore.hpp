/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file filesystem_restore.hpp
    /// \brief class filesystem_restores create inodes from a flow of libdar objects
    /// \ingroup Private

#ifndef FILESYSTEM_RESTORE_HPP
#define FILESYSTEM_RESTORE_HPP

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

#include <deque>
#include "crit_action.hpp"
#include "fsa_family.hpp"
#include "cat_all_entrees.hpp"
#include "filesystem_hard_link_read.hpp"
#include "filesystem_hard_link_write.hpp"

#include <set>

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// receive the flow of inode from the restoration filtering routing and promotes these to real filesystem objects

    class filesystem_restore : public filesystem_hard_link_write, public filesystem_hard_link_read
    {
    public:
	    /// constructor
        filesystem_restore(const std::shared_ptr<user_interaction> & dialog,
			   const path & root,
			   bool x_warn_overwrite,
			   bool x_info_details,
                           const mask & x_ea_mask,
			   comparison_fields what_to_check,
			   bool x_warn_remove_no_match,
			   bool empty,
			   const crit_action *x_overwrite,
			   bool x_only_overwrite,
			   const fsa_scope & scope);

	    /// copy constructor is forbidden
        filesystem_restore(const filesystem_restore & ref) = delete;

	    /// move constructor is forbidden
	filesystem_restore(filesystem_restore && ref) = delete;

	    /// assignment operator is forbidden
        filesystem_restore & operator = (const filesystem_restore  & ref) = delete;

	    /// move operator is forbidden
	filesystem_restore & operator = (filesystem_restore && ref) = delete;

	    /// destructor
        ~filesystem_restore() { restore_stack_dir_ownership(); detruire(); };

	    /// reset the writing process for the current object
        void reset_write();

	using action_done_for_data = enum
	{
	    done_data_restored,     //< data has been restored to filesystem
	    done_no_change_no_data, //< no change in filesystem because no data present in archive
	    done_no_change_policy,  //< no change in filesystem because of overwiting policy decision
	    done_data_removed       //< data (= whole inode) removed from filesystem
	};

	    /// restore a libdar object to a filesystem entry both data and EA

	    /// \param[in] x is the libdar object to restore
	    /// \param[out] data_restored true if data has been restored (inode or hard link created), false if either there is no data to restore or if this action is forbidden by the overwriting policy
	    /// \param[out] ea_restored  true if EA has been restored, false if either no EA to restore or if forbidden by overwriting policy
	    /// \param[out] data_created true if data has been restored leading to file creation, false in any other case
	    /// \param[out] hard_link true when data_restored is true and only a hard link to an already existing inode has been created
	    /// \param[out] fsa_restored true if FSA has been restored, false if either no FSA to restore or if forbidden by overwriting policy
	    /// \note any failure to restore data or EA that is not due to its absence in "x" nor to an interdiction from the overwriting policy is signaled
	    /// through an exception.
	void write(const cat_entree *x,
		   action_done_for_data & data_restored,
		   bool & ea_restored,
		   bool & data_created,
		   bool & hard_link,
		   bool & fsa_restored);


	    /// ask for no warning or user interaction for the next write operation

	    /// \note this is used when a file has been saved several times due to its changes at the time of the backup
	    /// and is restored in sequential read. Restoring each failed backup would lead to ask each time the
	    /// actions to take about overwriting... anoying for the user
	void ignore_overwrite_restrictions_for_next_write() { ignore_over_restricts = true; };


    private:
	class stack_dir_t : public cat_directory
	{
	public:
	    stack_dir_t(const cat_directory & ref, bool restore) : cat_directory(ref) { restore_date = restore; };

	    bool get_restore_date() const { return restore_date; };
	    void set_restore_date(bool val) { restore_date = val; };

	private:
	    bool restore_date;
	};

        path *fs_root;
        bool info_details;
	mask *ea_mask;
        bool warn_overwrite;
	comparison_fields what_to_check;
	bool warn_remove_no_match;
        std::deque<stack_dir_t> stack_dir;
        path *current_dir;
	bool empty;
	bool ignore_over_restricts;
	const crit_action *overwrite;
	bool only_overwrite;

        void detruire();
	void restore_stack_dir_ownership();
	user_interaction & get_ui() const { return filesystem_hard_link_read::get_ui(); };
	std::shared_ptr<user_interaction> get_pointer() const { return filesystem_hard_link_read::get_pointer(); };

	    // subroutines of write()

	    /// perform action due to the overwriting policy when the "to be added" entry is a detruit object
	void action_over_remove(const cat_inode *in_place,
				const cat_detruit *to_be_added,
				const std::string & spot,
				over_action_data action);
	    /// perform action for data due to the overwriting policy when the "to be added" entry is not a cat_detruit
	void action_over_data(const cat_inode *in_place,
			      const cat_nomme *to_be_added,
			      const std::string & spot,
			      over_action_data action,
			      action_done_for_data & data_done);
	    /// perform action for EA due to overwriting policy
	bool action_over_ea(const cat_inode *in_place,
			    const cat_nomme *to_be_added,
			    const std::string & spot,
			    over_action_ea action);
	    /// perform action for FSA due to overwriting policy
	bool action_over_fsa(const cat_inode *in_place,
			     const cat_nomme *to_be_added,
			     const std::string & spot,
			     over_action_ea action);

    };

	/// @}

} // end of namespace

#endif
