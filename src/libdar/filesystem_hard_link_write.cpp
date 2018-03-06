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

#include "filesystem_hard_link_write.hpp"
#include "tools.hpp"
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
    static void make_owner_perm(user_interaction & dialog,
				const cat_inode & ref,
				const string & chem,
				bool dir_perm,
				cat_inode::comparison_fields what_to_check,
				const fsa_scope & scope);
    static mode_t get_file_permission(const string & path);

    bool filesystem_hard_link_write::raw_set_ea(const cat_nomme *e,
						const ea_attributs & list_ea,
						const string & spot,
						const mask & ea_mask)
    {
        const cat_mirage *e_mir = dynamic_cast<const cat_mirage *>(e);

        bool ret = false;

        try
        {
            if(e == nullptr)
                throw SRC_BUG;

                // checking that we have not already restored the EA of this
                // inode through another hard link
            if(e_mir != nullptr)
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
            if(e == nullptr)
                throw SRC_BUG;

                // checking that we have not already restored the EA of this
                // inode through another hard link
            if(e_mir != nullptr)
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
	    ea_filesystem_clear_ea(spot, bool_mask(true));
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

        if(ref_ino == nullptr && ref_mir == nullptr)
            throw SRC_BUG; // neither an cat_inode nor a hard link

	const string display = (ou + ref->get_name()).display();
	const char *name = display.c_str();


	S_I ret = -1; // will carry the system call returned value used to create the requested file

	do
	{
	    try
	    {
		if(ref_mir != nullptr) // we potentially have to make a hard link
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
				get_ui().message(tools_printf(gettext("Error creating hard link %s : %s\n Trying to duplicate the inode"),
							      name, tmp.c_str()));
				create_file = true;
				clear_corres_if_pointing_to(ref_mir->get_etiquette(), old); // always succeeds as the etiquette points to "old"
				    // need to remove this entry to be able
				    // to restore EA for other copies
				break;
			    case ENOENT:  // path to the hard link to create does not exit
				if(ref_mir->get_inode()->get_saved_status() == saved_status::saved)
				{
				    create_file = true;
				    clear_corres_if_pointing_to(ref_mir->get_etiquette(), old); // always succeeds as the etiquette points to "old"
					// need to remove this entry to be able
					// to restore EA for other copies
				    get_ui().message(tools_printf(gettext("Error creating hard link : %s , the inode to link with [ %s ] has disappeared, re-creating it"),
								  name, old));

				}
				else
				{
				    create_file = false; // nothing to do;
				    get_ui().message(tools_printf(gettext("Error creating hard link : %s , the inode to link with [ %s ] is not present, cannot restore this hard link"), name, old));
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

		if(ref_dir != nullptr)
		{
		    ret = mkdir(name, 0700); // as the directory has been created we are its owner and we will need only
			// to create files under it so we need all rights for user, by security for now, no right are
			// allowed for group and others, but this will be set properly at the end, when all files will
			// be restored in that directory
		}
		else if(ref_fil != nullptr)
		{
		    generic_file *ou;
		    infinint seek;

		    fichier_local dest = fichier_local(get_pointer(), display, gf_write_only, 0700, false, true, false);
			// the implicit destruction of dest (exiting the block)
			// will close the 'ret' file descriptor (see ~fichier_local())
		    ou = ref_fil->get_data(cat_file::normal, nullptr, nullptr);

		    try
		    {
			const crc *crc_ori = nullptr;
			crc *crc_dyn = nullptr;
			infinint crc_size;

			try
			{

			    if(!ref_fil->get_crc_size(crc_size))
				crc_size = tools_file_size_to_crc_size(ref_fil->get_size());

			    ou->skip(0);
			    ou->read_ahead(ref_fil->get_storage_size());
			    ou->copy_to(dest, crc_size, crc_dyn);

			    if(crc_dyn == nullptr)
				throw SRC_BUG;

			    if(ref_fil->get_crc(crc_ori))
			    {
				if(crc_ori == nullptr)
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
			    if(crc_dyn != nullptr)
				delete crc_dyn;
			    throw;
			}
			if(crc_dyn != nullptr)
			    delete crc_dyn;

			    // nop we do not sync before, so maybe some pages
			    // will be kept in cache for Linux, maybe not for
			    // other systems that support fadvise(2)
			dest.fadvise(fichier_global::advise_dontneed);
		    }
		    catch(...)
		    {
			if(ou != nullptr)
			    delete ou;
			throw;
		    }
		    delete ou;
		    ret = 0; // to report a successful operation at the end of the if/else if chain
		}
		else if(ref_lie != nullptr)
		    ret = symlink(ref_lie->get_target().c_str(), name);
		else if(ref_blo != nullptr)
		    ret = mknod(name, S_IFBLK | 0700, makedev(ref_blo->get_major(), ref_blo->get_minor()));
		else if(ref_cha != nullptr)
		    ret = mknod(name, S_IFCHR | 0700, makedev(ref_cha->get_major(), ref_cha->get_minor()));
		else if(ref_tub != nullptr)
		    ret = mknod(name, S_IFIFO | 0700, 0);
		else if(ref_pri != nullptr)
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
		    if(ref_mir == nullptr)
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
		    if(ref_mir != nullptr)
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

	if(ref_ino != nullptr && ret >= 0)
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

	if(dynamic_cast<const cat_directory *>(&ref) != nullptr && !dir_perm && geteuid() != 0)
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
	    if(ref.get_saved_status() == saved_status::saved)
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

	try
	{
	    if(what_to_check == cat_inode::cf_all || what_to_check == cat_inode::cf_ignore_owner)
		if(ref_lie == nullptr) // not restoring permission for symbolic links, it would modify the target not the symlink itself
		    if(chmod(name, permission) < 0)
		    {
			string tmp = tools_strerror_r(errno);
			dialog.message(tools_printf(gettext("Cannot restore permissions of %s : %s"), name, tmp.c_str()));
		    }
	}
	catch(Egeneric &e)
	{
	    if(ref_lie == nullptr)
		throw;
		// else (the inode is a symlink), we simply ignore this error
	}
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
