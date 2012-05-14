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
// $Id: generic_file.hpp,v 1.7 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/

#ifndef GENERIC_FILE_HPP
#define GENERIC_FILE_HPP

#pragma interface

#include "../my_config.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "infinint.hpp"
#include "path.hpp"
#include "integers.hpp"

namespace libdar
{
    const int CRC_SIZE = 2;
    typedef char crc[CRC_SIZE];
    extern void clear(crc & value);
    extern void copy_crc(crc & dst, const crc & src);
    extern bool same_crc(const crc &a, const crc &b);

    enum gf_mode { gf_read_only, gf_write_only, gf_read_write };

// note :
// the read and write method are similar to the read and write system call
// except that they never return negative values, but throw exception instead
// returning zero means end of generic_file, and the call is blocking if
// no data is available
// write returns the number of bytes written, and never make partial writtingss
// this it is blocked until all bytes are written or occures an exception
// thus the returned value is always the value of the size argument.

    extern gf_mode generic_file_get_mode(S_I fd);
    extern const std::string generic_file_get_name(gf_mode mode);

    class generic_file
    {
    public :
        generic_file(gf_mode m) { rw = m; clear(value); crc_offset = 0; enable_crc(false); };
        virtual ~generic_file() {};
    
        gf_mode get_mode() const { return rw; };
        S_I read(char *a, size_t size);
        S_I write(char *a, size_t size);
        S_I write(const std::string & arg);
        S_I read_back(char &a);
        virtual bool skip(const infinint & pos) = 0;
        virtual bool skip_to_eof() = 0;
        virtual bool skip_relative(S_I x) = 0;
        virtual infinint get_position() = 0;
    
        void copy_to(generic_file &ref);
        void copy_to(generic_file &ref, crc & value);
            // generates CRC on copied data
        U_32 copy_to(generic_file &ref, U_32 size); // returns the number of byte effectively copied
        infinint copy_to(generic_file &ref, infinint size); // returns the number of byte effectively copied
        bool diff(generic_file & f); // return true if arg differs from "this"

            // CRC on read or writen data
        void reset_crc();
        void get_crc(crc & val) { enable_crc(false); copy_crc(val, value); };

    protected :
        void set_mode(gf_mode x) { rw = x; };
        virtual S_I inherited_read(char *a, size_t size) = 0;
            // must provide as much byte as requested up to end of file
            // stay blocked if not enough available
            // returning zero or less than requested means end of file
        virtual S_I inherited_write(char *a, size_t size) = 0;
            // must write all data or block or throw exceptions
            // thus always returns the second argument

    private :
        gf_mode rw;
        crc value;
        S_I crc_offset;
        S_I (generic_file::* active_read)(char *a, size_t size);
        S_I (generic_file::* active_write)(char *a, size_t size);

        void enable_crc(bool mode);
        void compute_crc(char *a, S_I size);
        S_I read_crc(char *a, size_t size);
        S_I write_crc(char *a, size_t size);
    };

    class fichier : public generic_file
    {
    public :
        fichier(S_I fd);
        fichier(char *name, gf_mode m);
        fichier(const path & chemin, gf_mode m);
        ~fichier() { close(filedesc); };

        infinint get_size() const;

            // herite de generic_file
        bool skip(const infinint & pos);
        bool skip_to_eof();
        bool skip_relative(S_I x);
        infinint get_position();

    protected :
        S_I inherited_read(char *a, size_t size);
        S_I inherited_write(char *a, size_t size);

    private :
        S_I filedesc;

        void open(const char *name, gf_mode m);
    };

} // end of namespace

#endif
