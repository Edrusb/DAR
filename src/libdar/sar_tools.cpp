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

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif
} // end extern "C"

#include "sar_tools.hpp"
#include "erreurs.hpp"
#include "user_interaction.hpp"
#include "sar.hpp"
#include "tools.hpp"
#include "tuyau.hpp"
#include "cygwin_adapt.hpp"

using namespace std;

namespace libdar
{

    trivial_sar *sar_tools_open_archive_tuyau(user_interaction & dialog,
					      memory_pool *pool,
					      S_I fd,
					      gf_mode mode,
					      const label & internal_name,
					      const label & data_name,
					      bool slice_header_format_07,
					      const std::string & execute)
    {
        generic_file *tmp = NULL;
        trivial_sar *ret = NULL;

        try
        {
            tmp = new (pool) tuyau(dialog, fd, mode);
            if(tmp == NULL)
                throw Ememory("sar_tools_open_archive_tuyau");
            ret = new (pool) trivial_sar(dialog,
					 tmp,
					 internal_name,
					 data_name,
					 slice_header_format_07,
					 execute);
            if(ret == NULL)
                throw Ememory("sar_tools_open_archive_tuyau");
	    else
		tmp = NULL;
        }
        catch(...)
        {
            if(ret != NULL)
                delete ret;
	    if(tmp != NULL)
		delete tmp;
            throw;
        }

        return ret;
    }


} // end of namespace
