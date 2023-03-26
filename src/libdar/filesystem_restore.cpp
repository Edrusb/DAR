/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
    char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_TIME_H
#include <time.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if STDC_HEADERS
#include <ctype.h>
#endif

#ifdef LIBDAR_NODUMP_FEATURE
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if LIBDAR_NODUMP_FEATURE == NODUMP_LINUX
#include <linux/ext2_fs.h>
#else
#if LIBDAR_NODUMP_FEATURE == NODUMP_EXT2FS
#include <ext2fs/ext2_fs.h>
#else
#error "unknown location of ext2_fs.h include file"
#endif
#endif
#endif

#if MAJOR_IN_MKDEV
#include <sys/mkdev.h>
#if !defined(makedev) && defined(mkdev)
#define makedev(a,b) mkdev((a),(b))
#endif
#else
#if MAJOR_IN_SYSMACROS
#include <sys/sysmacros.h>
#endif
#endif
} // end extern "C"


#include <map>
#include <algorithm>

#include "filesystem_restore.hpp"
#include "tools.hpp"
#include "filesystem_tools.hpp"
#include "erreurs.hpp"
#include "user_interaction.hpp"
#include "cat_all_entrees.hpp"
#include "ea_filesystem.hpp"
#include "cygwin_adapt.hpp"
#include "fichier_local.hpp"
#include "generic_rsync.hpp"
#include "null_file.hpp"
#include "cat_signature.hpp"
#include "compile_time_features.hpp"
#include "op_tools.hpp"

using namespace std;

namespace libdar
{
    filesystem_restore::filesystem_restore(const std::shared_ptr<user_interaction> & dialog,
					   const path &root,
					   bool x_warn_overwrite,
                                           bool x_info_details,
                                           const mask & x_ea_mask,
					   comparison_fields x_what_to_check,
					   bool x_warn_remove_no_match,
					   bool x_empty,
					   const crit_action *x_overwrite,
					   bool x_only_overwrite,
					   const fsa_scope & scope):
	filesystem_hard_link_write(dialog),
	filesystem_hard_link_read(dialog, compile_time::furtive_read(), scope)
    {
	fs_root = nullptr;
	ea_mask = nullptr;
	current_dir = nullptr;
	overwrite = nullptr;
	try
	{
	    fs_root = filesystem_tools_get_root_with_symlink(*dialog, root, x_info_details);
	    if(fs_root == nullptr)
		throw Ememory("filesystem_write::filesystem_write");
	    ea_mask = x_ea_mask.clone();
	    if(ea_mask == nullptr)
		throw Ememory("filesystem_restore::filesystem_restore");
	    if(x_overwrite == nullptr)
		throw SRC_BUG;
	    overwrite = x_overwrite->clone();
	    if(overwrite == nullptr)
		throw Ememory("filesystem_restore::filesystem_restore");
	}
	catch(...)
	{
	    detruire();
	    throw;
	}
	warn_overwrite = x_warn_overwrite;
	info_details = x_info_details;
	what_to_check = x_what_to_check;
	warn_remove_no_match = x_warn_remove_no_match;
	empty = x_empty;
	only_overwrite = x_only_overwrite;
	reset_write();
	zeroing_negative_dates_without_asking(); // when reading existing inode to evaluate overwriting action
    }

    void filesystem_restore::reset_write()
    {
        filesystem_hard_link_write::corres_reset();
        filesystem_hard_link_read::corres_reset();
        stack_dir.clear();
        if(current_dir != nullptr)
            delete current_dir;
        current_dir = new (nothrow) path(*fs_root);
        if(current_dir == nullptr)
            throw Ememory("filesystem_write::reset_write");
	ignore_over_restricts = false;
    }

    void filesystem_restore::write(const cat_entree *x, action_done_for_data & data_restored, bool & ea_restored, bool & data_created, bool & hard_link, bool & fsa_restored)
    {
	const cat_eod *x_eod = dynamic_cast<const cat_eod *>(x);
	const cat_nomme *x_nom = dynamic_cast<const cat_nomme *>(x);
	const cat_directory *x_dir = dynamic_cast<const cat_directory *>(x);
	const cat_detruit *x_det = dynamic_cast<const cat_detruit *>(x);
	const cat_inode *x_ino = dynamic_cast<const cat_inode *>(x);
	const cat_mirage *x_mir = dynamic_cast<const cat_mirage *>(x);
	const cat_file *x_fil = dynamic_cast<const cat_file *>(x);

	data_restored = done_no_change_no_data;
	ea_restored = false;
	data_created = false;
	fsa_restored = false;
	hard_link = x_mir != nullptr && known_etiquette(x_mir->get_etiquette());

	if(x_mir != nullptr)
	{
	    x_ino = x_mir->get_inode();
	    if(x_ino == nullptr)
		throw SRC_BUG;
	    x_fil = dynamic_cast<const cat_file *>(x_ino);
	}

	if(x_eod != nullptr)
	{
	    string tmp;
	    current_dir->pop(tmp);
	    if(!stack_dir.empty())
	    {
		try
		{
		    if(!empty && stack_dir.back().get_restore_date())
		    {
			string chem = (current_dir->append(stack_dir.back().get_name())).display();
			filesystem_tools_make_date(stack_dir.back(), chem, what_to_check, get_fsa_scope());
			filesystem_tools_make_owner_perm(get_ui(), stack_dir.back(), chem, what_to_check, get_fsa_scope());
		    }
		}
		catch(...)
		{
		    stack_dir.pop_back();
		    throw;
		}
		stack_dir.pop_back();
	    }
	    else
		throw SRC_BUG;

	    return;
	}

	if(x_nom == nullptr)
	    throw SRC_BUG; // neither "cat_nomme" nor "cat_eod"
	else // cat_nomme
	{
	    bool has_data_saved = (x_ino != nullptr && x_ino->get_saved_status() == saved_status::saved) || x_det != nullptr;
	    bool has_patch = x_ino != nullptr && x_ino->get_saved_status() == saved_status::delta;
	    bool has_just_inode = x_ino != nullptr && x_ino->get_saved_status() == saved_status::inode_only;
	    bool has_ea_saved = x_ino != nullptr && (x_ino->ea_get_saved_status() == ea_saved_status::full || x_ino->ea_get_saved_status() == ea_saved_status::removed);
	    bool has_fsa_saved = x_ino != nullptr && x_ino->fsa_get_saved_status() == fsa_saved_status::full;
	    path spot = current_dir->append(x_nom->get_name());
	    string spot_display = spot.display();

	    cat_nomme *exists = nullptr;

	    if(ignore_over_restricts)
		    // only used in sequential_read when a file has been saved several times due
		    // to its contents being modified at backup time while dar was reading it.
		    // here when ignore_over_restricts is true it is asked to remove the previously restored copy of
		    // that file because a better copy has been found in the archive.
	    {
		ignore_over_restricts = false; // just one shot state ; exists == nullptr : we are ignoring existing entry
		filesystem_tools_supprime(get_ui(), spot_display);
		if(x_det != nullptr)
		{
		    data_restored = done_data_removed;
		    data_created = false;
		    hard_link = false;
		    ea_restored = false;
		    fsa_restored = false;
		    if(!stack_dir.empty())
			stack_dir.back().set_restore_date(true);
		    return;
		}
	    }
	    else
		exists = make_read_entree(*current_dir, x_nom->get_name(), false, *ea_mask);

	    if(has_patch)
	    {
		if(exists == nullptr)
		    throw Erange("filesystem_restore::write", string(gettext("Cannot restore a delta binary patch without a file to patch on filesystem")));
		if(x_fil == nullptr)
		    throw SRC_BUG;
	    }

	    if(has_just_inode)
	    {
		if(exists == nullptr)
		    throw Erange("filesystem_restore::write", string(gettext("Cannot restore a inode metadata only without an existing file on filesystem")));
	    }

	    try
	    {
		cat_inode *exists_ino = dynamic_cast<cat_inode *>(exists);
		cat_directory *exists_dir = dynamic_cast<cat_directory *>(exists);
		cat_file *exists_file = dynamic_cast<cat_file *>(exists);

		if(exists_ino == nullptr && exists != nullptr)
		    throw SRC_BUG; // an object from filesystem should always be an cat_inode !?!

		if(exists == nullptr)
		{
			// no conflict: there is not an already existing file present in filesystem

		    if(x_det != nullptr)
			throw Erange("filesystem_restore::write", tools_printf(gettext("Cannot remove non-existent file from filesystem: %S"),& spot_display));

		    if((has_data_saved || hard_link || x_dir != nullptr) && !only_overwrite)
		    {
			if(info_details)
			    get_ui().message(string(gettext("Restoring file's data: ")) + spot_display);

			    // 1 - restoring data

			if(!empty)
			    make_file(x_nom, *current_dir);
			data_created = true;
			data_restored = done_data_restored;

			    // recording that we must set back the mtime of the parent directory
			if(!stack_dir.empty())
			    stack_dir.back().set_restore_date(true);

			    // we must try to restore EA or FSA only if data could be restored
			    // as in the current situation no file existed before

			    // 2 - restoring EA

			if(has_ea_saved)
			{
			    if(info_details)
				get_ui().message(string(gettext("Restoring file's EA: ")) + spot_display);

			    if(!empty)
			    {
				const ea_attributs *ea = x_ino->get_ea();
				try
				{
				    ea_restored = raw_set_ea(x_nom, *ea, spot_display, *ea_mask);
				}
				catch(Erange & e)
				{
				    get_ui().message(tools_printf(gettext("Restoration of EA for %S aborted: "), &spot_display) + e.get_message());
				}
			    }
			    else
				ea_restored = true;
			}

			    // 3 - restoring FSA (including Mac OSX birth date if present)

			if(has_fsa_saved)
			{
			    if(info_details)
				get_ui().message(string(gettext("Restoring file's FSA: ")) + spot_display);
			    if(!empty)
			    {
				const filesystem_specific_attribute_list * fsa = x_ino->get_fsa();
				if(fsa == nullptr)
				    throw SRC_BUG;
				try
				{
					// we must not yet restore linux::immutable flag in order
					// to restore the permissions and dates that must been
					// restored first:
				    fsa_restored = fsa->set_fsa_to_filesystem_for(spot_display, get_fsa_scope(), get_ui(), false);
				}
				catch(Erange & e)
				{
				    get_ui().message(tools_printf(gettext("Restoration of FSA for %S aborted: "), &spot_display) + e.get_message());
				}
			    }
			}

			    // now that FSA has been read (if sequential mode is used)
			    // we can restore dates in particular creation date from HFS+ FSA if present
			    // but this is useless for directory as we may restore files in them
			    // dates of directories will be restored when the corresponding EOD will
			    // be met
			if(!empty && x_dir == nullptr && x_ino != nullptr)
			{
				// 4 - restoring dates

			    filesystem_tools_make_date(*x_ino,
						       spot_display,
						       what_to_check,
						       get_fsa_scope());

				// 5 - restoring permission and ownership
				// first system may have removed linux capabilities (part of EA)
				// due to change of ownership,
				// second, if dar is not run as root, we may fail any further operation on
				// this file so we must not report failure if subsenquent action fails
			    filesystem_tools_make_owner_perm(get_ui(),
							     *x_ino,
							     spot_display,
							     what_to_check,
							     get_fsa_scope());
			}

			    // 6 - trying re-setting EA after ownership restoration to set back linux capabilities

			if(has_ea_saved && !empty)
			{
			    const ea_attributs *ea = x_ino->get_ea();
			    try
			    {
				raw_set_ea(x_nom, *ea, spot_display, *ea_mask);
			    }
			    catch(Erange & e)
			    {
				    // error probably due to permission context,
				    // as raw_set_ea() has been called earlier
				    // and either no error met or same error met
			    }
			}

			    // 7 - setting the linux immutable flag if present

			if(has_fsa_saved)
			{
			    const filesystem_specific_attribute_list * fsa = x_ino->get_fsa();
			    if(fsa == nullptr)
				throw SRC_BUG;
			    try
			    {
				if(info_details && fsa->has_linux_immutable_set())
				    get_ui().message(string(gettext("Restoring linux immutable FSA for ")) + spot_display);
				fsa_restored = fsa->set_fsa_to_filesystem_for(spot_display, get_fsa_scope(), get_ui(), true);
			    }
			    catch(Erange & e)
			    {
				get_ui().message(tools_printf(gettext("Restoration of linux immutable FSA for %S aborted: "), &spot_display) + e.get_message());
			    }
			}
		    }
		    else // no existing inode but no data to restore
		    {
			data_restored = done_no_change_no_data;
			data_created = false;
			hard_link = false;
			ea_restored = false;
			fsa_restored = false;
		    }
		}
		else // exists != nullptr
		{

			// conflict: an entry of that name is already present in filesystem

		    over_action_data act_data = data_undefined;
		    over_action_ea act_ea = EA_undefined;

			// 1 - restoring data

		    overwrite->get_action(*exists, *x_nom, act_data, act_ea);

		    if(x_ino == nullptr)
			if(x_det == nullptr)
			    throw SRC_BUG;
			else
			{
			    action_over_remove(exists_ino, x_det, spot_display, act_data);
			    data_restored = done_data_removed;
			    data_created = false;
			    hard_link = false;
			    ea_restored = false;
			    fsa_restored = false;

				// recording that we must set back the mtime of the parent directory
			    if(!stack_dir.empty())
				stack_dir.back().set_restore_date(true);
			}
		    else // a normal inode (or hard linked one) is to be restored
		    {
			if(has_data_saved || has_just_inode)
			{
			    action_over_data(exists_ino, x_nom, spot_display, act_data, data_restored);
			}
			else if(has_patch)
			{

			    if(exists_file != nullptr)
			    {
				    // we do not take care of overwriting data here as it is the
				    // expected behavior for delta binary patching
				if(info_details)
				    get_ui().message(string(gettext("Restoring file's data using a delta patching: ")) + spot_display);
				filesystem_tools_make_delta_patch(get_pointer(),
								  *exists_file,
								  spot_display,
								  *x_fil,
								  *current_dir);
				data_restored = done_data_restored;
			    }
			    else
			    {
				    // cannot overwrite the existing inode as we do not have the base data to use for patching
				get_ui().message(tools_printf(gettext("Cannot restore delta diff for %S as exsiting inode is not a plain file"), &spot_display));
			    }
			}
			else // no data saved in the object to restore
			{
			    data_restored = done_no_change_no_data;
			    if(x_mir != nullptr)
				write_hard_linked_target_if_not_set(x_mir, spot_display);
			}

			if(data_restored != done_data_restored)
			    hard_link = false;

			    // here we can restore EA even if no data has been restored
			    // it will modify EA of the existing file
			if(act_data != data_remove)
			{
				// 2 - restoring EA

			    if(has_ea_saved)
			    {
				try
				{
				    ea_restored = action_over_ea(exists_ino, x_nom, spot_display, act_ea);
				}
				catch(Erange & e)
				{
				    get_ui().message(tools_printf(gettext("Restoration of EA for %S aborted: "), &spot_display) + e.get_message());
				}
			    }

				// 3 - restoring FSA

			    if(has_fsa_saved)
			    {
				try
				{
				    fsa_restored = action_over_fsa(exists_ino, x_nom, spot_display, act_ea);
				}
				catch(Erange & e)
				{
				    get_ui().message(tools_printf(gettext("Restoration of FSA for %S aborted: "), &spot_display) + e.get_message());
				}
			    }
			}

			    // if we can restore metadata on something (existing inode)

			if(has_fsa_saved || has_ea_saved || has_patch || has_data_saved || has_just_inode)
			{
			    if(data_restored == done_data_restored)
				    // setting the date, perimssions and ownership to value found in the archive
				    // this can only be done once the EA and FSA have been restored (mtime)
				    // and data has been restore (mtime, atime)
			    {
				if(exists_dir == nullptr)
				{
					// 4 - restoring dates (birthtime has been restored with EA)

				    filesystem_tools_make_date(*x_ino,
							       spot_display,
							       what_to_check,
							       get_fsa_scope());

					// 5 - restoring owership and permission with two consequences :
					// - system erased linux capabilites (EA)
					// - may not be able to restore further and must not propagate failures
					// for subnsequent steps

				    filesystem_tools_make_owner_perm(get_ui(),
								     *x_ino,
								     spot_display,
								     what_to_check,
								     get_fsa_scope());
				}
				    // directory will get their metadata when the correspondant EOD
				    // will be met, meaning all subfile and directory could be treated
				    // (successfully or not, but no more change is expected in that dir)
			    }
			    else
				    // set back the mtime to value found in filesystem before restoration
				filesystem_tools_make_date(*exists_ino, spot_display, what_to_check,
							   get_fsa_scope());
			}

			    // 6 - restoring linux capabilities if possible

			if(ea_restored)
			{
				// if linux capabilities were restored, changing ownership let them
				// been removed by the system. And doing restoration of EA after ownership
				// may avoid being able to restore EA due to lack of privilege if
				// libdar is not ran as root
			    try
			    {
				(void)action_over_ea(exists_ino, x_nom, spot_display, act_ea);
			    }
			    catch(Erange & e)
			    {
				    // ignoring any error here
				    // we already restored EA
				    // previously
			    }
			}

			    // 7 - restoring linux immutable FSA if present

			if(fsa_restored)
			{
			    if(!empty)
			    {
				const cat_mirage *tba_mir = dynamic_cast<const cat_mirage *>(x_nom);
				const cat_inode *tba_ino = dynamic_cast<const cat_inode *>(x_nom);

				if(tba_mir != nullptr)
				    tba_ino = tba_mir->get_inode();
				if(tba_ino == nullptr)
				    throw SRC_BUG;
				if(tba_ino->fsa_get_saved_status() != fsa_saved_status::full)
				    throw SRC_BUG;
				const filesystem_specific_attribute_list * fsa = tba_ino->get_fsa();
				if(fsa == nullptr)
				    throw SRC_BUG;
				if(info_details && fsa->has_linux_immutable_set())
				    get_ui().message(string(gettext("Restoring linux immutable FSA for ")) + spot_display);
				fsa->set_fsa_to_filesystem_for(spot_display, get_fsa_scope(), get_ui(), true);
			    }
			}

			if(act_data == data_remove || data_restored == done_data_restored)
			{
				// recording that we must set back the mtime of the parent directory
			    if(!stack_dir.empty())
				stack_dir.back().set_restore_date(true);
			}
		    }
		}


		if(x_dir != nullptr && (exists == nullptr || exists_dir != nullptr || data_restored == done_data_restored))
		{
		    *current_dir += x_dir->get_name();
		    stack_dir.push_back(stack_dir_t(*x_dir, data_restored == done_data_restored));
		}
	    }
	    catch(...)
	    {
		if(exists != nullptr)
		    delete exists;
		throw;
	    }
	    if(exists != nullptr)
		delete exists;
	}
    }


    void filesystem_restore::action_over_remove(const cat_inode *in_place, const cat_detruit *to_be_added, const string & spot, over_action_data action)
    {
	if(in_place == nullptr || to_be_added == nullptr)
	    throw SRC_BUG;

	if(action == data_ask)
	    action = op_tools_crit_ask_user_for_data_action(get_ui(), spot, in_place, to_be_added);

	switch(action)
	{
	case data_preserve:
	case data_preserve_mark_already_saved:
		// nothing to do
	    break;
	case data_overwrite:
	case data_overwrite_mark_already_saved:
	case data_remove:
	    if(warn_overwrite)
		get_ui().pause(tools_printf(gettext("%S is about to be removed from filesystem, continue?"), &spot));

	    if(cat_signature::compatible_signature(in_place->signature(), to_be_added->get_signature()))
	    {
		if(info_details)
		    get_ui().printf(gettext("Removing file (reason is file recorded as removed in archive): %S"), &spot);
		if(!empty)
		    filesystem_tools_supprime(get_ui(), spot);
	    }
	    else
	    {
		if(warn_remove_no_match) // warning even if just warn_overwrite is not set
		    get_ui().pause(tools_printf(gettext("%S must be removed, but does not match expected type, remove it anyway ?"), &spot));
		if(info_details)
		    get_ui().printf(gettext("Removing file (reason is file recorded as removed in archive): %S"), &spot);
		if(!empty)
		    filesystem_tools_supprime(get_ui(), spot);
	    }
	    break;
	case data_undefined:
	    throw Erange("filesystem_restore::action_over_detruit", tools_printf(gettext("%S: Overwriting policy (Data) is undefined for that file, do not know whether removal is allowed or not!"), &spot));
	case data_ask:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}
    }

    void filesystem_restore::action_over_data(const cat_inode *in_place,
					      const cat_nomme *to_be_added,
					      const string & spot,
					      over_action_data action,
					      action_done_for_data & data_done)
    {
	const cat_mirage *tba_mir = dynamic_cast<const cat_mirage *>(to_be_added);
	const cat_inode *tba_ino = tba_mir == nullptr ? dynamic_cast<const cat_inode *>(to_be_added) : tba_mir->get_inode();
	const cat_directory *tba_dir = dynamic_cast<const cat_directory *>(to_be_added);
	const cat_detruit *tba_det = dynamic_cast<const cat_detruit *>(to_be_added);
	const cat_lien *in_place_symlink = dynamic_cast<const cat_lien *>(in_place);

	if(tba_ino == nullptr)
	    throw SRC_BUG;

	if(in_place == nullptr)
	    throw SRC_BUG;

	if(tba_det != nullptr)
	    throw SRC_BUG; // must be either a cat_mirage or an inode, not any other cat_nomme object

	if(action == data_ask)
	    action = op_tools_crit_ask_user_for_data_action(get_ui(), spot, in_place, to_be_added);

	switch(action)
	{
	case data_preserve:
	case data_preserve_mark_already_saved:
	    if(tba_dir != nullptr && !tba_ino->same_as(*in_place))
		throw Erange("filesystem_write::write", tools_printf(gettext("Directory %S cannot be restored: overwriting not allowed and a non-directory inode of that name already exists, all files in that directory will be skipped for restoration:"), &spot));
	    data_done = done_no_change_policy;
	    break;
	case data_overwrite:
	case data_overwrite_mark_already_saved:
	    if(warn_overwrite)
	    {
		try
		{
		    get_ui().pause(tools_printf(gettext("%S is about to be overwritten, OK?"), &spot));
		}
		catch(Euser_abort & e)
		{
		    if(tba_dir != nullptr && tba_ino->same_as(*in_place))
		    {
			data_done = done_no_change_policy;
			return; // if we throw exception here, we will not recurse in this directory, while we could as a directory exists on filesystem
		    }
		    else
			throw; // throwing the exception here, implies that no EA will be tried to be restored for that file
		}
	    }

	    if(info_details)
		get_ui().message(string(gettext("Restoring file's data: ")) + spot);

	    if((tba_dir != nullptr || tba_ino->get_saved_status() == saved_status::inode_only)
	       && tba_ino->same_as(*in_place))
	    {
		if(!empty)
		    filesystem_tools_widen_perm(get_ui(), *tba_ino, spot, what_to_check);
		data_done = done_data_restored;
	    }
	    else // not both in_place and to_be_added are directories, or we can only restore the inode metadata and existing file is of different type
	    {
		ea_attributs *ea = nullptr; // saving original EA of existing inode
		filesystem_specific_attribute_list fsa; // saving original FSA of existing inode
		bool got_ea = true;
		bool got_fsa = true;

		if(tba_ino->get_saved_status() == saved_status::inode_only)
		    throw Erange("filesystem_restore::write", string(gettext("Existing file is of a different nature, cannot only restore inode metadata")));

		try
		{

			// reading EA present on filesystem

		    try
		    {
			ea = ea_filesystem_read_ea(spot, bool_mask(true));
		    }
		    catch(Ethread_cancel & e)
		    {
			throw;
		    }
		    catch(Egeneric & ex)
		    {
			got_ea = false;
			get_ui().message(tools_printf(gettext("Existing EA for %S could not be read and preserved: "), &spot) + ex.get_message());
		    }

			// reading FSA present on filesystem

		    try
		    {
			fsa.get_fsa_from_filesystem_for(get_ui(),
							spot,
							all_fsa_families(),
							in_place_symlink != nullptr,
							get_ask_before_zeroing_neg_dates());
		    }
		    catch(Ethread_cancel & e)
		    {
			throw;
		    }
		    catch(Egeneric & ex)
		    {
			got_fsa = false;
			get_ui().message(tools_printf(gettext("Existing FSA for %S could not be read and preserved: "), &spot) + ex.get_message());
		    }

			// removing current entry and creating the new entry in place

		    if(!empty)
		    {
			if(filesystem_tools_has_immutable(*in_place)
			   && filesystem_tools_has_immutable(*tba_ino)
			   && in_place->same_as(*tba_ino))
			{
				// considering this situation is the restoration of a differential backup of the same file,
				// we must first remove the immutable flag in order to restore the new inode data
			    if(info_details)
				get_ui().printf(gettext("Removing existing immutable flag in order to restore data for %S"), &spot);
			    filesystem_tools_set_immutable(spot, false, get_ui());
			}

			filesystem_tools_supprime(get_ui(), spot); // this destroyes EA, (removes inode, or hard link to inode)
			make_file(to_be_added, *current_dir);
			data_done = done_data_restored;
		    }

			// restoring EA that were present on filesystem

		    try // if possible and available restoring original EA
		    {
			if(got_ea && !empty)
			    if(ea != nullptr) // if ea is nullptr no EA is present in the original file, thus nothing has to be restored
				(void)ea_filesystem_write_ea(spot, *ea, bool_mask(true));
			    // we don't care about the return value, here, errors are returned through exceptions
			    // the returned value is informative only and does not determine any subsequent actions
		    }
		    catch(Ethread_cancel & e)
		    {
			throw;
		    }
		    catch(Egeneric & e)
		    {
			if(ea != nullptr && !ea->size().is_zero())
			    get_ui().message(tools_printf(gettext("Existing EA for %S could not be preserved : "), &spot) + e.get_message());
		    }

			// restoring FSA that were present on filesystem

		    try // if possible and available restoring original FSA (except the possible immutable flag)
		    {
			if(got_fsa && !empty)
			    fsa.set_fsa_to_filesystem_for(spot, all_fsa_families(), get_ui(), false);
		    }
		    catch(Ethread_cancel & e)
		    {
			throw;
		    }
		    catch(Egeneric & e)
		    {
			if(ea != nullptr && !ea->size().is_zero())
			    get_ui().message(tools_printf(gettext("Existing FSA for %S could not be preserved : "), &spot) + e.get_message());
		    }
		}
		catch(...)
		{
		    if(ea != nullptr)
			delete ea;
		    throw;
		}
		if(ea != nullptr)
		    delete ea;
	    }
	    break;
	case data_remove:
	    if(warn_overwrite)
		get_ui().pause(tools_printf(gettext("%S is about to be deleted (required by overwriting policy), do you agree?"), &spot));
	    if(info_details)
		get_ui().printf(gettext("Removing file (reason is overwriting policy): %S"), &spot);
	    if(!empty)
		filesystem_tools_supprime(get_ui(), spot);
	    data_done = done_data_removed;
	    break;
	case data_undefined:
	    throw Erange("filesystem_restore::action_over_detruit", tools_printf(gettext("%S: Overwriting policy (Data) is undefined for that file, do not know whether overwriting is allowed or not!"), &spot));
	case data_ask:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}
    }

    bool filesystem_restore::action_over_ea(const cat_inode *in_place, const cat_nomme *to_be_added, const string & spot, over_action_ea action)
    {
	bool ret = false;
	const cat_inode *tba_ino = dynamic_cast<const cat_inode *>(to_be_added);
	const cat_mirage *tba_mir = dynamic_cast<const cat_mirage *>(to_be_added);
	bool real_overwrite = true;

	if(tba_mir != nullptr)
	    tba_ino = tba_mir->get_inode();

	if(tba_ino == nullptr)
	    throw SRC_BUG;

	if(in_place == nullptr || to_be_added == nullptr)
	    throw SRC_BUG;

	if(action == EA_ask)
	    action = op_tools_crit_ask_user_for_EA_action(get_ui(), spot, in_place, to_be_added);


	    // modifying the EA action when the in place inode has not EA
#ifdef EA_SUPPORT
	    // but only if EA could be read, which is not the case when EA_SUPPORT is not
	    // activated at compilation time
	if(in_place->ea_get_saved_status() != ea_saved_status::full) // no EA in filesystem
	{
	    if(action == EA_merge_preserve || action == EA_merge_overwrite)
	    {
		action = EA_overwrite; // merging when in_place has no EA is equivalent to overwriting
		real_overwrite = false;
	    }
	}
#endif

	if(tba_ino->ea_get_saved_status() == ea_saved_status::removed) // EA have been removed since archive of reference
	{
	    if(action == EA_merge_preserve || action == EA_merge_overwrite)
		action = EA_clear; // we must remove EA instead of merging
	}

	switch(action)
	{
	case EA_preserve:
	case EA_preserve_mark_already_saved:
		// nothing to do
	    ret = false;
	    break;
	case EA_overwrite:
	case EA_overwrite_mark_already_saved:
	    if(tba_ino->ea_get_saved_status() != ea_saved_status::full && tba_ino->ea_get_saved_status() != ea_saved_status::removed)
		throw SRC_BUG;
	    if(warn_overwrite && real_overwrite)
	    {
		try
		{
		    get_ui().pause(tools_printf(gettext("EA for %S are about to be overwritten, OK?"), &spot));
		}
		catch(Euser_abort & e)
		{
		    const cat_directory *tba_dir = dynamic_cast<const cat_directory *>(to_be_added);
		    if(tba_dir != nullptr && tba_ino->same_as(*in_place))
			return false;
		    else
			throw;
		}
	    }

	    if(!empty && !raw_clear_ea_set(to_be_added, spot))
	    {
		if(info_details)
		    get_ui().printf(gettext("EA for %S have not been overwritten because this file is a hard link pointing to an already restored inode"), &spot);
		ret = false;
	    }
	    else // successfully cleared EA
	    {
		if(info_details)
		    get_ui().message(string(gettext("Restoring file's EA: ")) + spot);

		const ea_attributs *tba_ea = tba_ino->get_ea();
		if(!empty)
		    ret = raw_set_ea(to_be_added, *tba_ea, spot, *ea_mask);
		else
		    ret = true;
	    }
	    break;
	case EA_clear:
	    if(warn_overwrite)
	    {
		try
		{
		    get_ui().pause(tools_printf(gettext("EA for %S are about to be removed, OK?"), &spot));
		}
		catch(Euser_abort & e)
		{
		    return false;
		}
	    }

	    if(!empty && !raw_clear_ea_set(to_be_added, spot))
	    {
		if(info_details)
		    get_ui().printf(gettext("EA for %S have not been cleared as requested by the overwriting policy because this file is a hard link pointing to an already restored inode"), &spot);
		ret = false;
	    }
	    else
	    {
		if(info_details)
		    get_ui().message(string(gettext("Clearing file's EA (requested by overwriting policy): ")) + spot);
		ret = true;
	    }
	    break;
	case EA_merge_preserve:
	case EA_merge_overwrite:
#ifdef EA_SUPPORT
	    if(in_place->ea_get_saved_status() != ea_saved_status::full)
		throw SRC_BUG; // should have been redirected to EA_overwrite !
#endif

	    if(warn_overwrite)
	    {
		try
		{
		    get_ui().pause(tools_printf(gettext("EA for %S are about to be merged, OK?"), &spot));
		}
		catch(Euser_abort & e)
		{
		    return false;
		}
	    }

	    if(tba_ino->ea_get_saved_status() == ea_saved_status::full) // Note, that ea_saved_status::removed is the other valid value
	    {
		const ea_attributs *tba_ea = tba_ino->get_ea();
#ifdef EA_SUPPORT
		const ea_attributs *ip_ea = in_place->get_ea();
#else
		const ea_attributs faked_ip_ea;
		const ea_attributs *ip_ea = & faked_ip_ea;
#endif
		ea_attributs result;

		if(action == EA_merge_preserve)
		    result = *tba_ea + *ip_ea;
		else // action == EA_merge_overwrite
		    result = *ip_ea + *tba_ea; // the + operator on ea_attributs is not reflexive !!!

		if(!empty)
		    ret = raw_set_ea(to_be_added, result, spot, *ea_mask);
		else
		    ret = true;
	    }
	    break;
	case EA_undefined:
	    throw Erange("filesystem_restore::action_over_detruit", tools_printf(gettext("%S: Overwriting policy (EA) is undefined for that file, do not know whether overwriting is allowed or not!"), &spot));
	case EA_ask:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}

	return ret;
    }

    bool filesystem_restore::action_over_fsa(const cat_inode *in_place, const cat_nomme *to_be_added, const string & spot, over_action_ea action)
    {
	bool ret = false;
	const cat_inode *tba_ino = dynamic_cast<const cat_inode *>(to_be_added);
	const cat_mirage *tba_mir = dynamic_cast<const cat_mirage *>(to_be_added);

	if(tba_mir != nullptr)
	    tba_ino = tba_mir->get_inode();

	if(tba_ino == nullptr)
	    throw SRC_BUG;

	if(in_place == nullptr || to_be_added == nullptr)
	    throw SRC_BUG;

	if(action == EA_ask)
	    action = op_tools_crit_ask_user_for_FSA_action(get_ui(), spot, in_place, to_be_added);


	    // modifying the FSA action when the in place inode has not FSA

	if(in_place->fsa_get_saved_status() != fsa_saved_status::full) // no EA in filesystem
	{
	    if(action == EA_merge_preserve || action == EA_merge_overwrite)
		action = EA_overwrite; // merging when in_place has no EA is equivalent to overwriting
	}

	switch(action)
	{
	case EA_preserve:
	case EA_preserve_mark_already_saved:
		// nothing to do
	    ret = false;
	    break;
	case EA_overwrite:
	case EA_overwrite_mark_already_saved:
	    if(tba_ino->fsa_get_saved_status() != fsa_saved_status::full)
		throw SRC_BUG;
	    if(warn_overwrite)
	    {
		try
		{
		    get_ui().pause(tools_printf(gettext("FSA for %S are about to be overwritten, OK?"), &spot));
		}
		catch(Euser_abort & e)
		{
		    const cat_directory *tba_dir = dynamic_cast<const cat_directory *>(to_be_added);
		    if(tba_dir != nullptr && tba_ino->same_as(*in_place))
			return false;
		    else
			throw;
		}
	    }

	    if(tba_mir != nullptr && known_etiquette(tba_mir->get_etiquette()))
	    {
		if(info_details)
		    get_ui().printf(gettext("FSA for %S have not been overwritten because this file is a hard link pointing to an already restored inode"), &spot);
		ret = false;
	    }
	    else
	    {
		if(info_details)
		    get_ui().message(string(gettext("Restoring file's FSA: ")) + spot);

		if(!empty)
		{
		    const filesystem_specific_attribute_list * fsa = tba_ino->get_fsa();
		    if(fsa == nullptr)
			throw SRC_BUG;

		    ret = fsa->set_fsa_to_filesystem_for(spot, get_fsa_scope(), get_ui(), false);
		}
		else
		    ret = true;
	    }
	    break;
	case EA_clear:
	    break;
	case EA_merge_preserve:
	case EA_merge_overwrite:
	    if(in_place->fsa_get_saved_status() != fsa_saved_status::full)
		throw SRC_BUG; // should have been redirected to EA_overwrite !

	    if(warn_overwrite)
	    {
		try
		{
		    get_ui().pause(tools_printf(gettext("FSA for %S are about to be overwritten, OK?"), &spot));
		}
		catch(Euser_abort & e)
		{
		    return false;
		}
	    }

	    if(tba_ino->fsa_get_saved_status() == fsa_saved_status::full)
	    {
		const filesystem_specific_attribute_list *tba_fsa = tba_ino->get_fsa();
		const filesystem_specific_attribute_list *ip_fsa = in_place->get_fsa();
		filesystem_specific_attribute_list result;

		if(action == EA_merge_preserve)
		    result = *tba_fsa + *ip_fsa;
		else // action == EA_merge_overwrite
		    result = *ip_fsa + *tba_fsa; // the + operator on FSA is not reflexive !!!

		if(!empty)
		    ret = result.set_fsa_to_filesystem_for(spot, get_fsa_scope(), get_ui(), false);
		else
		    ret = true;
	    }
	    break;
	case EA_undefined:
	    throw Erange("filesystem_restore::action_over_detruit", tools_printf(gettext("%S: Overwriting policy (FSA) is undefined for that file, do not know whether overwriting is allowed or not!"), &spot));
	case EA_ask:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}

	return ret;
    }

    void filesystem_restore::detruire()
    {
        if(fs_root != nullptr)
	{
            delete fs_root;
	    fs_root = nullptr;
	}
        if(current_dir != nullptr)
	{
            delete current_dir;
	    current_dir = nullptr;
	}
	if(ea_mask != nullptr)
	{
	    delete ea_mask;
	    ea_mask = nullptr;
	}
	if(overwrite != nullptr)
	{
	    delete overwrite;
	    overwrite = nullptr;
	}
    }

    void filesystem_restore::restore_stack_dir_ownership()
    {
	string tmp;

	while(!stack_dir.empty() && current_dir->pop(tmp))
	{
	    string chem = (current_dir->append(stack_dir.back().get_name())).display();
	    if(!empty)
		filesystem_tools_make_owner_perm(get_ui(), stack_dir.back(), chem, what_to_check, get_fsa_scope());
	    stack_dir.pop_back();
	}
	if(stack_dir.size() > 0)
	    throw SRC_BUG;
    }


} // end of namespace
