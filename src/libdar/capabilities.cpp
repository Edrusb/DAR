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
#if HAVE_SYS_CAPABILITY_H
#include <sys/capability.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
}

#include "capabilities.hpp"


using namespace std;

namespace libdar
{

#ifndef HAVE_CAPABILITIES

    capa_status capability_LINUX_IMMUTABLE(user_interaction & ui, bool verbose) { return capa_unknown; };

    capa_status capability_SYS_RESOURCE(user_interaction & ui, bool verbose) { return capa_unknown; };

#else

    static capa_status cap_check(cap_value_t capa, user_interaction & ui, bool verbose, const std::string & capa_name);

    capa_status capability_LINUX_IMMUTABLE(user_interaction & ui, bool verbose) { return cap_check(CAP_LINUX_IMMUTABLE, ui, verbose, "Immutable"); }

    capa_status capability_SYS_RESOURCE(user_interaction & ui, bool verbose) { return cap_check(CAP_SYS_RESOURCE, ui, verbose, "System Resource"); }

    static capa_status cap_check(cap_value_t capa, user_interaction & ui, bool verbose, const std::string & capa_name)
    {
	capa_status ret = capa_unknown;
	cap_t capaset = cap_get_proc();
	cap_flag_value_t val;

	try
	{
	    if(cap_get_flag(capaset, capa, CAP_EFFECTIVE, &val) == 0)
		ret = (val == CAP_SET) ? capa_set : capa_clear;
	    else
	    {
		ret = capa_unknown;
		if(verbose)
		{
		    string tmp = strerror(errno);
		    ui.printf(gettext("Error met while checking for capability %S: %S"), &capa_name, &tmp);
		}
	    }
	}
	catch(...) // well a try/catch may seems useless here, but it does not hurt ... :-)
	{
	    cap_free(capaset);
	    throw;
	}
	cap_free(capaset);

	return ret;
    }

#endif

} // end of namespace

