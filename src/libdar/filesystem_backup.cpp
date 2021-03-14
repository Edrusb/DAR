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

#include "filesystem_backup.hpp"
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

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 104
#endif

using namespace std;

namespace libdar
{

    filesystem_backup::filesystem_backup(const std::shared_ptr<user_interaction> & dialog,
					 const path &root,
					 bool x_info_details,
					 const mask & x_ea_mask,
					 bool check_no_dump_flag,
					 bool x_alter_atime,
					 bool x_furtive_read_mode,
					 bool x_cache_directory_tagging,
					 infinint & root_fs_device,
					 bool x_ignore_unknown,
					 const fsa_scope & scope):
	filesystem_hard_link_read(dialog, x_furtive_read_mode, scope)
    {
	fs_root = nullptr;
	current_dir = nullptr;
	ea_mask = nullptr;
	try
	{
	    fs_root = filesystem_tools_get_root_with_symlink(*dialog, root, x_info_details);
	    if(fs_root == nullptr)
		throw Ememory("filesystem_backup::filesystem_backup");
	    info_details = x_info_details;
	    no_dump_check = check_no_dump_flag;
	    alter_atime = x_alter_atime;
	    furtive_read_mode = x_furtive_read_mode;
	    cache_directory_tagging = x_cache_directory_tagging;
	    current_dir = nullptr;
	    ignore_unknown = x_ignore_unknown;
	    ea_mask = x_ea_mask.clone();
	    if(ea_mask == nullptr)
		throw Ememory("filesystem_backup::filesystem_backup");
	    reset_read(root_fs_device);
	}
	catch(...)
	{
	    detruire();
	    throw;
	}
    }

    void filesystem_backup::detruire()
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

    void filesystem_backup::reset_read(infinint & root_fs_device)
    {
        corres_reset();
        if(current_dir != nullptr)
            delete current_dir;
        current_dir = new (nothrow) path(*fs_root);
        if(current_dir == nullptr)
            throw Ememory("filesystem_backup::reset_read");
        pile.clear();

	const string display = current_dir->display();
	const char* tmp = display.c_str();
	cat_entree *ref = make_read_entree(*current_dir, "", true, *ea_mask);
	cat_directory *ref_dir = dynamic_cast<cat_directory *>(ref);

	try
	{
	    if(ref_dir != nullptr)
	    {
		pile.push_back(etage(get_ui(), tmp, ref_dir->get_last_access(), ref_dir->get_last_modif(), cache_directory_tagging, furtive_read_mode));
		root_fs_device = ref_dir->get_device();
	    }
	    else
		if(ref == nullptr)
		    throw Erange("filesystem_backup::reset_read", string(gettext("Non existent file: ")) + tmp);
		else
		    throw Erange("filesystem_backup::reset_read", string(gettext("File must be a directory: "))+ tmp);

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


    bool filesystem_backup::read(cat_entree * & ref, infinint & errors, infinint & skipped_dump)
    {
        bool once_again;
        ref = nullptr;
	errors = 0;
	skipped_dump = 0;


        if(current_dir == nullptr)
            throw SRC_BUG; // constructor not called or badly implemented.

        do
        {
            once_again = false;

            if(pile.empty())
                return false; // end of filesystem reading
            else // at least one directory to read
            {
                etage & inner = pile.back();
                string name;

                if(!inner.read(name))
                {
                    string tmp;

		    if(!alter_atime && !furtive_read_mode)
			tools_noexcept_make_date(current_dir->display(), false, inner.last_acc, inner.last_mod, inner.last_mod);
                    pile.pop_back();
                    if(pile.empty())
                        return false; // end of filesystem
		    else
		    {
                        if(! current_dir->pop(tmp))
                            throw SRC_BUG;
                        ref = new (nothrow) cat_eod();
                    }
                }
                else // could read a filename in directory
                {
                    try
                    {
                            // checking the EXT2 nodump flag (if set ignoring the file)

                        if(!no_dump_check || !filesystem_tools_is_nodump_flag_set(get_ui(), *current_dir, name, info_details))
                        {
			    ref = make_read_entree(*current_dir, name, true, *ea_mask);

			    try
			    {
				cat_directory *ref_dir = dynamic_cast<cat_directory *>(ref);

				if(ref_dir != nullptr)
				{
				    *current_dir += name;
				    const string display = current_dir->display();
				    const char* ptr_name = display.c_str();

				    try
				    {
					pile.push_back(etage(get_ui(),
							     ptr_name,
							     ref_dir->get_last_access(),
							     ref_dir->get_last_modif(),
							     cache_directory_tagging,
							     furtive_read_mode));
				    }
				    catch(Egeneric & e)
				    {
					string tmp;

					get_ui().message(tools_printf(gettext("Cannot read directory contents: %s : "), ptr_name) + e.get_message());

					try
					{
					    pile.push_back(etage());
					}
					catch(Egeneric & e)
					{
					    delete ref;
					    ref = nullptr; // we ignore this directory and skip to the next entry
					    errors++;
					    if(! current_dir->pop(tmp))
						throw SRC_BUG;
					}
				    }
				}

				if(ref == nullptr)
				    once_again = true;
				    // the file has been removed between the time
				    // the directory has been openned, and the time
				    // we try to read it, so we ignore it.
				    // this case is silently ignored and not counted
			    }
			    catch(...)
			    {
				if(ref != nullptr)
				{
				    delete ref;
				    ref = nullptr;
				}
				throw;
			    }
                        }
                        else // EXT2 nodump flag is set, and we must not consider such file for backup
                        {
                            if(info_details)
                                get_ui().message(string(gettext("Ignoring file with NODUMP flag set: ")) + (current_dir->append(name)).display());
			    skipped_dump++;
                            once_again = true;
                        }
                    }
		    catch(Edata & e)
		    {
			if(!ignore_unknown)
			    get_ui().message(string(gettext("Error reading directory contents: ")) + e.get_message() + gettext(" . Ignoring file or directory"));
                        once_again = true;
		    }
                    catch(Erange & e)
                    {
			get_ui().message(string(gettext("Error reading directory contents: ")) + e.get_message() + gettext(" . Ignoring file or directory"));
                        once_again = true;
			errors++;
                    }
                }
            }
        }
        while(once_again);

        if(ref == nullptr)
            throw Ememory("filesystem_backup::read");
        else
            return true;
    }

    void filesystem_backup::skip_read_to_parent_dir()
    {
        string tmp;

        if(pile.empty())
            throw SRC_BUG;
        else
        {
	    if(!alter_atime && !furtive_read_mode)
		tools_noexcept_make_date(current_dir->display(), false, pile.back().last_acc, pile.back().last_mod, pile.back().last_mod);
            pile.pop_back();
        }

        if(! current_dir->pop(tmp))
            throw SRC_BUG;
    }

} // end of namespace
