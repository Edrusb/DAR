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

}

#include <new>

#include "erreurs.hpp"
#include "tools.hpp"
#include "fsa_familly.hpp"

using namespace std;

namespace libdar
{

    string fsa_familly_to_string(fsa_familly f)
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
	case fsan_compressed:
	    return gettext("compressed");
	case fsan_no_dump:
	    return gettext("no dump flag");
	case fsan_immutable:
	    return gettext("immutable");
	case fsan_undeletable:
	    return gettext("undeletable");
	default:
	    throw SRC_BUG;
	}
    }

    fsa_scope all_fsa_familly()
    {
	fsa_scope ret;

	ret.insert(fsaf_hfs_plus);
	ret.insert(fsaf_linux_extX);

	return ret;
    }


    const infinint FSA_SCOPE_BIT_HFS_PLUS = 1;
    const infinint FSA_SCOPE_BIT_LINUX_EXTX = 2;

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


} // end of namespace

