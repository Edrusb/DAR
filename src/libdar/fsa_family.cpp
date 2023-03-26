/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

}

#include "erreurs.hpp"
#include "tools.hpp"
#include "fsa_family.hpp"

using namespace std;

namespace libdar
{

    string fsa_family_to_string(fsa_family f)
    {
	switch(f)
	{
	case fsaf_hfs_plus:
	    return "HFS+";
	case fsaf_linux_extX:
	    return "ext2/3/4";
	default:
	    throw SRC_BUG;
	}
    }

    string fsa_nature_to_string(fsa_nature n)
    {
	switch(n)
	{
	case fsan_unset:
	    throw SRC_BUG;
	case fsan_creation_date:
	    return gettext("creation date");
	case fsan_append_only:
	    return gettext("append only");
	case fsan_compressed:
	    return gettext("compressed");
	case fsan_no_dump:
	    return gettext("no dump flag");
	case fsan_immutable:
	    return gettext("immutable");
	case fsan_data_journaling:
	    return gettext("journalized");
	case fsan_secure_deletion:
	    return gettext("secure deletion");
	case fsan_no_tail_merging:
	    return gettext("no tail merging");
	case fsan_undeletable:
	    return gettext("undeletable");
	case fsan_noatime_update:
	    return gettext("no atime update");
	case fsan_synchronous_directory:
	    return gettext("synchronous directory");
	case fsan_synchronous_update:
	    return gettext("synchronous update");
	case fsan_top_of_dir_hierarchy:
	    return gettext("top of directory hierarchy");
	default:
	    throw SRC_BUG;
	}
    }

    fsa_scope all_fsa_families()
    {
	fsa_scope ret;

	ret.insert(fsaf_hfs_plus);
	ret.insert(fsaf_linux_extX);

	return ret;
    }


    const U_I FSA_SCOPE_BIT_HFS_PLUS = 1;
    const U_I FSA_SCOPE_BIT_LINUX_EXTX = 2;

    infinint fsa_scope_to_infinint(const fsa_scope & val)
    {
	infinint ret = 0;

	if(val.find(fsaf_hfs_plus) != val.end())
	    ret |= FSA_SCOPE_BIT_HFS_PLUS;
	if(val.find(fsaf_linux_extX) != val.end())
	    ret |= FSA_SCOPE_BIT_LINUX_EXTX;

	return ret;
    }

    fsa_scope infinint_to_fsa_scope(const infinint & ref)
    {
	fsa_scope ret;

	ret.clear();
	if((ref & FSA_SCOPE_BIT_HFS_PLUS) != 0)
	    ret.insert(fsaf_hfs_plus);
	if((ref & FSA_SCOPE_BIT_LINUX_EXTX) != 0)
	    ret.insert(fsaf_linux_extX);

	return ret;
    }

    string fsa_scope_to_string(bool saved, const fsa_scope & scope)
    {
	string ret = "";

	    // first letter

	if(scope.find(fsaf_hfs_plus) != scope.end())
	    if(saved)
		ret += "H";
	    else
		ret += "h";
	else
	    ret += "-";

	    // second letter

	if(scope.find(fsaf_linux_extX) != scope.end())
	    if(saved)
		ret += "L";
	    else
		ret += "l";
	else
	    ret += "-";

	return ret;
    }



} // end of namespace

