/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2052 Denis Corbin
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: wrapperlib.hpp,v 1.4 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/

#ifndef WRAPPERLIB_HPP
#define WRAPPERLIB_HPP

#include "../my_config.h"
#include <zlib.h>
#include <bzlib.h>

namespace libdar
{

    const int WR_OK            = 0;
    const int WR_MEM_ERROR     = 1;
    const int WR_VERSION_ERROR = 2;
    const int WR_STREAM_ERROR  = 3;
    const int WR_DATA_ERROR    = 4;
    const int WR_NO_FLUSH      = 5;
    const int WR_BUF_ERROR     = 6;
    const int WR_STREAM_END    = 7;
    const int WR_FINISH        = 8;

    enum wrapperlib_mode { zlib_mode, bzlib_mode };

    class wrapperlib
    {
    public:
        wrapperlib(wrapperlib_mode mode);
        wrapperlib(const wrapperlib & ref);
        wrapperlib & operator = (const wrapperlib & ref);
        ~wrapperlib();
        
        void set_next_in(char *x) { return (this->*x_set_next_in)(x); };
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
        z_stream *z_ptr;
        bz_stream *bz_ptr;
        S_I level;

        void (wrapperlib::*x_set_next_in)(char *x);
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

        S_I z_compressInit(U_I compression_level);
        S_I z_decompressInit();
        S_I z_compressEnd();
        S_I z_decompressEnd();
        S_I z_compress(S_I flag);
        S_I z_decompress(S_I flag);
        void z_set_next_in(char *x);
        void z_set_avail_in(U_I x);
        U_I z_get_avail_in() const;
        U_64 z_get_total_in() const;
        void z_set_next_out(char *x);
        char *z_get_next_out() const;
        void z_set_avail_out(U_I x);
        U_I z_get_avail_out() const;
        U_64 z_get_total_out() const;

            // set of routines for bzlib
    
        S_I bz_compressInit(U_I compression_level);
        S_I bz_decompressInit();
        S_I bz_compressEnd();
        S_I bz_decompressEnd();
        S_I bz_compress(S_I flag);
        S_I bz_decompress(S_I flag);
        void bz_set_next_in(char *x);
        void bz_set_avail_in(U_I x);
        U_I bz_get_avail_in() const;
        U_64 bz_get_total_in() const;
        void bz_set_next_out(char *x);
        char *bz_get_next_out() const;
        void bz_set_avail_out(U_I x);
        U_I bz_get_avail_out() const;
        U_64 bz_get_total_out() const;
    };

} // end of namespace

#endif
