/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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

#include "filesystem_diff.hpp"
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

using namespace std;

namespace libdar
{

    filesystem_diff::filesystem_diff(const std::shared_ptr<user_interaction> & dialog,
				     const path &root,
				     bool x_info_details,
				     const mask & x_ea_mask,
				     bool x_alter_atime,
				     bool x_furtive_read_mode,
				     const fsa_scope & scope):
	filesystem_hard_link_read(dialog, x_furtive_read_mode, scope)
    {
	fs_root = nullptr;
	ea_mask = nullptr;
	current_dir = nullptr;
	try
	{
	    fs_root = filesystem_tools_get_root_with_symlink(*dialog, root, x_info_details);
	    if(fs_root == nullptr)
		throw Ememory("filesystem_diff::filesystem_diff");
	    info_details = x_info_details;
	    ea_mask = x_ea_mask.clone();
	    if(ea_mask == nullptr)
		throw Ememory("filesystem_diff::filesystem_diff");
	    alter_atime = x_alter_atime;
	    furtive_read_mode = x_furtive_read_mode;
	    current_dir = nullptr;
	    reset_read();
	}
	catch(...)
	{
	    detruire();
	    throw;
	}
	zeroing_negative_dates_without_asking(); // when reading existing inode from filesystem
    }


    void filesystem_diff::reset_read()
    {
        corres_reset();
        if(current_dir != nullptr)
            delete current_dir;
        current_dir = new (nothrow) path(*fs_root);
        filename_pile.clear();
        if(current_dir == nullptr)
            throw Ememory("filesystem_diff::reset_read");
	const string display = current_dir->display();
	const char* tmp = display.c_str();

	cat_entree *ref = make_read_entree(*current_dir, "", true, *ea_mask);
	cat_directory *ref_dir = dynamic_cast<cat_directory *>(ref);
	try
	{
	    if(ref_dir != nullptr)
	    {
		filename_struct rfst;

		rfst.last_acc = ref_dir->get_last_access();
		rfst.last_mod = ref_dir->get_last_modif();
		filename_pile.push_back(rfst);
	    }
	    else
		if(ref == nullptr)
		    throw Erange("filesystem_diff::reset_read", string(gettext("Non existent file: ")) + tmp);
		else
		    throw Erange("filesystem_diff::reset_read", string(gettext("File must be a directory: ")) + tmp);
	}
	catch(...)
	{
	    if(ref != nullptr)
		delete ref;
	    throw;
	}
	if(ref != nullptr)
	    delete ref;
    }

    bool filesystem_diff::read_filename(const string & name, cat_nomme * &ref)
    {
        cat_directory *ref_dir = nullptr;
        if(current_dir == nullptr)
            throw SRC_BUG;
        ref = make_read_entree(*current_dir, name, false, *ea_mask);
        if(ref == nullptr)
            return false; // no file of that name
        else
        {
            ref_dir = dynamic_cast<cat_directory *>(ref);
            if(ref_dir != nullptr)
            {
                filename_struct rfst;

                rfst.last_acc = ref_dir->get_last_access();
                rfst.last_mod = ref_dir->get_last_modif();
                filename_pile.push_back(rfst);
                *current_dir += ref_dir->get_name();
            }
            return true;
        }
    }

    void filesystem_diff::skip_read_filename_in_parent_dir()
    {
        if(filename_pile.empty())
            throw SRC_BUG;

	string tmp;
	if(!alter_atime && !furtive_read_mode)
	    tools_noexcept_make_date(current_dir->display(), false, filename_pile.back().last_acc, filename_pile.back().last_mod, filename_pile.back().last_mod);
	filename_pile.pop_back();
	current_dir->pop(tmp);
    }

    void filesystem_diff::detruire()
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
    }

} // end of namespace
