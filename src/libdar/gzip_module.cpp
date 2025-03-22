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
#if HAVE_ZLIB_H
#include <zlib.h>
#endif
}

#include "gzip_module.hpp"
#include "int_tools.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    gzip_module::gzip_module(U_I compression_level)
    {
#if LIBZ_AVAILABLE
	if(compression_level > 9 || compression_level < 1)
	    throw Erange("gzip_module::gzip_module", tools_printf(gettext("out of range GZIP compression level: %d"), compression_level));
	level = compression_level;
#else
	throw Ecompilation(gettext("gzip compression"));
#endif
    }

    U_I gzip_module::get_max_compressing_size() const
    {
#if LIBZ_AVAILABLE
	U_I unused = 0;

	return int_tools_maxof_aggregate(unused);
#else
	throw Ecompilation(gettext("gzip compression"));
#endif
    }

    U_I gzip_module::get_min_size_to_compress(U_I clear_size) const
    {
#if LIBZ_AVAILABLE
	if(clear_size > get_max_compressing_size() || clear_size < 1)
	    throw Erange("gzip_module::get_min_size_to_compress", "out of range block size submitted to gzip_module::get_min_size_to_compress");

	return compressBound(clear_size);
#else
	throw Ecompilation(gettext("gzip compression"));
#endif
    }


    U_I gzip_module::compress_data(const char *normal,
				  const U_I normal_size,
				  char *zip_buf,
				  U_I zip_buf_size) const

    {
#if LIBZ_AVAILABLE
	S_I ret;
	uLong zip_buf_size_ulong = zip_buf_size;

	if(normal_size > get_max_compressing_size())
	    throw Erange("gzip_module::compress_data", "oversized uncompressed data given to GZIP compression engine");

	ret = compress2((Bytef*)zip_buf,
			&zip_buf_size_ulong,
			(const Bytef*)normal,
			normal_size,
			level);

	zip_buf_size = (U_I)(zip_buf_size_ulong);
	if((uLong)zip_buf_size != zip_buf_size_ulong)
	    throw SRC_BUG; // integer overflow occured

	switch(ret)
	{
	case Z_OK:
	    break;
	case Z_MEM_ERROR:
	    throw Erange("gzip_module::compress_data", "lack of memory to perform the gzip compression operation");
	case Z_BUF_ERROR:
	    throw Erange("gzip_module::compress_data", "too small buffer provided to receive compressed data");
	case Z_STREAM_ERROR:
	    throw Erange("gzip_module::compress_data", gettext("invalid compression level provided to the gzip compression engine"));
	default:
	    throw SRC_BUG;
	}

	return zip_buf_size; // compress2 modified zip_buf_size to give the resulting amount of compressed data
#else
	throw Ecompilation(gettext("gzip compression"));
#endif
    }

    U_I gzip_module::uncompress_data(const char *zip_buf,
				    const U_I zip_buf_size,
				    char *normal,
				    U_I normal_size) const
    {
#if LIBZ_AVAILABLE
	uLongf normal_size_ulong = normal_size;

	S_I ret = uncompress((Bytef*)normal, &normal_size_ulong, (const Bytef*)zip_buf, zip_buf_size);

	normal_size = normal_size_ulong;
	if((uLongf)(normal_size) != normal_size_ulong)
	    throw SRC_BUG; // integer overflow occured

	switch(ret)
	{
	case Z_OK:
	    break;
	case Z_MEM_ERROR:
	    throw Erange("gzip_module::uncompress_data", "lack of memory to perform the gzip decompression operation");
	case Z_BUF_ERROR:
	    throw Erange("gzip_module::uncompress_data", "too small buffer provided to receive decompressed data");
	case Z_DATA_ERROR:
	    throw Edata(gettext("corrupted compressed data met"));
	default:
	    throw SRC_BUG;
	}

	return normal_size;
#else
	throw Ecompilation(gettext("gzip compression"));
#endif
    }


    unique_ptr<compress_module> gzip_module::clone() const
    {
#if LIBZ_AVAILABLE
	try
	{
	    return std::make_unique<gzip_module>(*this);
	}
	catch(bad_alloc &)
	{
	    throw Ememory("gzip_module::clone");
	}
#else
	throw Ecompilation(gettext("gzip compression"));
#endif
    }


} // end of namespace
