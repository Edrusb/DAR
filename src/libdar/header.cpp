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
// $Id: header.cpp,v 1.16 2005/12/29 02:32:41 edrusb Rel $
//
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

    static void dummy_call(char *x)
    {
        static char id[]="$Id: header.cpp,v 1.16 2005/12/29 02:32:41 edrusb Rel $";
        dummy_call(id);
    }

    bool header_label_is_equal(const label &a, const label &b)
    {
        register U_I i = 0;
        while(i < LABEL_SIZE && a[i] == b[i])
            i++;
        return i >= LABEL_SIZE;
    }

    void header_generate_internal_filename(label &ret)
    {
        time_t src1 = time(NULL);
        pid_t src2 = getpid();
        U_I wrote = 0;
        U_I read = 0;

        while(read < sizeof(src1) && wrote < LABEL_SIZE)
            ret[wrote++] = ((char *)(&src1))[read++];

        read = 0;
        while(read < sizeof(src2) && wrote < LABEL_SIZE)
            ret[wrote++] = ((char *)(&src2))[read++];
    }

    header::header()
    {
        magic = 0;
        for(U_I i = 0; i < LABEL_SIZE; i++)
            internal_name[i] = '\0';
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
        for(U_I i = 0; i < LABEL_SIZE; i++)
            left[i] = right[i];

    }

} // end of namespace
