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
#if HAVE_ZSTD_H
#include <zstd.h>
#endif
}

#include "zstd_module.hpp"
#include "int_tools.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    zstd_module::zstd_module(U_I compression_level)
    {
#if LIBZSTD_AVAILABLE
	if(compression_level > (U_I)ZSTD_maxCLevel() || compression_level < 1)
	    throw Erange("zstd_module::zstd_module",
			 tools_printf(gettext("out of range ZSTD compression level: %d"), compression_level));
	level = compression_level;
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    U_I zstd_module::get_max_compressing_size() const
    {
#if LIBZSTD_AVAILABLE
	U_I unused = 0;

	return int_tools_maxof_aggregate(unused);
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    U_I zstd_module::get_min_size_to_compress(U_I clear_size) const
    {
#if LIBZSTD_AVAILABLE
	if(clear_size > get_max_compressing_size() || clear_size < 1)
	    throw Erange("zstd_module::get_min_size_to_compress",
			 gettext("out of range block size submitted to zstd_module::get_min_size_to_compress"));

	return ZSTD_compressBound(clear_size);
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }


    U_I zstd_module::compress_data(const char *normal,
				  const U_I normal_size,
				  char *zip_buf,
				  U_I zip_buf_size) const
    {
#if LIBZSTD_AVAILABLE
	size_t ret;

	if(normal_size > get_max_compressing_size())
	    throw Erange("zstd_module::compress_data",
			 "oversized uncompressed data given to ZSTD compression engine");

	ret = ZSTD_compress(zip_buf, zip_buf_size,
			    normal, normal_size,
			    level);

	if(ZSTD_isError(ret))
	    throw Erange("zstd_module::uncompress_data",
			 tools_printf(gettext("libzstd returned an error while performing block compression: %s"),
				      ZSTD_getErrorName(ret)));

	return (U_I)ret;
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    U_I zstd_module::uncompress_data(const char *zip_buf,
				    const U_I zip_buf_size,
				    char *normal,
				    U_I normal_size) const
    {
#if LIBZSTD_AVAILABLE
	size_t ret = ZSTD_decompress(normal, normal_size,
				     zip_buf, zip_buf_size);

	if(ZSTD_isError(ret))
	    throw Erange("zstd_module::uncompress_data",
			 tools_printf(gettext("libzstd returned an error while performing block decompression: %s"),
				      ZSTD_getErrorName(ret)));

	return (U_I)ret;

#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }


    unique_ptr<compress_module> zstd_module::clone() const
    {
#if LIBZSTD_AVAILABLE
	try
	{
	    return std::make_unique<zstd_module>(*this);
	}
	catch(bad_alloc &)
	{
	    throw Ememory("zstd_module::clone");
	}
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }


} // end of namespace
