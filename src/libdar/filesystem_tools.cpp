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

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if HAVE_STRING_H
# include <string.h>
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

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
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

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#ifdef LIBDAR_NODUMP_FEATURE
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if HAVE_LINUX_EXT2_FS_H
#include <linux/ext2_fs.h>
#endif
#if HAVE_EXT2FS_EXT2_FS_H
#include <ext2fs/ext2_fs.h>
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

#include "filesystem_tools.hpp"
#include "erreurs.hpp"
#include "generic_rsync.hpp"
#include "null_file.hpp"
#include "ea_filesystem.hpp"
#include "cygwin_adapt.hpp"
#include "etage.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    bool filesystem_tools_has_immutable(const cat_inode & arg)
    {
        if(arg.fsa_get_saved_status() == fsa_saved_status::full)
        {
            const filesystem_specific_attribute_list *fsal = arg.get_fsa();
            const filesystem_specific_attribute *fsa = nullptr;

            if(fsal == nullptr)
                throw SRC_BUG;

            if(fsal->find(fsaf_linux_extX, fsan_immutable, fsa))
            {
                const fsa_bool *fsab = dynamic_cast<const fsa_bool *>(fsa);
                if(fsab == nullptr)
                    throw SRC_BUG;
                return fsab->get_value();
            }
            else
                return false;
        }
        else
            return false;
    }

    void filesystem_tools_set_immutable(const string & target, bool val, user_interaction &ui)
    {
        fsa_bool fsa(fsaf_linux_extX, fsan_immutable, val);
        filesystem_specific_attribute_list fsal;

        fsal.add(fsa);
        (void)fsal.set_fsa_to_filesystem_for(target, all_fsa_families(), ui, true);
    }

    void filesystem_tools_supprime(user_interaction & ui, const string & ref)
    {
        const char *s = ref.c_str();

        struct stat buf;
        if(lstat(s, &buf) < 0)
            throw Erange("filesystem_tools_supprime", string(gettext("Cannot get inode information about file to remove ")) + s + " : " + tools_strerror_r(errno));

        if(S_ISDIR(buf.st_mode))
        {
            etage fils = etage(ui, s, datetime(0), datetime(0), false, false); // we don't care the access and modification time because directory will be destroyed
            string tmp;
	    inode_type tp;

                // first we destroy directory's children
            while(fils.read(tmp, tp))
	    {
		if(tp == inode_type::isdir)
		    filesystem_tools_supprime(ui, (path(ref).append(tmp)).display());
		else
		    tools_unlink((path(ref).append(tmp)).display());
	    }

                // then the directory itself
            if(rmdir(s) < 0)
                throw Erange("filesystem_tools_supprime (dir)", string(gettext("Cannot remove directory ")) + s + " : " + tools_strerror_r(errno));
        }
        else
            tools_unlink(s);
    }

    void filesystem_tools_widen_perm(user_interaction & dialog,
				     const cat_inode & ref,
				     const string & chem,
				     comparison_fields what_to_check)
    {
	const cat_directory *ref_dir = dynamic_cast<const cat_directory *>(&ref);
        S_I permission;
	const char *name = chem.c_str();

	if(ref_dir == nullptr)
	    return;
	    // nothing to do if not a directory

	if(what_to_check != comparison_fields::all
	   && what_to_check == comparison_fields::ignore_owner)
	    return;
	    // we do nothing if permission is not to take into account
	    // for the operation


	    // if we are not root (geteuid()!=0), we must try to have a chance to
            // be able to create/modify sub-files or sub-directory, so we set the user write access to
            // that directory. This chmod is allowed only if we own the directory (so
            // we only set the write bit for user). If this cannot be changed we are not the
            // owner of the directory, so we will try to restore as much as our permission
            // allows it (maybe "group" or "other" write bits are set for us).
        if(geteuid() != 0)
        {
            mode_t tmp;
            try
            {
                tmp = filesystem_tools_get_file_permission(name); // the current permission value
            }
            catch(Egeneric & e)
            {
                tmp = ref.get_perm(); // the value at the time of the backup if we failed reading permission from filesystem
            }
            permission =  tmp | 0200; // add user write access to be able to add some subdirectories and files
        }
        else
            permission = ref.get_perm() | 0200;

	(void)chmod(name, permission);
	    // we ignore if that failed maybe we will be more lucky
	    // if for example group ownership gives us the right to
	    // write in this existing directory
    }

    void filesystem_tools_make_owner_perm(user_interaction & dialog,
                                          const cat_inode & ref,
                                          const string & chem,
                                          comparison_fields what_to_check,
                                          const fsa_scope & scope)
    {
        const char *name = chem.c_str();
        const cat_lien *ref_lie = dynamic_cast<const cat_lien *>(&ref);

            // restoring fields that are defined by "what_to_check"

	    // do we have to restore ownership?

	if(what_to_check == comparison_fields::all)
	{
	    uid_t tmp_uid = 0;
	    gid_t tmp_gid = 0;
	    infinint tmp = ref.get_uid();
	    tmp.unstack(tmp_uid);
	    if(!tmp.is_zero())
		throw Erange("make_owner_perm", gettext("uid value is too high for this system for libdar be able to restore it properly"));
	    tmp = ref.get_gid();
	    tmp.unstack(tmp_gid);
	    if(!tmp.is_zero())
		throw Erange("make_owner_perm", gettext("gid value is too high for this system for libdar be able to restore it properly"));

#if HAVE_LCHOWN
	    if(lchown(name, tmp_uid, tmp_gid) < 0)
		dialog.message(chem + string(gettext("Could not restore original file ownership: ")) + tools_strerror_r(errno));
#else
	    if(dynamic_cast<const cat_lien *>(&ref) == nullptr) // not a symbolic link
		if(chown(name, tmp_uid, tmp_gid) < 0)
		    dialog.message(chem + string(gettext("Could not restore original file ownership: ")) + tools_strerror_r(errno));
		//
		// we don't/can't restore ownership for symbolic links (no system call to do that)
		//
#endif
	}

	    // do we have to restore permissions?

	    // note! permission must be restored after owner ship:
	    // in case suid bits would be set, restoring ownership
	    // would clear the suid bit, leading to improper restored
	    // permission

	try
	{
            if(what_to_check == comparison_fields::all || what_to_check == comparison_fields::ignore_owner)
                if(ref_lie == nullptr) // not restoring permission for symbolic links, it would modify the target not the symlink itself
		{
                    if(chmod(name, ref.get_perm()) < 0)
                    {
                        string tmp = tools_strerror_r(errno);
                        dialog.message(tools_printf(gettext("Cannot restore permissions of %s : %s"), name, tmp.c_str()));
                    }
		}
	}
	catch(Egeneric &e)
        {
            if(ref_lie == nullptr)
                throw;
                // else (the inode is a symlink), we simply ignore this error
        }
    }

    void filesystem_tools_make_date(const cat_inode & ref,
                                    const string & chem,
                                    comparison_fields what_to_check,
                                    const fsa_scope & scope)
    {
        const cat_lien *ref_lie = dynamic_cast<const cat_lien *>(&ref);

        if(what_to_check == comparison_fields::all || what_to_check == comparison_fields::ignore_owner || what_to_check == comparison_fields::mtime)
        {
            datetime birthtime = ref.get_last_modif();
            fsa_scope::iterator it = scope.find(fsaf_hfs_plus);

            if(ref.fsa_get_saved_status() == fsa_saved_status::full && it != scope.end())
            {
                const filesystem_specific_attribute_list * fsa = ref.get_fsa();
                const filesystem_specific_attribute *ptr = nullptr;

                if(fsa == nullptr)
                    throw SRC_BUG;
                if(fsa->find(fsaf_hfs_plus, fsan_creation_date, ptr)
		   || fsa->find(fsaf_linux_extX, fsan_creation_date, ptr))
                {
                    const fsa_time *ptr_time = dynamic_cast<const fsa_time *>(ptr);
                    if(ptr_time != nullptr)
                        birthtime = ptr_time->get_value();
                }
            }

            tools_make_date(chem, ref_lie != nullptr, ref.get_last_access(), ref.get_last_modif(), birthtime);
        }
    }

    void filesystem_tools_attach_ea(const string &chemin, cat_inode *ino, const mask & ea_mask)
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


    bool filesystem_tools_is_nodump_flag_set(user_interaction & dialog,
                                             const path & chem, const string & filename, bool info)
    {
#ifdef LIBDAR_NODUMP_FEATURE
        S_I fd, f = 0;
        const string display = (chem.append(filename)).display();
        const char *ptr = display.c_str();

        fd = ::open(ptr, O_RDONLY|O_BINARY|O_NONBLOCK);
        if(fd < 0)
        {
            if(info)
            {
                string tmp = tools_strerror_r(errno);
                dialog.message(tools_printf(gettext("Failed to open %S while checking for nodump flag: %s"), &filename, tmp.c_str()));
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
                            dialog.message(tools_printf(gettext("Cannot get ext2 attributes (and nodump flag value) for %S : %s"), &filename, tmp.c_str()));
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

    path *filesystem_tools_get_root_with_symlink(user_interaction & dialog,
                                                 const path & root,
                                                 bool info_details)
    {
        path *ret = nullptr;
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
            ret = new (nothrow) path(root);
            if(ret == nullptr)
                throw  Ememory("get_root_with_symlink");
        }
        else if(S_ISLNK(buf.st_mode))
        {
            ret = new (nothrow) path(tools_readlink(ptr));
            if(ret == nullptr)
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
                dialog.message(tools_printf(gettext("Replacing %s in the -R option by the directory pointed to by this symbolic link: "), ptr) + ret->display());
        }
        else // not a directory given as argument
            throw Erange("filesystem:get_root_with_symlink", tools_printf(gettext("The given path %s must be a directory (or symbolic link to an existing directory)"), ptr));

        if(ret == nullptr)
            throw SRC_BUG; // exit without exception, but ret not allocated !

        return ret;
    }

    mode_t filesystem_tools_get_file_permission(const string & path)
    {
        struct stat buf;

        if(lstat(path.c_str(), &buf) < 0)
        {
            string tmp = tools_strerror_r(errno);
            throw Erange("filesystem.cpp:get_file_permission", tools_printf("Cannot read file permission for %s: %s",path.c_str(), tmp.c_str()));
        }

        return buf.st_mode;
    }

    void filesystem_tools_make_delta_patch(const shared_ptr<user_interaction> & dialog,
                                           const cat_file & existing,
                                           const string & existing_pathname,
                                           const cat_file & patcher,
                                           const path & cur_directory,
					   bool hard_linked)
    {
        infinint patch_crc_size = tools_file_size_to_crc_size(patcher.get_size());
        infinint base_crc_size = tools_file_size_to_crc_size(existing.get_size());
        crc * calculated_patch_crc = nullptr;      ///< calculated CRC of the read patch data
        crc * calculated_base_crc = nullptr;       ///< calculated CRC of the base file to be patched
        const crc *expected_patch_crc = nullptr;   ///< expected CRC of the patched data
        const crc *expected_base_crc = nullptr;    ///< expected CRC of the base file to be patched
        const crc *expected_result_crc = nullptr;  ///< expected CRC of the resulting patched file
        string temporary_pathname;
        fichier_local *resulting = nullptr;        ///< used to temporarily saved to patch result
        generic_file *current = nullptr;           ///< will hold the data from the cat_file "existing"
        generic_file *delta = nullptr;             ///< will hold the data from the cat_file "patcher"
        generic_rsync *rdiffer = nullptr;          ///< interface to rsync to apply the patch
        null_file black_hole = gf_write_only;
	bool disable_base_check = patcher.get_archive_version() == archive_version(11,2); ///< workaround bug in that format
	bool mismatch = false; // used to record base crc mismatch when hard_linked is true
	string patcherror; // used to record error message of the patching operation while hard_linked is true

            // sanity checks

        if(!dialog)
            throw SRC_BUG; // dialog points to nothing
        if(existing.get_saved_status() != saved_status::saved)
            throw SRC_BUG;
        if(patcher.get_saved_status() != saved_status::delta)
            throw SRC_BUG;

            //

        try
        {
            try
            {

                    // creating a temporary file to write the result of the patch to

                resulting = filesystem_tools_create_non_existing_file_based_on(dialog,
                                                                               existing.get_name(),
                                                                               cur_directory,
                                                                               temporary_pathname);
                if(resulting == nullptr)
                    throw SRC_BUG;
                    // we do not activate CRC at that time because
                    // we have no clue of the resulting file size, thus
                    // of the crc size to use


                    // obtaining current file and calculating its CRC

                current = existing.get_data(cat_file::plain, nullptr, 0, nullptr);
                if(current == nullptr)
                    throw SRC_BUG;
                else
                {
		    if(! disable_base_check)
		    {
			    // calculating the crc of base file

			    // note: this file will be read with a mix of skip()
			    // by the generic_rsync object below, thus is is not
			    // possible to calculate its CRC at tha time, so we
			    // do it now for that reason
			current->reset_crc(base_crc_size);
			current->copy_to(black_hole);
			calculated_base_crc = current->get_crc();
			if(calculated_base_crc == nullptr)
			    throw SRC_BUG;
			current->skip(0);
		    }
                }

                    // obtaining the patch data

                delta = patcher.get_data(cat_file::plain, nullptr, 0, nullptr);
                if(delta == nullptr)
                    throw SRC_BUG;
                else
                    delta->reset_crc(patch_crc_size);


                    // creating the patcher object (read-only object)

                rdiffer = new (nothrow) generic_rsync(current,
                                                      delta);
                if(rdiffer == nullptr)
                    throw Ememory("filesystem_restore::make_delta_patch");


                    // patching the existing file to the resulting inode (which is a new file)

		try
		{
		    rdiffer->copy_to(*resulting);
		    rdiffer->terminate();
		    resulting->terminate();
		}
		catch(Ebug & e)
		{
		    throw;
		}
		catch(Egeneric & e)
		{
		    if(!hard_linked)
			throw;
		    patcherror = e.get_message();
			// we record the error message, but lose the exception type (but this is data related, thus Edata)
		}

                    // obtaining the expected CRC of the base file to patch
		    // CRC must be fetched after the patch has been read for it can be done in a sequential read mode

                if(!patcher.has_patch_base_crc())
                    throw SRC_BUG; // s_delta should have a ref CRC
                if(!patcher.get_patch_base_crc(expected_base_crc))
                    throw SRC_BUG; // has CRC true but fetching CRC failed!
                if(expected_base_crc == nullptr)
                    throw SRC_BUG;

                    // comparing the expected base crc with the calculated one

                if(!disable_base_check && *calculated_base_crc != *expected_base_crc)
		{
		    if(hard_linked)
			mismatch = true;
		    else
			throw Erange("filesystem.cpp::make_delta_patch", gettext("File the patch is about to be applied to is not the expected one, aborting the patch operation"));
		}
		else
		{
		    if(! patcherror.empty())
			throw Edata(tools_printf(gettext("Error trying to binary-patch hard-linked file while its base CRC was correct: %S"),
						 &patcherror));
		}


                    // reading the calculated CRC of the patch data

                calculated_patch_crc = delta->get_crc();
                if(calculated_patch_crc == nullptr)
                    throw SRC_BUG;

                    // checking the calculated CRC match the expected CRC for patch data

                if(patcher.get_crc(expected_patch_crc))
                {
                    if(expected_patch_crc == nullptr)
                        throw SRC_BUG;
                    if(*expected_patch_crc != *calculated_patch_crc)
                        throw Erange("filesystem.cpp::make_delta_patch", gettext("Patch data does not match its CRC, archive corruption took place"));
                }
                else
                    throw SRC_BUG; // at the archive format that support delta patch CRC is always present


                    // reading the expected CRC of the resulting patched file
                    // it will be provided for comparision with resulting data
                    // when copying content from temporary file to destination file

                if(!patcher.has_patch_result_crc())
                    throw SRC_BUG;
                if(!patcher.get_patch_result_crc(expected_result_crc))
                    throw SRC_BUG;
                if(expected_result_crc == nullptr)
                    throw SRC_BUG;


                    // replacing the original source file by the resulting patched file.
                    // doing that way to avoid loosing hard links toward that inode instead
                    // of unlinking the old inode and rename the tempory to the name of the
                    // original file
                try
                {
		    if(!mismatch)
			filesystem_tools_copy_content_from_to(dialog,
							      temporary_pathname,
							      existing_pathname,
							      expected_result_crc);
		    else
		    {
			    // checking whether the inode has not already been patched (hard-link context)
			if(*calculated_base_crc != *expected_result_crc)
			    throw Erange("filesystem.cpp::make_delta_patch", gettext("File the patch is about to be applied to is not the expected one, aborting the patch operation"));
		    }
                }
                catch(Erange & e)
                {
                    e.prepend_message(gettext("Error met while checking the resulting patched file: "));
                    throw;
                }
            }
            catch(...)
            {
                if(rdiffer != nullptr)
                    delete rdiffer;
                if(delta != nullptr)
                    delete delta;
                if(current != nullptr)
                    delete current;
		existing.clean_data();
                if(resulting != nullptr)
                    delete resulting;
                if(calculated_patch_crc != nullptr)
                    delete calculated_patch_crc;
                if(calculated_base_crc != nullptr)
                    delete calculated_base_crc;
                throw;
            }

            if(rdiffer != nullptr)
                delete rdiffer;
            if(delta != nullptr)
                delete delta;
            if(current != nullptr)
                delete current;
	    existing.clean_data();
            if(resulting != nullptr)
                delete resulting;
            if(calculated_patch_crc != nullptr)
                delete calculated_patch_crc;
            if(calculated_base_crc != nullptr)
                delete calculated_base_crc;
        }
        catch(...)
        {
            try
            {
                tools_unlink(temporary_pathname);
            }
            catch(...)
            {
                    // do nothing
            }
            throw; // propagate the original exception
        }
        tools_unlink(temporary_pathname);
    }


    fichier_local *filesystem_tools_create_non_existing_file_based_on(const std::shared_ptr<user_interaction> & dialog,
                                                                      string filename,
                                                                      path where,
                                                                      string & new_filename)
    {
        const char *extra = "~#-%.+="; // a set of char that should be accepted in all filesystems
        fichier_local *ret = nullptr;
        U_I index = 0;

        do
        {
            if(!dialog)
                throw SRC_BUG;

            new_filename = filename + extra[++index];
            where += new_filename;
            new_filename = where.display();

            try
            {
                ret = new (nothrow) fichier_local(dialog,
                                                  new_filename,
                                                  gf_read_write,
                                                  0600,
                                                  true, // fail_if_exists
                                                  false,
                                                  false);
            }
            catch(Esystem & e)
            {
                if(e.get_code() == Esystem::io_exist)
                {
                    where.pop(new_filename);
                    if(extra[index] == '\0')
                    {
                        index = 0;
                        filename += string(extra, extra+1);
                    }
                    else
                        ++index;
                }
                else
                    throw;
            }
        }
        while(ret == nullptr);

        return ret;
    }

    void filesystem_tools_copy_content_from_to(const shared_ptr<user_interaction> & dialog,
                                               const string & source_path,
                                               const string & destination_path,
                                               const crc *expected_crc)
    {
        if(!dialog)
            throw SRC_BUG; // dialog points to nothing

        fichier_local src = fichier_local(source_path);
        fichier_local dst = fichier_local(dialog,
                                          destination_path,
                                          gf_write_only,
                                          0600,
                                          false,
                                          true, // erase
                                          false);
        if(expected_crc != nullptr)
            src.reset_crc(expected_crc->get_size());
        src.copy_to(dst);
        if(expected_crc != nullptr)
        {
            crc * calculated_crc = src.get_crc();
            if(calculated_crc == nullptr)
                throw SRC_BUG;
            try
            {
                if(*calculated_crc != *expected_crc)
                    throw Erange("filesystem.cpp:copy_content_from_to", gettext("Copied data does not match expected CRC"));
            }
            catch(...)
            {
                if(calculated_crc != nullptr)
                    delete calculated_crc;
                throw;
            }
            if(calculated_crc != nullptr)
                delete calculated_crc;
        }
    }


    bool filesystem_tools_read_linux_birthtime(const std::string & target, datetime & val)
    {
#ifdef HAVE_STATX_SYSCALL
	struct statx value;

	int ret = statx(0, target.c_str(), 0, STATX_BTIME, &value);
	if(ret == 0)
	    if((value.stx_mask & STATX_BTIME) != 0)
	    {
		val = datetime(value.stx_btime.tv_sec,
			       value.stx_btime.tv_nsec,
			       datetime::tu_nanosecond);
		return true;
	    }
	    else
		return false;
	else
	    return false;
#else
	return false;
#endif
    }

} // end of namespace
