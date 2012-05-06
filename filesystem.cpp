/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: filesystem.cpp,v 1.26.1.2 2002/04/25 19:37:17 denis Rel $
//
/*********************************************************************/

#include <errno.h>
#include <dirent.h>
#include <utime.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <list>
#include "tools.hpp"
#include "erreurs.hpp"
#include "filesystem.hpp"
#include "user_interaction.hpp"
#include "catalogue.hpp"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

struct etage
{
    etage() { fichier.clear(); };
    etage(char *dirname)
	{
	    struct dirent *ret;
	    DIR *tmp = opendir(dirname);

	    if(tmp == NULL)
		throw Erange("filesystem etage::etage" , strerror(errno));

	    fichier.clear();
	    while((ret = readdir(tmp)) != NULL)
		if(strcmp(ret->d_name, ".") != 0 && strcmp(ret->d_name, "..") != 0) 
		    fichier.push_back(string(ret->d_name));
	    closedir(tmp);
	}
    
    bool read(string & ref)
	{
	    if(fichier.size() > 0)
	    {
		ref = fichier.front();
		fichier.pop_front();
		return true;
	    }
	    else 
		return false;
	}

    list<string> fichier;
};

static vector<etage> read_pile;
static path *read_current_dir = NULL;
static path *fs_root = NULL;
static path *write_current_dir = NULL;
static vector<directory> write_stack_dir;
static bool filesystem_allow_overwrite = false;
static bool filesystem_warn_overwrite = true;
static bool filesystem_info_details = false;

static inode *make_read_entree(path & lieu, const string & name);
static void make_file(const inode & ref, const path & ou, bool dir_perm);
static void make_owner_perm(const inode & ref, const path & ou, bool dir_perm);
static void supprime(const path & ref);
    
void filesystem_set_root(const path &root, bool allow_overwrite, bool warn_overwrite, bool info_details)
{
    char *ptr = tools_str2charptr(root.display());

    try
    {
	if(fs_root != NULL)
	    delete fs_root;
	fs_root = new path(root);
	if(fs_root == NULL)
	    throw Ememory("filesystem_set_root");
	filesystem_allow_overwrite = allow_overwrite;
	filesystem_warn_overwrite = warn_overwrite;
	filesystem_info_details = info_details;
	
	struct stat buf;
	if(stat(ptr, &buf) < 0)
	    throw Erange("filesystem_set_root", strerror(errno));
	if(!S_ISDIR(buf.st_mode))
	    throw Erange("filesystem_set_root", "root is not a directory");
    }
    catch(...)
    {
	delete ptr;
	throw;
    }
    delete ptr;
}

void filesystem_freemem()
{
    read_pile.clear();
    if(read_current_dir != NULL)
    {
	delete read_current_dir;
	read_current_dir = NULL;
    }
    if(fs_root != NULL)
    {
	delete fs_root;
	fs_root = NULL;
    }
    if(write_current_dir != NULL)
    {
	delete write_current_dir;
	write_current_dir = NULL;
    }
    write_stack_dir.clear();
}
 
void filesystem_reset_read()
{
    char *tmp;

    if(read_current_dir != NULL)
	delete read_current_dir;
    read_current_dir = new path(*fs_root);
    if(read_current_dir == NULL)
	throw Ememory("reset_read");
    read_pile.clear();
    tmp = tools_str2charptr(read_current_dir->display());
    try
    {
	read_pile.push_back(etage(tmp));
    }
    catch(...)
    {
	delete tmp;
	throw;
    }
    delete tmp;
}

bool filesystem_read(entree * & ref)
{
    ref = NULL;
    bool once_again;

    if(read_current_dir == NULL)
	throw Erange("filesystem_read", "filesystem module not initialized, call filesystem_set_root first then filesystem_reset_read");
    
    do
    {
	once_again = false;

	if(read_pile.size() == 0)
	    return false; // end of filesystem reading
	else
	{
	    etage & inner = read_pile.back();
	    string name;
	    
	    if(!inner.read(name))
	    {
		string tmp;
		
		read_pile.pop_back();
		if(read_pile.size() > 0)
		{
		    if(! read_current_dir->pop(tmp))
			throw SRC_BUG;
		    ref = new eod();
		}
		else
		    return false; // end of filesystem
	    }
	    else
	    {
		ref = make_read_entree(*read_current_dir, name);
		if(dynamic_cast<directory *>(ref) != NULL)
		{
		    char *ptr_name;
		    *read_current_dir += name;
		    ptr_name = tools_str2charptr(read_current_dir->display());
		    
		    try
		    {
			read_pile.push_back(etage(ptr_name));
		    }
		    catch(Erange & e)
		    {
			string tmp;
			
			user_interaction_warning(string("error openning ") + ptr_name + " : " + e.get_message());
			try
			{
			    read_pile.push_back(etage());
			}
			catch(Erange & e)
			{
			    delete ref;
			    once_again = true;
			    ref = NULL;
			    if(! read_current_dir->pop(tmp))
				throw SRC_BUG;
			}
		    }
		    catch(Egeneric & e)
		    {
			delete ptr_name;
			delete ref;
			throw;
		    }

		    delete ptr_name;
		}

		if(ref == NULL)
		    once_again = true;
		    // the file has been removed between from the time
		    // the directory has been openned, and the time
		    // we try to read it, so we ignore it.
	    }
	}
    }
    while(once_again);

    if(ref == NULL)
	throw Ememory("filesystem_read");
    else
	return true;
}

void filesystem_skip_read_to_parent_dir()
{
    string tmp;

    if(read_pile.size() == 0)
	throw SRC_BUG;
    else
	read_pile.pop_back();
    
    if(! read_current_dir->pop(tmp))
	throw SRC_BUG;
}
	
void filesystem_reset_write()
{
    write_stack_dir.clear();
    if(write_current_dir != NULL)
	delete write_current_dir;
    write_current_dir = new path(*fs_root);
    if(write_current_dir == NULL)
	throw Ememory("filesystem_reset_write"); 
}
 
static inode *make_read_entree(path & lieu, const string & name)
{
    char *ptr_name = tools_str2charptr((lieu + path(name)).display());
    inode *ref = NULL;
    
    try
    {
	struct stat buf;

	if(lstat(ptr_name, &buf) < 0)
	{
	    if(errno != ENOENT)
		throw Erange("filesystem make_read_entree", string("lstat(") + ptr_name + ") : " + strerror(errno));
		// else function returns NULL (meaning file does not exists)
	}
	else
	{ 
	    if(S_ISLNK(buf.st_mode))
	    {
		const int buf_size = 4096;
		char buffer[buf_size];
		
		int sz = readlink(ptr_name, buffer, buf_size);
		if(sz < 0)
		    throw Erange("filesystem_read", strerror(errno));
		if(sz == buf_size)
		    throw Erange("filesystem_read", string("link target name too long, increase buffer size in source code file ") + __FILE__);
		else
		    buffer[sz] = '\0';
		
		ref = new lien(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
			       buf.st_atime,
			       buf.st_mtime,
			       name,
			       buffer);
	    }
	    else if(S_ISREG(buf.st_mode))
		ref = new file(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
			       buf.st_atime,
			       buf.st_mtime,
			       name,
			       lieu,
			       buf.st_size);
	    else if(S_ISDIR(buf.st_mode))
	    {
		ref = new directory(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
				    buf.st_atime,
				    buf.st_mtime,
				    name);
	    }
	    else if(S_ISCHR(buf.st_mode))
		ref = new chardev(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
				  buf.st_atime,
				  buf.st_mtime,
				  name,
				  major(buf.st_rdev),
				  minor(buf.st_rdev)); // makedev(major, minor)
	    else if(S_ISBLK(buf.st_mode))
		ref = new blockdev(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
				   buf.st_atime,
				   buf.st_mtime,
				   name,
				   major(buf.st_rdev),
				   minor(buf.st_rdev)); // makedev(major, minor)
	    else if(S_ISFIFO(buf.st_mode))
		ref = new tube(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
			       buf.st_atime,
			       buf.st_mtime,
			       name);
	    else if(S_ISSOCK(buf.st_mode))
		ref = new prise(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
				buf.st_atime,
				buf.st_mtime,
				name);
	    else
		throw Erange("make_read_entree", string("unknown file type ! file is ") + string(ptr_name));

	    if(ref == NULL)
		throw Ememory("make_read_entree");
	}
    }
    catch(...)
    {
	delete ptr_name;
	throw;
    }
    delete ptr_name;

    return ref;
}

void filesystem_write(const entree *x)
{
    const eod *x_eod = dynamic_cast<const eod *>(x);
    const directory *x_dir = dynamic_cast<const directory *>(x);
    const nomme *x_nom = dynamic_cast<const nomme *>(x);
    
    if(x_eod != NULL)
    {
	string tmp;
	write_current_dir->pop(tmp);
	make_owner_perm(write_stack_dir.back(), *write_current_dir, true);
	write_stack_dir.pop_back();
    }
    else 
	if(x_nom == NULL)
	    throw SRC_BUG; // neither "nomme" nor "eod"
	else
	{ 
	    path spot = *write_current_dir + x_nom->get_name();
	    const detruit *x_det = dynamic_cast<const detruit *>(x);
	    const inode *x_ino = dynamic_cast<const inode *>(x);

	    inode *exists = make_read_entree(*write_current_dir, x_nom->get_name());

	    try
	    {
		if(x_ino == NULL && x_det == NULL)
		    throw SRC_BUG; // should be either inode or detruit
		
		if(x_det != NULL) // this is an object of class "detruit"
		{
		    if(exists != NULL) // the file to destroy exists
		    {
			if(!filesystem_allow_overwrite)
			    throw Erange("filesystem_write", spot.display() + " will not be remove from filesystem, overwritting not allowed");
			if(filesystem_warn_overwrite)
			    user_interaction_pause(spot.display() + " is about to be removed from filesystem, continue ? ");

			if(tolower(exists->signature()) == tolower(x_det->get_signature()))
			    supprime(spot);
			else
			{
				// warning even if just allow_overwite is set (not espetially warn_overwrite
			    user_interaction_pause(spot.display() + " must be removed, but does not match expected type, remove it anyway ?");
			    supprime(spot);
			}
		    }
		}
		else // inode which is not a detruit
		{
		    if(exists == NULL) // nothing of this name in filesystem
			make_file(*x_ino, *write_current_dir, false);
		    else // an entry of this name exists in filesystem
		    {
			const inode *x_ino = dynamic_cast<const inode *>(x);
			inode *exists_ino = dynamic_cast<inode *>(exists);
			
			if(x_ino == NULL || exists_ino == NULL)
			    throw SRC_BUG; // should be both of class inode
			
			if(filesystem_allow_overwrite)
			{
			    if(filesystem_warn_overwrite && x_dir == NULL)
				user_interaction_pause(spot.display() + " is about to be overwritten, OK ?");
			    if(x_ino->same_as(*exists_ino))
			    {
				if(x_dir == NULL)
				{
				    supprime(spot);
				    make_file(*x_ino, *write_current_dir, false);
				}
				else
				    make_owner_perm(*x_ino, *write_current_dir, false);
			    }
			    else
			    {
				supprime(spot);
				make_file(*x_ino, *write_current_dir, false);
			    }
			}
			else
			    if(filesystem_info_details)
				user_interaction_warning(spot.display() + " has not been overwritten (action not allowed)");
		    }
		    
		    if(x_dir != NULL)
		    {
			*write_current_dir += x_dir->get_name();
			write_stack_dir.push_back(directory(*x_dir));
		    }
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

void filesystem_pseudo_write(const directory *dir)
{
    if(dir == NULL)
	throw SRC_BUG;

    path spot = *write_current_dir + dir->get_name();
    inode *exists = make_read_entree(*write_current_dir, dir->get_name());
    
    try
    {
	if(exists == NULL)
	    make_file(*dir, *write_current_dir, false);  // need to create the directory to be able to restore any file in it
	else
	{
	    const directory *e_dir = dynamic_cast<const directory *>(exists);
	    
	    if(e_dir == NULL) // an inode of that name exists, but it is not a directory
	    {
		if(filesystem_allow_overwrite)
		{
		    if(filesystem_warn_overwrite)
			user_interaction_pause(spot.display() + " is about to be removed and replaced by a directory, OK ?");
		    supprime(spot);
		    make_file(*dir, *write_current_dir, false);
		}
		else
		    throw Erange("filesystem_pseudo_write", 
				 spot.display() + 
				 " could not be restored, because a file of that name exists and overwrite is not allowed");
	    }
	    else // just setting permission to allow creation of any sub-dir or sub_file
	    {
		char *name = tools_str2charptr(spot.display());
		try
		{
		    if(chmod(name, 0777) < 0)
			throw Erange("filesystem_pseudo_write", spot.display() + " : " + strerror(errno));
		}
		catch(...)
		{
		    delete name;
		    throw;
		}
		delete name;
	    }
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
		
    *write_current_dir += dir->get_name();
    write_stack_dir.push_back(directory(*dir));
}

static void supprime(const path & ref)
{
    char *s = tools_str2charptr(ref.display());

    try
    {
	struct stat buf;
	if(lstat(s, &buf) < 0)
	    throw Erange("filesystem supprime", strerror(errno));

	if(S_ISDIR(buf.st_mode))
	{
	    etage fils = s;
	    string tmp;
	    
	    while(fils.read(tmp))
		supprime(ref+tmp);
	    
	    if(rmdir(s) < 0)
		throw Erange("supprime (dir)", strerror(errno));
	}
	else
	    if(unlink(s) < 0)
		throw Erange("supprime (file)", strerror(errno));
    }
    catch(...)
    {
	delete s;
	throw;
    }

    delete s;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: filesystem.cpp,v 1.26.1.2 2002/04/25 19:37:17 denis Rel $";
    dummy_call(id);
}

static void make_file(const inode & ref, const path & ou, bool dir_perm)
{
    const directory *ref_dir = dynamic_cast<const directory *>(&ref);
    const file *ref_fil = dynamic_cast<const file *>(&ref);
    const lien *ref_lie = dynamic_cast<const lien *>(&ref);
    const blockdev *ref_blo = dynamic_cast<const blockdev *>(&ref);
    const chardev *ref_cha = dynamic_cast<const chardev *>(&ref);
    const tube *ref_tub = dynamic_cast<const tube *>(&ref);
    const prise *ref_pri = dynamic_cast<const prise *>(&ref);

    char *name = tools_str2charptr((ou + ref.get_name()).display());

    try
    {
	int ret;
	
	do
	{
	    if(ref_dir != NULL)
	    {
		ret = mkdir(name, 0777);
	    } 
	    else if(ref_fil != NULL)
	    {
		generic_file *ou;
		infinint seek;
		
		ret = open(name, O_WRONLY|O_CREAT, 0777);
		if(ret >= 0)
		{
		    fichier dest = ret;
		    ou = ref_fil->get_data();
		    try
		    {
			ou->skip(0);
			ou->copy_to(dest);
		    }
		    catch(...)
		    {
			delete ou;
			throw;
		    }
		    delete ou;
		}
	    }
	    else if(ref_lie != NULL)
	    {
		char *cible = tools_str2charptr(ref_lie->get_target());
		ret = symlink(cible ,name);
		delete cible;
	    }
	    else if(ref_blo != NULL)
		ret = mknod(name, S_IFBLK | 0777, makedev(ref_blo->get_major(), ref_blo->get_minor()));
	    else if(ref_cha != NULL)
		ret = mknod(name, S_IFCHR | 0777, makedev(ref_cha->get_major(), ref_cha->get_minor()));
	    else if(ref_tub != NULL)
		ret = mknod(name, S_IFIFO | 0777, 0);
	    else if(ref_pri != NULL)
	    {
		ret = socket(PF_UNIX, SOCK_STREAM, 0);
		if(ret >= 0)
		{
		    int sd = ret;
		    struct sockaddr_un addr;
		    addr.sun_family = AF_UNIX;
		    
		    try
		    {
			strncpy(addr.sun_path, name, UNIX_PATH_MAX - 1);
			addr.sun_path[UNIX_PATH_MAX - 1] = '\0';
			if(bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			    throw Erange("make_file (socket bind)", strerror(errno));
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
		throw SRC_BUG; // unknown inode type
	    
	    if(ret < 0)
		if(ret != ENOSPC)
		    throw Erange("make_file (directory)", strerror(errno));
		else
		    user_interaction_pause(string("can't create inode : ") + strerror(errno) + " Ready to continue ? ");
	}
	while(ret == ENOSPC);
	
	make_owner_perm(ref, ou, dir_perm);
    }
    catch(...)
    {
	delete name;
	throw;
    }

    delete name;
}

static void make_owner_perm(const inode & ref, const path & ou, bool dir_perm)
{
    char *name = tools_str2charptr((ou + ref.get_name()).display());
    const lien *ref_lie = dynamic_cast<const lien *>(&ref);
    int permission;

    if(dynamic_cast<const directory *>(&ref) != NULL && !dir_perm)
	permission = 0777;
    else
	permission = ref.get_perm();
    
    try
    {
	if(lchown(name, ref.get_uid(), ref.get_gid()) < 0)
	    if(ref.get_saved_status())
		user_interaction_warning(string(name) + string(" could not restore original ownership of file : ") + strerror(errno));

	try
	{
	    if(ref_lie == NULL) // not restoring permission for symbolic links 
		if(chmod(name, permission) < 0)
		    throw Erange("make_owner_perm (permission)", strerror(errno));
	}
	catch(Egeneric &e)
	{
	    if(ref_lie == NULL)
		throw;
		// else (the inode is a symlink), we simply ignore this error
	}
	
	struct utimbuf temps;
	unsigned long tmp = 0;
	ref.get_last_access().unstack(tmp);
	temps.actime = tmp;
	tmp = 0;
	ref.get_last_modif().unstack(tmp);
	temps.modtime = tmp;

	if(ref_lie == NULL) // not restoring atime & ctime for symbolic links
	    if(utime(name, &temps) < 0)
		Erange("make_owner_perm (date&time)", strerror(errno));	

    }
    catch(...)
    {
	delete name;
	throw;
    }
    delete name;
}
