/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_LIMITS_H
#include <limits.h>
#endif

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

} // end extern "C"

#include "tools.hpp"
#include "compressor.hpp"

#define BUFFER_SIZE 102400
#ifdef SSIZE_MAX
#if SSIZE_MAX < BUFFER_SIZE
#undef BUFFER_SIZE
#define BUFFER_SIZE SSIZE_MAX
#endif
#endif

using namespace std;

namespace libdar
{

    compressor::compressor(compression algo, generic_file & compressed_side, U_I compression_level) : proto_compressor(compressed_side.get_mode())
    {

        wrapperlib_mode wr_mode;
        current_algo = algo;
	suspended = false;
	current_level = compression_level;
        compr = decompr = nullptr;

        if(compression_level > 9)
            throw SRC_BUG;

	switch(algo)
	{
	case compression::none:
	    break;
	case compression::gzip:
	    wr_mode = zlib_mode;
	    break;
	case compression::bzip2:
	    wr_mode = bzlib_mode;
	    break;
	case compression::lzo:
	    throw SRC_BUG;
	case compression::xz:
	    wr_mode = xz_mode;
	    break;
	case compression::lzo1x_1_15:
	    throw SRC_BUG;
	case compression::lzo1x_1:
	    throw SRC_BUG;
	case compression::zstd:
	    throw SRC_BUG;
	case compression::lz4:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}

	try
	{

	    compr = new (nothrow) xfer(BUFFER_SIZE, wr_mode);
	    if(compr == nullptr)
		throw Ememory("compressor::compressor");

	    decompr = new (nothrow) xfer(BUFFER_SIZE, wr_mode);
	    if(decompr == nullptr)
		throw Ememory("compressor::compressor");

	    switch(compr->wrap.compressInit(compression_level))
	    {
	    case WR_OK:
		break;
	    case WR_MEM_ERROR:
		delete compr;
		compr = nullptr;
		delete decompr;
		decompr = nullptr;
		throw Ememory("compressor::compressor");
	    case WR_VERSION_ERROR:
		delete compr;
		compr = nullptr;
		delete decompr;
		decompr = nullptr;
		throw Erange("compressor::compressor", gettext("incompatible compression library version or unsupported feature required from compression library"));
	    case WR_STREAM_ERROR:
	    default:
		delete compr;
		compr = nullptr;
		delete decompr;
		decompr = nullptr;
		throw SRC_BUG;
	    }

	    switch(decompr->wrap.decompressInit())
	    {
	    case WR_OK:
		decompr->wrap.set_avail_in(0);
		break;
	    case WR_MEM_ERROR:
		compr->wrap.compressEnd();
		delete compr;
		compr = nullptr;
		delete decompr;
		decompr = nullptr;
		throw Ememory("compressor::compressor");
	    case WR_VERSION_ERROR:
		compr->wrap.compressEnd();
		delete compr;
		compr = nullptr;
		delete decompr;
		decompr = nullptr;
		throw Erange("compressor::compressor", gettext("incompatible compression library version or unsupported feature required from compression library"));
	    case WR_STREAM_ERROR:
	    default:
		compr->wrap.compressEnd();
		delete compr;
		compr = nullptr;
		delete decompr;
		decompr = nullptr;
		throw SRC_BUG;
	    }


	    compressed = & compressed_side;
	}
	catch(...)
	{
	    if(compr != nullptr)
		delete compr;
	    if(decompr != nullptr)
		delete decompr;
	    throw;
	}

    }

    compressor::~compressor()
    {
	try
	{
	    terminate();
	}
	catch(...)
	{
		// ignore all exceptions
	}
	if(compr != nullptr)
	    delete compr;
	if(decompr != nullptr)
	    delete decompr;
    }

    compression compressor::get_algo() const
    {
	if(suspended)
	    return compression::none;
	else
	    return current_algo;
    }

    void compressor::suspend_compression()
    {
	if(!suspended)
	{
	    inherited_sync_write();
	    reset_compr_engine();
	    suspended = true;
	}
    }

    void compressor::resume_compression()
    {
	if(suspended)
	    suspended = false;
    }

    void compressor::reset_compr_engine()
    {
	if(compr != nullptr)
	    compr->wrap.compressReset();
	if(decompr != nullptr)
	    decompr->wrap.decompressReset();
    }

    void compressor::inherited_truncate(const infinint & pos)
    {
	if(pos < get_position())
	{
	    inherited_sync_write();
	    inherited_flush_read();
	}
	compressed->truncate(pos);
    }

    void compressor::inherited_terminate()
    {
        if(compr != nullptr)
        {
            S_I ret;

                // flushing the pending data
	    inherited_sync_write();
            clean_write();

            ret = compr->wrap.compressEnd();
            delete compr;
	    compr = nullptr;

            switch(ret)
            {
            case WR_OK:
                break;
            case WR_DATA_ERROR: // some data remains in the compression pipe (data loss)
                throw SRC_BUG;
            case WR_STREAM_ERROR:
                throw Erange("compressor::~compressor", gettext("compressed data is corrupted"));
            default :
                throw SRC_BUG;
            }
        }

        if(decompr != nullptr)
        {
                // flushing data
            inherited_flush_read();

            S_I ret = decompr->wrap.decompressEnd();
            delete decompr;
	    decompr = nullptr;

            switch(ret)
            {
            case WR_OK:
                break;
            default:
                throw SRC_BUG;
            }
        }
    }

    compressor::xfer::xfer(U_I sz, wrapperlib_mode mode) : wrap(mode)
    {
        buffer = new (nothrow) char[sz];
        if(buffer == nullptr)
            throw Ememory("compressor::xfer::xfer");
        size = sz;
    }

    compressor::xfer::~xfer()
    {
	if(buffer != nullptr)
	{
	    delete [] buffer;
	    buffer = nullptr;
	}
    }

    U_I compressor::inherited_read(char *a, U_I size)
    {
        S_I ret;
        S_I flag = WR_NO_FLUSH;
	U_I mem_avail_out = 0;
	enum { normal, no_more_input, eof } processing = normal;

        if(size == 0)
            return 0;

	if(suspended)
	    return compressed->read(a, size);

        decompr->wrap.set_next_out(a);
        decompr->wrap.set_avail_out(size);

        do
        {
                // feeding the input buffer if necessary
            if(decompr->wrap.get_avail_in() == 0)
            {
                decompr->wrap.set_next_in(decompr->buffer);
                decompr->wrap.set_avail_in(compressed->read(decompr->buffer,
                                                            decompr->size));

		if(decompr->wrap.get_avail_in() == 0)
		    mem_avail_out = decompr->wrap.get_avail_out();
		    // could not add compressed data, so if no more clear data is produced
		    // we must break the endless loop if WR_STREAM_END is not returned by decompress()
		    // this situation can occur upon data corruption
		else
		    mem_avail_out = 0;
            }
	    if(decompr->wrap.get_avail_in() == 0)
		processing = no_more_input;

            ret = decompr->wrap.decompress(flag);
	    if(ret == 0 && processing == no_more_input) // nothing extracted from decompression engine and no more compression data available
		processing = eof;


            switch(ret)
            {
            case WR_OK:
            case WR_STREAM_END:
                break;
            case WR_DATA_ERROR:
                throw Erange("compressor::gzip_read", gettext("compressed data CRC error"));
            case WR_MEM_ERROR:
                throw Ememory("compressor::gzip_read");
            case WR_BUF_ERROR:
                    // no process is possible:
                if(decompr->wrap.get_avail_in() == 0) // because we reached EOF
                    ret = WR_STREAM_END; // lib did not returned WR_STREAM_END (why ?)
                else // nothing explains why no process is possible:
                    if(decompr->wrap.get_avail_out() == 0)
                        throw SRC_BUG; // bug from DAR: no output possible
                    else
                        throw SRC_BUG; // unexpected comportment from lib
                break;
            default:
                throw SRC_BUG;
            }
        }
        while(decompr->wrap.get_avail_out() != mem_avail_out && ret != WR_STREAM_END && processing != eof);

        return decompr->wrap.get_next_out() - a;
    }

    void compressor::inherited_write(const char *a, U_I size)
    {
        compr->wrap.set_next_in(a);
        compr->wrap.set_avail_in(size);

        if(a == nullptr)
            throw SRC_BUG;

	if(suspended)
	    compressed->write(a, size);
	else
	{
	    while(compr->wrap.get_avail_in() > 0)
	    {
		    // making room for output
		compr->wrap.set_next_out(compr->buffer);
		compr->wrap.set_avail_out(compr->size);

		switch(compr->wrap.compress(WR_NO_FLUSH))
		{
		case WR_OK:
		case WR_STREAM_END:
		    break;
		case WR_STREAM_ERROR:
		    throw SRC_BUG;
		case WR_BUF_ERROR:
		    throw SRC_BUG;
		default :
		    throw SRC_BUG;
		}

		if(compr->wrap.get_next_out() != compr->buffer)
		    compressed->write(compr->buffer, (char *)compr->wrap.get_next_out() - compr->buffer);
	    }
	}
    }

    void compressor::inherited_sync_write()
    {
        S_I ret;

	if(is_terminated())
	    throw SRC_BUG;

	    // no more input
	compr->wrap.set_avail_in(0);
	do
	{
		// setting the buffer for reception of data
	    compr->wrap.set_next_out(compr->buffer);
	    compr->wrap.set_avail_out(compr->size);

	    ret = compr->wrap.compress(WR_FINISH);

	    switch(ret)
	    {
	    case WR_OK:
	    case WR_STREAM_END:
		if(compr->wrap.get_next_out() != compr->buffer)
		    compressed->write(compr->buffer, (char *)compr->wrap.get_next_out() - compr->buffer);
		break;
	    case WR_BUF_ERROR :
		throw SRC_BUG;
	    case WR_STREAM_ERROR :
		throw SRC_BUG;
	    default :
		throw SRC_BUG;
	    }
	}
	while(ret != WR_STREAM_END);

	if(compr->wrap.compressReset() != WR_OK)
	    throw SRC_BUG;

    }

    void compressor::inherited_flush_read()
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(decompr != nullptr) // zlib
            if(decompr->wrap.decompressReset() != WR_OK)
                throw SRC_BUG;
            // keep in the buffer the bytes already read, these are discarded in case of a call to skip

        if(decompr != nullptr)
            decompr->wrap.set_avail_in(0);
    }

    void compressor::clean_write()
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(compr != nullptr)
        {
            S_I ret;

            do
            {
                compr->wrap.set_next_out(compr->buffer);
                compr->wrap.set_avail_out(compr->size);
                compr->wrap.set_avail_in(0);

                ret = compr->wrap.compress(WR_FINISH);
            }
            while(ret == WR_OK);
        }
    }

} // end of namespace
