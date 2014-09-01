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

#if HAVE_SYS_TYPE_H
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

#include "filesystem.hpp"
#include "tools.hpp"
#include "erreurs.hpp"
#include "user_interaction.hpp"
#include "cat_all_entrees.hpp"
#include "ea_filesystem.hpp"
#include "cygwin_adapt.hpp"
#include "fichier_local.hpp"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 104
#endif

using namespace std;

namespace libdar
{

    static void supprime(user_interaction & ui, const string & ref);
    static void make_owner_perm(user_interaction & dialog,
				const cat_inode & ref,
				const string & chem,
				bool dir_perm,
				cat_inode::comparison_fields what_to_check,
				const fsa_scope & scope);
    static void make_date(const cat_inode & ref,
			  const string & chem,
			  cat_inode::comparison_fields what_to_check,
			  const fsa_scope & scope);

    static void attach_ea(const string &chemin,
			  cat_inode *ino,
			  const mask & ea_mask,
			  memory_pool *pool);
    static bool is_nodump_flag_set(user_interaction & dialog,
				   const path & chem, const string & filename,
				   bool info);
    static path *get_root_with_symlink(user_interaction & dialog,
				       const path & root,
				       bool info_details,
				       memory_pool *pool);
    static mode_t get_file_permission(const string & path);

///////////////////////////////////////////////////////////////////
///////////////// filesystem_hard_link_read methods ///////////////
///////////////////////////////////////////////////////////////////

    cat_nomme *filesystem_hard_link_read::make_read_entree(path & lieu, const string & name, bool see_hard_link, const mask & ea_mask)
    {
	const string display = name.empty() ? lieu.display() : (lieu + path(name)).display();
        const char *ptr_name = display.c_str();
        cat_nomme *ref = NULL;
	struct stat buf;
	string tmp;

	try
	{

	    if(lstat(ptr_name, &buf) < 0)
	    {
		switch(errno)
		{
		case EACCES:
		    tmp = tools_strerror_r(errno);
		    get_ui().warning(tools_printf(gettext("Error reading inode of file %s : %s"), ptr_name, tmp.c_str()));
		    break;
		case ENOENT:
		    break;
		default:
		    throw Erange("filesystem_hard_link_read::make_read_entree", string(gettext("Cannot read inode for ")) + ptr_name + " : " + tools_strerror_r(errno));
		}

		    // the current method returns NULL (= ref)  (meaning file does not exists)
	    }
	    else
	    {
#ifdef LIBDAR_MICROSECOND_READ_ACCURACY
		datetime atime = datetime(buf.st_atim.tv_sec, buf.st_atim.tv_nsec, datetime::tu_nanosecond);
		datetime mtime = datetime(buf.st_mtim.tv_sec, buf.st_mtim.tv_nsec, datetime::tu_nanosecond);
		datetime ctime = datetime(buf.st_ctim.tv_sec, buf.st_ctim.tv_nsec, datetime::tu_nanosecond);

		if(atime.is_null())
		    atime = datetime(buf.st_atime, 0, datetime::tu_second);
		if(mtime.is_null())
		    mtime = datetime(buf.st_mtime, 0, datetime::tu_second);
		if(ctime.is_null())
		    ctime = datetime(buf.st_ctime, 0, datetime::tu_second);
#else
		datetime atime = datetime(buf.st_atime, 0, datetime::tu_second);
		datetime mtime = datetime(buf.st_mtime, 0, datetime::tu_second);
		datetime ctime = datetime(buf.st_ctime, 0, datetime::tu_second);
#endif

		if(S_ISLNK(buf.st_mode))
		{
		    string pointed = tools_readlink(ptr_name);

		    ref = new (get_pool()) cat_lien(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
						atime,
						mtime,
						ctime,
						name,
						pointed,
						buf.st_dev);
		}
		else if(S_ISREG(buf.st_mode))
		    ref = new (get_pool()) cat_file(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
						atime,
						mtime,
						ctime,
						name,
						lieu,
						buf.st_size,
						buf.st_dev,
						furtive_read_mode);
		else if(S_ISDIR(buf.st_mode))
		    ref = new (get_pool()) cat_directory(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
						     atime,
						     mtime,
						     ctime,
						     name,
						     buf.st_dev);
		else if(S_ISCHR(buf.st_mode))
		    ref = new (get_pool()) cat_chardev(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
						   atime,
						   mtime,
						   ctime,
						   name,
						   major(buf.st_rdev),
						   minor(buf.st_rdev), // makedev(major, minor)
						   buf.st_dev);
		else if(S_ISBLK(buf.st_mode))
		    ref = new (get_pool()) cat_blockdev(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
						    atime,
						    mtime,
						    ctime,
						    name,
						    major(buf.st_rdev),
						    minor(buf.st_rdev), // makedev(major, minor)
						    buf.st_dev);
		else if(S_ISFIFO(buf.st_mode))
		    ref = new (get_pool()) cat_tube(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
						atime,
						mtime,
						ctime,
						name,
						buf.st_dev);
		else if(S_ISSOCK(buf.st_mode))
		    ref = new (get_pool()) cat_prise(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
						 atime,
						 mtime,
						 ctime,
						 name,
						 buf.st_dev);
#if HAVE_DOOR
		else if(S_ISDOOR(buf.st_mode))
		    ref = new (get_pool()) door(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
						atime,
						mtime,
						ctime,
						name,
						lieu,
						buf.st_dev);
#endif
		else
		    throw Edata(string(gettext("Unknown file type! file name is: ")) + string(ptr_name));


		cat_inode *ino = dynamic_cast<cat_inode *>(ref);
		if(ino != NULL)
		{

			//
			// Extended Attributes Considerations
			//

		    try
		    {
			attach_ea(ptr_name, ino, ea_mask, get_pool());
		    }
		    catch(Ebug & e)
		    {
			throw;
		    }
		    catch(Euser_abort & e)
		    {
			throw;
		    }
		    catch(Ethread_cancel & e)
		    {
			throw;
		    }
		    catch(Ememory & e)
		    {
			throw;
		    }
		    catch(Egeneric & ex)
		    {
			get_ui().warning(string(gettext("Error reading EA for "))+ptr_name+ " : " + ex.get_message());
			    // no throw !
			    // we must be able to continue without EA
		    }

		    //
		    // Filesystem Specific Attributes Considerations
		    //

		    filesystem_specific_attribute_list *fsal = new (get_pool()) filesystem_specific_attribute_list();
		    if(fsal == NULL)
			throw Ememory("filesystem_hard_link_read::make_entree");
		    try
		    {
			fsal->get_fsa_from_filesystem_for(display, sc, buf.st_mode);
			if(!fsal->empty())
			{
			    ino->fsa_set_saved_status(cat_inode::fsa_full);
			    ino->fsa_attach(fsal);
			    fsal = NULL; // now managed by *ino
			}
			else
			{
			    ino->fsa_set_saved_status(cat_inode::fsa_none);
			    delete fsal;
			    fsal = NULL;
			}
		    }
		    catch(...)
		    {
			if(fsal != NULL)
			    delete fsal;
			throw;
		    }
		}

		    //
		    // hard link detection
		    //

		if(ref == NULL)
		    throw Ememory("filesystem_hard_link_read::make_read_entree");

		if(buf.st_nlink > 1 && see_hard_link && dynamic_cast<cat_directory *>(ref) == NULL)
		{
		    map<node, couple>::iterator it = corres_read.find(node(buf.st_ino, buf.st_dev));

		    if(it == corres_read.end()) // inode not yet seen, creating the etoile object
		    {
			cat_inode *ino_ref = dynamic_cast<cat_inode *>(ref);
			etoile *tmp_et = NULL;

			if(ino_ref == NULL)
			    throw SRC_BUG;
			tmp_et = new (get_pool()) etoile(ino_ref, etiquette_counter++);
			if(tmp_et == NULL)
			    throw Ememory("filesystem_hard_link_read::make_read_entree");
			try
			{
			    ref = NULL; // the object pointed to by ref is now managed by tmp_et
			    couple tmp = couple(tmp_et, buf.st_nlink - 1);
			    pair <node, couple> p_tmp(node(buf.st_ino, buf.st_dev), tmp);
			    corres_read.insert(p_tmp);
			    it = corres_read.find(node(buf.st_ino,buf.st_dev));
			    if(it == corres_read.end())
				throw SRC_BUG; // the addition of the entry to the map failed !!!
			    else
				it->second.obj->get_inode()->change_name("");
				// name of inode attached to an etoile is not used so we don't want to waste space in this field.
			}
			catch(...)
			{
			    if(tmp_et != NULL)
				delete tmp_et;
			    throw;
			}

			ref = new (get_pool()) cat_mirage(name, tmp_et);
		    }
		    else // inode already seen creating a new cat_mirage on the given etoile
		    {
			    // some sanity checks
			if(it->second.obj == NULL)
			    throw SRC_BUG;

			if(ref != NULL)
			    delete ref;  // we don't need this just created inode as it is already attached to the etoile object
			ref = new (get_pool()) cat_mirage(name, it->second.obj);
			if(ref != NULL)
			{
			    it->second.count--;
			    if(it->second.count == 0)
				corres_read.erase(it);
				// this deletes the couple entry, implying the release of memory used by the holder object, but the etoile will only be destroyed once its internal counter drops to zero
			}
		    }
		}

		if(ref == NULL)
		    throw Ememory("filesystem_hard_link_read::make_read_entree");
	    }
	}
	catch(...)
	{
	    if(ref != NULL)
	    {
		delete ref;
		ref = NULL;
	    }
	    throw;
	}

        return ref;
    }


///////////////////////////////////////////////////////////////////
///////////////// filesystem_backup methods ///////////////////////
///////////////////////////////////////////////////////////////////


    filesystem_backup::filesystem_backup(user_interaction & dialog,
					 const path &root,
					 bool x_info_details,
					 const mask & x_ea_mask,
					 bool check_no_dump_flag,
					 bool x_alter_atime,
					 bool x_furtive_read_mode,
					 bool x_cache_directory_tagging,
					 infinint & root_fs_device,
					 bool x_ignore_unknown,
					 const fsa_scope & scope)
	: mem_ui(dialog), filesystem_hard_link_read(dialog, x_furtive_read_mode, scope)
    {
	fs_root = NULL;
	current_dir = NULL;
	ea_mask = NULL;
	try
	{
	    fs_root = get_root_with_symlink(get_ui(), root, x_info_details, get_pool());
	    if(fs_root == NULL)
		throw Ememory("filesystem_backup::filesystem_backup");
	    info_details = x_info_details;
	    no_dump_check = check_no_dump_flag;
	    alter_atime = x_alter_atime;
	    furtive_read_mode = x_furtive_read_mode;
	    cache_directory_tagging = x_cache_directory_tagging;
	    current_dir = NULL;
	    ignore_unknown = x_ignore_unknown;
	    ea_mask = x_ea_mask.clone();
	    if(ea_mask == NULL)
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
        if(fs_root != NULL)
        {
            delete fs_root;
            fs_root = NULL;
        }
        if(current_dir != NULL)
	{
            delete current_dir;
	    current_dir = NULL;
	}
	if(ea_mask != NULL)
	{
	    delete ea_mask;
	    ea_mask = NULL;
	}
    }

    void filesystem_backup::copy_from(const filesystem_backup & ref)
    {
	const filesystem_hard_link_read *proto_ref = &ref;
	filesystem_hard_link_read *proto_me = this;
	*proto_me = *proto_ref;  // invoke the copy from parent class

	fs_root = NULL;
	current_dir = NULL;
	ea_mask = NULL;
	try
	{
	    if(ref.fs_root != NULL)
	    {
		fs_root = new (get_pool()) path(*ref.fs_root);
		if(fs_root == NULL)
		    throw Ememory("filesystem_backup::copy_from");
	    }
	    else
		fs_root = NULL;

	    if(ref.current_dir != NULL)
	    {
		current_dir = new (get_pool()) path(*ref.current_dir);
		if(current_dir == NULL)
		    throw Ememory("filesystem_backup::copy_from");
	    }
	    else
		current_dir = NULL;
	    info_details = ref.info_details;
	    ea_mask = ref.ea_mask->clone();
	    if(ea_mask == NULL)
		throw Ememory("filesystem_backup::copy_from");
	    no_dump_check = ref.no_dump_check;
	    alter_atime = ref.alter_atime;
	    furtive_read_mode = ref.furtive_read_mode;
	    cache_directory_tagging = ref.cache_directory_tagging;
	    pile = ref.pile;
	    ignore_unknown = ref.ignore_unknown;
	}
	catch(...)
	{
	    detruire();
	    throw;
	}
    }

    void filesystem_backup::reset_read(infinint & root_fs_device)
    {
        corres_reset();
        if(current_dir != NULL)
            delete current_dir;
        current_dir = new (get_pool()) path(*fs_root);
        if(current_dir == NULL)
            throw Ememory("filesystem_backup::reset_read");
        pile.clear();

	const string display = current_dir->display();
	const char* tmp = display.c_str();
	cat_entree *ref = make_read_entree(*current_dir, "", true, *ea_mask);
	cat_directory *ref_dir = dynamic_cast<cat_directory *>(ref);

	try
	{
	    if(ref_dir != NULL)
	    {
		pile.push_back(etage(get_ui(), tmp, ref_dir->get_last_access(), ref_dir->get_last_modif(), cache_directory_tagging, furtive_read_mode));
		root_fs_device = ref_dir->get_device();
	    }
	    else
		if(ref == NULL)
		    throw Erange("filesystem_backup::reset_read", string(gettext("Non existent file: ")) + tmp);
		else
		    throw Erange("filesystem_backup::reset_read", string(gettext("File must be a directory: "))+ tmp);

	}
	catch(...)
	{
	    if(ref != NULL)
		delete ref;
	    throw;
	}
	if(ref != NULL)
	    delete ref;
    }


    bool filesystem_backup::read(cat_entree * & ref, infinint & errors, infinint & skipped_dump)
    {
        bool once_again;
        ref = NULL;
	errors = 0;
	skipped_dump = 0;


        if(current_dir == NULL)
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
                        ref = new (get_pool()) cat_eod();
                    }
                }
                else // could read a filename in directory
                {
                    try
                    {
                            // checking the EXT2 nodump flag (if set ignoring the file)

                        if(!no_dump_check || !is_nodump_flag_set(get_ui(), *current_dir, name, info_details))
                        {
			    ref = make_read_entree(*current_dir, name, true, *ea_mask);

			    try
			    {
				cat_directory *ref_dir = dynamic_cast<cat_directory *>(ref);

				if(ref_dir != NULL)
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

					get_ui().warning(tools_printf(gettext("Cannot read directory contents: %s : "), ptr_name) + e.get_message());

					try
					{
					    pile.push_back(etage());
					}
					catch(Egeneric & e)
					{
					    delete ref;
					    ref = NULL; // we ignore this directory and skip to the next entry
					    errors++;
					    if(! current_dir->pop(tmp))
						throw SRC_BUG;
					}
				    }
				}

				if(ref == NULL)
				    once_again = true;
				    // the file has been removed between the time
				    // the directory has been openned, and the time
				    // we try to read it, so we ignore it.
				    // this case is silently ignored and not counted
			    }
			    catch(...)
			    {
				if(ref != NULL)
				{
				    delete ref;
				    ref = NULL;
				}
				throw;
			    }
                        }
                        else // EXT2 nodump flag is set, and we must not consider such file for backup
                        {
                            if(info_details)
                                get_ui().warning(string(gettext("Ignoring file with NODUMP flag set: ")) + (*current_dir + name).display());
			    skipped_dump++;
                            once_again = true;
                        }
                    }
		    catch(Edata & e)
		    {
			if(!ignore_unknown)
			    get_ui().warning(string(gettext("Error reading directory contents: ")) + e.get_message() + gettext(" . Ignoring file or directory"));
                        once_again = true;
		    }
                    catch(Erange & e)
                    {
			get_ui().warning(string(gettext("Error reading directory contents: ")) + e.get_message() + gettext(" . Ignoring file or directory"));
                        once_again = true;
			errors++;
                    }
                }
            }
        }
        while(once_again);

        if(ref == NULL)
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


///////////////////////////////////////////////////////////////////
////////////////// filesystem_diff methods  ///////////////////////
///////////////////////////////////////////////////////////////////


    filesystem_diff::filesystem_diff(user_interaction & dialog,
				     const path &root,
				     bool x_info_details,
				     const mask & x_ea_mask,
				     bool x_alter_atime,
				     bool x_furtive_read_mode,
				     const fsa_scope & scope) :
	mem_ui(dialog),
	filesystem_hard_link_read(dialog, x_furtive_read_mode, scope)
    {
	fs_root = NULL;
	ea_mask = NULL;
	current_dir = NULL;
	try
	{
	    fs_root = get_root_with_symlink(get_ui(), root, x_info_details, get_pool());
	    if(fs_root == NULL)
		throw Ememory("filesystem_diff::filesystem_diff");
	    info_details = x_info_details;
	    ea_mask = x_ea_mask.clone();
	    if(ea_mask == NULL)
		throw Ememory("filesystem_diff::filesystem_diff");
	    alter_atime = x_alter_atime;
	    furtive_read_mode = x_furtive_read_mode;
	    current_dir = NULL;
	    reset_read();
	}
	catch(...)
	{
	    detruire();
	    throw;
	}
    }


    void filesystem_diff::reset_read()
    {
        corres_reset();
        if(current_dir != NULL)
            delete current_dir;
        current_dir = new (get_pool()) path(*fs_root);
        filename_pile.clear();
        if(current_dir == NULL)
            throw Ememory("filesystem_diff::reset_read");
	const string display = current_dir->display();
	const char* tmp = display.c_str();

	cat_entree *ref = make_read_entree(*current_dir, "", true, *ea_mask);
	cat_directory *ref_dir = dynamic_cast<cat_directory *>(ref);
	try
	{
	    if(ref_dir != NULL)
	    {
		filename_struct rfst;

		rfst.last_acc = ref_dir->get_last_access();
		rfst.last_mod = ref_dir->get_last_modif();
		filename_pile.push_back(rfst);
	    }
	    else
		if(ref == NULL)
		    throw Erange("filesystem_diff::reset_read", string(gettext("Non existent file: ")) + tmp);
		else
		    throw Erange("filesystem_diff::reset_read", string(gettext("File must be a directory: ")) + tmp);
	}
	catch(...)
	{
	    if(ref != NULL)
		delete ref;
	    throw;
	}
	if(ref != NULL)
	    delete ref;
    }

    bool filesystem_diff::read_filename(const string & name, cat_nomme * &ref)
    {
        cat_directory *ref_dir = NULL;
        if(current_dir == NULL)
            throw SRC_BUG;
        ref = make_read_entree(*current_dir, name, false, *ea_mask);
        if(ref == NULL)
            return false; // no file of that name
        else
        {
            ref_dir = dynamic_cast<cat_directory *>(ref);
            if(ref_dir != NULL)
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
        if(fs_root != NULL)
	{
            delete fs_root;
	    fs_root = NULL;
	}
        if(current_dir != NULL)
	{
            delete current_dir;
	    current_dir = NULL;
	}
	if(ea_mask != NULL)
	{
	    delete ea_mask;
	    ea_mask = NULL;
	}
    }

    void filesystem_diff::copy_from(const filesystem_diff & ref)
    {
	fs_root = NULL;
	ea_mask = NULL;
	current_dir = NULL;
	try
	{
	    const filesystem_hard_link_read *proto_ref = &ref;
	    filesystem_hard_link_read *proto_me = this;
	    *proto_me = *proto_ref;
	    if(ref.fs_root != NULL)
	    {
		fs_root = new (get_pool()) path(*ref.fs_root);
		if(fs_root == NULL)
		    throw Ememory("filesystem_diff::copy_from");
	    }
	    else
		fs_root = NULL;
	    if(ref.current_dir != NULL)
	    {
		current_dir = new (get_pool()) path(*ref.current_dir);
		if(current_dir == NULL)
		    throw Ememory("filesystem_diff::copy_from");
	    }
	    else
		current_dir = NULL;
	    info_details = ref.info_details;
	    ea_mask = ref.ea_mask->clone();
	    if(ea_mask == NULL)
		throw Ememory("filesystem_diff::copy_from");
	    alter_atime = ref.alter_atime;
	    furtive_read_mode = ref.furtive_read_mode;
	    filename_pile = ref.filename_pile;
	}
	catch(...)
	{
	    detruire();
	    throw;
	}
    }



///////////////////////////////////////////////////////////////////
////////////////// filesystem_hard_link_write methods  ////////////
///////////////////////////////////////////////////////////////////


    bool filesystem_hard_link_write::raw_set_ea(const cat_nomme *e,
						const ea_attributs & list_ea,
						const string & spot,
						const mask & ea_mask)
    {
        const cat_mirage *e_mir = dynamic_cast<const cat_mirage *>(e);

        bool ret = false;

        try
        {
            if(e == NULL)
                throw SRC_BUG;

                // checking that we have not already restored the EA of this
                // inode through another hard link
            if(e_mir != NULL)
            {
                map<infinint, corres_ino_ea>::iterator it;

                it = corres_write.find(e_mir->get_etiquette());
                if(it == corres_write.end())
                {
                        // inode never restored; (no data saved just EA)
                        // we must record it
                    corres_ino_ea tmp;
                    tmp.chemin = spot;
                    tmp.ea_restored = true;
                    corres_write[e_mir->get_etiquette()] = tmp;
                }
                else
                    if(it->second.ea_restored)
                        return false; // inode already restored
                    else
                        it->second.ea_restored = true;
            }

                // restoring Extended Attributes
                //
	    (void)ea_filesystem_write_ea(spot, list_ea, ea_mask);
	    ret = true;
        }
        catch(Euser_abort & e)
        {
            ret = false;
        }

        return ret;
    }

    bool filesystem_hard_link_write::raw_clear_ea_set(const cat_nomme *e, const string & spot)
    {
        const cat_mirage *e_mir = dynamic_cast<const cat_mirage *>(e);

        bool ret = false;

        try
        {
            if(e == NULL)
                throw SRC_BUG;

                // checking that we have not already restored the EA of this
                // inode through another hard link
            if(e_mir != NULL)
            {
                map<infinint, corres_ino_ea>::iterator it;

                it = corres_write.find(e_mir->get_etiquette());
                if(it == corres_write.end())
                {
                        // inode never restored; (no data saved just EA)
                        // we must record it
                    corres_ino_ea tmp;
                    tmp.chemin = spot;
                    tmp.ea_restored = false;   // clearing the EA does not mean we have restored them, setting here "false" let raw_set_ea() being call afterward
                    corres_write[e_mir->get_etiquette()] = tmp;
                }
                else // entry found
                    if(it->second.ea_restored)
                        return false; // inode already restored
            }


                // Clearing all EA
                //
	    ea_filesystem_clear_ea(spot, bool_mask(true), get_pool());
	    ret = true;
        }
        catch(Euser_abort & e)
        {
            ret = false;
        }

        return ret;
    }


    void filesystem_hard_link_write::write_hard_linked_target_if_not_set(const cat_mirage *ref, const string & chemin)
    {
        if(!known_etiquette(ref->get_etiquette()))
        {
            corres_ino_ea tmp;
            tmp.chemin = chemin;
            tmp.ea_restored = false; // if EA have to be restored next
            corres_write[ref->get_etiquette()] = tmp;
        }
    }

    bool filesystem_hard_link_write::known_etiquette(const infinint & eti)
    {
        return corres_write.find(eti) != corres_write.end();
    }

    void filesystem_hard_link_write::make_file(const cat_nomme * ref,
					       const path & ou,
					       bool dir_perm,
					       cat_inode::comparison_fields what_to_check,
					       const fsa_scope & scope)
    {
        const cat_directory *ref_dir = dynamic_cast<const cat_directory *>(ref);
        const cat_file *ref_fil = dynamic_cast<const cat_file *>(ref);
        const cat_lien *ref_lie = dynamic_cast<const cat_lien *>(ref);
        const cat_blockdev *ref_blo = dynamic_cast<const cat_blockdev *>(ref);
        const cat_chardev *ref_cha = dynamic_cast<const cat_chardev *>(ref);
        const cat_tube *ref_tub = dynamic_cast<const cat_tube *>(ref);
        const cat_prise *ref_pri = dynamic_cast<const cat_prise *>(ref);
        const cat_mirage *ref_mir = dynamic_cast <const cat_mirage *>(ref);
        const cat_inode *ref_ino = dynamic_cast <const cat_inode *>(ref);

        if(ref_ino == NULL && ref_mir == NULL)
            throw SRC_BUG; // neither an cat_inode nor a hard link

	const string display = (ou + ref->get_name()).display();
	const char *name = display.c_str();


	S_I ret = -1; // will carry the system call returned value used to create the requested file

	do
	{
	    try
	    {
		if(ref_mir != NULL) // we potentially have to make a hard link
		{
		    bool create_file = false;

		    map<infinint, corres_ino_ea>::iterator it = corres_write.find(ref_mir->get_etiquette());
		    if(it == corres_write.end()) // first time, we have to create the inode
			create_file = true;
		    else // the inode already exists, making hard link if possible
		    {
			const char *old = it->second.chemin.c_str();
			ret = link(old, name);
			if(ret < 0)
			{
			    string tmp;

			    switch(errno)
			    {
			    case EXDEV:  // crossing filesystem
			    case EPERM:  // filesystem does not support hard link creation
				    // can't make hard link, trying to duplicate the inode
				tmp = tools_strerror_r(errno);
				get_ui().warning(tools_printf(gettext("Error creating hard link %s : %s\n Trying to duplicate the inode"),
							      name, tmp.c_str()));
				create_file = true;
				clear_corres_if_pointing_to(ref_mir->get_etiquette(), old); // always succeeds as the etiquette points to "old"
				    // need to remove this entry to be able
				    // to restore EA for other copies
				break;
			    case ENOENT:  // path to the hard link to create does not exit
				if(ref_mir->get_inode()->get_saved_status() == s_saved)
				{
				    create_file = true;
				    clear_corres_if_pointing_to(ref_mir->get_etiquette(), old); // always succeeds as the etiquette points to "old"
					// need to remove this entry to be able
					// to restore EA for other copies
				    get_ui().warning(tools_printf(gettext("Error creating hard link : %s , the inode to link with [ %s ] has disappeared, re-creating it"),
								  name, old));

				}
				else
				{
				    create_file = false; // nothing to do;
				    get_ui().warning(tools_printf(gettext("Error creating hard link : %s , the inode to link with [ %s ] is not present, cannot restore this hard link"), name, old));
				}
				break;
			    default :
				    // nothing to do (ret < 0 and create_file == false)
				break;
			    }
			}
			else
			    create_file = false;
		    }

		    if(create_file)
		    {
			ref_ino = ref_mir->get_inode();
			ref_fil = dynamic_cast<const cat_file *>(ref_mir->get_inode());
			ref_lie = dynamic_cast<const cat_lien *>(ref_mir->get_inode());
			ref_blo = dynamic_cast<const cat_blockdev *>(ref_mir->get_inode());
			ref_cha = dynamic_cast<const cat_chardev *>(ref_mir->get_inode());
			ref_tub = dynamic_cast<const cat_tube *>(ref_mir->get_inode());
			ref_pri = dynamic_cast<const cat_prise *>(ref_mir->get_inode());
			ref_mir->get_inode()->change_name(ref_mir->get_name()); // we temporarily change the name of the attached inode (it is not used usually), by the name of the cat_mirage object
		    }
		    else // hard link made
			ret = 0; // not necessary, but avoids a warning from compilator ("ret" might be used uninitialized)
		}

		    // build plain inode object (or initial inode for hard link --- if create_file was true above)

		if(ref_dir != NULL)
		{
		    ret = mkdir(name, 0700); // as the directory has been created we are its owner and we will need only
			// to create files under it so we need all rights for user, by security for now, no right are
			// allowed for group and others, but this will be set properly at the end, when all files will
			// be restored in that directory
		}
		else if(ref_fil != NULL)
		{
		    generic_file *ou;
		    infinint seek;

		    fichier_local dest = fichier_local(get_ui(), display, gf_write_only, 0700, false, true, false);
			// telling to 'dest' to flush data from the cache as soon as possible
		    dest.fadvise(fichier_global::advise_dontneed);
			// the implicit destruction of dest (exiting the block)
			// will close the 'ret' file descriptor (see ~fichier_local())
		    ou = ref_fil->get_data(cat_file::normal);

		    try
		    {
			const crc *crc_ori = NULL;
			crc *crc_dyn = NULL;
			infinint crc_size;

			try
			{

			    if(!ref_fil->get_crc_size(crc_size))
				crc_size = tools_file_size_to_crc_size(ref_fil->get_size());

			    ou->skip(0);
			    ou->read_ahead(ref_fil->get_size());
			    ou->copy_to(dest, crc_size, crc_dyn);

			    if(crc_dyn == NULL)
				throw SRC_BUG;

			    if(ref_fil->get_crc(crc_ori))
			    {
				if(crc_ori == NULL)
				    throw SRC_BUG;
				if(typeid(*crc_dyn) != typeid(*crc_ori))
				    throw SRC_BUG;
				if(*crc_dyn != *crc_ori)
				    throw Erange("filesystem_hard_link_write::make_file", gettext("Bad CRC, data corruption occurred"));
				    // else nothing to do, nor to signal
			    }
				// else this is a very old archive
			}
			catch(...)
			{
			    if(crc_dyn != NULL)
				delete crc_dyn;
			    throw;
			}
			if(crc_dyn != NULL)
			    delete crc_dyn;
		    }
		    catch(...)
		    {
			if(ou != NULL)
			    delete ou;
			throw;
		    }
		    delete ou;
		    ret = 0; // to report a successful operation at the end of the if/else if chain
		}
		else if(ref_lie != NULL)
		    ret = symlink(ref_lie->get_target().c_str(), name);
		else if(ref_blo != NULL)
		    ret = mknod(name, S_IFBLK | 0700, makedev(ref_blo->get_major(), ref_blo->get_minor()));
		else if(ref_cha != NULL)
		    ret = mknod(name, S_IFCHR | 0700, makedev(ref_cha->get_major(), ref_cha->get_minor()));
		else if(ref_tub != NULL)
		    ret = mknod(name, S_IFIFO | 0700, 0);
		else if(ref_pri != NULL)
		{
		    ret = socket(PF_UNIX, SOCK_STREAM, 0);
		    if(ret >= 0)
		    {
			S_I sd = ret;
			struct sockaddr_un addr;
			addr.sun_family = AF_UNIX;

			try
			{
			    strncpy(addr.sun_path, name, UNIX_PATH_MAX - 1);
			    addr.sun_path[UNIX_PATH_MAX - 1] = '\0';
			    if(::bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
				throw Erange("filesystem_hard_link_write::make_file (socket bind)", string(gettext("Error creating Unix socket file: ")) + name + " : " + tools_strerror_r(errno));
			}
			catch(...)
			{
			    shutdown(sd, 2);
			    close(sd);
			    throw;
			}
			shutdown(sd, 2);
			close(sd);
		    }
		}
		else
		    if(ref_mir == NULL)
			throw SRC_BUG; // unknown inode type
		    // else nothing do do cat_mirage have been handled far above

		if(ret < 0)
		{
		    if(errno != ENOSPC)
			throw Erange("filesystem_hard_link_write::make_file", string(gettext("Could not create inode: ")) + name + " : " + tools_strerror_r(errno));
		    else
			get_ui().pause(string(gettext("Cannot create inode: ")) + tools_strerror_r(errno) + gettext(" Ready to continue ?"));
		}
		else // inode successfully created
		    if(ref_mir != NULL)
		    {
			map<infinint, corres_ino_ea>::iterator it = corres_write.find(ref_mir->get_etiquette());
			if(it == corres_write.end()) // we just created the first hard linked to that inode, so we must record its intial link to build subsequent ones
			{
			    corres_ino_ea tmp;
			    tmp.chemin = string(name);
			    tmp.ea_restored = false;
			    corres_write[ref_mir->get_etiquette()] = tmp;
			}
		    }
	    }
	    catch(Ethread_cancel & e)
	    {
		if(ret >= 0) // we need to remove the filesystem entry we were creating
			// we don't let something uncompletely restored in the filesystem
			// in any case (immediate cancel or not)
		    (void)unlink(name); // ignoring exit status
		throw;
	    }
	}
	while(ret < 0 && errno == ENOSPC);

	if(ref_ino != NULL && ret >= 0)
	    make_owner_perm(get_ui(), *ref_ino, display, dir_perm, what_to_check, scope);
    }

    void filesystem_hard_link_write::clear_corres_if_pointing_to(const infinint & ligne, const string & path)
    {
        map<infinint, corres_ino_ea>::iterator it = corres_write.find(ligne);
        if(it != corres_write.end())
	{
	    if(it->second.chemin == path)
		corres_write.erase(it);
	}
    }


///////////////////////////////////////////////////////////////////
////////////////// filesystem_restore methods  ////////////////////
///////////////////////////////////////////////////////////////////


    filesystem_restore::filesystem_restore(user_interaction & dialog,
					   const path &root,
					   bool x_warn_overwrite,
                                           bool x_info_details,
                                           const mask & x_ea_mask,
					   cat_inode::comparison_fields x_what_to_check,
					   bool x_warn_remove_no_match,
					   bool x_empty,
					   const crit_action *x_overwrite,
					   bool x_only_overwrite,
					   const fsa_scope & scope) :
	mem_ui(dialog), filesystem_hard_link_write(dialog), filesystem_hard_link_read(dialog, true, scope)
    {
	fs_root = NULL;
	ea_mask = NULL;
	current_dir = NULL;
	overwrite = NULL;
	try
	{
	    fs_root = get_root_with_symlink(get_ui(), root, x_info_details, get_pool());
	    if(fs_root == NULL)
		throw Ememory("filesystem_write::filesystem_write");
	    ea_mask = x_ea_mask.clone();
	    if(ea_mask == NULL)
		throw Ememory("filesystem_restore::filesystem_restore");
	    if(x_overwrite == NULL)
		throw SRC_BUG;
	    overwrite = x_overwrite->clone();
	    if(overwrite == NULL)
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
    }

    void filesystem_restore::reset_write()
    {
        filesystem_hard_link_write::corres_reset();
        filesystem_hard_link_read::corres_reset();
        stack_dir.clear();
        if(current_dir != NULL)
            delete current_dir;
        current_dir = new (get_pool()) path(*fs_root);
        if(current_dir == NULL)
            throw Ememory("filesystem_write::reset_write");
	ignore_over_restricts = false;
    }

    void filesystem_restore::write(const cat_entree *x, action_done_for_data & data_restored, bool & ea_restored, bool & data_created, bool & hard_link, bool & fsa_restored)
    {
	const cat_eod *x_eod = dynamic_cast<const cat_eod *>(x);
	const cat_nomme *x_nom = dynamic_cast<const cat_nomme *>(x);
	const cat_directory *x_dir = dynamic_cast<const cat_directory *>(x);
	const detruit *x_det = dynamic_cast<const detruit *>(x);
	const cat_inode *x_ino = dynamic_cast<const cat_inode *>(x);
	const cat_mirage *x_mir = dynamic_cast<const cat_mirage *>(x);

	data_restored = done_no_change_no_data;
	ea_restored = false;
	data_created = false;
	fsa_restored = false;
	hard_link = x_mir != NULL && known_etiquette(x_mir->get_etiquette());

	if(x_mir != NULL)
	{
	    x_ino = x_mir->get_inode();
	    if(x_ino == NULL)
		throw SRC_BUG;
	}

	if(x_eod != NULL)
	{
	    string tmp;
	    current_dir->pop(tmp);
	    if(!stack_dir.empty())
	    {
		if(!empty && stack_dir.back().get_restore_date())
		{
		    string chem = (*current_dir + stack_dir.back().get_name()).display();

		    make_owner_perm(get_ui(), stack_dir.back(), chem, true, what_to_check, get_fsa_scope());
		    make_date(stack_dir.back(), chem, what_to_check, get_fsa_scope());
		}
	    }
	    else
		throw SRC_BUG;
	    stack_dir.pop_back();
	    return;
	}

	if(x_nom == NULL)
	    throw SRC_BUG; // neither "cat_nomme" nor "cat_eod"
	else // cat_nomme
	{
	    bool has_data_saved = (x_ino != NULL && x_ino->get_saved_status() == s_saved) || x_det != NULL;
	    bool has_ea_saved = x_ino != NULL && (x_ino->ea_get_saved_status() == cat_inode::ea_full || x_ino->ea_get_saved_status() == cat_inode::ea_removed);
	    bool has_fsa_saved = x_ino != NULL && x_ino->fsa_get_saved_status() == cat_inode::fsa_full;
	    path spot = *current_dir + x_nom->get_name();
	    string spot_display = spot.display();

	    cat_nomme *exists = NULL;

	    if(ignore_over_restricts)
		    // only used in sequential_read when a file has been saved several times due
		    // to its contents being modified at backup time while dar was reading it.
		    // here when ignore_over_restricts is true it is asked to remove the previously restored copy of
		    // that file because a better copy has been found in the archive.
	    {
		ignore_over_restricts = false; // just one shot state ; exists == NULL : we are ignoring existing entry
		supprime(get_ui(), spot_display);
		if(x_det != NULL)
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

	    try
	    {
		cat_inode *exists_ino = dynamic_cast<cat_inode *>(exists);
		cat_directory *exists_dir = dynamic_cast<cat_directory *>(exists);

		if(exists_ino == NULL && exists != NULL)
		    throw SRC_BUG; // an object from filesystem should always be an cat_inode !?!

		if(exists == NULL)
		{

			// no conflict: there is not an already existing file present in filesystem

		    if(x_det != NULL)
			throw Erange("filesystem_restore::write", string(gettext("Cannot remove non-existent file from filesystem: ")) + spot_display);

		    if((has_data_saved || hard_link || x_dir != NULL) && !only_overwrite)
		    {
			if(info_details)
			    get_ui().warning(string(gettext("Restoring file's data: ")) + spot_display);

			if(!empty)
			    make_file(x_nom, *current_dir, false, what_to_check, get_fsa_scope());
			data_created = true;
			data_restored = done_data_restored;

			    // recording that we must set back the mtime of the parent directory
			if(!stack_dir.empty())
			    stack_dir.back().set_restore_date(true);

			    // we must try to restore EA or FSA only if data could be restored
			    // as in the current situation no file existed before

			if(has_ea_saved)
			{
			    if(info_details)
				get_ui().warning(string(gettext("Restoring file's EA: ")) + spot_display);

			    if(!empty)
			    {
				const ea_attributs *ea = x_ino->get_ea();
				try
				{
				    ea_restored = raw_set_ea(x_nom, *ea, spot_display, *ea_mask);
				}
				catch(Erange & e)
				{
				    get_ui().warning(tools_printf(gettext("Restoration of EA for %S aborted: "), &spot_display) + e.get_message());
				}
			    }
			    else
				ea_restored = true;
			}

			if(has_fsa_saved)
			{
			    if(info_details)
				get_ui().warning(string(gettext("Restoring file's FSA: ")) + spot_display);

			    if(!empty)
			    {
				const filesystem_specific_attribute_list * fsa = x_ino->get_fsa();
				if(fsa == NULL)
				    throw SRC_BUG;
				try
				{
				    fsa_restored = fsa->set_fsa_to_filesystem_for(spot_display, get_fsa_scope(), get_ui());
				}
				catch(Erange & e)
				{
				    get_ui().warning(tools_printf(gettext("Restoration of FSA for %S aborted: "), &spot_display) + e.get_message());
				}
			    }
			}

			    // now that FSA has been read (if sequential mode is used)
			    // we can restore dates in particular creation date from HFS+ FSA if present
			if(!empty)
			    make_date(*x_ino, spot_display, what_to_check,
				      get_fsa_scope());
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
		else // exists != NULL
		{
		    over_action_data act_data = data_undefined;
		    over_action_ea act_ea = EA_undefined;

			// conflict: an entry of that name is already present in filesystem

		    overwrite->get_action(*exists, *x_nom, act_data, act_ea);

		    if(x_ino == NULL)
			if(x_det == NULL)
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
			if(has_data_saved)
			{
			    try
			    {
				action_over_data(exists_ino, x_nom, spot_display, act_data, data_restored);
			    }
			    catch(Egeneric & e)
			    {

			    }
			}
			else // no data saved in the object to restore
			{
			    data_restored = done_no_change_no_data;
			    if(x_mir != NULL)
				write_hard_linked_target_if_not_set(x_mir, spot_display);
			}

			if(data_restored != done_data_restored)
			    hard_link = false;

			    // here we can restore EA even if no data has been restored
			    // it will modify EA of the existing file
			if(act_data != data_remove)
			{
			    if(has_ea_saved)
			    {
				try
				{
				    ea_restored = action_over_ea(exists_ino, x_nom, spot_display, act_ea);
				}
				catch(Erange & e)
				{
				    get_ui().warning(tools_printf(gettext("Restoration of EA for %S aborted: "), &spot_display) + e.get_message());
				}
			    }

			    if(has_fsa_saved)
			    {
				try
				{
				    fsa_restored = action_over_fsa(exists_ino, x_nom, spot_display, act_ea);
				}
				catch(Erange & e)
				{
				    get_ui().warning(tools_printf(gettext("Restoration of FSA for %S aborted: "), &spot_display) + e.get_message());
				}
			    }

			    if(has_fsa_saved || has_ea_saved)
			    {
				    // to accomodate MacOS X we set again mtime to its expected value
				    // because restoring EA modifies mtime on this OS.
				    // Same point but concerning extX FSA, setting them may modify atime
				    // unless furtive read is available.
				    // This does not hurt other Unix systems.

				if(data_restored == done_data_restored)
					// set back the mtime to value found in the archive
				    make_date(*x_ino, spot_display, what_to_check,
					      get_fsa_scope());
				else
					// set back the mtime to value found in filesystem before restoration
				    make_date(*exists_ino, spot_display, what_to_check,
					      get_fsa_scope());
			    }

			}

			if(act_data == data_remove)
			{
				// recording that we must set back the mtime of the parent directory
			    if(!stack_dir.empty())
				stack_dir.back().set_restore_date(true);
			}
		    }
		}

		if(x_dir != NULL && (exists == NULL || exists_dir != NULL || data_restored == done_data_restored))
		{
		    *current_dir += x_dir->get_name();
		    stack_dir.push_back(stack_dir_t(*x_dir, data_restored == done_data_restored));
		}
	    }
	    catch(...)
	    {
		if(exists != NULL)
		    delete exists;
		throw;
	    }
	    if(exists != NULL)
		delete exists;
	}
    }


    void filesystem_restore::action_over_remove(const cat_inode *in_place, const detruit *to_be_added, const string & spot, over_action_data action)
    {
	if(in_place == NULL || to_be_added == NULL)
	    throw SRC_BUG;

	if(action == data_ask)
	    action = crit_ask_user_for_data_action(get_ui(), spot, in_place, to_be_added);

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

	    if(compatible_signature(in_place->signature(), to_be_added->get_signature()))
	    {
		if(info_details)
		    get_ui().printf(gettext("Removing file (reason is file recorded as removed in archive): %S"), &spot);
		if(!empty)
		    supprime(get_ui(), spot);
	    }
	    else
	    {
		if(warn_remove_no_match) // warning even if just warn_overwrite is not set
		    get_ui().pause(tools_printf(gettext("%S must be removed, but does not match expected type, remove it anyway ?"), &spot));
		if(info_details)
		    get_ui().printf(gettext("Removing file (reason is file recorded as removed in archive): %S"), &spot);
		if(!empty)
		    supprime(get_ui(), spot);
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
	const cat_inode *tba_ino = tba_mir == NULL ? dynamic_cast<const cat_inode *>(to_be_added) : tba_mir->get_inode();
	const cat_directory *tba_dir = dynamic_cast<const cat_directory *>(to_be_added);
	const detruit *tba_det = dynamic_cast<const detruit *>(to_be_added);
	const cat_lien *in_place_symlink = dynamic_cast<const cat_lien *>(in_place);

	if(tba_ino == NULL)
	    throw SRC_BUG;

	if(in_place == NULL)
	    throw SRC_BUG;

	if(tba_det != NULL)
	    throw SRC_BUG; // must be either a cat_mirage or an inode, not any other cat_nomme object

	if(action == data_ask)
	    action = crit_ask_user_for_data_action(get_ui(), spot, in_place, to_be_added);

	switch(action)
	{
	case data_preserve:
	case data_preserve_mark_already_saved:
	    if(tba_dir != NULL && !tba_ino->same_as(*in_place))
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
		    if(tba_dir != NULL && tba_ino->same_as(*in_place))
		    {
			data_done = done_no_change_policy;
			return; // if we throw exception here, we will not recurse in this directory, while we could as a directory exists on filesystem
		    }
		    else
			throw; // throwing the exception here, implies that no EA will be tried to be restored for that file
		}
	    }

	    if(info_details)
		get_ui().warning(string(gettext("Restoring file's data: ")) + spot);

	    if(tba_dir != NULL && tba_ino->same_as(*in_place))
	    {
		if(!empty)
		    make_owner_perm(get_ui(), *tba_ino, spot, false, what_to_check, get_fsa_scope());
		data_done = done_data_restored;
	    }
	    else // not both in_place and to_be_added are directories
	    {
		ea_attributs *ea = NULL; // saving original EA of existing inode
		filesystem_specific_attribute_list fsa; // saving original FSA of existing inode
		bool got_ea = true;
		bool got_fsa = true;

		try
		{

			// reading EA present on filesystem

		    try
		    {
			ea = ea_filesystem_read_ea(spot, bool_mask(true), get_pool());
		    }
		    catch(Ethread_cancel & e)
		    {
			throw;
		    }
		    catch(Egeneric & ex)
		    {
			got_ea = false;
			get_ui().warning(tools_printf(gettext("Existing EA for %S could not be read and preserved: "), &spot) + ex.get_message());
		    }

			// reading FSA present on filesystem

		    try
		    {
			fsa.get_fsa_from_filesystem_for(spot,
							all_fsa_families(),
							in_place_symlink != NULL);
		    }
		    catch(Ethread_cancel & e)
		    {
			throw;
		    }
		    catch(Egeneric & ex)
		    {
			got_fsa = false;
			get_ui().warning(tools_printf(gettext("Existing FSA for %S could not be read and preserved: "), &spot) + ex.get_message());
		    }

			// removing current entry and creating the new entry in place

		    if(!empty)
		    {
			supprime(get_ui(), spot); // this destroyes EA, (removes inode, or hard link to inode)
			make_file(to_be_added, *current_dir, false, what_to_check, get_fsa_scope());
			data_done = done_data_restored;
		    }

			// restoring EA that were present on filesystem

		    try // if possible and available restoring original EA
		    {
			if(got_ea && !empty)
			    if(ea != NULL) // if ea is NULL no EA is present in the original file, thus nothing has to be restored
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
			if(ea != NULL && ea->size() > 0)
			    get_ui().warning(tools_printf(gettext("Existing EA for %S could not be preserved : "), &spot) + e.get_message());
		    }

			// restoring FSA that were present on filesystem

		    try // if possible and available restoring original FSA
		    {
			if(got_fsa && !empty)
			    fsa.set_fsa_to_filesystem_for(spot, all_fsa_families(), get_ui());
		    }
		    catch(Ethread_cancel & e)
		    {
			throw;
		    }
		    catch(Egeneric & e)
		    {
			if(ea != NULL && ea->size() > 0)
			    get_ui().warning(tools_printf(gettext("Existing FSA for %S could not be preserved : "), &spot) + e.get_message());
		    }
		}
		catch(...)
		{
		    if(ea != NULL)
			delete ea;
		    throw;
		}
		if(ea != NULL)
		    delete ea;
	    }
	    break;
	case data_remove:
	    if(warn_overwrite)
		get_ui().pause(tools_printf(gettext("%S is about to be deleted (required by overwriting policy), do you agree?"), &spot));
	    if(info_details)
		get_ui().printf(gettext("Removing file (reason is overwriting policy): %S"), &spot);
	    if(!empty)
		supprime(get_ui(), spot);
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

	if(tba_mir != NULL)
	    tba_ino = tba_mir->get_inode();

	if(tba_ino == NULL)
	    throw SRC_BUG;

	if(in_place == NULL || to_be_added == NULL)
	    throw SRC_BUG;

	if(action == EA_ask)
	    action = crit_ask_user_for_EA_action(get_ui(), spot, in_place, to_be_added);


	    // modifying the EA action when the in place inode has not EA

	if(in_place->ea_get_saved_status() != cat_inode::ea_full) // no EA in filesystem
	{
	    if(action == EA_merge_preserve || action == EA_merge_overwrite)
		action = EA_overwrite; // merging when in_place has no EA is equivalent to overwriting
	}

	if(tba_ino->ea_get_saved_status() == cat_inode::ea_removed) // EA have been removed since archive of reference
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
	    if(tba_ino->ea_get_saved_status() != cat_inode::ea_full && tba_ino->ea_get_saved_status() != cat_inode::ea_removed)
		throw SRC_BUG;
	    if(warn_overwrite)
	    {
		try
		{
		    get_ui().pause(tools_printf(gettext("EA for %S are about to be overwritten, OK?"), &spot));
		}
		catch(Euser_abort & e)
		{
		    const cat_directory *tba_dir = dynamic_cast<const cat_directory *>(to_be_added);
		    if(tba_dir != NULL && tba_ino->same_as(*in_place))
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
		    get_ui().warning(string(gettext("Restoring file's EA: ")) + spot);

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
		    get_ui().warning(string(gettext("Clearing file's EA (requested by overwriting policy): ")) + spot);
		ret = true;
	    }
	    break;
	case EA_merge_preserve:
	case EA_merge_overwrite:
	    if(in_place->ea_get_saved_status() != cat_inode::ea_full)
		throw SRC_BUG; // should have been redirected to EA_overwrite !

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

	    if(tba_ino->ea_get_saved_status() == cat_inode::ea_full) // Note, that cat_inode::ea_removed is the other valid value
	    {
		const ea_attributs *tba_ea = tba_ino->get_ea();
		const ea_attributs *ip_ea = in_place->get_ea();
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

	if(tba_mir != NULL)
	    tba_ino = tba_mir->get_inode();

	if(tba_ino == NULL)
	    throw SRC_BUG;

	if(in_place == NULL || to_be_added == NULL)
	    throw SRC_BUG;

	if(action == EA_ask)
	    action = crit_ask_user_for_FSA_action(get_ui(), spot, in_place, to_be_added);


	    // modifying the FSA action when the in place inode has not FSA

	if(in_place->fsa_get_saved_status() != cat_inode::fsa_full) // no EA in filesystem
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
	    if(tba_ino->fsa_get_saved_status() != cat_inode::fsa_full)
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
		    if(tba_dir != NULL && tba_ino->same_as(*in_place))
			return false;
		    else
			throw;
		}
	    }

	    if(tba_mir != NULL && known_etiquette(tba_mir->get_etiquette()))
	    {
		if(info_details)
		    get_ui().printf(gettext("FSA for %S have not been overwritten because this file is a hard link pointing to an already restored inode"), &spot);
		ret = false;
	    }
	    else
	    {
		if(info_details)
		    get_ui().warning(string(gettext("Restoring file's FSA: ")) + spot);

		if(!empty)
		{
		    const filesystem_specific_attribute_list * fsa = tba_ino->get_fsa();
		    if(fsa == NULL)
			throw SRC_BUG;

		    ret = fsa->set_fsa_to_filesystem_for(spot, get_fsa_scope(), get_ui());
		}
		else
		    ret = true;
	    }
	    break;
	case EA_clear:
	    break;
	case EA_merge_preserve:
	case EA_merge_overwrite:
	    if(in_place->fsa_get_saved_status() != cat_inode::fsa_full)
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

	    if(tba_ino->fsa_get_saved_status() == cat_inode::fsa_full)
	    {
		const filesystem_specific_attribute_list *tba_fsa = tba_ino->get_fsa();
		const filesystem_specific_attribute_list *ip_fsa = in_place->get_fsa();
		filesystem_specific_attribute_list result;

		if(action == EA_merge_preserve)
		    result = *tba_fsa + *ip_fsa;
		else // action == EA_merge_overwrite
		    result = *ip_fsa + *tba_fsa; // the + operator on FSA is not reflexive !!!

		if(!empty)
		    ret = result.set_fsa_to_filesystem_for(spot, get_fsa_scope(), get_ui());
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
        if(fs_root != NULL)
	{
            delete fs_root;
	    fs_root = NULL;
	}
        if(current_dir != NULL)
	{
            delete current_dir;
	    current_dir = NULL;
	}
	if(ea_mask != NULL)
	{
	    delete ea_mask;
	    ea_mask = NULL;
	}
	if(overwrite != NULL)
	{
	    delete overwrite;
	    overwrite = NULL;
	}
    }

    void filesystem_restore::restore_stack_dir_ownership()
    {
	string tmp;

	while(!stack_dir.empty() && current_dir->pop(tmp))
	{
	    string chem = (*current_dir + stack_dir.back().get_name()).display();
	    if(!empty)
		make_owner_perm(get_ui(), stack_dir.back(), chem, true, what_to_check, get_fsa_scope());
	    stack_dir.pop_back();
	}
	if(stack_dir.size() > 0)
	    throw SRC_BUG;
    }


///////////////////////////////////////////////////////////////////
////////////////// static functions ///////////////////////////////
///////////////////////////////////////////////////////////////////

    static void supprime(user_interaction & ui, const string & ref)
    {
	const char *s = ref.c_str();

	struct stat buf;
	if(lstat(s, &buf) < 0)
	    throw Erange("filesystem supprime", string(gettext("Cannot get inode information about file to remove ")) + s + " : " + tools_strerror_r(errno));

	if(S_ISDIR(buf.st_mode))
	{
	    etage fils = etage(ui, s, datetime(0), datetime(0), false, false); // we don't care the access and modification time because directory will be destroyed
	    string tmp;

		// first we destroy directory's children
	    while(fils.read(tmp))
		supprime(ui, (path(ref)+tmp).display());

		// then the directory itself
	    if(rmdir(s) < 0)
		throw Erange("supprime (dir)", string(gettext("Cannot remove directory ")) + s + " : " + tools_strerror_r(errno));
	}
	else
	    if(unlink(s) < 0)
		throw Erange("supprime (file)", string(gettext("Cannot remove file ")) + s + " : " + tools_strerror_r(errno));
    }

    static void make_owner_perm(user_interaction & dialog,
				const cat_inode & ref,
				const string & chem,
				bool dir_perm,
				cat_inode::comparison_fields what_to_check,
				const fsa_scope & scope)
    {
        const char *name = chem.c_str();
        const cat_lien *ref_lie = dynamic_cast<const cat_lien *>(&ref);
        S_I permission;


	    // if we are not root (geteuid()!=0) and if we are restoring in an already
	    // existing (!dir_perm) directory (dynamic_cast...), we must try, to have a chance to
	    // be able to create/modify sub-files or sub-directory, so we set the user write access to
	    // that directory. This chmod is allowed only if we own the directory (so
	    // we only set the write bit for user). If this cannot be changed we are not the
	    // owner of the directory, so we will try to restore as much as our permission
	    // allows it (maybe "group" or "other" write bits are set for us).

	if(dynamic_cast<const cat_directory *>(&ref) != NULL && !dir_perm && geteuid() != 0)
	{
	    mode_t tmp;
	    try
	    {
		tmp = get_file_permission(name); // the current permission value
	    }
	    catch(Egeneric & e)
	    {
		tmp = ref.get_perm(); // the value at the time of the backup if we failed reading permission from filesystem
	    }
	    permission =  tmp | 0200; // add user write access to be able to add some subdirectories and files
	}
	else
	    permission = ref.get_perm();

	    // restoring fields that are defined by "what_to_check"

	if(what_to_check == cat_inode::cf_all)
	    if(ref.get_saved_status() == s_saved)
	    {
		uid_t tmp_uid = 0;
		gid_t tmp_gid = 0;
		infinint tmp = ref.get_uid();
		tmp.unstack(tmp_uid);
		if(tmp != 0)
		    throw Erange("make_owner_perm", gettext("uid value is too high for this system for libdar be able to restore it properly"));
		tmp = ref.get_gid();
		tmp.unstack(tmp_gid);
		if(tmp != 0)
		    throw Erange("make_owner_perm", gettext("gid value is too high for this system for libdar be able to restore it properly"));

#if HAVE_LCHOWN
		if(lchown(name, tmp_uid, tmp_gid) < 0)
		    dialog.warning(chem + string(gettext("Could not restore original file ownership: ")) + tools_strerror_r(errno));
#else
		if(dynamic_cast<const cat_lien *>(&ref) == NULL) // not a symbolic link
		    if(chown(name, tmp_uid, tmp_gid) < 0)
			dialog.warning(chem + string(gettext("Could not restore original file ownership: ")) + tools_strerror_r(errno));
		    //
		    // we don't/can't restore ownership for symbolic links (no system call to do that)
		    //
#endif
	    }

	try
	{
	    if(what_to_check == cat_inode::cf_all || what_to_check == cat_inode::cf_ignore_owner)
		if(ref_lie == NULL) // not restoring permission for symbolic links, it would modify the target not the symlink itself
		    if(chmod(name, permission) < 0)
		    {
			string tmp = tools_strerror_r(errno);
			dialog.warning(tools_printf(gettext("Cannot restore permissions of %s : %s"), name, tmp.c_str()));
		    }
	}
	catch(Egeneric &e)
	{
	    if(ref_lie == NULL)
		throw;
		// else (the inode is a symlink), we simply ignore this error
	}
    }

    static void make_date(const cat_inode & ref,
			  const string & chem,
			  cat_inode::comparison_fields what_to_check,
			  const fsa_scope & scope)
    {
	const cat_lien *ref_lie = dynamic_cast<const cat_lien *>(&ref);

	if(what_to_check == cat_inode::cf_all || what_to_check == cat_inode::cf_ignore_owner || what_to_check == cat_inode::cf_mtime)
	{
	    datetime birthtime = ref.get_last_modif();
	    fsa_scope::iterator it = scope.find(fsaf_hfs_plus);

	    if(ref.fsa_get_saved_status() == cat_inode::fsa_full && it != scope.end())
	    {
		const filesystem_specific_attribute_list * fsa = ref.get_fsa();
		const filesystem_specific_attribute *ptr = NULL;

		if(fsa == NULL)
		    throw SRC_BUG;
		if(fsa->find(fsaf_hfs_plus, fsan_creation_date, ptr))
		{
		    const fsa_time *ptr_time = dynamic_cast<const fsa_time *>(ptr);
		    if(ptr_time != NULL)
			birthtime = ptr_time->get_value();
		}
	    }

	    tools_make_date(chem, ref_lie != NULL, ref.get_last_access(), ref.get_last_modif(), birthtime);
	}
    }

    static void attach_ea(const string &chemin, cat_inode *ino, const mask & ea_mask, memory_pool *pool)
    {
        ea_attributs *eat = NULL;
        try
        {
            if(ino == NULL)
                throw SRC_BUG;
            eat = ea_filesystem_read_ea(chemin, ea_mask, pool);
            if(eat != NULL)
            {
		if(eat->size() <= 0)
		    throw SRC_BUG;
                ino->ea_set_saved_status(cat_inode::ea_full);
                ino->ea_attach(eat);
                eat = NULL; // allocated memory now managed by the cat_inode object
            }
            else
                ino->ea_set_saved_status(cat_inode::ea_none);
        }
        catch(...)
        {
            if(eat != NULL)
                delete eat;
            throw;
        }
        if(eat != NULL)
            throw SRC_BUG;
    }


    static bool is_nodump_flag_set(user_interaction & dialog,
				   const path & chem, const string & filename, bool info)
    {
#ifdef LIBDAR_NODUMP_FEATURE
        S_I fd, f = 0;
	const string display = (chem + filename).display();
	const char *ptr = display.c_str();

	fd = ::open(ptr, O_RDONLY|O_BINARY|O_NONBLOCK);
	if(fd < 0)
	{
	    if(info)
	    {
		string tmp = tools_strerror_r(errno);
		dialog.warning(tools_printf(gettext("Failed to open %S while checking for nodump flag: %s"), &filename, tmp.c_str()));
	    }
	}
	else
	{
	    try
	    {
		if(ioctl(fd, EXT2_IOC_GETFLAGS, &f) < 0)
		{
		    if(errno != ENOTTY)
		    {
			if(info)
			{
			    string tmp = tools_strerror_r(errno);
			    dialog.warning(tools_printf(gettext("Cannot get ext2 attributes (and nodump flag value) for %S : %s"), &filename, tmp.c_str()));
			}
		    }
		    f = 0;
		}
	    }
	    catch(...)
	    {
		close(fd);
		throw;
	    }
	    close(fd);
	}

        return (f & EXT2_NODUMP_FL) != 0;
#else
        return false;
#endif
    }

    static path *get_root_with_symlink(user_interaction & dialog,
				       const path & root,
				       bool info_details,
				       memory_pool *pool)
    {
	path *ret = NULL;
	const string display = root.display();
	const char *ptr = display.c_str();

	struct stat buf;
	if(lstat(ptr, &buf) < 0) // stat not lstat, thus we eventually get the symlink pointed to inode
	{
	    string tmp = tools_strerror_r(errno);
	    throw Erange("filesystem:get_root_with_symlink", tools_printf(gettext("Cannot get inode information for %s : %s"), ptr, tmp.c_str()));
	}

	if(S_ISDIR(buf.st_mode))
	{
	    ret = new (pool) path(root);
	    if(ret == NULL)
		throw  Ememory("get_root_with_symlink");
	}
	else if(S_ISLNK(buf.st_mode))
	{
	    ret = new (pool) path(tools_readlink(ptr));
	    if(ret == NULL)
		throw Ememory("get_root_with_symlink");
	    if(ret->is_relative())
	    {
		string tmp;
		path base = root;

		if(base.pop(tmp))
		    *ret = base + *ret;
		else
		    if(!root.is_relative())
			throw SRC_BUG;
		    // symlink name is not "popable" and is absolute, it is thus the filesystem root '/'
		    // and it is a symbolic link !!! How is it possible that "/" be a symlink ?
		    // a symlink to where ???
	    }
	    if(info_details && ! (*ret == root) )
		dialog.warning(tools_printf(gettext("Replacing %s in the -R option by the directory pointed to by this symbolic link: "), ptr) + ret->display());
	}
	else // not a directory given as argument
	    throw Erange("filesystem:get_root_with_symlink", tools_printf(gettext("The given path %s must be a directory (or symbolic link to an existing directory)"), ptr));

	if(ret == NULL)
	    throw SRC_BUG; // exit without exception, but ret not allocated !

	return ret;
    }

    static mode_t get_file_permission(const string & path)
    {
	struct stat buf;

	if(lstat(path.c_str(), &buf) < 0)
	{
	    string tmp = tools_strerror_r(errno);
	    throw Erange("filesystem.cpp:get_file_permission", tools_printf("Cannot read file permission for %s: %s",path.c_str(), tmp.c_str()));
	}

	return buf.st_mode;
    }

} // end of namespace
