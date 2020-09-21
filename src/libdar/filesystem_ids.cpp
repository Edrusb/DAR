/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"


#include "filesystem_ids.hpp"
#include "tools.hpp"
#include "erreurs.hpp"

using namespace std;

namespace libdar
{

    filesystem_ids::filesystem_ids(const path & root)
    {
	change_root_fs(root);
    }

    filesystem_ids::filesystem_ids(bool same_fs, const path & root)
    {
	change_root_fs(root);
	if(same_fs)
	    included.insert(root_fs);
	else
	    excluded.insert(root_fs);
    }

    void filesystem_ids::change_root_fs(const path & root)
    {
	if(root.is_relative())
	    throw Erange("filesystem_ids::set_root_fs","path to a filesystem must be an absolute path");
	root_fs = path2fs_id(root.display());
    }

    void filesystem_ids::include_fs_at(const path & chem)
    {
	if(chem.is_relative())
	    throw Erange("filesystem_ids::set_root_fs","path to a filesystem must be an absolute path");
	included.insert(path2fs_id(chem.display()));
    }

    void filesystem_ids::exclude_fs_at(const path & chem)
    {
	if(chem.is_relative())
	    throw Erange("filesystem_ids::set_root_fs","path to a filesystem must be an absolute path");
	excluded.insert(path2fs_id(chem.display()));
    }

    bool filesystem_ids::is_covered(const infinint & fs_id) const
    {
	set<infinint>::iterator it;

	if(fs_id == root_fs)
	    return true;

	if(included.empty())
	{
	    if(excluded.empty())
		return true;
	    else
	    {
		it = excluded.find(fs_id);
		return it == excluded.end();
	    }
	}
	else // included not empty
	{
	    if(excluded.empty())
	    {
		it = included.find(fs_id);
		return it != included.end();
	    }
	    else // both included and excluded are not empty
	    {
		it = included.find(fs_id);
		if(it != included.end())
		{
		    it = excluded.find(fs_id);
		    return it == excluded.end();
		}
		else
		    return false;
	    }
	}
    }

    bool filesystem_ids::is_covered(const path & chem) const
    {
	if(chem.is_relative())
	    throw Erange("filesystem_ids::set_root_fs","path to a filesystem must be an absolute path");
	return is_covered(path2fs_id(chem.display()));
    }

    infinint filesystem_ids::path2fs_id(const std::string & path)
    {
	struct stat buf;
	int val = 0;

	val = stat(path.c_str(), &buf);
	if(val < 0)
	{
	    string errmsg = tools_strerror_r(errno);

	    throw Erange("filesystem_ids", tools_printf(gettext("Cannot read filesystem information at %S: %S"),
						       path,
						       errmsg));
	}

	return buf.st_dev;
    }


} // end of namespace
