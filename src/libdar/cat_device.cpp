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
// to allow compilation under Cygwin we need <sys/types.h>
// else Cygwin's <netinet/in.h> lack __int16_t symbol !?!
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef STDC_HEADERS
#include <ctype.h>
#endif
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
# include <sys/time.h>
# else
# include <time.h>
# endif
#endif
#if HAVE_LIMITS_H
#include <limits.h>
#endif
} // end extern "C"

#include "cat_device.hpp"

using namespace std;

namespace libdar
{

    cat_device::cat_device(const infinint & uid, const infinint & gid, U_16 perm,
			   const datetime & last_access,
			   const datetime & last_modif,
			   const datetime & last_change,
			   const string & name,
			   U_16 major,
			   U_16 minor,
			   const infinint & fs_dev) : cat_inode(uid, gid, perm, last_access, last_modif, last_change, name, fs_dev)
    {
	xmajor = major;
	xminor = minor;
	set_saved_status(s_saved);
    }

    cat_device::cat_device(const shared_ptr<user_interaction> & dialog,
			   const smart_pointer<pile_descriptor> & pdesc,
			   const archive_version & reading_ver,
			   saved_status saved,
			   bool small) : cat_inode(dialog, pdesc, reading_ver, saved, small)
    {
	U_16 tmp;
	generic_file *ptr = nullptr;

	pdesc->check(small);
	if(small)
	    ptr = pdesc->esc;
	else
	    ptr = pdesc->stack;

	if(saved == s_saved)
	{
	    if(ptr->read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
		throw Erange("special::special", gettext("missing data to build a special device"));
	    xmajor = ntohs(tmp);
	    if(ptr->read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
		throw Erange("special::special", gettext("missing data to build a special device"));
	    xminor = ntohs(tmp);
	}
    }

    bool cat_device::operator == (const cat_entree & ref) const
    {
	const cat_device *ref_dev = dynamic_cast<const cat_device *>(&ref);

	if(ref_dev == nullptr)
	    return false;
	else
	    return xmajor == ref_dev->xmajor
		&& xminor == ref_dev->xminor
		&& cat_inode::operator == (ref);
    }


    void cat_device::inherited_dump(const pile_descriptor & pdesc, bool small) const
    {
	U_16 tmp;
	generic_file *ptr = nullptr;

	pdesc.check(small);
	if(small)
	    ptr = pdesc.esc;
	else
	    ptr = pdesc.stack;

	cat_inode::inherited_dump(pdesc, small);

	if(get_saved_status() == s_saved)
	{
	    tmp = htons(xmajor);
	    ptr->write((char *)&tmp, sizeof(tmp));
	    tmp = htons(xminor);
	    ptr->write((char *)&tmp, sizeof(tmp));
	}
    }

    void cat_device::sub_compare(const cat_inode & other, bool isolated_mode) const
    {
	const cat_device *d_other = dynamic_cast<const cat_device *>(&other);
	if(d_other == nullptr)
	    throw SRC_BUG; // bug in cat_inode::compare
	if(get_saved_status() == s_saved && d_other->get_saved_status() == s_saved)
	{
	    if(get_major() != d_other->get_major())
		throw Erange("cat_device::sub_compare", tools_printf(gettext("devices have not the same major number: %d <--> %d"), get_major(), d_other->get_major()));
	    if(get_minor() != d_other->get_minor())
		throw Erange("cat_device::sub_compare", tools_printf(gettext("devices have not the same minor number: %d <--> %d"), get_minor(), d_other->get_minor()));
	}
    }

} // end of namespace
