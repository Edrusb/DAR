/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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

    /// \file wrapperlib.hpp
    /// \brief libz and libbz2 wrapper to have identical interface to these libraries.
    /// \ingroup Private
    ///
    /// libz and libbz2 library differ in the way they return values
    /// in certain circumpstances. This module defines the wrapperlib class
    /// that make their use homogeneous.

#ifndef WRAPPERLIB_HPP
#define WRAPPERLIB_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_ZLIB_H && LIBZ_AVAILABLE
#include <zlib.h>
#endif

#if HAVE_BZLIB_H && LIBBZ2_AVAILABLE
#include <bzlib.h>
#endif

#if HAVE_LZMA_H && LIBLZMA_AVAILABLE
#include <lzma.h>
#endif
} // end extern "C"

#include "integers.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    const int WR_OK            = 0;  // operation successful
    const int WR_MEM_ERROR     = 1;  // lack of memory
    const int WR_VERSION_ERROR = 2;  // incompatible version of the compression library with the one expected by libdar
    const int WR_STREAM_ERROR  = 3;  // not a valid compression level, incoherent data provided to the compression library
    const int WR_DATA_ERROR    = 4;  // data has been corrupted
    const int WR_NO_FLUSH      = 5;  // parameter to let the compression library decide at which time to output data (from zlib Z_NO_FLUSH, no other way is done in libdar)
    const int WR_BUF_ERROR     = 6;  // no possible work to perform for the request action without additional provided data/or storage space to the compression library
    const int WR_STREAM_END    = 7;  // end of compressed data met
    const int WR_FINISH        = 8;  // parameter requiring the compression library to cleanly stop the running operation

    enum wrapperlib_mode { zlib_mode, bzlib_mode, xz_mode };

	/// this class encapsulates calls to libz or libbz2

	/// this is mainly an adaptation of libbz2 specificities to
	/// have libb2 acting exactly as libz does.
    class wrapperlib
    {
    public:
        wrapperlib(wrapperlib_mode mode);
        wrapperlib(const wrapperlib & ref) = delete;
	wrapperlib(wrapperlib && ref) noexcept = delete;
        wrapperlib & operator = (const wrapperlib & ref) = delete;
	wrapperlib & operator = (wrapperlib && ref) noexcept = delete;
        ~wrapperlib();

        void set_next_in(const char *x) { return (this->*x_set_next_in)(x); };
        void set_avail_in(U_I x) { return (this->*x_set_avail_in)(x); };
        U_I get_avail_in() const { return (this->*x_get_avail_in)(); };
        U_64 get_total_in() const { return (this->*x_get_total_in)(); };

        void set_next_out(char *x) { return (this->*x_set_next_out)(x); };
        char *get_next_out() const { return (this->*x_get_next_out)(); };
        void set_avail_out(U_I x) { return (this->*x_set_avail_out)(x); };
        U_I get_avail_out() const { return (this->*x_get_avail_out)(); };
        U_64 get_total_out() const { return (this->*x_get_total_out)(); };

        S_I compressInit(U_I compression_level) { level = compression_level; return (this->*x_compressInit)(compression_level); };
        S_I decompressInit() { return (this->*x_decompressInit)(); };
        S_I compressEnd() { return (this->*x_compressEnd)(); };
        S_I decompressEnd() { return (this->*x_decompressEnd)(); };
        S_I compress(S_I flag) { return (this->*x_compress)(flag); };
        S_I decompress(S_I flag) { return (this->*x_decompress)(flag);};
        S_I compressReset();
        S_I decompressReset();

    private:
#if LIBZ_AVAILABLE
        z_stream *z_ptr;
#endif
#if LIBBZ2_AVAILABLE
        bz_stream *bz_ptr;
#endif
#if LIBLZMA_AVAILABLE
	lzma_stream *lzma_ptr;
#endif

        S_I level;

        void (wrapperlib::*x_set_next_in)(const char *x);
        void (wrapperlib::*x_set_avail_in)(U_I x);
        U_I (wrapperlib::*x_get_avail_in)() const;
        U_64 (wrapperlib::*x_get_total_in)() const;

        void (wrapperlib::*x_set_next_out)(char *x);
        char *(wrapperlib::*x_get_next_out)() const;
        void (wrapperlib::*x_set_avail_out)(U_I x);
        U_I (wrapperlib::*x_get_avail_out)() const;
        U_64 (wrapperlib::*x_get_total_out)() const;

        S_I (wrapperlib::*x_compressInit)(U_I compression_level);
        S_I (wrapperlib::*x_decompressInit)();
        S_I (wrapperlib::*x_compressEnd)();
        S_I (wrapperlib::*x_decompressEnd)();
        S_I (wrapperlib::*x_compress)(S_I flag);
        S_I (wrapperlib::*x_decompress)(S_I flag);


            // set of routines for zlib
#if LIBZ_AVAILABLE
        S_I z_compressInit(U_I compression_level);
        S_I z_decompressInit();
        S_I z_compressEnd();
        S_I z_decompressEnd();
        S_I z_compress(S_I flag);
        S_I z_decompress(S_I flag);
        void z_set_next_in(const char *x);
        void z_set_avail_in(U_I x);
        U_I z_get_avail_in() const;
        U_64 z_get_total_in() const;
        void z_set_next_out(char *x);
        char *z_get_next_out() const;
        void z_set_avail_out(U_I x);
        U_I z_get_avail_out() const;
        U_64 z_get_total_out() const;
#endif

            // set of routines for bzlib
#if LIBBZ2_AVAILABLE
        S_I bz_compressInit(U_I compression_level);
        S_I bz_decompressInit();
        S_I bz_compressEnd();
        S_I bz_decompressEnd();
        S_I bz_compress(S_I flag);
        S_I bz_decompress(S_I flag);
        void bz_set_next_in(const char *x);
        void bz_set_avail_in(U_I x);
        U_I bz_get_avail_in() const;
        U_64 bz_get_total_in() const;
        void bz_set_next_out(char *x);
        char *bz_get_next_out() const;
        void bz_set_avail_out(U_I x);
        U_I bz_get_avail_out() const;
        U_64 bz_get_total_out() const;
#endif

            // set of routines for liblzma
#if LIBLZMA_AVAILABLE
        S_I lzma_compressInit(U_I compression_level);
        S_I lzma_decompressInit();
        S_I lzma_end();
        S_I lzma_encode(S_I flag);
        void lzma_set_next_in(const char *x);
        void lzma_set_avail_in(U_I x);
        U_I lzma_get_avail_in() const;
        U_64 lzma_get_total_in() const;
        void lzma_set_next_out(char *x);
        char *lzma_get_next_out() const;
        void lzma_set_avail_out(U_I x);
        U_I lzma_get_avail_out() const;
        U_64 lzma_get_total_out() const;
#endif

    };

	/// @}

} // end of namespace

#endif
