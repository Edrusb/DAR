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
#if LIBLZO2_AVAILABLE
#if HAVE_LZO_LZO1X_H
#include <lzo/lzo1x.h>
#endif
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
#include "compressor_lzo.hpp"

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

    compressor_lzo::compressor_lzo(compression algo, generic_file & compressed_side, U_I compression_level) : proto_compressor(compressed_side.get_mode())
    {
        current_algo = algo;
	suspended = false;
	current_level = compression_level;

        if(compression_level > 9)
            throw SRC_BUG;

	lzo_read_buffer = lzo_write_buffer = nullptr;
	lzo_compressed = nullptr;
	lzo_wrkmem = nullptr;

        switch(algo)
        {
        case compression::none:
            throw SRC_BUG;
        case compression::bzip2:
	case compression::xz:
        case compression::gzip:
	    throw SRC_BUG;
	case compression::lzo:
	case compression::lzo1x_1_15:
	case compression::lzo1x_1:
#if LIBLZO2_AVAILABLE
	    reset_compr_engine();
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
		    throw Ememory("compressor_lzo::init");
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
	case compression::zstd:
	    throw SRC_BUG;
		// zstd is now handled by class compressor_lzo_zstd
        default :
            throw SRC_BUG;
        }

        compressed = & compressed_side;

    }

    compressor_lzo::~compressor_lzo()
    {
	try
	{
	    terminate();
	}
	catch(...)
	{
		// ignore all exceptions
	}
	if(lzo_read_buffer != nullptr)
	    delete [] lzo_read_buffer;
	if(lzo_write_buffer != nullptr)
	    delete [] lzo_write_buffer;
	if(lzo_compressed != nullptr)
	    delete [] lzo_compressed;
	if(lzo_wrkmem != nullptr)
	    delete [] lzo_wrkmem;
    }

    compression compressor_lzo::get_algo() const
    {
	if(suspended)
	    return compression::none;
	else
	    return (current_algo == compression::lzo1x_1_15 || current_algo == compression::lzo1x_1) ? compression::lzo : current_algo;
    }

    void compressor_lzo::suspend_compression()
    {
	if(!suspended)
	{
	    inherited_sync_write();
	    reset_compr_engine();
	    suspended = true;
	}
    }

    void compressor_lzo::resume_compression()
    {
	if(suspended)
	    suspended = false;
    }

    void compressor_lzo::inherited_truncate(const infinint & pos)
    {
	if(pos < get_position())
	{
	    inherited_sync_write();
	    inherited_flush_read();
	}
	compressed->truncate(pos);
    }

    void compressor_lzo::inherited_terminate()
    {
	if(lzo_read_buffer != nullptr)
	{
	    inherited_flush_read();
	    delete [] lzo_read_buffer;
	    lzo_read_buffer = nullptr;
	}

	if(lzo_write_buffer != nullptr)
	{
	    inherited_sync_write();
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


    U_I compressor_lzo::inherited_read(char *a, U_I size)
    {
#if LIBLZO2_AVAILABLE
	U_I read = 0;

	if(suspended)
	    return compressed->read(a, size);

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
	throw Ecompilation(gettext("lzo compression"));
#endif
    }

    void compressor_lzo::inherited_write(const char *a, U_I size)
    {
#if LIBLZO2_AVAILABLE
	if(suspended)
	    compressed->write(a, size);
	else
	{
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
	}
#else
	throw Ecompilation(gettext("lzo compression"));
#endif
    }

    void compressor_lzo::inherited_sync_write()
    {
	if(is_terminated())
	    throw SRC_BUG;

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

    void compressor_lzo::inherited_flush_read()
    {
	if(is_terminated())
	    throw SRC_BUG;

	lzo_read_reached_eof = false;

	if(lzo_read_buffer != nullptr) // lzo
	{
	    lzo_read_start = 0;
	    lzo_read_size = 0;
	}

    }

    void compressor_lzo::clean_write()
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(lzo_write_buffer != nullptr) // lzo
	    lzo_write_size = 0;

    }


    void compressor_lzo::lzo_compress_buffer_and_write()
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
	    throw Erange("compressor_lzo::lzo_compress_buffer_and_write", tools_printf(gettext("Probable bug in liblzo2: lzo1x_*_compress returned unexpected code %d"), status));
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
	throw Ecompilation(gettext("lzo compression"));
#endif
    }

    void compressor_lzo::lzo_read_and_uncompress_to_buffer()
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
	    throw Erange("compressor_lzo::lzo_read_and_uncompress_to_buffer", gettext("data corruption detected: Incoherence in LZO compressed data"));
	if(lzo_bh.type == BLOCK_HEADER_EOF)
	{
	    if(lzo_bh.size != 0)
		throw Erange("compressor_lzo::lzo_read_and_uncompress_to_buffer", gettext("compressed data corruption detected"));
	    lzo_read_size = 0;
	    lzo_read_start = 0;
	}
	else
	{
	    lzo_uint read;

	    if(lzo_bh.size > LZO_COMPRESSED_BUFFER_SIZE)
#if !defined(SSIZE_MAX) || SSIZE_MAX > BUFFER_SIZE
		throw Erange("compressor_lzo::lzo_read_and_uncompress_to_buffer", gettext("data corruption detected: Too large block of compressed data"));
#else
	    throw Erange("compressor_lzo::lzo_read_and_uncompress_to_buffer", gettext("Too large block of compressed data: Either due to data corruption or current system limitation where SSIZE_MAX value implied smaller buffers than required"));
#endif

	    compr_size = 0;
	    lzo_bh.size.unstack(compr_size);
	    if(lzo_bh.size != 0)
		throw SRC_BUG;

	    read = compressed->read(lzo_compressed, compr_size);
	    if(read != compr_size)
		Erange("compressor_lzo::lzo_read_and_uncompress_to_buffer", gettext("compressed data corruption detected"));
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
		throw Erange("compressor_lzo::lzo_read_and_uncompress_to_buffer", gettext("compressed data corruption detected"));
	    }
	}
#else
	throw Ecompilation(gettext("lzo compression"));
#endif
    }

    void compressor_lzo::reset_compr_engine()
    {
	    // read
	lzo_read_start = 0;
	lzo_read_size = 0;
	lzo_read_reached_eof = false;
	    // write
	lzo_write_size = 0;
	lzo_write_flushed = true;
    }

    void compressor_lzo::lzo_block_header::dump(generic_file & f)
    {
	f.write(&type, 1);
	size.dump(f);
    }

    void compressor_lzo::lzo_block_header::set_from(generic_file & f)
    {
	f.read(&type, 1);
	size.read(f);
    }

} // end of namespace
