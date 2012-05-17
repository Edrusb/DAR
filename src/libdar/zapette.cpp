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
// $Id: zapette.cpp,v 1.21.2.1 2007/07/22 16:35:00 edrusb Rel $
//
/*********************************************************************/
//

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
#include "zapette.hpp"
#include "infinint.hpp"
#include "user_interaction.hpp"
#include "tools.hpp"

#define ANSWER_TYPE_DATA 'D'
#define ANSWER_TYPE_INFININT 'I'

#define REQUEST_SIZE_SPECIAL_ORDER 0
#define REQUEST_OFFSET_END_TRANSMIT 0
#define REQUEST_OFFSET_GET_FILESIZE 1
#define REQUEST_OFFSET_CHANGE_CONTEXT_STATUS 2

using namespace std;

namespace libdar
{

    struct request
    {
        char serial_num;
        U_16 size; // size or REQUEST_SIZE_SPECIAL_ORDER
        infinint offset; // offset or REQUEST_OFFSET_END_TRANSMIT or REQUEST_OFFSET_GET_FILESIZE, REQUEST_OFFSET_* ...
	string info; // new contextual_status

        void write(generic_file *f); // master side
        void read(generic_file *f);  // slave side
    };

    struct answer
    {
        char serial_num;
        char type;
        U_16 size;
        infinint arg;

        void write(generic_file *f, char *data); // slave side
        void read(generic_file *f, char *data, U_16 max);  // master side
    };

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
        offset = infinint(f->get_gf_ui(), NULL, f);
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
            if(data != NULL)
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
            pas = 0;
            while(pas < size)
                pas += f->read(data+pas, size-pas);

            if(size > max) // need to drop the remaining data
            {
                char black_hole;

                for(tmp = max; tmp < size; ++tmp)
                    f->read(&black_hole, 1);
                    // might not be very performant code
            }
            arg = 0;
            break;
        case ANSWER_TYPE_INFININT:
            arg = infinint(f->get_gf_ui(), NULL, f);
            size = 0;
            break;
        default:
            throw Erange("answer::read", gettext("Corrupted data read on pipe"));
        }
    }

    slave_zapette::slave_zapette(generic_file *input, generic_file *output, contextual *data)
    {
        if(input == NULL)
            throw SRC_BUG;
        if(output == NULL)
            throw SRC_BUG;
        if(data == NULL)
            throw SRC_BUG;

        if(input->get_mode() == gf_write_only)
            throw Erange("slave_zapette::slave_zapette", gettext("Input cannot be read"));
        if(output->get_mode() == gf_read_only)
            throw Erange("slave_zapette::slave_zapette", gettext("Cannot write to output"));
        if(data->get_mode() != gf_read_only)
            throw Erange("slave_zapette::slave_zapette", gettext("Data should be read-only"));
        in = input;
        out = output;
        src = data;
    }

    slave_zapette::~slave_zapette()
    {
        if(in != NULL)
            delete in;
        if(out != NULL)
            delete out;
        if(src != NULL)
            delete src;
    }

    void slave_zapette::action()
    {
        request req;
        answer ans;
        char *buffer = NULL;
        U_16 buf_size = 0;

        try
        {
            do
            {
                req.read(in);
                ans.serial_num = req.serial_num;

                if(req.size != REQUEST_SIZE_SPECIAL_ORDER)
                {
                    ans.type = ANSWER_TYPE_DATA;
                    if(src->skip(req.offset))
                    {
                            // enlarge buffer if necessary
                        if(req.size > buf_size)
                        {
                            if(buffer != NULL)
                                delete [] buffer;
                            buffer = new char[req.size];
                            if(buffer == NULL)
                                throw Ememory("slave_zapette::action");
                            else
                                buf_size = req.size;
                        }

                        ans.size = src->read(buffer, req.size);
                        ans.write(out, buffer);
                    }
                    else // bad position
                    {
                        ans.size = 0;
                        ans.write(out, NULL);
                    }
                }
                else // special orders
                {
                    if(req.offset == REQUEST_OFFSET_END_TRANSMIT) // stop communication
                    {
                        ans.type = ANSWER_TYPE_DATA;
                        ans.size = 0;
                        ans.write(out, NULL);
                    }
                    else if(req.offset == REQUEST_OFFSET_GET_FILESIZE) // return file size
                    {
                        ans.type = ANSWER_TYPE_INFININT;
                        if(!src->skip_to_eof())
                            throw Erange("slave_zapette::action", gettext("Cannot skip at end of file"));
                        ans.arg = src->get_position();
                        ans.write(out, NULL);
                    }
		    else if(req.offset == REQUEST_OFFSET_CHANGE_CONTEXT_STATUS) // contextual status change requested
		    {
			ans.type = ANSWER_TYPE_INFININT;
			ans.arg = 1;
			src->set_info_status(req.info);
			ans.write(out, NULL);
		    }
                    else
                        throw Erange("zapette::action", gettext("Received unknown special order"));
                }
            }
            while(req.size != REQUEST_SIZE_SPECIAL_ORDER || req.offset != REQUEST_OFFSET_END_TRANSMIT);
        }
        catch(...)
        {
            if(buffer != NULL)
                delete [] buffer;
            throw;
        }
        if(buffer != NULL)
            delete [] buffer;
    }

    zapette::zapette(user_interaction & dialog, generic_file *input, generic_file *output) : contextual(dialog, gf_read_only)
    {
        if(input == NULL)
            throw SRC_BUG;
        if(output == NULL)
            throw SRC_BUG;
        if(input->get_mode() == gf_write_only)
            throw Erange("zapette::zapette", gettext("Cannot read on input"));
        if(output->get_mode() == gf_read_only)
            throw Erange("zapette::zapette", gettext("Cannot write on output"));

        in = input;
        out = output;
        position = 0;
        serial_counter = 0;
	info = CONTEXT_INIT;

            //////////////////////////////
            // retreiving the file size
            //
        S_I tmp = 0;
        make_transfert(REQUEST_SIZE_SPECIAL_ORDER, REQUEST_OFFSET_GET_FILESIZE, NULL, "", tmp, file_size);
    }

    zapette::~zapette()
    {
        S_I tmp = 0;
        make_transfert(REQUEST_SIZE_SPECIAL_ORDER, REQUEST_OFFSET_END_TRANSMIT, NULL, "", tmp, file_size);

        delete in;
        delete out;
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: zapette.cpp,v 1.21.2.1 2007/07/22 16:35:00 edrusb Rel $";
        dummy_call(id);
    }

    bool zapette::skip(const infinint & pos)
    {
        if(pos >= file_size)
        {
            position = file_size;
            return false;
        }
        else
        {
            position = pos;
            return true;
        }
    }

    bool zapette::skip_relative(S_I x)
    {
        if(x >= 0)
        {
            position += x;
            if(position > file_size)
            {
                position = file_size;
                return false;
            }
            else
                return true;
        }
        else
            if(infinint(-x) > position)
            {
                position = 0;
                return false;
            }
            else
            {
                position -= (-x); // implicit conversion of "-x" to infinint (positive)
                return true;
            }
    }

    void zapette::set_info_status(const std::string & s)
    {
	infinint val;
	S_I tmp;

	make_transfert(REQUEST_SIZE_SPECIAL_ORDER, REQUEST_OFFSET_CHANGE_CONTEXT_STATUS, NULL, s, tmp, val);
    }


    S_I zapette::inherited_read(char *a, size_t size)
    {
        static const U_16 max_short = ~0;
        U_I lu = 0;

        if(size > 0)
        {
            infinint not_used;
            U_16 pas;
            S_I ret;

            do
            {
                if(size - lu > max_short)
                    pas = max_short;
                else
                    pas = size - lu;
                make_transfert(pas, position, a+lu, "", ret, not_used);
                position += ret;
                lu += ret;
            }
            while(lu < size && ret != 0);
        }

        return lu;
    }

    S_I zapette::inherited_write(const char *a, size_t size)
    {
        throw SRC_BUG; // zapette is read-only
    }

    void zapette::make_transfert(U_16 size, const infinint &offset, char *data, const string & info, S_I & lu, infinint & arg)
    {
        request req;
        answer ans;

            // building the request
        req.serial_num = serial_counter++; // may loopback to 0
        req.offset = offset;
        req.size = size;
	req.info = info;
        req.write(out);

            // reading the answer
        do
        {
            ans.read(in, data, size);
            if(ans.serial_num != req.serial_num)
                get_gf_ui().pause(gettext("Communication problem with peer, retry ?"));
	}
        while(ans.serial_num != req.serial_num);

            // argument affectation
        switch(ans.type)
        {
        case ANSWER_TYPE_DATA:
            lu = ans.size;
            arg = 0;
            break;
        case ANSWER_TYPE_INFININT:
            lu = 0;
            arg = ans.arg;
            break;
        default:  // might be a transmission error do to weak transport layer
            throw Erange("zapette::make_transfert", gettext("Incoherent answer from peer"));
        }

            // sanity checks
        if(req.size == REQUEST_SIZE_SPECIAL_ORDER)
        {
            if(req.offset == REQUEST_OFFSET_END_TRANSMIT)
            {
                if(ans.size != 0 && ans.type != ANSWER_TYPE_DATA)
                    get_gf_ui().warning(gettext("Bad answer from peer, while closing connection"));
            }
            else if(req.offset == REQUEST_OFFSET_GET_FILESIZE)
            {
                if(ans.size != 0 && ans.type != ANSWER_TYPE_INFININT)
                    throw Erange("zapette::make_transfert", gettext("Incoherent answer from peer"));
            }
	    else if(req.offset == REQUEST_OFFSET_CHANGE_CONTEXT_STATUS)
	    {
		if(ans.arg != 1)
		    throw Erange("zapette::set_info_status", gettext("Unexpected answer from slave, communication problem or bug may hang the operation"));
	    }
            else
                throw Erange("zapette::make_transfert", gettext("Corrupted data read from pipe"));
        }
    }

} // end of namespace
