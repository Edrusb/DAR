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
// to contact the author : dar.linux@free.fr
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

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_STRING_H
#include <string.h>
#endif
} // end extern "C"

#include "header.hpp"

namespace libdar
{

    void header::read(generic_file & f)
    {
        magic_number tmp;

        f.read((char *)&tmp, sizeof(magic_number));
        magic = ntohl(tmp);
        f.read(internal_name, LABEL_SIZE);
        f.read(&flag, 1);
        f.read(&extension, 1);
        switch(extension)
        {
        case EXTENSION_NO:
            break;
        case EXTENSION_SIZE:
            size_ext = infinint(f.get_gf_ui(), NULL, &f);
            break;
	case EXTENSION_TLV:
	    throw Efeature(gettext("Archive format too recent for this version of DAR"));
        default :
            throw Erange("header::read", gettext("Badly formatted SAR header"));
        }
    }

    void header::write(generic_file & f)
    {
        magic_number tmp;

        tmp = htonl(magic);
        f.write((char *)&tmp, sizeof(magic));
        f.write(internal_name, LABEL_SIZE);
        f.write(&flag, 1);
        f.write(&extension, 1);
        switch(extension)
        {
        case EXTENSION_NO:
            break;
        case EXTENSION_SIZE:
            size_ext.dump(f);
            break;
        default:
            throw SRC_BUG;
        }
    }

    void header::read(user_interaction & dialog, S_I fd)
    {
        fichier fic = fichier(dialog, dup(fd));
        read(fic);
    }

    void header::write(user_interaction & dialog, S_I fd)
    {
        fichier fic = fichier(dialog, dup(fd));
        write(fic);
    }

    bool header_label_is_equal(const label &a, const label &b)
    {
        return memcmp(a, b, LABEL_SIZE) == 0;
    }

    void header_generate_internal_filename(label &ret)
    {
        const time_t src1 = time(NULL);

	if(sizeof(src1) >= LABEL_SIZE)
	    memcpy(ret, &src1, LABEL_SIZE);
	else
	{
	    const U_I left = LABEL_SIZE - sizeof(src1);
	    const pid_t src2 = getpid();

	    if(sizeof(src2) >= left)
		memcpy(ret, &src1, left);
	    else
		memcpy(ret, &src1, sizeof(src2));
	}
    }

    header::header()
    {
        magic = 0;
        memset(internal_name, '\0', LABEL_SIZE);
        extension = flag = '\0';
        size_ext = 0;
    }

    void header::copy_from(const header & ref)
    {
        magic = ref.magic;
        label_copy(internal_name,ref.internal_name);
        flag = ref.flag;
        extension = ref.extension;
        size_ext = ref.size_ext;
    }

    void label_copy(label & left, const label & right)
    {
    	memcpy(left, right, LABEL_SIZE);
    }

} // end of namespace
