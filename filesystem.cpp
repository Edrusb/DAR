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
// $Id: filesystem.cpp,v 1.49 2002/06/18 21:16:06 denis Rel $
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
#include <map>
#include <ctype.h>
#include "tools.hpp"
#include "erreurs.hpp"
#include "filesystem.hpp"
#include "user_interaction.hpp"
#include "catalogue.hpp"
#include "ea_filesystem.hpp"

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
static infinint read_filename_degree = 0;
static path *fs_root = NULL;
static path *write_current_dir = NULL;
static vector<directory> write_stack_dir;
static bool filesystem_allow_overwrite = false;
static bool filesystem_warn_overwrite = true;
static bool filesystem_info_details = false;
static bool filesystem_ea_root = false;
static bool filesystem_ea_user = false;
static bool filesystem_ignore_ownership = false;

static nomme *make_read_entree(path & lieu, const string & name, bool see_hard_link);
static void make_file(const nomme * ref, const path & ou, bool dir_perm);
static void make_owner_perm(const inode & ref, const path & ou, bool dir_perm);
static void supprime(const path & ref);
static void attach_ea(const string &chemin, inode *ino);
static void clear_corres_write(const infinint & ligne);

///////////////// structure and variable for hard links managment
struct couple
{
    nlink_t count;
    file_etiquette *obj;
};
static map <ino_t, couple> corres_read;

struct corres_ino_ea
{
    string chemin;
    bool ea_restored;
};

static map <infinint, corres_ino_ea> corres_write;
/////////////////


void filesystem_set_root(const path &root, bool allow_overwrite, bool warn_overwrite, bool info_details, bool root_ea, bool user_ea)
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
	filesystem_ea_root = root_ea;
	filesystem_ea_user = user_ea;
	struct stat buf;
	if(stat(ptr, &buf) < 0)
	    throw Erange("filesystem_set_root", string("can't open directory ")+fs_root->display() + " : " + strerror(errno));
	if(!S_ISDIR(buf.st_mode))
	    throw Erange("filesystem_set_root", fs_root->display() + " is not a directory");
    }
    catch(...)
    {
	delete ptr;
	if(fs_root != NULL)
	{
	    delete fs_root;
	    fs_root = NULL;
	}
	throw;
    }
    delete ptr;
}

void filesystem_ignore_owner(bool mode)
{
    filesystem_ignore_ownership = mode;
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
    corres_read.clear();
    corres_write.clear();
    read_filename_degree = 0;
}

void filesystem_reset_read()
{
    char *tmp;

    corres_read.clear();
    file_etiquette::reset_etiquette_counter();
    if(read_current_dir != NULL)
	delete read_current_dir;
    read_current_dir = new path(*fs_root);
    read_filename_degree = 0;
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
		ref = make_read_entree(*read_current_dir, name, true);

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
		    // the file has been removed between the time
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

extern bool filesystem_read_filename(const string & name, nomme * &ref)
{
    directory *ref_dir = NULL;
    ref = make_read_entree(*read_current_dir, name, false);
    if(ref == NULL)
	return false; // no file of that name
    else
    {
	ref_dir = dynamic_cast<directory *>(ref);
	if(ref_dir != NULL)
	{
	    *read_current_dir += ref_dir->get_name();
	    read_filename_degree++;
	}
	return true;
    }
}

extern void filesystem_skip_read_filename_in_parent_dir()
{
    if(read_filename_degree != 0)
    {
	string tmp;
	read_current_dir->pop(tmp);
	read_filename_degree--;
    }
    else
	throw SRC_BUG;
}

void filesystem_reset_write()
{
    corres_write.clear();
    write_stack_dir.clear();
    if(write_current_dir != NULL)
	delete write_current_dir;
    write_current_dir = new path(*fs_root);
    if(write_current_dir == NULL)
	throw Ememory("filesystem_reset_write");
}

static nomme *make_read_entree(path & lieu, const string & name, bool see_hard_link)
{
    char *ptr_name = tools_str2charptr((lieu + path(name)).display());
    nomme *ref = NULL;

    try
    {
	struct stat buf;

	if(lstat(ptr_name, &buf) < 0)
	{
	    switch(errno)
	    {
	    case EACCES:
		user_interaction_warning(string("error reading inode of ") + ptr_name + " : " + strerror(errno));
		break;
	    case ENOENT:
		break;
	    default:
		throw Erange("filesystem make_read_entree", string("lstat(") + ptr_name + ") : " + strerror(errno));
	    }

		// the function returns NULL (meaning file does not exists)
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
	    {
		if(buf.st_nlink == 1 || ! see_hard_link) // file without hard link
		    ref = new file(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
				   buf.st_atime,
				   buf.st_mtime,
				   name,
				   lieu,
				   buf.st_size);
		else // file with hard link(s)
		{
		    map <ino_t, couple>::iterator it = corres_read.find(buf.st_ino);

		    if(it == corres_read.end()) // inode not seen yet, first link on it
		    {
			file_etiquette *ptr = new file_etiquette(buf.st_uid, buf.st_gid, buf.st_mode & 07777,
								 buf.st_atime,
								 buf.st_mtime,
								 name,
								 lieu,
								 buf.st_size);
			ref = ptr;
			if(ref != NULL)
			{
			    couple tmp;
			    tmp.count = buf.st_nlink - 1;
			    tmp.obj = ptr;
			    corres_read[buf.st_ino] = tmp;
			}
		    }
		    else  // inode already seen previously
		    {
			ref = new hard_link(name, it->second.obj);

			if(ref != NULL)
			{
			    it->second.count--;
			    if(it->second.count == 0)
				corres_read.erase(it);
			}
		    }
		}
	    }
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

	    inode *ino = dynamic_cast<inode *>(ref);
	    if(ino != NULL)
	    {
		try
		{
		    attach_ea(ptr_name, ino);
		    if(ino->ea_get_saved_status() != inode::ea_none)
			ino->set_last_change(buf.st_ctime);
		}
		catch(Ebug & e)
		{
		    throw;
		}
		catch(Euser_abort & e)
		{
		    throw;
		}
		catch(Ememory & e)
		{
		    throw;
		}
		catch(Egeneric & ex)
		{
		    user_interaction_warning(string("error reading EA for ")+ptr_name+ " : " + ex.get_message());
			// no throw !
			// we must be able to continue without EA
		}
	    }
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

bool filesystem_write(const entree *x)
{
    const eod *x_eod = dynamic_cast<const eod *>(x);
    const nomme *x_nom = dynamic_cast<const nomme *>(x);

    bool ret = true;

    try
    {
	if(x_eod != NULL)
	{
	    string tmp;
	    write_current_dir->pop(tmp);
	    if(write_stack_dir.size() > 0)
		make_owner_perm(write_stack_dir.back(), *write_current_dir, true);
	    else
		throw SRC_BUG;
	    write_stack_dir.pop_back();
	}
	else
	    if(x_nom == NULL)
		throw SRC_BUG; // neither "nomme" nor "eod"
	    else // nomme
	    {
		path spot = *write_current_dir + x_nom->get_name();
		const detruit *x_det = dynamic_cast<const detruit *>(x);
		const inode *x_ino = dynamic_cast<const inode *>(x);
		const etiquette *x_eti = dynamic_cast<const etiquette *>(x);
		const directory *x_dir = dynamic_cast<const directory *>(x);

		nomme *exists = make_read_entree(*write_current_dir, x_nom->get_name(), false);

		try
		{
		    if(x_ino == NULL && x_det == NULL && x_eti == NULL)
			throw SRC_BUG; // should be either inode or detruit or hard link

		    if(x_det != NULL) // this is an object of class "detruit"
		    {
			if(exists != NULL) // the file to destroy exists
			{
			    if(!filesystem_allow_overwrite)
				throw Erange("filesystem_write", spot.display() + " will not be remove from filesystem, overwriting not allowed");
			    if(filesystem_warn_overwrite)
				user_interaction_pause(spot.display() + " is about to be removed from filesystem, continue ? ");

			    if(tolower(exists->signature()) == tolower(x_det->get_signature()))
				supprime(spot);
			    else
			    {
				// warning even if just allow_overwite is set (not espetially warn_overwrite)
				user_interaction_pause(spot.display() + " must be removed, but does not match expected type, remove it anyway ?");
				supprime(spot);
			    }
			}
		    }
		    else // hard_link or inode
		    {
			if(exists == NULL) // nothing of this name in filesystem
			    make_file(x_nom, *write_current_dir, false);
			else // an entry of this name exists in filesystem
			{
			    const inode *exists_ino = dynamic_cast<inode *>(exists);

			    if((x_eti == NULL && x_ino == NULL) || exists_ino == NULL)
				throw SRC_BUG; // should be both of class inode (or hard_link for x)

			    if(filesystem_allow_overwrite)
			    {
				if(filesystem_warn_overwrite && x_dir == NULL)
				    user_interaction_pause(spot.display() + " is about to be overwritten, OK ?");
				if(x_dir != NULL && x_ino->same_as(*exists_ino))
				    make_owner_perm(*x_ino, *write_current_dir, false);
				else
				{
				    ea_attributs ea; // saving original EA of existing inode
				    bool got_it = true;
				    try
				    {
					ea_filesystem_read_ea(spot.display(), ea, true, true);
				    }
				    catch(Egeneric & ex)
				    {
					got_it = false;
					user_interaction_warning(string("existing EA for ") + spot.display() + " could not be read and preserved : " + ex.get_message());
				    }
				    supprime(spot); // this destroyes EA
				    make_file(x_nom, *write_current_dir, false);
				    try // if possible and available restoring original EA
				    {
					if(got_it)
					    (void)ea_filesystem_write_ea(spot.display(), ea, true, true);
					    // we don't care about the return value, here
				    }
				    catch(Egeneric & e)
				    {
					if(ea.size() >0)
					    user_interaction_warning(string("existing EA for ")+ spot.display() + " could not be preserved : " + e.get_message());
				    }
				}
			    }
			    else // overwriting not allowed
			    {
				if(x_dir != NULL && !x_ino->same_as(*exists_ino))
				    throw Erange("filesystem_write", string("directory ")+spot.display()+" cannot be restored: overwriting not allowed and a non-directory inode of that name exists");
				else
				    if(filesystem_info_details)
					user_interaction_warning(spot.display() + " has not been overwritten (action not allowed)");
				ret = false;
			    }
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
    catch(Euser_abort & e)
    {
	ret = false;
    }

    return ret;
}

nomme *filesystem_get_before_write(const nomme *x)
{
    return make_read_entree(*write_current_dir, x->get_name(), false);
}

void filesystem_pseudo_write(const directory *dir)
{
    if(dir == NULL)
	throw SRC_BUG;

    path spot = *write_current_dir + dir->get_name();
    nomme *exists = make_read_entree(*write_current_dir, dir->get_name(), false);

    try
    {
	if(exists == NULL)
	    make_file(dir, *write_current_dir, false);  // need to create the directory to be able to restore any file in it
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
		    make_file(dir, *write_current_dir, false);
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

bool filesystem_ea_has_been_restored(const hard_link *h)
{
    if(h == NULL)
	throw SRC_BUG;
    map<infinint, corres_ino_ea>::iterator it = corres_write.find(h->get_etiquette());
    if(it == corres_write.end())
	return false;
    else
	return it->second.ea_restored;
}

bool filesystem_set_ea(const nomme *e, const ea_attributs & l)
{
    path spot = *write_current_dir;
    const etiquette *e_eti = dynamic_cast<const etiquette *>(e);
    const directory *e_dir = dynamic_cast<const directory *>(e);
    bool ret = false;

    try
    {
	if(e == NULL)
	    throw SRC_BUG;
	    // buidling the path to file
	if(e_dir == NULL) // not a directory (directory get a chdir in them so write_current_dir is up to date)
	    spot += e->get_name();

	    // checking that we have not already restored the EA of this
	    // inode throw a hard link
	if(e_eti != NULL)
	{
	    map<infinint, corres_ino_ea>::iterator it;

	    it = corres_write.find(e_eti->get_etiquette());
	    if(it == corres_write.end())
	    {
		    // inode never restored; (no data saved just EA)
		    // we must record it
		corres_ino_ea tmp;
		tmp.chemin = spot.display();
		tmp.ea_restored = true;
		corres_write[e_eti->get_etiquette()] = tmp;
	    }
	    else
		if(it->second.ea_restored)
		    return false; // inode already restored
		else
		    it->second.ea_restored = true;
	}

	string chemin = spot.display();
	bool exists;

	    // restoring the root EA
	    //
	exists = ea_filesystem_is_present(chemin, ea_root);
	if(!exists || filesystem_allow_overwrite)
	{
	    if(filesystem_ea_root)
	    {
		if(exists && filesystem_warn_overwrite)
		    user_interaction_pause(string("system EA for ")+chemin+ " are about to be overwriten, continue ? ");
		ea_filesystem_clear_ea(chemin, ea_root);
		if(ea_filesystem_write_ea(chemin, l, true, false))
		{
		    if(filesystem_info_details)
			user_interaction_warning(string("restoring system EA for ")+chemin);
		    ret = true;
		}
		else
		    if(exists && l.size() == 0)
			ret = true; // EA have changed, (no more EA)
	    }
	}
	else
	    if(filesystem_ea_root)
		user_interaction_warning(string("system EA for ")+chemin+" will not be restored, (overwriting not allowed)");

	    // restoring the user EA
	    //
	exists = ea_filesystem_is_present(chemin, ea_user);
	if(!exists || filesystem_allow_overwrite)
	{
	    if(filesystem_ea_user)
	    {
		if(exists && filesystem_warn_overwrite)
		    user_interaction_pause(string("user EA for ")+chemin+ " are about to be overwriten, continue ? ");
		ea_filesystem_clear_ea(chemin, ea_user);
		if(ea_filesystem_write_ea(chemin, l, false, true))
		{
		    if(filesystem_info_details)
			user_interaction_warning(string("restoring user EA for ")+chemin);
		    ret = true;
		}
		else
		    if(exists && l.size() == 0)
			ret = true; // EA have changed, (no more EA)
	    }
	}
	else
	    if(filesystem_ea_user)
		user_interaction_warning(string("user EA for ")+chemin+" will not be restored, (overwriting not allowed)");
    }
    catch(Euser_abort & e)
    {
	ret = false;
    }

    return ret;
}

void filesystem_write_hard_linked_target_if_not_set(const etiquette *ref, const string & chemin)
{
    if(!filesystem_known_etiquette(ref->get_etiquette()))
    {
	corres_ino_ea tmp;
	tmp.chemin = chemin;
	tmp.ea_restored = false; // if EA have to be restored next
	corres_write[ref->get_etiquette()] = tmp;
    }
}

bool filesystem_known_etiquette(const infinint & eti)
{
    return corres_write.find(eti) != corres_write.end();
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
    static char id[]="$Id: filesystem.cpp,v 1.49 2002/06/18 21:16:06 denis Rel $";
    dummy_call(id);
}

static void make_file(const nomme * ref, const path & ou, bool dir_perm)
{
    const directory *ref_dir = dynamic_cast<const directory *>(ref);
    const file *ref_fil = dynamic_cast<const file *>(ref);
    const lien *ref_lie = dynamic_cast<const lien *>(ref);
    const blockdev *ref_blo = dynamic_cast<const blockdev *>(ref);
    const chardev *ref_cha = dynamic_cast<const chardev *>(ref);
    const tube *ref_tub = dynamic_cast<const tube *>(ref);
    const prise *ref_pri = dynamic_cast<const prise *>(ref);
    const etiquette *ref_eti = dynamic_cast <const etiquette *>(ref);
    const inode *ref_ino = dynamic_cast <const inode *>(ref);

    if(ref_ino == NULL && ref_eti == NULL)
	throw SRC_BUG; // neither an inode nor a hard link

    char *name = tools_str2charptr((ou + ref->get_name()).display());

    try
    {
	int ret;

	do
	{
	    if(ref_eti != NULL) // we potentially have to make a hard link
	    {
		bool create_file = false;
		map<infinint, corres_ino_ea>::iterator it = corres_write.find(ref_eti->get_etiquette());
		if(it == corres_write.end()) // first time, we have to create the inode
		{
		    corres_ino_ea tmp;
		    tmp.chemin = string(name);
		    tmp.ea_restored = false;
		    corres_write[ref_eti->get_etiquette()] = tmp;
		    create_file = true;
		}
		else // the inode already exists, making hard link if possible
		{
		    char *old = tools_str2charptr(it->second.chemin);
		    try
		    {
			ret = link(old, name);
			if(ret < 0)
			{
			    switch(errno)
			    {
			    case EXDEV:
			    case EPERM:
				// can't make hard link, trying to duplicate the inode
				user_interaction_warning(string("error creating hard link ") + name + " : " + strerror(errno) + "\n Trying to duplicate the inode");
				create_file = true;
				clear_corres_write(ref_eti->get_etiquette());
				    // need to remove this entry to be able
				    // to restore EA for other copies
				break;
			    case ENOENT:
				if(ref_eti->get_inode()->get_saved_status())
				{
				    create_file = true;
				    clear_corres_write(ref_eti->get_etiquette());
					// need to remove this entry to be able
					// to restore EA for other copies
				    user_interaction_warning(string("error creating hard link : ") + name + " , the inode to link with [" + old + "] has disapeared, re-creating it");

				}
				else
				{
				    create_file = false; // nothing to do;
				    user_interaction_warning(string("error creating hard link : ") + name + " , the inode to link with [" + old + "] is not present, cannot restore this hard link");
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
		    catch(...)
		    {
			delete old;
			throw;
		    }
		    delete old;
		}

		if(create_file)
		{
		    file remplacant = file(*ref_eti->get_inode());

		    remplacant.change_name(ref->get_name());
		    make_file(&remplacant, ou, dir_perm); // recursive call but with a plain file as argument
		    ref_ino = NULL; // to avoid setting the owner & permission twice (previous line, and below)
		    ret = 0; // to exist from while loop
		}
		else // hard link made
		    ret = 0; // not necessary, but avoids a warning from compilator (ret might be used uninitialized)
	    }
	    else if(ref_dir != NULL)
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
			crc crc_dyn, crc_ori;
			ou->skip(0);
			ou->copy_to(dest, crc_dyn);
			if(ref_fil->get_crc(crc_ori))  // CRC is not present in format "01"
			    if(!same_crc(crc_dyn, crc_ori))
				throw Erange("filesystem make_file", "bad CRC, data corruption occured");
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
	    {
		if(errno != ENOSPC)
		    throw Erange("make_file", strerror(errno));
		else
		    user_interaction_pause(string("can't create inode : ") + strerror(errno) + " Ready to continue ? ");
	    }
	}
	while(ret < 0 && errno == ENOSPC);

	if(ref_ino != NULL && ret >= 0)
	    make_owner_perm(*ref_ino, ou, dir_perm);
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
	if(!filesystem_ignore_ownership)
	    if(ref.get_saved_status())
		if(lchown(name, ref.get_uid(), ref.get_gid()) < 0)
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

static void attach_ea(const string &chemin, inode *ino)
{
    ea_attributs *eat = new ea_attributs();
    if(eat == NULL)
	throw Ememory("filesystem : attach_ea");
    try
    {
	if(ino == NULL)
	    throw SRC_BUG;
	ea_filesystem_read_ea(chemin, *eat, filesystem_ea_root, filesystem_ea_user);
	if(eat->size() > 0)
	{
	    ino->ea_set_saved_status(inode::ea_full);
	    ino->ea_attach(eat);
	    eat = NULL;
		// allocated memory now managed by the inode object
	}
	else
	    ino->ea_set_saved_status(inode::ea_none);
    }
    catch(...)
    {
	if(eat != NULL)
	    delete eat;
	throw;
    }
    if(eat != NULL)
	delete eat;
}

static void clear_corres_write(const infinint & ligne)
{
    map<infinint, corres_ino_ea>::iterator it = corres_write.find(ligne);
    if(it != corres_write.end())
	corres_write.erase(it);
}
