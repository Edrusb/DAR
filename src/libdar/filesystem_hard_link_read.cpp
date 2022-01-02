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

#include "filesystem_hard_link_read.hpp"
#include "tools.hpp"
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
    static void attach_ea(const string &chemin,
			  cat_inode *ino,
			  const mask & ea_mask);

    cat_nomme *filesystem_hard_link_read::make_read_entree(const path & lieu, const string & name, bool see_hard_link, const mask & ea_mask)
    {
	const string display = name.empty() ? lieu.display() : (lieu.append(name)).display();
        const char *ptr_name = display.c_str();
        cat_nomme *ref = nullptr;
	struct stat buf;
	string tmp;

	try
	{
	    int val = 0;
	    bool use_stat = ignore_if_symlink(display);

	    if(use_stat)
		val = stat(ptr_name, &buf);
	    else
		val = lstat(ptr_name, &buf);

	    if(val < 0)
	    {
		switch(errno)
		{
		case EACCES:
		    tmp = tools_strerror_r(errno);
		    get_ui().message(tools_printf(gettext("Error reading inode of file %s : %s"), ptr_name, tmp.c_str()));
		    break;
		case ENOENT:
		    if(display.size() >= PATH_MAX
		       || name.size() >= NAME_MAX)
			get_ui().message(tools_printf(gettext("Failed reading inode information for %s: "), ptr_name) + tools_strerror_r(errno));
			// Cygwin may return shorter name than expected using readdir (see class etage) which
			// leads the file to be truncated, and thus when we here fetch inode information
			// we get file non existent error. In that situation this is not the case of a
			// file that has been removed between the time we read the directory content and the
			// time here we read inode details, so we issue a warning in that situation
		    break;
		default:
		    throw Erange("filesystem_hard_link_read::make_read_entree", string(gettext("Cannot read inode for ")) + ptr_name + " : " + tools_strerror_r(errno));
		}

		    // the current method returns nullptr (= ref)  (meaning file does not exists)
	    }
	    else
	    {
#if LIBDAR_TIME_READ_ACCURACY == LIBDAR_TIME_ACCURACY_MICROSECOND || LIBDAR_TIME_READ_ACCURACY == LIBDAR_TIME_ACCURACY_NANOSECOND
		tools_check_negative_date(buf.st_atim.tv_sec,
					  get_ui(),
					  ptr_name,
					  gettext("atime, data access time"),
					  ask_before_zeroing_neg_dates,
					  false); // not silent
		tools_check_negative_date(buf.st_mtim.tv_sec,
					  get_ui(),
					  ptr_name,
					  gettext("mtime, data modification time"),
					  ask_before_zeroing_neg_dates,
					  false); // not silent
		tools_check_negative_date(buf.st_ctim.tv_sec,
					  get_ui(),
					  ptr_name,
					  gettext("ctime, inode change time"),
					  ask_before_zeroing_neg_dates,
					  false); // not silent
#if LIBDAR_TIME_READ_ACCURACY == LIBDAR_TIME_ACCURACY_MICROSECOND
		    /* saving some place in archive not recording nanosecond time fraction */
		datetime atime = datetime(buf.st_atim.tv_sec, buf.st_atim.tv_nsec/1000, datetime::tu_microsecond);
		datetime mtime = datetime(buf.st_mtim.tv_sec, buf.st_mtim.tv_nsec/1000, datetime::tu_microsecond);
		datetime ctime = datetime(buf.st_ctim.tv_sec, buf.st_ctim.tv_nsec/1000, datetime::tu_microsecond);
#else
		datetime atime = datetime(buf.st_atim.tv_sec, buf.st_atim.tv_nsec, datetime::tu_nanosecond);
		datetime mtime = datetime(buf.st_mtim.tv_sec, buf.st_mtim.tv_nsec, datetime::tu_nanosecond);
		datetime ctime = datetime(buf.st_ctim.tv_sec, buf.st_ctim.tv_nsec, datetime::tu_nanosecond);
#endif

		if(atime.is_null()) // assuming an error avoids getting time that way
		    atime = datetime(buf.st_atime, 0, datetime::tu_second);
		if(mtime.is_null()) // assuming an error avoids getting time that way
		    mtime = datetime(buf.st_mtime, 0, datetime::tu_second);
		if(ctime.is_null()) // assuming an error avoids getting time that way
		    ctime = datetime(buf.st_ctime, 0, datetime::tu_second);
#else
		tools_check_negative_date(buf.st_atime,
					  get_ui(),
					  ptr_name,
					  gettext("atime, data access time"),
					  ask_before_zeroing_neg_dates,
					  false); // not silent
		tools_check_negative_date(buf.st_mtime,
					  get_ui(),
					  ptr_name,
					  gettext("mtime, data modification time"),
					  ask_before_zeroing_neg_dates,
					  false); // not silent
		tools_check_negative_date(buf.st_ctime,
					  get_ui(),
					  ptr_name,
					  gettext("ctime, inode change time"),
					  ask_before_zeroing_neg_dates,
					  false); // not silent
		datetime atime = datetime(buf.st_atime, 0, datetime::tu_second);
		datetime mtime = datetime(buf.st_mtime, 0, datetime::tu_second);
		datetime ctime = datetime(buf.st_ctime, 0, datetime::tu_second);
#endif

		if(S_ISLNK(buf.st_mode))
		{
		    string pointed = tools_readlink(ptr_name);

		    ref = new (nothrow) cat_lien(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
						    atime,
						    mtime,
						    ctime,
						    name,
						    pointed,
						    buf.st_dev);
		}
		else if(S_ISREG(buf.st_mode))
		    ref = new (nothrow) cat_file(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
						    atime,
						    mtime,
						    ctime,
						    name,
						    lieu,
						    buf.st_size,
						    buf.st_dev,
						    furtive_read_mode);
		else if(S_ISDIR(buf.st_mode))
		    ref = new (nothrow) cat_directory(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
							 atime,
							 mtime,
							 ctime,
							 name,
							 buf.st_dev);
		else if(S_ISCHR(buf.st_mode))
		    ref = new (nothrow) cat_chardev(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
						       atime,
						       mtime,
						       ctime,
						       name,
						       major(buf.st_rdev),
						       minor(buf.st_rdev), // makedev(major, minor)
						       buf.st_dev);
		else if(S_ISBLK(buf.st_mode))
		    ref = new (nothrow) cat_blockdev(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
							atime,
							mtime,
							ctime,
							name,
							major(buf.st_rdev),
							minor(buf.st_rdev), // makedev(major, minor)
							buf.st_dev);
		else if(S_ISFIFO(buf.st_mode))
		    ref = new (nothrow) cat_tube(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
						    atime,
						    mtime,
						    ctime,
						    name,
						    buf.st_dev);
		else if(S_ISSOCK(buf.st_mode))
		    ref = new (nothrow) cat_prise(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
						     atime,
						     mtime,
						     ctime,
						     name,
						     buf.st_dev);
#if HAVE_DOOR
		else if(S_ISDOOR(buf.st_mode))
		    ref = new (nothrow) cat_door(buf.st_uid,
						 buf.st_gid,
						 buf.st_mode & 07777,
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
		if(ino != nullptr)
		{

			//
			// Extended Attributes Considerations
			//

		    try
		    {
			attach_ea(ptr_name, ino, ea_mask);
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
			get_ui().message(string(gettext("Error reading EA for "))+ptr_name+ " : " + ex.get_message());
			    // no throw !
			    // we must be able to continue without EA
		    }

			//
			// Filesystem Specific Attributes Considerations
			//

		    filesystem_specific_attribute_list *fsal = new (nothrow) filesystem_specific_attribute_list();
		    if(fsal == nullptr)
			throw Ememory("filesystem_hard_link_read::make_entree");
		    try
		    {
			fsal->get_fsa_from_filesystem_for(get_ui(),
							  display,
							  sc,
							  buf.st_mode,
							  get_ask_before_zeroing_neg_dates());
			if(!fsal->empty())
			{
			    ino->fsa_set_saved_status(fsa_saved_status::full);
			    ino->fsa_attach(fsal);
			    fsal = nullptr; // now managed by *ino
			}
			else
			{
			    ino->fsa_set_saved_status(fsa_saved_status::none);
			    delete fsal;
			    fsal = nullptr;
			}
		    }
		    catch(...)
		    {
			if(fsal != nullptr)
			    delete fsal;
			throw;
		    }
		}

		    //
		    // hard link detection
		    //

		if(ref == nullptr)
		    throw Ememory("filesystem_hard_link_read::make_read_entree");

		if(buf.st_nlink > 1 && see_hard_link && dynamic_cast<cat_directory *>(ref) == nullptr)
		{
		    map<node, couple>::iterator it = corres_read.find(node(buf.st_ino, buf.st_dev));

		    if(it == corres_read.end()) // inode not yet seen, creating the cat_etoile object
		    {
			cat_inode *ino_ref = dynamic_cast<cat_inode *>(ref);
			cat_etoile *tmp_et = nullptr;

			if(ino_ref == nullptr)
			    throw SRC_BUG;
			tmp_et = new (nothrow) cat_etoile(ino_ref, etiquette_counter++);
			if(tmp_et == nullptr)
			    throw Ememory("filesystem_hard_link_read::make_read_entree");
			try
			{
			    ref = nullptr; // the object pointed to by ref is now managed by tmp_et
			    couple tmp = couple(tmp_et, buf.st_nlink - 1);
			    pair <node, couple> p_tmp(node(buf.st_ino, buf.st_dev), tmp);
			    corres_read.insert(p_tmp);
			    it = corres_read.find(node(buf.st_ino,buf.st_dev));
			    if(it == corres_read.end())
				throw SRC_BUG; // the addition of the entry to the map failed !!!
			    else
				it->second.obj->get_inode()->change_name("");
				// name of inode attached to an cat_etoile is not used so we don't want to waste space in this field.
			}
			catch(...)
			{
			    if(tmp_et != nullptr)
				delete tmp_et;
			    throw;
			}

			ref = new (nothrow) cat_mirage(name, tmp_et);
		    }
		    else // inode already seen creating a new cat_mirage on the given cat_etoile
		    {
			    // some sanity checks
			if(it->second.obj == nullptr)
			    throw SRC_BUG;

			if(ref != nullptr)
			    delete ref;  // we don't need this just created inode as it is already attached to the cat_etoile object
			ref = new (nothrow) cat_mirage(name, it->second.obj);
			if(ref != nullptr)
			{
			    it->second.count--;
			    if(it->second.count == 0)
				corres_read.erase(it);
				// this deletes the couple entry, implying the release of memory used by the holder object, but the cat_etoile will only be destroyed once its internal counter drops to zero
			}
		    }
		}

		if(ref == nullptr)
		    throw Ememory("filesystem_hard_link_read::make_read_entree");
	    }
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

        return ref;
    }

    static void attach_ea(const string &chemin, cat_inode *ino, const mask & ea_mask)
    {
        ea_attributs *eat = nullptr;
        try
        {
            if(ino == nullptr)
                throw SRC_BUG;
            eat = ea_filesystem_read_ea(chemin, ea_mask);
            if(eat != nullptr)
            {
		if(eat->size() <= 0)
		    throw SRC_BUG;
                ino->ea_set_saved_status(ea_saved_status::full);
                ino->ea_attach(eat);
                eat = nullptr; // allocated memory now managed by the cat_inode object
            }
            else
                ino->ea_set_saved_status(ea_saved_status::none);
        }
        catch(...)
        {
            if(eat != nullptr)
                delete eat;
            throw;
        }
        if(eat != nullptr)
            throw SRC_BUG;
    }

} // end of namespace
