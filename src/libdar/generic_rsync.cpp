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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_LIBRSYNC_H
#include <stdio.h>
#include <librsync.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif
} // end extern "C"

#include "generic_rsync.hpp"
#include "memory_file.hpp"
#include "null_file.hpp"
#include "erreurs.hpp"

#define BUFFER_SIZE 102400
#ifdef SSIZE_MAX
#if SSIZE_MAX < BUFFER_SIZE
#undef BUFFER_SIZE
#define BUFFER_SIZE SSIZE_MAX
#endif
#endif

#define SMALL_BUF 10

using namespace std;

namespace libdar
{

	// signature creation

    generic_rsync::generic_rsync(generic_file *signature_storage,
				 U_I signature_block_size,
				 generic_file *below): generic_file(gf_read_only)
    {
#if LIBRSYNC_AVAILABLE

	    // sanity checks

	if(signature_storage == nullptr)
	    throw SRC_BUG;
	if(below == nullptr)
	    throw SRC_BUG;

	    // setting up the object

	working_buffer = new (nothrow) char[BUFFER_SIZE];
	if(working_buffer == nullptr)
	    throw Ememory("generic_rsync::generic_rsync (sign)");
	try
	{
	    working_size = 0;
	    status = sign;
	    x_below = below;
	    x_output = signature_storage;
	    x_input = nullptr;
	    sumset = nullptr;
	    initial = true;
	    patching_completed = false; // not used in sign mode
	    data_crc = nullptr;
#ifdef RS_DEFAULT_STRONG_LEN
	    job = rs_sig_begin(signature_block_size, RS_DEFAULT_STRONG_LEN);
#else
		// should use RS_BLAKE2_SIG_MAGIC in place of RS_MD4_SIG_MAGIC
		// but not compatible with librsync < 1.0
	    job = rs_sig_begin(signature_block_size, 0, RS_MD4_SIG_MAGIC);
#endif
	}
	catch(...)
	{
	    delete [] working_buffer;
	    throw;
	}
#else
	throw Ecompilation("librsync support");
#endif
    }


	// delta creation

    generic_rsync::generic_rsync(generic_file *base_signature,
				 generic_file *below,
				 const infinint & crc_size,
				 const crc **checksum): generic_file(gf_read_only)
    {
#if LIBRSYNC_AVAILABLE
	char *inbuf = nullptr;
	char *outbuf = nullptr;
	U_I lu = 0;
	U_I out;
	bool eof = false;
	rs_result err;

	    // sanity checks

	if(base_signature == nullptr)
	    throw SRC_BUG;
	if(below == nullptr)
	    throw SRC_BUG;

	    // setting up the object

	working_size = 0;
	status = delta;
	initial = true;
	patching_completed = false; // not used in delta mode
	data_crc = nullptr;
	working_buffer = new (nothrow) char[BUFFER_SIZE];
	if(working_buffer == nullptr)
	    throw Ememory("generic_rsync::generic_rsync (sign)");

	try
	{
		// loading signature into memory

	    job = rs_loadsig_begin(&sumset);
	    try
	    {
		inbuf = new (nothrow) char[BUFFER_SIZE];
		outbuf = new (nothrow) char [SMALL_BUF]; // nothing should be output
		if(inbuf == nullptr || outbuf == nullptr)
		    throw Ememory("generic_rsync::generic_rsync (delta)");

		base_signature->skip(0);

		do
		{
		    lu += base_signature->read(inbuf + lu, BUFFER_SIZE - lu);
		    if(lu == 0)
			eof = true;
		    out = SMALL_BUF;
		    if(!step_forward(inbuf, lu,
				     true,
				     outbuf, out)
		       && eof)
			throw SRC_BUG;
		    if(out != 0)
			throw SRC_BUG; // loading signature into memory
			// should never produce data on output
		}
		while(!eof);

		if(inbuf != nullptr)
		{
		    delete [] inbuf;
		    inbuf = nullptr;
		}
		if(outbuf != nullptr)
		{
		    delete [] outbuf;
		    outbuf = nullptr;
		}
		free_job();
	    }
	    catch(...)
	    {
		if(inbuf != nullptr)
		{
		    delete [] inbuf;
		    inbuf = nullptr;
		}
		if(outbuf != nullptr)
		{
		    delete [] outbuf;
		    outbuf = nullptr;
		}
		free_job();
		rs_free_sumset(sumset);
		throw;
	    }

		// creating the delta job

	    if(checksum != nullptr)
		data_crc = create_crc_from_size(crc_size);
	    if(data_crc == nullptr)
		throw Ememory("generic_rsync::generic_rsync");

	    try
	    {
		err = rs_build_hash_table(sumset);
		if(err != RS_DONE)
		    throw Erange("generic_rsync::generic_rsync", string(gettext("Error met building the rsync hash table: ")) + string(rs_strerror(err)));
		job = rs_delta_begin(sumset);
		x_below = below;
		x_input = nullptr;
		x_output = nullptr;
	    }
	    catch(...)
	    {
	 	if(data_crc != nullptr)
		    delete data_crc;
		throw;
	    }

	    if(data_crc != nullptr)
		*checksum = data_crc;
	}
	catch(...)
	{
	    delete [] working_buffer;
	    throw;
	}
#else
	throw Ecompilation("librsync support");
#endif
    }

	// patch creation

    generic_rsync::generic_rsync(generic_file *current_data,
				 generic_file *delta): generic_file(gf_read_only)
    {
#if LIBRSYNC_AVAILABLE

	    // sanity checks

	if(current_data == nullptr)
	    throw SRC_BUG;
	if(delta == nullptr)
	    throw SRC_BUG;

	    // setting up the object

	status = patch;
	patching_completed = false;
	x_input = current_data;
	x_output = nullptr;
	x_below = delta;
	sumset = nullptr;
	initial = true;
	working_size = 0;
	data_crc = nullptr;
	working_buffer = new (nothrow) char[BUFFER_SIZE];
	if(working_buffer == nullptr)
	    throw Ememory("generic_rsync::generic_rsync (sign)");
	try
	{
	    job = rs_patch_begin(generic_rsync::patch_callback, this);
	}
	catch(...)
	{
	    delete [] working_buffer;
	    throw;
	}
#else
	throw Ecompilation("librsync support");
#endif
    }


    generic_rsync::~generic_rsync() noexcept(false)
    {
	terminate();
	delete [] working_buffer;
    }

    U_I generic_rsync::inherited_read(char *a, U_I size)
    {
	U_I lu = 0;
	U_I remain;
	bool eof = false;

	initial = false;

	if(patching_completed)
	    return 0;

	switch(status)
	{
	case sign:
	    lu = x_below->read(a, size);
	    remain = lu;
	    do
	    {
		working_size = BUFFER_SIZE;
		(void)step_forward(a + lu - remain, remain,
				   false,
				   working_buffer, working_size);

		if(working_size > 0)
		    x_output->write(working_buffer, working_size);
	    }
	    while(remain > 0);
	    break;
	case delta:
	    do
	    {
		if(!eof)
		{
		    U_I tmp = x_below->read(working_buffer + working_size,
					    BUFFER_SIZE - working_size);
		    if(tmp > 0)
		    {
			if(data_crc != nullptr)
			    data_crc->compute(working_buffer + working_size, tmp);
			working_size += tmp;
		    }

		    if(working_size == 0)
			eof = true;
		}
		else
		    working_size = 0;

		remain = size - lu;
		(void)step_forward(working_buffer, working_size,
				   true,
				   a + lu, remain);
		lu += remain;
	    }
	    while(lu < size && (!eof || remain != 0));
	    break;
	case patch:
	    do
	    {
		if(!eof)
		{
		    working_size += x_below->read(working_buffer + working_size,
						  BUFFER_SIZE - working_size);
		    if(working_size == 0)
			eof = true;
		}
		else
		    working_size = 0;

		remain = size - lu;
		if(step_forward(working_buffer,
				working_size,
				true,
				a + lu,
				remain))
		{
		    if(working_size > 0 && remain == 0)
			throw Edata("While patching file, librsync tells it has finished processing data while we still have pending data to send to it");
		    patching_completed = true;
		}
		else
		{
		    if(eof && remain == 0)
			throw Edata("While patching file, librsync tells it has not finished processing data while we have no more to feed to it and librsync did not made any progression in the last cycle (it did not produce new data)");
		}
		lu += remain;
	    }
	    while(lu < size && (remain > 0 || !eof) && !patching_completed);
	    break;
	default:
	    throw SRC_BUG;
	}

	return lu;
    }

    void generic_rsync::inherited_write(const char *a, U_I size)
    {
	initial = false;

	switch(status)
	{
	case sign:
	    throw SRC_BUG;
	case delta:
	    throw SRC_BUG;
	case patch:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}
    }

    void generic_rsync::inherited_terminate()
    {
	switch(status)
	{
	case sign:
	case delta:
	    send_eof();
	    break;
      	case patch:
	    break;
	default:
	    throw SRC_BUG;
	}

#if LIBRSYNC_AVAILABLE
	if(sumset != nullptr)
	{
	    rs_free_sumset(sumset);
	    sumset = nullptr;
	}
#endif
	free_job();
    }

#if LIBRSYNC_AVAILABLE
    rs_result generic_rsync::patch_callback(void *opaque,
					    rs_long_t pos,
					    size_t *len,
					    void **buf)
    {
	rs_result ret;
	generic_rsync *me = (generic_rsync *)(opaque);
	U_I lu;

	if(me == nullptr)
	    throw SRC_BUG;
	if(me->x_input == nullptr)
	    throw SRC_BUG;

	try
	{
	    me->x_input->skip(pos);
	    lu = me->x_input->read((char *)*buf, *len);
	    if(*len > 0 && lu == 0)
		ret = RS_INPUT_ENDED;
	    else
		ret = RS_DONE;
	    *len = lu;
	}
	catch(...)
	{
	    *len = 0;
	    ret = RS_IO_ERROR;
	}

	return ret;
    }
#endif

    bool generic_rsync::step_forward(const char *buffer_in,
				     U_I & avail_in,
				     bool shift_input,
				     char *buffer_out,
				     U_I  & avail_out)
    {
#if LIBRSYNC_AVAILABLE
	bool ret;
	rs_buffers_t buf;
	rs_result res;

	buf.next_in = const_cast<char *>(buffer_in);
	buf.avail_in = avail_in;
	buf.next_out = buffer_out;
	buf.avail_out = avail_out;
	if(avail_in == 0) // EOF reached when called with no input bytes
	    buf.eof_in = 1;
	else
	    buf.eof_in = 0;

	res = rs_job_iter(job, &buf);
	switch(res)
	{
	case RS_DONE:
	    ret = true;
	    break;
	case RS_BLOCKED:
	    ret = false;
	    break;
	default:
	    throw Erange("generic_rsync::step_forward", string(gettext("Error met while feeding data to librsync: ")) + rs_strerror(res));
	}

	if(buf.avail_in > 0 && shift_input)
	    (void)memmove(const_cast<char *>(buffer_in), buf.next_in, buf.avail_in);
	avail_in = buf.avail_in;
	avail_out = buf.next_out - buffer_out;

	return ret;
#else
	return false;
#endif
    }

    void generic_rsync::free_job()
    {
#if LIBRSYNC_AVAILABLE
    	if(job != nullptr)
	{
	    rs_result err = rs_job_free(job);
	    job = nullptr;
	    if(err != RS_DONE)
		throw Erange("generic_rsync::inherited_terminate", string(gettext("Error releasing librsync job: ")) + string(rs_strerror(err)));
	}
#endif
    }

    void generic_rsync::send_eof()
    {
	U_I tmp;
	bool finished;

	do
	{
	    tmp = 0;
	    working_size = BUFFER_SIZE;

	    finished = step_forward(working_buffer, tmp, // as tmp is set to zero we can use working_buffer in input too
				    true,
				    working_buffer, working_size);
	    if(working_size > 0)
		x_output->write(working_buffer, working_size);
	    if(tmp > 0)
		throw SRC_BUG;
	}
	while(working_size > 0 && !finished);
    }

} // end of namespace


