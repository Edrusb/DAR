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
// $Id: compressor.cpp,v 1.14 2004/07/31 17:45:24 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
} // end extern "C"

#include "compressor.hpp"

#define BUFFER_SIZE 102400

using namespace std;

namespace libdar
{

    compressor::compressor(user_interaction & dialog, compression algo, generic_file & compressed_side, U_I compression_level) : generic_file(dialog, compressed_side.get_mode())
    {
        init(algo, &compressed_side, compression_level);
        compressed_owner = false;
    }

    compressor::compressor(user_interaction & dialog, compression algo, generic_file *compressed_side, U_I compression_level) : generic_file(dialog, compressed_side->get_mode())
    {
        init(algo, compressed_side, compression_level);
        compressed_owner = true;
    }

    void compressor::init(compression algo, generic_file *compressed_side, U_I compression_level)
    {
            // theses are eventually overwritten below
        wrapperlib_mode wr_mode = zlib_mode;
        current_algo = algo;

        if(compressed_side == NULL)
            throw SRC_BUG;
        if(compression_level > 9)
            throw SRC_BUG;

        compr = decompr = NULL;
        switch(algo)
        {
        case none :
            read_ptr = &compressor::none_read;
            write_ptr = &compressor::none_write;
            break;
        case zip :
            throw Efeature(gettext("zip compression not implemented"));
            break;
        case bzip2 :
            wr_mode = bzlib_mode;
                // NO BREAK !
        case gzip :
            read_ptr = &compressor::gzip_read;
            write_ptr = &compressor::gzip_write;
            compr = new xfer(BUFFER_SIZE, wr_mode);
            if(compr == NULL)
                throw Ememory("compressor::compressor");
            decompr = new xfer(BUFFER_SIZE, wr_mode);
            if(decompr == NULL)
            {
                delete compr;
                throw Ememory("compressor::compressor");
            }

            switch(compr->wrap.compressInit(compression_level))
            {
            case WR_OK:
                break;
            case WR_MEM_ERROR:
                delete compr;
                delete decompr;
                throw Ememory("compressor::compressor");
            case WR_VERSION_ERROR:
                delete compr;
                delete decompr;
                throw Erange("compressor::compressor", gettext("incompatible Zlib version"));
            case WR_STREAM_ERROR:
            default:
                delete compr;
                delete decompr;
                throw SRC_BUG;
            }

            switch(decompr->wrap.decompressInit())
            {
            case WR_OK:
                decompr->wrap.set_avail_in(0);
                break;
            case WR_MEM_ERROR:
                compr->wrap.compressEnd();
                delete compr;
                delete decompr;
                throw Ememory("compressor::compressor");
            case WR_VERSION_ERROR:
                compr->wrap.compressEnd();
                delete compr;
                delete decompr;
                throw Erange("compressor::compressor", gettext("incompatible Zlib version"));
            case WR_STREAM_ERROR:
            default:
                compr->wrap.compressEnd();
                delete compr;
                delete decompr;
                throw SRC_BUG;
            }
            break;
        default :
            throw SRC_BUG;
        }

        compressed = compressed_side;
    }

    compressor::~compressor()
    {
        terminate();
        if(compressed_owner)
            delete compressed;
    }

    void compressor::terminate()
    {
        if(compr != NULL)
        {
            S_I ret;

                // flushing the pending data
            flush_write();
            clean_write();

            ret = compr->wrap.compressEnd();
            delete compr;

            switch(ret)
            {
            case WR_OK:
                break;
            case WR_DATA_ERROR: // some data remains in the compression pipe (data loss)
                throw SRC_BUG;
            case WR_STREAM_ERROR:
                throw Erange("compressor::~compressor", gettext("compressed data is corrupted"));
            default :
                throw SRC_BUG;
            }
        }

        if(decompr != NULL)
        {
                // flushing data
            flush_read();
            clean_read();

            S_I ret = decompr->wrap.decompressEnd();
            delete decompr;

            switch(ret)
            {
            case WR_OK:
                break;
            default:
                throw SRC_BUG;
            }
        }
    }

    void compressor::change_algo(compression new_algo, U_I new_compression_level)
    {
        if(new_algo == get_algo())
            return;

            // flush data and release zlib memory structures
        terminate();

            // change to new algorithm
        init(new_algo, compressed, new_compression_level);
    }


    compressor::xfer::xfer(U_I sz, wrapperlib_mode mode) : wrap(mode)
    {
        buffer = new char[sz];
        if(buffer == NULL)
            throw Ememory("compressor::xfer::xfer");
        size = sz;
    }

    compressor::xfer::~xfer()
    {
        delete buffer;
    }

    S_I compressor::none_read(char *a, size_t size)
    {
        return compressed->read(a, size);
    }

    S_I compressor::none_write(char *a, size_t size)
    {
        return compressed->write(a, size);
    }

    S_I compressor::gzip_read(char *a, size_t size)
    {
        S_I ret;
        S_I flag = WR_NO_FLUSH;

        if(size == 0)
            return 0;

        decompr->wrap.set_next_out(a);
        decompr->wrap.set_avail_out(size);

        do
        {
                // feeding the input buffer if necessary
            if(decompr->wrap.get_avail_in() == 0)
            {
                decompr->wrap.set_next_in(decompr->buffer);
                decompr->wrap.set_avail_in(compressed->read(decompr->buffer,
                                                            decompr->size));
            }

            ret = decompr->wrap.decompress(flag);

            switch(ret)
            {
            case WR_OK:
            case WR_STREAM_END:
                break;
            case WR_DATA_ERROR:
                throw Erange("compressor::gzip_read", gettext("compressed data CRC error"));
            case WR_MEM_ERROR:
                throw Ememory("compressor::gzip_read");
            case WR_BUF_ERROR:
                    // no process is possible:
                if(decompr->wrap.get_avail_in() == 0) // because we reached EOF
                    ret = WR_STREAM_END; // lib did not returned WR_STREAM_END (why ?)
                else // nothing explains why no process is possible:
                    if(decompr->wrap.get_avail_out() == 0)
                        throw SRC_BUG; // bug from DAR: no output possible
                    else
                        throw SRC_BUG; // unexpected comportment from lib
                break;
            default:
                throw SRC_BUG;
            }
        }
        while(decompr->wrap.get_avail_out() > 0 && ret != WR_STREAM_END);

        return decompr->wrap.get_next_out() - a;
    }

    S_I compressor::gzip_write(char *a, size_t size)
    {
        compr->wrap.set_next_in(a);
        compr->wrap.set_avail_in(size);

        if(a == NULL)
            throw SRC_BUG;

        while(compr->wrap.get_avail_in() > 0)
        {
                // making room for output
            compr->wrap.set_next_out(compr->buffer);
            compr->wrap.set_avail_out(compr->size);

            switch(compr->wrap.compress(WR_NO_FLUSH))
            {
            case WR_OK:
            case WR_STREAM_END:
                break;
            case WR_STREAM_ERROR:
                throw SRC_BUG;
            case WR_BUF_ERROR:
                throw SRC_BUG;
            default :
                throw SRC_BUG;
            }

            if(compr->wrap.get_next_out() != compr->buffer)
                compressed->write(compr->buffer, (char *)compr->wrap.get_next_out() - compr->buffer);
        }

        return size;
    }

    void compressor::flush_write()
    {
        S_I ret;

        if(compr != NULL && compr->wrap.get_total_in() != 0)  // (z/bz)lib
        {
                // no more input
            compr->wrap.set_avail_in(0);
            do
            {
                    // setting the buffer for reception of data
                compr->wrap.set_next_out(compr->buffer);
                compr->wrap.set_avail_out(compr->size);

                ret = compr->wrap.compress(WR_FINISH);

                switch(ret)
                {
                case WR_OK:
                case WR_STREAM_END:
                    if(compr->wrap.get_next_out() != compr->buffer)
                        compressed->write(compr->buffer, (char *)compr->wrap.get_next_out() - compr->buffer);
                    break;
                case WR_BUF_ERROR :
                    throw SRC_BUG;
                case WR_STREAM_ERROR :
                    throw SRC_BUG;
                default :
                    throw SRC_BUG;
                }
            }
            while(ret != WR_STREAM_END);

            if(compr->wrap.compressReset() != WR_OK)
                throw SRC_BUG;
        }
    }

    void compressor::flush_read()
    {
        if(decompr != NULL) // zlib
            if(decompr->wrap.decompressReset() != WR_OK)
                throw SRC_BUG;
            // keep in the buffer the bytes already read, theses are discarded in case of a call to skip
    }

    void compressor::clean_read()
    {
        if(decompr != NULL)
            decompr->wrap.set_avail_in(0);
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: compressor.cpp,v 1.14 2004/07/31 17:45:24 edrusb Rel $";
        dummy_call(id);
    }

    void compressor::clean_write()
    {
        if(compr != NULL)
        {
            S_I ret;

            do
            {
                compr->wrap.set_next_out(compr->buffer);
                compr->wrap.set_avail_out(compr->size);
                compr->wrap.set_avail_in(0);

                ret = compr->wrap.compress(WR_FINISH);
            }
            while(ret == WR_OK);
        }
    }

    compression char2compression(char a)
    {
        switch(a)
        {
        case 'n':
            return none;
        case 'p':
            return zip;
        case 'z':
            return gzip;
        case 'y':
            return bzip2;
        default :
            throw Erange("char2compression", gettext("unknown compression"));
        }
    }

    char compression2char(compression c)
    {
        switch(c)
        {
        case none:
            return 'n';
        case zip:
            return 'p';
        case gzip:
            return 'z';
        case bzip2:
            return 'y';
        default :
            throw Erange("char2compression", gettext("unknown compression"));
        }
    }

    string compression2string(compression c)
    {
        switch(c)
        {
        case none :
            return "none";
        case zip :
            return "zip";
        case gzip :
            return "gzip";
        case bzip2 :
            return "bzip2";
        default :
            throw Erange("compresion2char", gettext("unknown compression"));
        }
    }

} // end of namespace
