/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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
#if HAVE_SYS_CAPABILITY_H
#include <sys/capability.h>
#else
#if HAVE_LINUX_CAPABILITY_H
#include <linux/capability.h>
#endif
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
#include "tools.hpp"

using namespace std;

namespace libdar
{

#ifndef HAVE_CAPABILITIES

    capa_status capability_LINUX_IMMUTABLE(user_interaction & ui, bool verbose) { return capa_unknown; }

    capa_status capability_SYS_RESOURCE(user_interaction & ui, bool verbose) { return capa_unknown; }

    capa_status capability_FOWNER(user_interaction & ui, bool verbose) { return capa_unknown; }

    capa_status capability_CHOWN(user_interaction & ui, bool verbose) { return capa_unknown; }

#else

    static capa_status lxcapa_check(cap_value_t capa,
				    cap_flag_t capa_from_set,
				    user_interaction & ui,
				    bool verbose,
				    const std::string & capa_name);

    static bool lxcapa_set(cap_value_t capa,
			   cap_flag_t capa_from_set,
			   bool value,
			   user_interaction & ui,
			   bool verbose,
			   const std::string & capa_name);

    capa_status lxcapa_activate(cap_value_t capa,
				user_interaction & ui,
				bool verbose,
				const std::string & capa_name);

    capa_status capability_LINUX_IMMUTABLE(user_interaction & ui, bool verbose)
    { return lxcapa_activate(CAP_LINUX_IMMUTABLE, ui, verbose, "Immutable"); }

    capa_status capability_SYS_RESOURCE(user_interaction & ui, bool verbose)
    { return lxcapa_activate(CAP_SYS_RESOURCE, ui, verbose, "System Resource"); }

    capa_status capability_FOWNER(user_interaction & ui, bool verbose)
    { return lxcapa_activate(CAP_FOWNER, ui, verbose, "File Owner for all files"); }

    capa_status capability_CHOWN(user_interaction & ui, bool verbose)
    { return lxcapa_activate(CAP_CHOWN, ui, verbose, "change ownership"); }

    static capa_status lxcapa_check(cap_value_t capa,
				    cap_flag_t capa_from_set,
				    user_interaction & ui,
				    bool verbose,
				    const std::string & capa_name)
    {
	capa_status ret = capa_unknown;
	cap_t capaset = cap_get_proc();
	cap_flag_value_t val;

	try
	{
	    if(cap_get_flag(capaset, capa, capa_from_set, &val) == 0)
		ret = (val == CAP_SET) ? capa_set : capa_clear;
	    else
	    {
		ret = capa_unknown;
		if(verbose)
		{
		    string tmp = tools_strerror_r(errno);
		    ui.printf(gettext("Error met while checking for capability %S: %s"), &capa_name, tmp.c_str());
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

    static bool lxcapa_set(cap_value_t capa,
			   cap_flag_t capa_from_set,
			   bool value,
			   user_interaction & ui,
			   bool verbose,
			   const std::string & capa_name)
    {
	cap_t capaset = cap_get_proc();
	cap_flag_value_t flag_value = value ? CAP_SET : CAP_CLEAR;
	bool ret = false;

	try
	{
	    if(cap_set_flag(capaset, capa_from_set, 1, &capa, flag_value) != 0)
	    {
		string tmp = tools_strerror_r(errno);
		ui.printf(gettext("Error met while setting capability %S: %s"), &capa_name, tmp.c_str());
	    }
	    else // no error so far
	    {
		if(cap_set_proc(capaset) != 0)
		{
		    string tmp = tools_strerror_r(errno);
		    ui.printf(gettext("Error met while setting capability %S: %s"), &capa_name, tmp.c_str());
		}
		else // no error so far
		    ret = true;
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

    capa_status lxcapa_activate(cap_value_t capa,
				user_interaction & ui,
				bool verbose,
				const std::string & capa_name)
    {
	capa_status ret = lxcapa_check(capa,
				       CAP_EFFECTIVE,
				       ui,
				       verbose,
				       capa_name);

	if(ret == capa_clear)
	{
		// maybe capability is available in CAP_PERMITTED

	    if(lxcapa_check(capa,
			    CAP_PERMITTED,
			    ui,
			    verbose,
			    capa_name) == capa_set)
	    {
		    // yes it is we can try activating the capability in CAP_EFFECTIVE
		if(lxcapa_set(capa,
			      CAP_EFFECTIVE,
			      true,
			      ui,
			      verbose,
			      capa_name))
		{
			// checking if the status has changed
		    ret = lxcapa_check(capa,
				       CAP_EFFECTIVE,
				       ui,
				       verbose,
				       capa_name);
		}
	    }
		// capability not in CAP_PERMITTED, not trying to activating it
	}

	return ret;
    }


#endif

} // end of namespace

