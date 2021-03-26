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
#if HAVE_LZO_LZO1X_H
#include <lzo/lzo1x.h>
#endif
}

#include "lzo_module.hpp"
#include "int_tools.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    U_I lzo_module::get_max_compressing_size() const
    {
#if LIBLZO2_AVAILABLE
	return 256*1024l*1024l; // from MAX_BLOCK_SIZE in p.lzo.c (lzop source code)
#else
	throw Ecompilation(gettext("lzo compression"));
#endif
    }

    U_I lzo_module::get_min_size_to_compress(U_I clear_size) const
    {
#if LIBLZO2_AVAILABLE
	return clear_size + clear_size / 16 + 64 + 3;
	    // from MAX_COMPRESSED_SIZE(x) macro in p_lzo.c from lzop source code
#else
	throw Ecompilation(gettext("lzo compression"));
#endif
    }


    U_I lzo_module::compress_data(const char *normal,
				  const U_I normal_size,
				  char *zip_buf,
				  U_I zip_buf_size) const
    {
#if LIBLZO2_AVAILABLE
	S_I status;
	lzo_uint zip_buf_size_lzo = zip_buf_size;

	switch(lzo_algo)
	{
	case compression::lzo:
	    status = lzo1x_999_compress_level((lzo_bytep)normal, normal_size, (lzo_bytep)zip_buf, &zip_buf_size_lzo, wrkmem_compr.get(), nullptr, 0, nullptr, level);
	    break;
	case compression::lzo1x_1_15:
	    status = lzo1x_1_15_compress((lzo_bytep)normal, normal_size, (lzo_bytep)zip_buf, &zip_buf_size_lzo, wrkmem_compr.get());
	    break;
	case compression::lzo1x_1:
	    status = lzo1x_1_compress((lzo_bytep)normal, normal_size, (lzo_bytep)zip_buf, &zip_buf_size_lzo, wrkmem_compr.get());
	    break;
	default:
	    throw SRC_BUG;
	}

	zip_buf_size = zip_buf_size_lzo;
	if((lzo_uint)(zip_buf_size) != zip_buf_size_lzo)
	    throw SRC_BUG;

	switch(status)
	{
	case LZO_E_OK:
	    break; // all is fine
	case LZO_E_ERROR:
	    throw Erange("lzo_module::compress_data", "invalid compresion level or argument provided");
	default:
	    throw Erange("lzo_module::compress_data", tools_printf(gettext("Probable bug in liblzo2: lzo1x_*_compress returned unexpected/undocumented code %d"), status));
	}

	return zip_buf_size;
#else
	throw Ecompilation(gettext("lzo compression"));
#endif
    }

    U_I lzo_module::uncompress_data(const char *zip_buf,
				    const U_I zip_buf_size,
				    char *normal,
				    U_I normal_size) const
    {
#if LIBLZO2_AVAILABLE
	S_I status;
	lzo_uint normal_size_lzo = normal_size;

	status = lzo1x_decompress_safe((lzo_bytep)zip_buf, zip_buf_size, (lzo_bytep)normal, & normal_size_lzo, wrkmem_decompr.get());

	normal_size = normal_size_lzo;
	if((lzo_uint)(normal_size) != normal_size_lzo)
	    throw SRC_BUG; // integer overflow occured

	switch(status)
	{
	case LZO_E_OK:
	    break; // all is fine
	case LZO_E_INPUT_NOT_CONSUMED:
	case LZO_E_INPUT_OVERRUN:
	case LZO_E_LOOKBEHIND_OVERRUN:
	    throw Edata(gettext("corrupted compressed data met"));
	default:
	    throw Edata(gettext("Corrupted compressed data met"));
	}

	return normal_size;
#else
	throw Ecompilation(gettext("lzo compression"));
#endif
    }


    unique_ptr<compress_module> lzo_module::clone() const
    {
#if LIBLZO2_AVAILABLE
	try
	{
	    return std::make_unique<lzo_module>(*this);
	}
	catch(bad_alloc &)
	{
	    throw Ememory("lzo_module::clone");
	}
#else
	throw Ecompilation(gettext("lzo compression"));
#endif
    }

    void lzo_module::init(compression algo, U_I compression_level)
    {
#if LIBLZO2_AVAILABLE
	if(compression_level > 9 || compression_level < 1)
	    throw Erange("lzo_module::lzo_module", tools_printf(gettext("out of range LZO compression level: %d"), compression_level));
	level = compression_level;

	if(algo != compression::lzo
	   && algo != compression::lzo1x_1_15
	   && algo != compression::lzo1x_1)
	    throw Erange("lzo_module::lzo_module", "invalid lzo compression algoritm provided");
	lzo_algo = algo;

	try
	{
	    if(LZO1X_MEM_DECOMPRESS > 0)
		wrkmem_decompr = make_unique<char[]>(LZO1X_MEM_DECOMPRESS);
	    else
		wrkmem_decompr.reset(); // points to nullptr

	    switch(lzo_algo)
	    {
	    case compression::lzo:
		wrkmem_compr = make_unique<char[]>(LZO1X_999_MEM_COMPRESS);
		break;
	    case compression::lzo1x_1_15:
		wrkmem_compr = make_unique<char[]>(LZO1X_1_15_MEM_COMPRESS);
		break;
	    case compression::lzo1x_1:
		wrkmem_compr = make_unique<char[]>(LZO1X_1_MEM_COMPRESS);
		break;
	    default:
		throw SRC_BUG;
	    }

	}
	catch(bad_alloc &)
	{
	    throw Ememory("lzo_module::lzo_module");
	}
#else
	throw Ecompilation(gettext("lzo compression"));
#endif
    }

} // end of namespace
