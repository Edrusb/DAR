/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2026 Denis Corbin
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

#include "slave_zapette.hpp"
#include "infinint.hpp"
#include "tools.hpp"
#include "trivial_sar.hpp"
#include "sar.hpp"
#include "zapette_protocol.hpp"

using namespace std;

namespace libdar
{

    slave_zapette::slave_zapette(const std::shared_ptr<user_interaction> & dialog,
				 generic_file *input,
				 generic_file *output,
				 generic_file *data) : mem_ui(dialog)
    {
        if(input == nullptr)
            throw SRC_BUG;
        if(output == nullptr)
            throw SRC_BUG;
        if(data == nullptr)
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
	src_ctxt = dynamic_cast<contextual *>(data);
	if(src_ctxt == nullptr)
	    throw Erange("slave_zapette::slave_zapette", "Object given to data must inherit from contextual class");
    }

    slave_zapette::~slave_zapette()
    {
        if(in != nullptr)
            delete in;
        if(out != nullptr)
            delete out;
        if(src != nullptr)
            delete src;
    }

    void slave_zapette::action()
    {
        request req;
        answer ans;
        char *buffer = nullptr;
        U_16 buf_size = 1024;

	buffer = new (nothrow) char[buf_size];
	if(buffer == nullptr)
	    throw Ememory();

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
                            if(buffer != nullptr)
				delete [] buffer;

			    buffer = new (nothrow) char [req.size];
                            if(buffer == nullptr)
                                throw Ememory();
                            else
                                buf_size = req.size;
                        }

                        ans.size = src->read(buffer, req.size);
                        ans.write(out, buffer);
                    }
                    else // bad position
                    {
                        ans.size = 0;
                        ans.write(out, nullptr);
                    }
                }
                else // special orders
                {
                    if(req.offset == REQUEST_OFFSET_END_TRANSMIT) // stop communication
                    {
                        ans.type = ANSWER_TYPE_DATA;
                        ans.size = 0;
                        ans.write(out, nullptr);
                    }
                    else if(req.offset == REQUEST_OFFSET_GET_FILESIZE) // return file size
                    {
                        ans.type = ANSWER_TYPE_INFININT;
                        if(!src->skip_to_eof())
                            throw Erange("slave_zapette::action", gettext("Cannot skip at end of file"));
                        ans.arg = src->get_position();
                        ans.write(out, nullptr);
                    }
		    else if(req.offset == REQUEST_OFFSET_CHANGE_CONTEXT_STATUS) // contextual status change requested
		    {
			ans.type = ANSWER_TYPE_INFININT;
			ans.arg = 1;
 			src_ctxt->set_info_status(req.info);
			ans.write(out, nullptr);
		    }
                    else if(req.offset == REQUEST_IS_OLD_START_END_ARCHIVE) // return whether the underlying archive has an old slice header or not
		    {
			ans.type = ANSWER_TYPE_INFININT;
			ans.arg = src_ctxt->get_slice_info().get_format_07_compatibility() ? 1 : 0;
			ans.write(out, nullptr);
		    }
		    else if(req.offset == REQUEST_GET_DATA_NAME) // return the data_name of the underlying sar
		    {
			ans.type = ANSWER_TYPE_DATA;
			ans.arg = 0;
			ans.size = src_ctxt->get_slice_info().get_data_name().size();
			ans.write(out, (char *)(src_ctxt->get_slice_info().get_data_name().data()));
		    }
		    else if(req.offset == REQUEST_FIRST_SLICE_HEADER_SIZE)
		    {
			trivial_sar *src_triv = dynamic_cast<trivial_sar *>(src);
			sar *src_sar = dynamic_cast<sar *>(src);

			ans.type = ANSWER_TYPE_INFININT;
			if(src_triv != nullptr)
			    ans.arg = src_triv->get_slice_header_size();
			else if(src_sar != nullptr)
			    ans.arg = src_sar->get_slice_info().get_first_slice_header_size();
			else
			    ans.arg = 0; // means unknown
			ans.write(out, nullptr);
		    }
		    else if(req.offset == REQUEST_OTHER_SLICE_HEADER_SIZE)
		    {
			trivial_sar *src_triv = dynamic_cast<trivial_sar *>(src);
			sar *src_sar = dynamic_cast<sar *>(src);

			ans.type = ANSWER_TYPE_INFININT;
			if(src_triv != nullptr)
			    ans.arg = src_triv->get_slice_header_size();
			else if(src_sar != nullptr)
			    ans.arg = src_sar->get_slice_info().get_common_slice_header_size();
			else
			    ans.arg = 0; // means unknown
			ans.write(out, nullptr);
		    }
		    else if(req.offset == REQUEST_SLICE_INFO_SIZE)
		    {
			slice_header tmp = src_ctxt->get_slice_info();

			serialzd.reset();
			tmp.write(get_ui(), serialzd, true, true);

			ans.type = ANSWER_TYPE_INFININT;
			ans.arg = serialzd.size();
			ans.write(out, nullptr);
		    }
		    else if(req.offset == REQUEST_SLICE_INFO_DATA)
		    {
			infinint data_size = serialzd.size();
			char* tmp_data = nullptr;

			if(data_size.is_zero())
			    throw SRC_BUG; // a previous call with REQUEST_SLICE_INFO_SIZE has not taken place

			ans.size = 0;
			data_size.unstack(ans.size);
			if(! data_size.is_zero())
			    throw Erange("slave_zapette::action", gettext("too large slice header information for zapette protocol"));

			tmp_data = new (nothrow) char[ans.size];
			if(tmp_data == nullptr)
			    throw Ememory();

			try
			{
			    serialzd.dump_to((unsigned char*)(tmp_data), infinint(ans.size));
			    ans.type = ANSWER_TYPE_DATA;
			    ans.write(out, tmp_data);
			}
			catch(...)
			{
			    delete [] tmp_data;
			    throw;
			}
			if(tmp_data != nullptr)
			    delete [] tmp_data;

			serialzd.reset();
		    }
		    else
                        throw Erange("zapette::action", gettext("Received unknown special order"));
                }
            }
            while(req.size != REQUEST_SIZE_SPECIAL_ORDER || req.offset != REQUEST_OFFSET_END_TRANSMIT);
        }
        catch(...)
        {
            if(buffer != nullptr)
		delete [] buffer;
            throw;
        }

        if(buffer != nullptr)
	    delete [] buffer;
    }

} // end of namespace
