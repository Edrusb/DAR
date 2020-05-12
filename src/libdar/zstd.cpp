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

} // end extern "C"

#include "zstd.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "null_file.hpp"

using namespace std;

namespace libdar
{

    zstd::zstd(gf_mode x_mode,
	       U_I compression_level,
	       generic_file* x_compressed,
	       U_I workers): mode(x_mode), compressed(x_compressed)
    {
#if LIBZSTD_AVAILABLE
	comp = nullptr;
	decomp = nullptr;
	clear_inbuf();
	clear_outbuf();
	below_tampon = nullptr;
	no_comp_data = false;

	try
	{
	    switch(mode)
	    {
	    case gf_read_only:
		decomp = ZSTD_createDStream();
		if(decomp == nullptr)
		    throw Ememory("zstd::zstd");
		below_tampon_size = ZSTD_DStreamInSize();
		above_tampon_size = ZSTD_DStreamOutSize();
		flueof = false;
		break;
	    case gf_write_only:
	    case gf_read_write: // but read operation will fail
		comp = ZSTD_createCStream();
		if(comp == nullptr)
		    throw Ememory("zsts::zstd");
		below_tampon_size = ZSTD_CStreamOutSize();
		above_tampon_size = ZSTD_CStreamInSize();
		flueof = true;
		break;
	    default:
		throw SRC_BUG;
	    }
	    setup_context(mode, compression_level, workers);

	    below_tampon = new (nothrow) char[below_tampon_size];
	    if(below_tampon == nullptr)
		throw Ememory("zstd::zstd");
	}
	catch(...)
	{
	    release_mem();
	    throw;
	}
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    zstd::~zstd()
    {
	release_mem();
    }

    U_I zstd::read(char *a, U_I size)
    {
#if LIBZSTD_AVAILABLE
	U_I err = 0;
	U_I wrote = 0;

	switch(mode)
	{
	case gf_read_only:
	    break;
	case gf_read_write:
	    throw Efeature("read-write mode for zstd class");
	case gf_write_only:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}

	if(decomp == nullptr)
	    throw SRC_BUG;
	if(below_tampon == nullptr)
	    throw SRC_BUG;

	if(inbuf.src == nullptr)
	{
	    inbuf.src = below_tampon;
	    inbuf.size = 0;
	    inbuf.pos = 0;
	}

	while(!flueof && wrote < size)
	{
	    U_I delta_in = below_tampon_size - inbuf.size;

	    if(delta_in > 0 && !no_comp_data)
	    {
	 	U_I lu = compressed->read((char *)inbuf.src + inbuf.size, delta_in);
		if(lu < delta_in)
		    no_comp_data = true;
		inbuf.size += lu;
	    }

	    outbuf.dst = a + wrote;
	    outbuf.size = size - wrote < above_tampon_size? size - wrote : above_tampon_size;
	    outbuf.pos = 0;

	    err = ZSTD_decompressStream(decomp, &outbuf, &inbuf);
	    if(ZSTD_isError(err))
		throw Erange("zstd::read", tools_printf(gettext("Error returned by libzstd while uncompressing data: %s"), ZSTD_getErrorName(err)));

	    if(err == 0)
		flueof = true;

	    if(inbuf.pos > 0)
	    {
		    // some input data has been consumed

		if(inbuf.pos < inbuf.size)
		{
			// only a part of the data has been consumed

		    (void)memmove(below_tampon, below_tampon + inbuf.pos, inbuf.size - inbuf.pos);
		    inbuf.size -= inbuf.pos;
		    inbuf.pos = 0;
		}
		else
		{
			// all data has been consumed

		    inbuf.pos = 0;
		    inbuf.size = 0;
		}
	    }

	    wrote += outbuf.pos;

	    if(no_comp_data && outbuf.pos == 0 && wrote < size && !flueof)
		throw Erange("zstd::read", gettext("uncompleted compressed stream, some compressed data is missing or corruption occured"));
	}

	return wrote;
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    void zstd::write(const char *a, U_I size)
    {
#if LIBZSTD_AVAILABLE
	U_I err;
	U_I lu = 0;
	U_I next_bs = above_tampon_size;

	if(comp == nullptr)
	    throw SRC_BUG;
	if(below_tampon == nullptr)
	    throw SRC_BUG;

	    // we need that to be able to flush_write later on
	flueof = false;

	outbuf.dst = below_tampon;
	outbuf.size = below_tampon_size;

	while(lu < size)
	{
	    inbuf.src = (const void *)(a + lu);
	    inbuf.size = size-lu < next_bs ? size-lu : next_bs;
	    inbuf.pos = 0;

	    outbuf.pos = 0;

	    err = ZSTD_compressStream(comp, &outbuf, &inbuf);
	    if(ZSTD_isError(err))
		throw Erange("zstd::write", tools_printf(gettext("Error met while sending data for compression to libzstd: %s"), ZSTD_getErrorName(err)));

	    next_bs = err;

	    if(outbuf.pos > 0)
		compressed->write((char *)outbuf.dst, outbuf.pos); // generic::write never does partial writes
	    lu += inbuf.pos;
	}

#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    void zstd::compr_flush_write()
    {
#if LIBZSTD_AVAILABLE
	U_I err;

	if(flueof || mode == gf_read_only)
	    return;

	outbuf.dst = below_tampon;
	outbuf.size = below_tampon_size;
	outbuf.pos = 0;

	err = ZSTD_endStream(comp, &outbuf);
	if(ZSTD_isError(err))
	    throw Erange("zstd::compr_flush_write", tools_printf(gettext("Error met while asking libzstd for compression end: %s"), ZSTD_getErrorName(err)));

	compressed->write((char *)outbuf.dst, outbuf.pos);

	while(err > 0)
	{
	    outbuf.pos = 0;

	    err = ZSTD_flushStream(comp, &outbuf);
	    if(ZSTD_isError(err))
		throw Erange("zstd::compr_flush_write", tools_printf(gettext("Error met while asking libzstd to flush data once compression end has been asked: %s"), ZSTD_getErrorName(err)));

	    compressed->write((char *)outbuf.dst, outbuf.pos);
	}
	flueof = true;
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    void zstd::compr_flush_read()
    {
#if LIBZSTD_AVAILABLE
	if(mode != gf_read_only)
	    return;
	flueof = false;
	no_comp_data = false;
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    void zstd::clean_read()
    {
#if LIBZSTD_AVAILABLE
	if(mode != gf_read_only)
	    return;
	flueof = false;
	no_comp_data = false;
	clear_inbuf();
	clear_outbuf();
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    void zstd::clean_write()
    {
#if LIBZSTD_AVAILABLE
	if(mode == gf_read_only)
	    return;

	if(!flueof)
	{
	    null_file null(gf_write_only);
	    generic_file *original_compressed = compressed;

	    compressed = &null;
	    try
	    {
		compr_flush_write();
		    // flushing to reset the libzstd engine
		    // but sending compressed data to /dev/null
	    }
	    catch(...)
	    {
		compressed = original_compressed;
		throw;
	    }
	}
	clear_inbuf();
	clear_outbuf();
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    void zstd::clear_inbuf()
    {
#if LIBZSTD_AVAILABLE
	inbuf.src = nullptr;
	inbuf.size = 0;
	inbuf.pos = 0;
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    void zstd::clear_outbuf()
    {
#if LIBZSTD_AVAILABLE
	outbuf.dst = nullptr;
	outbuf.size = 0;
	outbuf.pos = 0;
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    void zstd::release_mem()
    {
#if LIBZSTD_AVAILABLE
	if(decomp != nullptr)
	    ZSTD_freeDStream(decomp);
	if(comp != nullptr)
	    ZSTD_freeCStream(comp);
	if(below_tampon != nullptr)
	    delete [] below_tampon;
#endif
    }

    void zstd::setup_context(gf_mode mode, U_I compression_level, U_I workers)
    {
#if LIBZSTD_AVAILABLE
	int err;
	static const U_I maxcomp = ZSTD_maxCLevel();

	switch(mode)
	{
	case gf_read_only:
	    if(decomp == nullptr)
		throw SRC_BUG;

	    err = ZSTD_initDStream(decomp);
	    if(ZSTD_isError(err))
		throw Erange("zstd::setup_context", tools_printf(gettext("Error while initializing libzstd for decompression: %s"),
								 ZSTD_getErrorName(err)));
	    break;
	case gf_write_only:
	case gf_read_write:
	    if(comp == nullptr)
		throw SRC_BUG;

		// setting ZSTD_c_compressionLevel parameter

	    if(compression_level > maxcomp)
		throw Erange("zstd::setup_context", tools_printf(gettext("the requested compression level (%d) is higher than the maximum available for libzstd: %d"),
								 compression_level,
								 maxcomp));


	    err = ZSTD_initCStream(comp, compression_level);
	    if(ZSTD_isError(err))
		throw Erange("zstd::setup_context", tools_printf(gettext("Error while setting libzstd compression level to %d: %s"),
								 compression_level,
								 ZSTD_getErrorName(err)));

	    break;
	default:
	    throw SRC_BUG;
	}
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

} // end of namespace
