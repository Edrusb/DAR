/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
} // end extern "C"

#include <string>
#include <new>

#include "zapette_protocol.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    void request::write(generic_file *f)
    {
        U_16 tmp = htons(size);

        f->write(&serial_num, 1);
        offset.dump(*f);
        f->write((char *)&tmp, sizeof(tmp));
	if(size == REQUEST_SIZE_SPECIAL_ORDER && offset == REQUEST_OFFSET_CHANGE_CONTEXT_STATUS)
	    tools_write_string(*f, info);
    }

    void request::read(generic_file *f)
    {
        U_16 tmp;
        U_16 pas;

        if(f->read(&serial_num, 1) == 0)
            throw Erange("request::read", gettext("Partial request received, aborting\n"));
	if(f == nullptr)
	    throw SRC_BUG;
        offset = infinint(*f);
        pas = 0;
        while(pas < sizeof(tmp))
            pas += f->read((char *)&tmp+pas, sizeof(tmp)-pas);
        size = ntohs(tmp);
	if(size == REQUEST_SIZE_SPECIAL_ORDER && offset == REQUEST_OFFSET_CHANGE_CONTEXT_STATUS)
	    tools_read_string(*f, info);
	else
	    info = "";
    }

    void answer::write(generic_file *f, char *data)
    {
        U_16 tmp = htons(size);

        f->write(&serial_num, 1);
        f->write(&type, 1);
        switch(type)
        {
        case ANSWER_TYPE_DATA:
            f->write((char *)&tmp, sizeof(tmp));
            if(data != nullptr)
                f->write(data, size);
            else
                if(size != 0)
                    throw SRC_BUG;
            break;
        case ANSWER_TYPE_INFININT:
            arg.dump(*f);
            break;
        default:
            throw SRC_BUG;
        }
    }

    void answer::read(generic_file *f, char *data, U_16 max)
    {
        U_16 tmp;
        U_16 pas;

        f->read(&serial_num, 1);
        f->read(&type, 1);
        switch(type)
        {
        case ANSWER_TYPE_DATA:
            pas = 0;
            while(pas < sizeof(tmp))
                pas += f->read((char *)&tmp+pas, sizeof(tmp)-pas);
            size = ntohs(tmp);

		// recycling tmp to carry the max data to store in the data arg of this method
	    if(size > max)
		tmp = max;
	    else
		tmp = size;

		// recycling pas to follow the steps of data fullfilment
            pas = 0;
            while(pas < tmp)
                pas += f->read(data+pas, tmp - pas);

		// need to drop the remaining data
            if(size > max)
            {
                char black_hole;

                for(tmp = max; tmp < size; ++tmp)
                    f->read(&black_hole, 1);
                    // might not be very performant code
            }
            arg = 0;
            break;
        case ANSWER_TYPE_INFININT:
	    if(f == nullptr)
		throw SRC_BUG;
            arg = infinint(*f);
            size = 0;
            break;
        default:
            throw Erange("answer::read", gettext("Corrupted data read on pipe"));
        }
    }

} // end of namespace
