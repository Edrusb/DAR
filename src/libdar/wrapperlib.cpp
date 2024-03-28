/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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

#include "wrapperlib.hpp"
#include "erreurs.hpp"

#define CHECK_Z if(z_ptr == nullptr) throw SRC_BUG
#define CHECK_BZ if(bz_ptr == nullptr) throw SRC_BUG
#define CHECK_LZMA if(lzma_ptr == nullptr) throw SRC_BUG;

using namespace std;

namespace libdar
{

#if LIBZ_AVAILABLE
    static S_I zlib2wrap_code(S_I code);
    static S_I wrap2zlib_code(S_I code);
#endif
#if LIBBZ2_AVAILABLE
    static S_I bzlib2wrap_code(S_I code);
    static S_I wrap2bzlib_code(S_I code);
#endif
#if LIBLZMA_AVAILABLE
    static S_I lzma2wrap_code(S_I code);
    static lzma_action wrap2lzma_code(S_I code);
#endif

    wrapperlib::wrapperlib(wrapperlib_mode mode)
    {
        switch(mode)
        {
        case zlib_mode:
#if LIBZ_AVAILABLE
	    z_ptr = new (nothrow) z_stream;
            if(z_ptr == nullptr)
                throw Ememory("wrapperlib::wrapperlib");
#if LIBBZ2_AVAILABLE
            bz_ptr = nullptr;
#endif
#if LIBLZMA_AVAILABLE
	    lzma_ptr = nullptr;
#endif
            z_ptr->zalloc = nullptr;
            z_ptr->zfree = nullptr;
            z_ptr->opaque = nullptr;
            x_compressInit = & wrapperlib::z_compressInit;
            x_decompressInit = & wrapperlib::z_decompressInit;
            x_compressEnd = & wrapperlib::z_compressEnd;
            x_decompressEnd = & wrapperlib::z_decompressEnd;
            x_compress = & wrapperlib::z_compress;
            x_decompress = & wrapperlib::z_decompress;
            x_set_next_in = & wrapperlib::z_set_next_in;
            x_set_avail_in = & wrapperlib::z_set_avail_in;
            x_get_avail_in = & wrapperlib::z_get_avail_in;
            x_get_total_in = & wrapperlib::z_get_total_in;
            x_set_next_out = & wrapperlib::z_set_next_out;
            x_get_next_out = & wrapperlib::z_get_next_out;
            x_set_avail_out = & wrapperlib::z_set_avail_out;
            x_get_avail_out = & wrapperlib::z_get_avail_out;
            x_get_total_out = & wrapperlib::z_get_total_out;
            break;
#else
	    throw Ecompilation("gzip compression support (libz)");
#endif
        case bzlib_mode:
#if LIBBZ2_AVAILABLE
	    bz_ptr = new (nothrow) bz_stream;
            if(bz_ptr == nullptr)
                throw Ememory("wrapperlib::wrapperlib");
#if LIBZ_AVAILABLE
            z_ptr = nullptr;
#endif
#if LIBLZMA_AVAILABLE
	    lzma_ptr = nullptr;
#endif
            bz_ptr->bzalloc = nullptr;
            bz_ptr->bzfree = nullptr;
            bz_ptr->opaque = nullptr;
            x_compressInit = & wrapperlib::bz_compressInit;
            x_decompressInit = & wrapperlib::bz_decompressInit;
            x_compressEnd = & wrapperlib::bz_compressEnd;
            x_decompressEnd = & wrapperlib::bz_decompressEnd;
            x_compress = & wrapperlib::bz_compress;
            x_decompress = & wrapperlib::bz_decompress;
            x_set_next_in = & wrapperlib::bz_set_next_in;
            x_set_avail_in = & wrapperlib::bz_set_avail_in;
            x_get_avail_in = & wrapperlib::bz_get_avail_in;
            x_get_total_in = & wrapperlib::bz_get_total_in;
            x_set_next_out = & wrapperlib::bz_set_next_out;
            x_get_next_out = & wrapperlib::bz_get_next_out;
            x_set_avail_out = & wrapperlib::bz_set_avail_out;
            x_get_avail_out = & wrapperlib::bz_get_avail_out;
            x_get_total_out = & wrapperlib::bz_get_total_out;
            break;
#else
	    throw Ecompilation("bzip2 compression support (libbz2)");
#endif
	case xz_mode:
#if LIBLZMA_AVAILABLE
#if LIBZ_AVAILABLE
            z_ptr = nullptr;
#endif
#if LIBBZ2_AVAILABLE
            bz_ptr = nullptr;
#endif
	    lzma_ptr = new (nothrow) lzma_stream;
	    if(lzma_ptr == nullptr)
		throw Ememory("wrapperlib::wrapperlib");
	    *lzma_ptr = LZMA_STREAM_INIT;
            x_compressInit = & wrapperlib::lzma_compressInit;
            x_decompressInit = & wrapperlib::lzma_decompressInit;
            x_compressEnd = & wrapperlib::lzma_end;
            x_decompressEnd = & wrapperlib::lzma_end;
            x_compress = & wrapperlib::lzma_encode;
            x_decompress = & wrapperlib::lzma_encode;
            x_set_next_in = & wrapperlib::lzma_set_next_in;
            x_set_avail_in = & wrapperlib::lzma_set_avail_in;
            x_get_avail_in = & wrapperlib::lzma_get_avail_in;
            x_get_total_in = & wrapperlib::lzma_get_total_in;
            x_set_next_out = & wrapperlib::lzma_set_next_out;
            x_get_next_out = & wrapperlib::lzma_get_next_out;
            x_set_avail_out = & wrapperlib::lzma_set_avail_out;
            x_get_avail_out = & wrapperlib::lzma_get_avail_out;
            x_get_total_out = & wrapperlib::lzma_get_total_out;
#else
	    throw Ecompilation("xz compression support (libxz)");
#endif
	    break;
        default:
            throw SRC_BUG;
        }
        level = -1;
    }

    wrapperlib::~wrapperlib()
    {
#if LIBZ_AVAILABLE
        if(z_ptr != nullptr)
	    delete z_ptr;
#endif
#if LIBBZ2_AVAILABLE
        if(bz_ptr != nullptr)
	    delete bz_ptr;
#endif
#if LIBLZMA_AVAILABLE
	if(lzma_ptr != nullptr)
	{
	    ::lzma_end(lzma_ptr);
	    delete lzma_ptr;
	}
#endif
    }

////////////// Zlib routines /////////////

#if LIBZ_AVAILABLE
    S_I wrapperlib::z_compressInit(U_I compression_level)
    {
        CHECK_Z;
        return zlib2wrap_code(deflateInit(z_ptr, compression_level));
    }

    S_I wrapperlib::z_decompressInit()
    {
        CHECK_Z;
        return zlib2wrap_code(inflateInit(z_ptr));
    }

    S_I wrapperlib::z_compressEnd()
    {
        CHECK_Z;
        return zlib2wrap_code(deflateEnd(z_ptr));
    }

    S_I wrapperlib::z_decompressEnd()
    {
        CHECK_Z;
        return zlib2wrap_code(inflateEnd(z_ptr));
    }

    S_I wrapperlib::z_compress(S_I flag)
    {
        CHECK_Z;
        return zlib2wrap_code(deflate(z_ptr, wrap2zlib_code(flag)));
    }

    S_I wrapperlib::z_decompress(S_I flag)
    {
        CHECK_Z;
        return zlib2wrap_code(inflate(z_ptr, wrap2zlib_code(flag)));
    }

    void wrapperlib::z_set_next_in(const char *x)
    {
        CHECK_Z;
        z_ptr->next_in = (Bytef *)x;
    }

    void wrapperlib::z_set_avail_in(U_I x)
    {
        CHECK_Z;
        z_ptr->avail_in = x;
    }

    U_I wrapperlib::z_get_avail_in() const
    {
        CHECK_Z;
        return z_ptr->avail_in;
    }

    U_64 wrapperlib::z_get_total_in() const
    {
        CHECK_Z;
        return z_ptr->total_in;
    }

    void wrapperlib::z_set_next_out(char *x)
    {
        CHECK_Z;
        z_ptr->next_out = (Bytef *)x;
    }

    char *wrapperlib::z_get_next_out() const
    {
        CHECK_Z;
        return (char *)z_ptr->next_out;
    }

    void wrapperlib::z_set_avail_out(U_I x)
    {
        CHECK_Z;
        z_ptr->avail_out = x;
    }

    U_I wrapperlib::z_get_avail_out() const
    {
        CHECK_Z;
        return z_ptr->avail_out;
    }

    U_64 wrapperlib::z_get_total_out() const
    {
        CHECK_Z;
        return z_ptr->total_out;
    }
#endif

////////////// BZlib routines /////////////

#if LIBBZ2_AVAILABLE
    void wrapperlib::bz_set_next_in(const char *x)
    {
        CHECK_BZ;
        bz_ptr->next_in = (char*)x;	// It must be a bug in bz that the input is not a const char*
    }

    void wrapperlib::bz_set_avail_in(U_I x)
    {
        CHECK_BZ;
        bz_ptr->avail_in = x;
    }

    U_I wrapperlib::bz_get_avail_in() const
    {
        CHECK_BZ;
        return bz_ptr->avail_in;
    }

    U_64 wrapperlib::bz_get_total_in() const
    {
        CHECK_BZ;
        return ((U_64)(bz_ptr->total_in_hi32) << 32) | ((U_64)(bz_ptr->total_in_lo32));
    }

    void wrapperlib::bz_set_next_out(char *x)
    {
        CHECK_BZ;
        bz_ptr->next_out = x;
    }

    char *wrapperlib::bz_get_next_out() const
    {
        CHECK_BZ;
        return bz_ptr->next_out;
    }

    void wrapperlib::bz_set_avail_out(U_I x)
    {
        CHECK_BZ;
        bz_ptr->avail_out = x;
    }

    U_I wrapperlib::bz_get_avail_out() const
    {
        CHECK_BZ;
        return bz_ptr->avail_out;
    }

    U_64 wrapperlib::bz_get_total_out() const
    {
        CHECK_BZ;
        return ((U_64)(bz_ptr->total_out_hi32) << 32) | ((U_64)(bz_ptr->total_out_lo32));
    }


    S_I wrapperlib::bz_compressInit(U_I compression_level)
    {
        CHECK_BZ;
        return bzlib2wrap_code(BZ2_bzCompressInit(bz_ptr, compression_level, 0, 30));
    }

    S_I wrapperlib::bz_decompressInit()
    {
        CHECK_BZ;
        return bzlib2wrap_code(BZ2_bzDecompressInit(bz_ptr, 0,0));
    }

    S_I wrapperlib::bz_compressEnd()
    {
        CHECK_BZ;
        return bzlib2wrap_code(BZ2_bzCompressEnd(bz_ptr));
    }

    S_I wrapperlib::bz_decompressEnd()
    {
        CHECK_BZ;
        return bzlib2wrap_code(BZ2_bzDecompressEnd(bz_ptr));
    }

    S_I wrapperlib::bz_compress(S_I flag)
    {
        S_I ret;
        CHECK_BZ;
        ret = BZ2_bzCompress(bz_ptr, wrap2bzlib_code(flag));
        if(ret == BZ_SEQUENCE_ERROR)
            ret = BZ_STREAM_END;
        return bzlib2wrap_code(ret);
    }

    S_I wrapperlib::bz_decompress(S_I flag)
    {
            // flag is not used here.
        S_I ret;
        CHECK_BZ;
        ret = BZ2_bzDecompress(bz_ptr);
        if(ret == BZ_SEQUENCE_ERROR)
            ret = BZ_STREAM_END;
        return bzlib2wrap_code(ret);
    }
#endif

////////////// LZMA routines /////////////

#if LIBLZMA_AVAILABLE
    S_I wrapperlib::lzma_compressInit(U_I compression_level)
    {
        CHECK_LZMA;
        return lzma2wrap_code(lzma_easy_encoder(lzma_ptr,
						compression_level,
						LZMA_CHECK_CRC32));
	    // CR32 is large enough, even no LZMA_CHECK_NONE would
	    // be possible as compressed data is protected by libdar
	    // CRC which width is proportionnal to the size of the
	    // compressed file.
    }

    S_I wrapperlib::lzma_decompressInit()
    {
        CHECK_LZMA;
        return lzma2wrap_code(lzma_auto_decoder(lzma_ptr,
						UINT64_MAX,
						0));
    }

    S_I wrapperlib::lzma_end()
    {
        CHECK_LZMA;
        return WR_OK; // nothing done
    }

    S_I wrapperlib::lzma_encode(S_I flag)
    {
        CHECK_LZMA;
        return lzma2wrap_code(lzma_code(lzma_ptr, wrap2lzma_code(flag)));
    }

    void wrapperlib::lzma_set_next_in(const char *x)
    {
        CHECK_LZMA;
        lzma_ptr->next_in = (Bytef *)x;
    }

    void wrapperlib::lzma_set_avail_in(U_I x)
    {
        CHECK_LZMA;
        lzma_ptr->avail_in = x;
    }

    U_I wrapperlib::lzma_get_avail_in() const
    {
        CHECK_LZMA;
        return lzma_ptr->avail_in;
    }

    U_64 wrapperlib::lzma_get_total_in() const
    {
        CHECK_LZMA;
        return lzma_ptr->total_in;
    }

    void wrapperlib::lzma_set_next_out(char *x)
    {
        CHECK_LZMA;
        lzma_ptr->next_out = (Bytef *)x;
    }

    char *wrapperlib::lzma_get_next_out() const
    {
        CHECK_LZMA;
        return (char *)lzma_ptr->next_out;
    }

    void wrapperlib::lzma_set_avail_out(U_I x)
    {
        CHECK_LZMA;
        lzma_ptr->avail_out = x;
    }

    U_I wrapperlib::lzma_get_avail_out() const
    {
        CHECK_LZMA;
        return lzma_ptr->avail_out;
    }

    U_64 wrapperlib::lzma_get_total_out() const
    {
        CHECK_LZMA;
        return lzma_ptr->total_out;
    }
#endif

    S_I wrapperlib::compressReset()
    {
        S_I ret;

        if(level < 0)
            throw Erange("wrapperlib::compressReset", gettext("compressReset called but compressInit never called before"));
        ret = compressEnd();
        if(ret == WR_OK)
            return compressInit(level);
        else
            return ret;
    }

    S_I wrapperlib::decompressReset()
    {
        S_I ret = decompressEnd();

        if(ret == WR_OK)
            return decompressInit();
        else
            return ret;
    }

#if LIBZ_AVAILABLE
    static S_I zlib2wrap_code(S_I code)
    {
        switch(code)
        {
        case Z_OK:
            return WR_OK;
        case Z_MEM_ERROR:
            return WR_MEM_ERROR;
        case Z_VERSION_ERROR:
            return WR_VERSION_ERROR;
        case Z_STREAM_END:
            return WR_STREAM_END;
        case Z_DATA_ERROR:
            return WR_DATA_ERROR;
        case Z_BUF_ERROR:
            return WR_BUF_ERROR;
        case Z_STREAM_ERROR:
            return WR_STREAM_ERROR;
        case Z_NEED_DICT:
	    return WR_DATA_ERROR;
		// we do not use explicit dictionnary for compression,
		// this is zlib assumes it requires a dictionnary, this is
		// to be considered a data error.
        default:
            throw SRC_BUG; // unexpected error code
        }
    }

    static S_I wrap2zlib_code(S_I code)
    {
        switch(code)
        {
        case WR_NO_FLUSH:
            return Z_NO_FLUSH;
        case WR_FINISH:
            return Z_FINISH;
        default:
            throw SRC_BUG;
        }
    }


#endif

#if LIBBZ2_AVAILABLE
    static S_I bzlib2wrap_code(S_I code)
    {
        switch(code)
        {
        case BZ_OK:
        case BZ_RUN_OK:
        case BZ_FLUSH_OK:
        case BZ_FINISH_OK:
            return WR_OK;
        case BZ_PARAM_ERROR:
            return WR_STREAM_ERROR;
        case BZ_CONFIG_ERROR:
            return WR_VERSION_ERROR;
        case BZ_MEM_ERROR:
            return WR_MEM_ERROR;
        case BZ_DATA_ERROR:
        case BZ_DATA_ERROR_MAGIC:
            return WR_DATA_ERROR;
        case BZ_STREAM_END:
            return WR_STREAM_END;
        case BZ_SEQUENCE_ERROR:
        default:
            throw SRC_BUG;
        }
    }

    static S_I wrap2bzlib_code(S_I code)
    {
        switch(code)
        {
        case WR_NO_FLUSH:
            return BZ_RUN;
        case WR_FINISH:
            return BZ_FINISH;
        default:
            throw SRC_BUG;
        }
    }
#endif

#if LIBLZMA_AVAILABLE
    static S_I lzma2wrap_code(S_I code)
    {
        switch(code)
        {
        case LZMA_OK:
            return WR_OK;
        case LZMA_MEM_ERROR:
            return WR_MEM_ERROR;
        case LZMA_OPTIONS_ERROR:
            return WR_VERSION_ERROR;
	case LZMA_FORMAT_ERROR: // no memory usage limit used from libdar, only file format error can generate this code
	    return WR_DATA_ERROR;
        case LZMA_STREAM_END:
            return WR_STREAM_END;
        case LZMA_DATA_ERROR:
            return WR_DATA_ERROR;
        case LZMA_BUF_ERROR:
            return WR_BUF_ERROR;
	case LZMA_NO_CHECK:
	case LZMA_UNSUPPORTED_CHECK:
            return WR_STREAM_ERROR;
	case LZMA_PROG_ERROR:
	    throw SRC_BUG; // error in libdar calling liblzma
	case LZMA_GET_CHECK:
	    throw SRC_BUG;
		// can be retured by lzma_code "only if the decoder was initialized with the LZMA_TELL_ANY_CHECK flag"
		// flag we do not use from libdar
        default:
            throw SRC_BUG; // unexpected error code
        }
    }

    static lzma_action wrap2lzma_code(S_I code)
    {
        switch(code)
        {
        case WR_NO_FLUSH:
            return LZMA_RUN;
        case WR_FINISH:
            return LZMA_FINISH;
        default:
            throw SRC_BUG;
        }
    }

#endif

} // end of namespace
