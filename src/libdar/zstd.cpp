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
	eof = false;

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
		break;
	    case gf_write_only:
	    case gf_read_write: // but read operation will fail
		comp = ZSTD_createCStream();
		if(comp == nullptr)
		    throw Ememory("zsts::zstd");
		below_tampon_size = ZSTD_CStreamOutSize();
		above_tampon_size = ZSTD_CStreamInSize();
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
	bool no_comp_data = false;

	if(mode == gf_read_write)
	    throw Efeature("read-write mode for zstd class");

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

	while(!eof && wrote < size)
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
		eof = true;

	    if(inbuf.pos < inbuf.size)
	    {
		(void)memmove(below_tampon, below_tampon + inbuf.pos, inbuf.size - inbuf.pos);
		inbuf.size -= inbuf.pos;
		inbuf.pos = 0;
	    }
	    else
	    {
		inbuf.pos = 0;
		inbuf.size = 0;
	    }

	    wrote += outbuf.pos;

	    if(no_comp_data && outbuf.pos == 0 && wrote < size && !eof)
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

    void zstd::write_eof_and_flush()
    {
#if LIBZSTD_AVAILABLE
	U_I err;

	if(eof || mode == gf_read_only)
	    return;

	outbuf.dst = below_tampon;
	outbuf.size = below_tampon_size;
	outbuf.pos = 0;

	err = ZSTD_endStream(comp, &outbuf);
	if(ZSTD_isError(err))
	    throw Erange("zstd::write_eof_and_flush", tools_printf(gettext("Error met while asking libzstd for compression end: %s"), ZSTD_getErrorName(err)));

	compressed->write((char *)outbuf.dst, outbuf.pos);

	while(err > 0)
	{
	    outbuf.pos = 0;

	    err = ZSTD_flushStream(comp, &outbuf);
	    if(ZSTD_isError(err))
		throw Erange("zstd::write_eof_and_flush", tools_printf(gettext("Error met while asking libzstd to flush data once compression end has been asked: %s"), ZSTD_getErrorName(err)));

	    compressed->write((char *)outbuf.dst, outbuf.pos);
	}
	eof = true;
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    void zstd::reset()
    {
#if LIBZSTD_AVAILABLE
	int err;
	eof = false;

	switch(mode)
	{
	case gf_read_only:
	    if(decomp == nullptr)
		throw SRC_BUG;

	    err = ZSTD_DCtx_reset(decomp, ZSTD_reset_session_only);
	    if(ZSTD_isError(err))
		throw Erange("zstd::reset", tools_printf(gettext("Error met while resetting libzstd decompression engine: %s"), ZSTD_getErrorName(err)));

	    break;
	case gf_write_only:
	case gf_read_write:
	    if(comp == nullptr)
		throw SRC_BUG;

	    err = ZSTD_CCtx_reset(comp, ZSTD_reset_session_only);
	    if(ZSTD_isError(err))
		throw Erange("zstd::reset", tools_printf(gettext("Error met while resetting libzstd compression engine: %s"), ZSTD_getErrorName(err)));

	    break;
	default:
	    throw SRC_BUG;
	}
#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    void zstd::clear_inbuf()
    {
	inbuf.src = nullptr;
	inbuf.size = 0;
	inbuf.pos = 0;
    }

    void zstd::clear_outbuf()
    {
	outbuf.dst = nullptr;
	outbuf.size = 0;
	outbuf.pos = 0;
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
	ZSTD_bounds limites;
	int err;

	switch(mode)
	{
	case gf_read_only:
	    if(decomp == nullptr)
		throw SRC_BUG;

		// resetting the decompression context to default parameter

	    err = ZSTD_DCtx_reset(decomp, ZSTD_reset_parameters);
	    if(ZSTD_isError(err))
		throw Erange("zstd::setup_context", tools_printf(gettext("Error met while resetting parameters of libzstd decompression context: %s"), ZSTD_getErrorName(err)));
	    break;

	case gf_write_only:
	case gf_read_write:
	    if(comp == nullptr)
		throw SRC_BUG;

		// resetting the compression context to default parameter

	    err = ZSTD_CCtx_reset(comp, ZSTD_reset_parameters);
	    if(ZSTD_isError(err))
		throw Erange("zstd::setup_context", tools_printf(gettext("Error met while resetting parameters of libzstd compression context: %s"), ZSTD_getErrorName(err)));

		// setting ZSTD_c_compressionLevel parameter

	    limites = ZSTD_cParam_getBounds(ZSTD_c_compressionLevel);

	    if(ZSTD_isError(limites.error))
		throw Erange("zstd::setup_context", tools_printf(gettext("Error returned from libzstd when asking for compression level available range: %s"), ZSTD_getErrorName(limites.error)));

	    if(!check_range(compression_level, limites))
	       throw Erange("zstd::setup_context", tools_printf(gettext("The requested compression level (%d) is out of range ([%d - %d])"), compression_level, limites.lowerBound, limites.upperBound));

	    err = ZSTD_CCtx_setParameter(comp, ZSTD_c_compressionLevel, compression_level);
	    if(ZSTD_isError(err))
		throw Erange("zstd::setup_context", tools_printf(gettext("Error while setting libzstd compression level to %d (reported valid range is %d - %d): %s"),
								compression_level,
								limites.lowerBound,
								limites.upperBound,
								ZSTD_getErrorName(err)));

		// setting ZSTD_c_nbWorkers parameter

	    if(workers > 1)
	    {
		limites = ZSTD_cParam_getBounds(ZSTD_c_nbWorkers);

		if(!ZSTD_isError(limites.error))
		{
		    if(!check_range(workers, limites))
			throw Erange("zstd::setup_context", tools_printf(gettext("The requested number of libzstd workers (%d) is out of range ([%d - %d])"), workers, limites.lowerBound, limites.upperBound));
		}

		err = ZSTD_CCtx_setParameter(comp, ZSTD_c_nbWorkers, workers);
		if(ZSTD_isError(err))
		    throw Erange("zstd::setup_context", tools_printf(gettext("Error while setting the libzstd number of worker to %d: %s"),
								     workers,
								     ZSTD_getErrorName(err)));
	    }

	    break;
	default:
	    throw SRC_BUG;
	}
    }

    bool zstd::check_range(S_I value, const ZSTD_bounds & range)
    {
	if(range.lowerBound == 0 && range.upperBound == 0)
	    return true;

	return range.lowerBound <= value && value <= range.upperBound;
    }

} // end of namespace
