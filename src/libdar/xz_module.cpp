/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
#if HAVE_LZMA_H
#include <lzma.h>
#endif

}

#include "xz_module.hpp"
#include "int_tools.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    xz_module::xz_module(U_I compression_level)
    {
#if LIBLZMA_AVAILABLE
	if(compression_level > 9 || compression_level < 1)
	    throw Erange("xz_module::xz_module",
			 tools_printf(gettext("out of range XZ compression level: %d"), compression_level));
	setup(compression_level);
#else
	throw Ecompilation(gettext("xz/lzma compression"));
#endif
    }

    U_I xz_module::get_max_compressing_size() const
    {
#if LIBLZMA_AVAILABLE
	U_I unused = 0;

	return int_tools_maxof_aggregate(unused);
#else
	throw Ecompilation(gettext("xz/lzma compression"));
#endif
    }

    U_I xz_module::get_min_size_to_compress(U_I clear_size) const
    {
#if LIBLZMA_AVAILABLE
	if(clear_size > get_max_compressing_size() || clear_size < 1)
	    throw Erange("xz_module::get_min_size_to_compress",
			 gettext("out of range block size submitted to xz_module::get_min_size_to_compress"));

	return clear_size * 2;
	    // should be large enough, liblzma does not seem
	    // to provide mean to upper bound compressed data
	    // given a clear data size
#else
	throw Ecompilation(gettext("xz/lzma compression"));
#endif
    }


    U_I xz_module::compress_data(const char *normal,
				  const U_I normal_size,
				  char *zip_buf,
				  U_I zip_buf_size) const
    {
#if LIBLZMA_AVAILABLE

	U_I ret;

	init_compr();

	lzma_str.next_out = (uint8_t*)zip_buf;
	lzma_str.avail_out = zip_buf_size;
	lzma_str.next_in = (uint8_t*)normal;
	lzma_str.avail_in = normal_size;

	switch(lzma_code(& lzma_str, LZMA_FINISH))
	{
	case LZMA_OK:
	case LZMA_STREAM_END:
	    break; // normal end
	case LZMA_DATA_ERROR:
	    throw Edata(gettext("corrupted compressed data met"));
	case LZMA_BUF_ERROR:
	    if(lzma_str.next_out == (uint8_t*)(zip_buf + zip_buf_size))
		throw SRC_BUG;
	    else
		throw Edata(gettext("corrupted compressed data met"));
	default:
	    throw SRC_BUG;
	}

	ret = (char*)lzma_str.next_out - zip_buf;

	if(ret == zip_buf_size)
	    throw SRC_BUG; // compressed data does not completely hold in buffer size

	end_process();

	return ret;
#else
	throw Ecompilation(gettext("xz/lzma compression"));
#endif
    }

    U_I xz_module::uncompress_data(const char *zip_buf,
				    const U_I zip_buf_size,
				    char *normal,
				    U_I normal_size) const
    {
#if LIBLZMA_AVAILABLE
	U_I ret;

	init_decompr();

	lzma_str.next_in = (uint8_t*)zip_buf;
	lzma_str.avail_in = zip_buf_size;
	lzma_str.next_out = (uint8_t*)normal;
	lzma_str.avail_out = normal_size;

	switch(lzma_code(& lzma_str, LZMA_FINISH))
	{
	case LZMA_OK:
	case LZMA_STREAM_END:
	    break; // normal end
	case LZMA_DATA_ERROR:
	    throw Edata(gettext("corrupted compressed data met"));
	case LZMA_BUF_ERROR:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}

	ret = (char*)lzma_str.next_out - normal;
	end_process();

	return ret;
#else
	throw Ecompilation(gettext("xz/lzma compression"));
#endif
    }


    unique_ptr<compress_module> xz_module::clone() const
    {
#if LIBLZMA_AVAILABLE
	try
	{
	    return std::make_unique<xz_module>(*this);
	}
	catch(bad_alloc &)
	{
	    throw Ememory("xz_module::clone");
	}
#else
	throw Ecompilation(gettext("xz/lzma compression"));
#endif
    }

    void xz_module::setup(U_I compression_level)
    {
#if LIBLZMA_AVAILABLE
	level = compression_level;
	lzma_str = LZMA_STREAM_INIT;
#else
	throw Ecompilation(gettext("lz4 compression"));
#endif
    }

    void xz_module::init_decompr() const
    {
#if LIBLZMA_AVAILABLE
	switch(lzma_auto_decoder(& lzma_str,
				 UINT64_MAX,
				 0))
	{
	case LZMA_OK:
	    break;
	case LZMA_MEM_ERROR:
	    throw Ememory("xz_module::init_decompr");
	case LZMA_OPTIONS_ERROR:
	    throw Ecompilation("The expected compression preset is not supported by this build of liblzma");
	case LZMA_PROG_ERROR:
	    throw SRC_BUG; // One or more of the parameters have values that will never be valid
	default:
	    throw SRC_BUG;
	}
#else
	throw Ecompilation(gettext("lz4 compression"));
#endif
    }

    void xz_module::init_compr() const
    {
#if LIBLZMA_AVAILABLE
    	switch(lzma_easy_encoder(& lzma_str,
				 level,
				 LZMA_CHECK_CRC32))
	{
	case LZMA_OK:
	    break;
	case LZMA_MEM_ERROR:
	    throw Ememory("xz_module::init_decompr");
	case LZMA_OPTIONS_ERROR:
	    throw Ecompilation("The given compression preset is not supported by this build of liblzma");
	case LZMA_UNSUPPORTED_CHECK:
	    throw Ecompilation("The requested check is not supported by this liblzma build");
	case LZMA_PROG_ERROR:
	    throw SRC_BUG; // One or more of the parameters have values that will never be valid
	default:
	    throw SRC_BUG; // undocumented possible return code
	}
#else
	throw Ecompilation(gettext("lz4 compression"));
#endif
    }

    void xz_module::end_process() const
    {
#if LIBLZMA_AVAILABLE
	lzma_end(& lzma_str);
#else
	throw Ecompilation(gettext("lz4 compression"));
#endif
    }



} // end of namespace
