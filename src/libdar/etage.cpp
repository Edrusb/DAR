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
// this was necessary to compile under Mac OS-X (boggus dirent.h)
#if HAVE_STDINT_H
#include <stdint.h>
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"

#include "etage.hpp"
#include "tools.hpp"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "cygwin_adapt.hpp"
#include "user_interaction.hpp"
#include "fichier_local.hpp"

#define CACHE_DIR_TAG_FILENAME "CACHEDIR.TAG"
#define CACHE_DIR_TAG_FILENAME_CONTENTS "Signature: 8a477f597d28d172789f06886806bc55"

using namespace std;

namespace libdar
{
	// check if the given file is a tag tellig if the current directory is a cache directory
    static bool cache_directory_tagging_check(const char *cpath, const char *filename);


    etage::etage(user_interaction &ui,
		 const char *dirname,
		 const infinint & x_last_acc,
		 const infinint & x_last_mod,
		 bool cache_directory_tagging,
		 bool furtive_read_mode)
    {
        struct dirent *ret;
	DIR *tmp = NULL;
#if FURTIVE_READ_MODE_AVAILABLE
	int fddir = -1;

	if(furtive_read_mode)
	{
	    fddir = ::open(dirname, O_RDONLY|O_BINARY|O_NOATIME);

	    if(fddir < 0)
	    {
		if(errno != EPERM)
		    throw Erange("etage::etage",
				 string(gettext("Error opening directory in furtive read mode: ")) + dirname + " : " + strerror(errno));
		else // using back normal access mode
		    ui.warning(tools_printf(gettext("Could not open directory %s in furtive read mode (%s), using normal mode"), dirname, strerror(errno)));
	    }
	}
#endif
	try
	{
#if FURTIVE_READ_MODE_AVAILABLE
	    if(furtive_read_mode && fddir >= 0)
	    {
		tmp = fdopendir(fddir);
		if(tmp == NULL)
		    close(fddir);
		    // else, tmp != NULL:
		    // fddir is now under control of the system
		    // we must not close it.
	    }
	    else
		tmp = opendir(dirname);
#else
	    tmp = opendir(dirname);
#endif
	    bool is_cache_dir = false;

	    if(tmp == NULL)
		throw Erange("etage::etage" , string(gettext("Error opening directory: ")) + dirname + " : " + strerror(errno));

	    fichier.clear();
	    while(!is_cache_dir && (ret = readdir(tmp)) != NULL)
		if(strcmp(ret->d_name, ".") != 0 && strcmp(ret->d_name, "..") != 0)
		{
		    if(cache_directory_tagging)
			is_cache_dir = cache_directory_tagging_check(dirname, ret->d_name);
		    fichier.push_back(string(ret->d_name));
		}
	    closedir(tmp);
	    tmp = NULL;

	    if(is_cache_dir)
	    {
		fichier.clear();
		ui.warning(tools_printf(gettext("Detected Cache Directory Tagging Standard for %s, the contents of that directory will not be saved"), dirname));
		    // drop all the contents of the directory because it follows the Cache Directory Tagging Standard
	    }

	    last_mod = x_last_mod;
	    last_acc = x_last_acc;
	}
	catch(...)
	{
	    if(tmp != NULL)
		closedir(tmp);
	    throw;
	}
    }

    bool etage::read(string & ref)
    {
        if(fichier.empty())
            return false;
	else
	{
	    ref = fichier.front();
	    fichier.pop_front();
	    return true;
	}
    }

	///////////////////////////////////////////
	////////////// static functions ///////////
	///////////////////////////////////////////


    static bool cache_directory_tagging_check(const char *cpath, const char *filename)
    {
	bool ret = false;

	if(strcmp(CACHE_DIR_TAG_FILENAME, filename) != 0)
	    ret = false;
	else // we need to inspect the few first bytes of the file
	{
	    try
	    {
		path chem = path(cpath)+string(filename);
		fichier_local fic = fichier_local(chem.display(), false);
		U_I len = strlen(CACHE_DIR_TAG_FILENAME_CONTENTS);
		char *buffer = new (nothrow) char[len+1];
		S_I lu;

		if(buffer == NULL)
		    throw Ememory("etage:cache_directory_tagging_check");
		try
		{
		    lu = fic.read(buffer, len);
		    if(lu < 0 || (U_I)(lu) < len)
			ret = false;
		    else
			ret = strncmp(buffer, CACHE_DIR_TAG_FILENAME_CONTENTS, len) == 0;
		}
		catch(...)
		{
		    delete [] buffer;
		    throw;
		}
		delete [] buffer;
	    }
	    catch(Ememory & e)
	    {
		throw;
	    }
	    catch(Ebug & e)
	    {
		throw;
	    }
	    catch(...)
	    {
		ret = false;
	    }
	}

	return ret;
    }

} // end of namespace
