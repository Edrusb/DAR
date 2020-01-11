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

#if LIBLZO2_AVAILABLE
#if HAVE_LZO_LZO1X_H
#include <lzo/lzo1x.h>
#endif
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

    // value found in lzop source code p_lzo.c
#define LZO_COMPRESSED_BUFFER_SIZE 256*1024l
#ifdef SSIZE_MAX
#if SSIZE_MAX < LZO_COMPRESSED_BUFFER_SIZE
#undef LZO_COMPRESSED_BUFFER_SIZE
#define LZO_COMPRESSED_BUFFER_SIZE SSIZE_MAX
#endif
#endif

#define LZO_CLEAR_BUFFER_SIZE ((LZO_COMPRESSED_BUFFER_SIZE - 64 - 3)*16/17)
    // value calculated with information found in LZO's FAQ

#if LZO_CLEAR_BUFFER_SIZE < 1
#error "System's SSIZE_MAX too small to handle LZO compression"
#endif

#define BLOCK_HEADER_LZO 1
#define BLOCK_HEADER_EOF 2

using namespace std;

namespace libdar
{

    compressor::compressor(compression algo, generic_file & compressed_side, U_I compression_level) : generic_file(compressed_side.get_mode())
    {
        init(algo, &compressed_side, compression_level);
        compressed_owner = false;
    }

    compressor::compressor(compression algo, generic_file *compressed_side, U_I compression_level) : generic_file(compressed_side->get_mode())
    {
        init(algo, compressed_side, compression_level);
        compressed_owner = true;
    }

    void compressor::init(compression algo, generic_file *compressed_side, U_I compression_level)
    {
            // these are eventually overwritten below
        wrapperlib_mode wr_mode = zlib_mode;
        current_algo = algo;
	suspended = false;
	current_level = compression_level;

        if(compressed_side == nullptr)
            throw SRC_BUG;
        if(compression_level > 9)
            throw SRC_BUG;

        compr = decompr = nullptr;
	lzo_read_buffer = lzo_write_buffer = nullptr;
	lzo_compressed = nullptr;
	lzo_wrkmem = nullptr;

        switch(algo)
        {
        case compression::none:
            read_ptr = &compressor::none_read;
            write_ptr = &compressor::none_write;
            break;
        case compression::bzip2:
	case compression::xz:
	    if(algo == compression::bzip2)
		wr_mode = bzlib_mode;
	    if(algo == compression::xz)
		wr_mode = xz_mode;

                // NO BREAK !
        case compression::gzip:
            read_ptr = &compressor::gzip_read;
            write_ptr = &compressor::gzip_write;
            compr = new (nothrow) xfer(BUFFER_SIZE, wr_mode);
            if(compr == nullptr)
                throw Ememory("compressor::compressor");
            decompr = new (nothrow) xfer(BUFFER_SIZE, wr_mode);
            if(decompr == nullptr)
            {
                delete compr;
		compr = nullptr;
                throw Ememory("compressor::compressor");
            }

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
            break;
	case compression::lzo:
	case compression::lzo1x_1_15:
	case compression::lzo1x_1:
#if LIBLZO2_AVAILABLE
	    read_ptr = &compressor::lzo_read;
	    write_ptr = &compressor::lzo_write;
	    lzo_read_size = lzo_write_size = 0;
	    lzo_read_start = 0;
	    lzo_write_flushed = true;
	    lzo_read_reached_eof = false;
	    try
	    {
		lzo_read_buffer = new (nothrow) char[LZO_CLEAR_BUFFER_SIZE];
		lzo_write_buffer = new (nothrow) char[LZO_CLEAR_BUFFER_SIZE];
		lzo_compressed = new (nothrow) char[LZO_COMPRESSED_BUFFER_SIZE];
		switch(algo)
		{
		case compression::lzo:
		    lzo_wrkmem = new (nothrow) char[LZO1X_999_MEM_COMPRESS];
		    break;
		case compression::lzo1x_1_15:
		    lzo_wrkmem = new (nothrow) char[LZO1X_1_15_MEM_COMPRESS];
		    break;
		case compression::lzo1x_1:
		    lzo_wrkmem = new (nothrow) char[LZO1X_1_MEM_COMPRESS];
		    break;
		default:
		    throw SRC_BUG;
		}
		if(lzo_read_buffer == nullptr || lzo_write_buffer == nullptr || lzo_compressed == nullptr || lzo_wrkmem == nullptr)
		    throw Ememory("compressor::init");
	    }
	    catch(...)
	    {
		if(lzo_read_buffer != nullptr)
		{
		    delete [] lzo_read_buffer;
		    lzo_read_buffer = nullptr;
		}
		if(lzo_write_buffer != nullptr)
		{
		    delete [] lzo_write_buffer;
		    lzo_write_buffer = nullptr;
		}
		if(lzo_compressed != nullptr)
		{
		    delete [] lzo_compressed;
		    lzo_compressed = nullptr;
		}
		if(lzo_wrkmem != nullptr)
		{
		    delete [] lzo_wrkmem;
		    lzo_wrkmem = nullptr;
		}
		throw;
	    }
	    break;
#else
	    throw Ecompilation("lzo compression support (liblzo2)");
#endif
        default :
            throw SRC_BUG;
        }

        compressed = compressed_side;
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
	if(lzo_read_buffer != nullptr)
	    delete [] lzo_read_buffer;
	if(lzo_write_buffer != nullptr)
	    delete [] lzo_write_buffer;
	if(lzo_compressed != nullptr)
	    delete [] lzo_compressed;
	if(lzo_wrkmem != nullptr)
	    delete [] lzo_wrkmem;
	if(compressed_owner)
	    if(compressed != nullptr)
		delete compressed;
    }

    void compressor::suspend_compression()
    {
	if(!suspended)
	{
	    suspended_compr = current_algo;
	    change_algo(compression::none);
	    suspended = true;
	}
    }

    void compressor::resume_compression()
    {
	if(suspended)
	{
	    change_algo(suspended_compr);
	    suspended = false;
	}
    }

    void compressor::local_terminate()
    {
        if(compr != nullptr)
        {
            S_I ret;

                // flushing the pending data
	    compr_flush_write();
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
            compr_flush_read();
            clean_read();

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

	if(lzo_read_buffer != nullptr)
	{
	    compr_flush_read();
	    clean_read();
	    delete [] lzo_read_buffer;
	    lzo_read_buffer = nullptr;
	}

	if(lzo_write_buffer != nullptr)
	{
	    compr_flush_write();
	    clean_write();
	    delete [] lzo_write_buffer;
	    lzo_write_buffer = nullptr;
	}

	if(lzo_compressed != nullptr)
	{
	    delete [] lzo_compressed;
	    lzo_compressed = nullptr;
	}

	if(lzo_wrkmem != nullptr)
	{
	    delete [] lzo_wrkmem;
	    lzo_wrkmem = nullptr;
	}
    }

    void compressor::change_algo(compression new_algo, U_I new_compression_level)
    {
        if(new_algo == get_algo() && new_compression_level == current_level)
            return;

	if(is_terminated())
	    throw SRC_BUG;

            // flush data and release zlib memory structures
        local_terminate();

            // change to new algorithm
        init(new_algo, compressed, new_compression_level);
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

    U_I compressor::none_read(char *a, U_I size)
    {
        return compressed->read(a, size);
    }

    void compressor::none_write(const char *a, U_I size)
    {
        compressed->write(a, size);
    }

    U_I compressor::gzip_read(char *a, U_I size)
    {
        S_I ret;
        S_I flag = WR_NO_FLUSH;
	U_I mem_avail_out = 0;
	enum { normal, no_more_input, eof } processing = normal;

        if(size == 0)
            return 0;

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

    void compressor::gzip_write(const char *a, U_I size)
    {
        compr->wrap.set_next_in(a);
        compr->wrap.set_avail_in(size);

        if(a == nullptr)
            throw SRC_BUG;

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

    U_I compressor::lzo_read(char *a, U_I size)
    {
#if LIBLZO2_AVAILABLE
	U_I read = 0;

	while(read < size && !lzo_read_reached_eof)
	{
	    U_I available = lzo_read_size - lzo_read_start;
	    U_I to_read = size - read;

	    if(available > to_read)
	    {
		(void)memcpy(a+read, lzo_read_buffer+lzo_read_start, to_read);
		lzo_read_start += to_read;
		read += to_read;
	    }
	    else
	    {
		if(available > 0)
		{
		    (void)memcpy(a+read, lzo_read_buffer+lzo_read_start, available);
		    lzo_read_start += available;
		    read += available;
		}
		if(lzo_read_start < lzo_read_size)
		    throw SRC_BUG;
		lzo_read_and_uncompress_to_buffer();
		lzo_read_reached_eof = (lzo_read_size == 0); // either true or false
	    }
	}

	return read;
#else
	throw Efeature(gettext("lzo compression"));
#endif
    }

    void compressor::lzo_write(const char *a, U_I size)
    {
#if LIBLZO2_AVAILABLE
	U_I wrote = 0;
	lzo_write_flushed = false;

	while(wrote < size)
	{
	    U_I to_write = size - wrote;
	    U_I space = LZO_CLEAR_BUFFER_SIZE - lzo_write_size;

	    if(to_write < space)
	    {
		(void)memcpy(lzo_write_buffer + lzo_write_size, a + wrote, to_write);
		wrote += to_write;
		lzo_write_size += to_write;
	    }
	    else
	    {
		(void)memcpy(lzo_write_buffer + lzo_write_size, a + wrote, space);
		wrote += space;
		lzo_write_size += space;
		lzo_compress_buffer_and_write();
	    }
	}
#else
	throw Efeature(gettext("lzo compression"));
#endif
    }

    void compressor::compr_flush_write()
    {
        S_I ret;

	if(is_terminated())
	    throw SRC_BUG;

        if(compr != nullptr && compr->wrap.get_total_in() != 0)  // (z/bz)lib
        {
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

	if(lzo_write_buffer != nullptr && ! lzo_write_flushed) // lzo
	{
	    lzo_block_header lzo_bh;

	    lzo_compress_buffer_and_write();
	    lzo_bh.type = BLOCK_HEADER_EOF;
	    lzo_bh.size = 0;
	    if(compressed == nullptr)
		throw SRC_BUG;
	    lzo_bh.dump(*compressed);
	    lzo_write_flushed = true;
	}
    }

    void compressor::compr_flush_read()
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(decompr != nullptr) // zlib
            if(decompr->wrap.decompressReset() != WR_OK)
                throw SRC_BUG;
            // keep in the buffer the bytes already read, these are discarded in case of a call to skip
	lzo_read_reached_eof = false;
    }

    void compressor::clean_read()
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(decompr != nullptr)
            decompr->wrap.set_avail_in(0);

	if(lzo_read_buffer != nullptr) // lzo
	{
	    lzo_read_start = 0;
	    lzo_read_size = 0;
	}
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

	if(lzo_write_buffer != nullptr) // lzo
	    lzo_write_size = 0;
    }


    void compressor::lzo_compress_buffer_and_write()
    {
#if LIBLZO2_AVAILABLE
	lzo_block_header lzo_bh;
	lzo_uint compr_size = LZO_COMPRESSED_BUFFER_SIZE;
	S_I status;

	    //compressing data to lzo_compress buffer
	switch(current_algo)
	{
	case compression::lzo:
	    status = lzo1x_999_compress_level((lzo_bytep)lzo_write_buffer, lzo_write_size, (lzo_bytep)lzo_compressed, &compr_size, lzo_wrkmem, nullptr, 0, 0, current_level);
	    break;
	case compression::lzo1x_1_15:
	    status = lzo1x_1_15_compress((lzo_bytep)lzo_write_buffer, lzo_write_size, (lzo_bytep)lzo_compressed, &compr_size, lzo_wrkmem);
	    break;
	case compression::lzo1x_1:
	    status = lzo1x_1_compress((lzo_bytep)lzo_write_buffer, lzo_write_size, (lzo_bytep)lzo_compressed, &compr_size, lzo_wrkmem);
	    break;
	default:
	    throw SRC_BUG;
	}

	switch(status)
	{
	case LZO_E_OK:
	    break; // all is fine
	default:
	    throw Erange("compressor::lzo_compress_buffer_and_write", tools_printf(gettext("Probable bug in liblzo2: lzo1x_*_compress returned unexpected code %d"), status));
	}

	    // writing down the TL(V) before the compressed data
	lzo_bh.type = BLOCK_HEADER_LZO;
	lzo_bh.size = compr_size;
	if(compressed == nullptr)
	    throw SRC_BUG;

	lzo_bh.dump(*compressed);
	compressed->write(lzo_compressed, compr_size);

	lzo_write_size = 0;
#else
	throw Efeature(gettext("lzo compression"));
#endif
    }

    void compressor::lzo_read_and_uncompress_to_buffer()
    {
#if LIBLZO2_AVAILABLE
	lzo_block_header lzo_bh;
	lzo_uint compr_size;
	int status;
#if LZO1X_MEM_DECOMPRESS > 0
	char wrkmem[LZO1X_MEM_DECOMPRESS];
#else
	char *wrkmem = nullptr;
#endif

	if(compressed == nullptr)
	    throw SRC_BUG;

	lzo_bh.set_from(*compressed);
	if(lzo_bh.type != BLOCK_HEADER_LZO && lzo_bh.type != BLOCK_HEADER_EOF)
	    throw Erange("compressor::lzo_read_and_uncompress_to_buffer", gettext("data corruption detected: Incoherence in LZO compressed data"));
	if(lzo_bh.type == BLOCK_HEADER_EOF)
	{
	    if(lzo_bh.size != 0)
		throw Erange("compressor::lzo_read_and_uncompress_to_buffer", gettext("compressed data corruption detected"));
	    lzo_read_size = 0;
	    lzo_read_start = 0;
	}
	else
	{
	    lzo_uint read;

	    if(lzo_bh.size > LZO_COMPRESSED_BUFFER_SIZE)
#if !defined(SSIZE_MAX) || SSIZE_MAX > BUFFER_SIZE
		throw Erange("compressor::lzo_read_and_uncompress_to_buffer", gettext("data corruption detected: Too large block of compressed data"));
#else
	    throw Erange("compressor::lzo_read_and_uncompress_to_buffer", gettext("Too large block of compressed data: Either due to data corruption or current system limitation where SSIZE_MAX value implied smaller buffers than required"));
#endif

	    compr_size = 0;
	    lzo_bh.size.unstack(compr_size);
	    if(lzo_bh.size != 0)
		throw SRC_BUG;

	    read = compressed->read(lzo_compressed, compr_size);
	    if(read != compr_size)
		Erange("compressor::lzo_read_and_uncompress_to_buffer", gettext("compressed data corruption detected"));
	    read = LZO_CLEAR_BUFFER_SIZE;
	    status = lzo1x_decompress_safe((lzo_bytep)lzo_compressed, compr_size, (lzo_bytep)lzo_read_buffer, &read, wrkmem);
	    lzo_read_size = read;
	    lzo_read_start = 0;

	    switch(status)
	    {
	    case LZO_E_OK:
		break; // all is fine
	    case LZO_E_INPUT_NOT_CONSUMED:
		throw SRC_BUG;
	    default:
		lzo_read_size = 0;
		throw Erange("compressor::lzo_read_and_uncompress_to_buffer", gettext("compressed data corruption detected"));
	    }
	}
#else
	throw Efeature(gettext("lzo compression"));
#endif
    }

    void compressor::lzo_block_header::dump(generic_file & f)
    {
	f.write(&type, 1);
	size.dump(f);
    }

    void compressor::lzo_block_header::set_from(generic_file & f)
    {
	f.read(&type, 1);
	size.read(f);
    }

} // end of namespace
