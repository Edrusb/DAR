/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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
} // end extern "C"

#include <string>
#include <new>

#include "zapette.hpp"
#include "infinint.hpp"
#include "tools.hpp"
#include "trivial_sar.hpp"
#include "sar.hpp"
#include "zapette_protocol.hpp"

using namespace std;

namespace libdar
{
    zapette::zapette(const shared_ptr<user_interaction> & dialog,
		     generic_file *input,
		     generic_file *output,
		     bool by_the_end) : generic_file(gf_read_only), mem_ui(dialog)
    {
	if(input == nullptr)
	    throw SRC_BUG;
	if(output == nullptr)
	    throw SRC_BUG;
	if(input->get_mode() == gf_write_only)
	    throw Erange("zapette::zapette", gettext("Cannot read on input"));
	if(output->get_mode() == gf_read_only)
	    throw Erange("zapette::zapette", gettext("Cannot write on output"));

	in = input;
	out = output;
	position = 0;
	serial_counter = 0;
	contextual::set_info_status(CONTEXT_INIT);

	    //////////////////////////////
	    // retreiving the file size
	    //
	S_I tmp = 0;
	make_transfert(REQUEST_SIZE_SPECIAL_ORDER, REQUEST_OFFSET_GET_FILESIZE, nullptr, "", tmp, file_size);

	    //////////////////////////////
	    // positionning cursor for next read
	    // depending on the by_the_end value
	    //

	try
	{
	    if(by_the_end)
	    {
		try
		{
		    skip_to_eof();
		}
		catch(Erange & e)
		{
		    string tmp = e.get_message();
		    get_ui().printf(gettext("Failed driving dar_slave to the end of archive: %S. Trying to open the archive from the first bytes"), &tmp);
		    skip(0);
		}
	    }
	    else
		skip(0);
	}
	catch(...)
	{
	    terminate();
	    throw;
	}
    }

    zapette::~zapette()
    {
	try
	{
	    terminate();
	}
	catch(...)
	{
		// ignore all exceptions
	}
        delete in;
        delete out;
    }

    void zapette::inherited_terminate()
    {
        S_I tmp = 0;
        make_transfert(REQUEST_SIZE_SPECIAL_ORDER, REQUEST_OFFSET_END_TRANSMIT, nullptr, "", tmp, file_size);
    }

    bool zapette::skip(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

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
	if(is_terminated())
	    throw SRC_BUG;

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
	S_I tmp = 0;

	if(is_terminated())
	    throw SRC_BUG;

	make_transfert(REQUEST_SIZE_SPECIAL_ORDER, REQUEST_OFFSET_CHANGE_CONTEXT_STATUS, nullptr, s, tmp, val);
	contextual::set_info_status(s);
    }

    bool zapette::is_an_old_start_end_archive() const
    {
	infinint val;
	S_I tmp = 0;

	if(is_terminated())
	    throw SRC_BUG;

  	make_transfert(REQUEST_SIZE_SPECIAL_ORDER, REQUEST_IS_OLD_START_END_ARCHIVE, nullptr, "", tmp, val);
	return val == 1;
    }

    const label & zapette::get_data_name() const
    {
	static label data_name;
	infinint arg;
	S_I lu = data_name.size(); // used to specify the amount of space allocated for the answer

	if(is_terminated())
	    throw SRC_BUG;

	make_transfert(REQUEST_SIZE_SPECIAL_ORDER, REQUEST_GET_DATA_NAME, data_name.data(), "", lu, arg);

	if(lu != (S_I)data_name.size())
	    throw Erange("zapette::get_data_name", gettext("Uncomplete answer received from peer"));

	return data_name;
    }

    infinint zapette::get_first_slice_header_size() const
    {
	infinint ret;
	S_I tmp;

	if(is_terminated())
	    throw SRC_BUG;

	make_transfert(REQUEST_SIZE_SPECIAL_ORDER, REQUEST_FIRST_SLICE_HEADER_SIZE, nullptr, "", tmp, ret);

	return ret;
    }

    infinint zapette::get_non_first_slice_header_size() const
    {
	infinint ret;
	S_I tmp = 0;

	if(is_terminated())
	    throw SRC_BUG;

	make_transfert(REQUEST_SIZE_SPECIAL_ORDER, REQUEST_OTHER_SLICE_HEADER_SIZE, nullptr, "", tmp, ret);

	return ret;
    }


    U_I zapette::inherited_read(char *a, U_I size)
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

    void zapette::inherited_write(const char *a, U_I size)
    {
        throw SRC_BUG; // zapette is read-only
    }

    void zapette::make_transfert(U_16 size, const infinint &offset, char *data, const string & info, S_I & lu, infinint & arg) const
    {
        request req;
        answer ans;

            // building the request
        req.serial_num = const_cast<char &>(serial_counter)++; // may loopback to 0
        req.offset = offset;
        req.size = size;
	req.info = info;
        req.write(out);

	if(req.size == REQUEST_SIZE_SPECIAL_ORDER)
	    size = lu;

            // reading the answer
        do
        {
            ans.read(in, data, size);
            if(ans.serial_num != req.serial_num)
                get_ui().pause(gettext("Communication problem with peer, retry ?"));
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
                    get_ui().message(gettext("Bad answer from peer, while closing connection"));
            }
            else if(req.offset == REQUEST_OFFSET_GET_FILESIZE)
            {
                if(ans.size != 0 && ans.type != ANSWER_TYPE_INFININT)
                    throw Erange("zapette::make_transfert", gettext("Incoherent answer from peer"));
            }
	    else if(req.offset == REQUEST_OFFSET_CHANGE_CONTEXT_STATUS)
	    {
		if(ans.arg != 1)
		    throw Erange("zapette::make_transfert", gettext("Unexpected answer from slave, communication problem or bug may hang the operation"));
	    }
            else if(req.offset == REQUEST_IS_OLD_START_END_ARCHIVE)
	    {
		if(ans.type != ANSWER_TYPE_INFININT || (ans.arg != 0 && ans.arg != 1) )
		    throw Erange("zapetee::make_transfert", gettext("Unexpected answer from slave, communication problem or bug may hang the operation"));
	    }
	    else if(req.offset == REQUEST_GET_DATA_NAME)
	    {
		if(ans.type != ANSWER_TYPE_DATA && lu != (S_I)(label::common_size()))
		    throw Erange("zapetee::make_transfert", gettext("Unexpected answer from slave, communication problem or bug may hang the operation"));
	    }
	    else if(req.offset == REQUEST_FIRST_SLICE_HEADER_SIZE)
	    {
		if(ans.size != 0 && ans.type != ANSWER_TYPE_INFININT)
                    throw Erange("zapette::make_transfert", gettext("Incoherent answer from peer"));
	    }
	    else if(req.offset == REQUEST_OTHER_SLICE_HEADER_SIZE)
	    {
		if(ans.size != 0 && ans.type != ANSWER_TYPE_INFININT)
                    throw Erange("zapette::make_transfert", gettext("Incoherent answer from peer"));
	    }
	    else
                throw Erange("zapette::make_transfert", gettext("Corrupted data read from pipe"));
        }
    }

} // end of namespace
