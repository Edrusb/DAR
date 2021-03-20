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
#if HAVE_BZLIB_H
#include <bzlib.h>
#endif
}

#include "bzip2_module.hpp"
#include "int_tools.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    bzip2_module::bzip2_module(U_I compression_level)
    {
#if LIBBZ2_AVAILABLE
	if(compression_level > 9 || compression_level < 1)
	    throw Erange("bzip2_module::bzip2_module", tools_printf(gettext("out of range BZIP2 compression level: %d"), compression_level));
	level = compression_level;
#else
	throw Ecompilation(gettext("bzip2 compression"));
#endif
    }

    U_I bzip2_module::get_max_compressing_size() const
    {
#if LIBBZ2_AVAILABLE
	U_I unused = 0;

	return int_tools_maxof_aggregate(unused);
#else
	throw Ecompilation(gettext("bzip2 compression"));
#endif
    }

    U_I bzip2_module::get_min_size_to_compress(U_I clear_size) const
    {
#if LIBBZ2_AVAILABLE
	if(clear_size > get_max_compressing_size() || clear_size < 1)
	    throw Erange("bzip2_module::get_min_size_to_compress", "out of range block size submitted to bzip2_module::get_min_size_to_compress");

	    // extract for libzip2 manual:
	    // * To guarantee that the compressed data will fit in its buffer,
	    // * allocate an output buffer of size 1% larger than the uncompressed data,
	    // * plus six hundred extra bytes.

	return clear_size + (clear_size+100)/100 + 600;

	    // "(clear_size+100)/100" stands for "clear_size/100" (thus 1% of clear_size)
	    // but rounded to the upper percent value
#else
	throw Ecompilation(gettext("bzip2 compression"));
#endif
    }


    U_I bzip2_module::compress_data(const char *normal,
				  const U_I normal_size,
				  char *zip_buf,
				  U_I zip_buf_size) const
    {
#if LIBBZ2_AVAILABLE
	S_I ret;
	unsigned int tmp_zip_buf_size = zip_buf_size;
	    // needed for long/non-long int conversion

	if(normal_size > get_max_compressing_size())
	    throw Erange("bzip2_module::compress_data", "oversized uncompressed data given to BZIP2 compression engine");

	ret = BZ2_bzBuffToBuffCompress(zip_buf,
				       & tmp_zip_buf_size,
				       const_cast<char *>(normal),
				       normal_size,
				       level,
				       0,
				       30); // 30 is the default "workFactor"

	switch(ret)
	{
	case BZ_OK:
	    break;
	case BZ_CONFIG_ERROR:
	    throw Erange("bzip2_module::uncompress_data", "libbzip2 error: \"the library has been mis-compiled\"");
	case BZ_PARAM_ERROR:
	    throw SRC_BUG; // zip, or zip_size is NULL or other parameter is out of range
	case BZ_MEM_ERROR:
	    throw Erange("bzip2_module::uncompress_data", "lack of memory to perform the bzip2 compression operation");
	case BZ_OUTBUFF_FULL:
	    throw Erange("bzip2_module::uncompress_data", "too small buffer provided to receive compressed data");
	default:
	    throw SRC_BUG;
	}

	return tmp_zip_buf_size;
#else
	throw Ecompilation(gettext("bzip2 compression"));
#endif
    }

    U_I bzip2_module::uncompress_data(const char *zip_buf,
				    const U_I zip_buf_size,
				    char *normal,
				    U_I normal_size) const
    {
#if LIBBZ2_AVAILABLE
	unsigned int tmp_normal_size = normal_size;
	    // needed for long/non-long int conversion

	S_I ret = BZ2_bzBuffToBuffDecompress(normal,
					     &tmp_normal_size,
					     const_cast<char *>(zip_buf),
					     zip_buf_size,
					     0,  // small
					     0); // verbosity

	switch(ret)
	{
	case BZ_OK:
	    break;
	case BZ_CONFIG_ERROR:
	    throw Erange("bzip2_module::uncompress_data", "libbzip2 error: \"the library has been mis-compiled\"");
	case BZ_PARAM_ERROR:
	    throw SRC_BUG; // normal, or normal_size is NULL or other parameter is out of range
	case BZ_MEM_ERROR:
	    throw Erange("bzip2_module::uncompress_data", "lack of memory to perform the bzip2 decompression operation");
	case BZ_OUTBUFF_FULL:
	    throw Erange("bzip2_module::uncompress_data", "too small buffer provided to receive decompressed data");
	case BZ_DATA_ERROR:
	case BZ_DATA_ERROR_MAGIC:
	case BZ_UNEXPECTED_EOF:
	    throw Edata(gettext("corrupted compressed data met"));
	default:
	    throw SRC_BUG;
	}

	return tmp_normal_size;
#else
	throw Ecompilation(gettext("bzip2 compression"));
#endif
    }


    unique_ptr<compress_module> bzip2_module::clone() const
    {
#if LIBBZ2_AVAILABLE
	try
	{
	    return std::make_unique<bzip2_module>(*this);
	}
	catch(bad_alloc &)
	{
	    throw Ememory("bzip2_module::clone");
	}
#else
	throw Ecompilation(gettext("bzip2 compression"));
#endif
    }


} // end of namespace
