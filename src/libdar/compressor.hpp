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
// $Id: compressor.hpp,v 1.12.4.3 2009/05/14 17:33:39 edrusb Rel $
//
/*********************************************************************/
    /// \file compressor.hpp
    /// \brief compression engine implementation

#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#include "../my_config.h"

#include "infinint.hpp"
#include "generic_file.hpp"
#include "integers.hpp"
#include "wrapperlib.hpp"

namespace libdar
{

	/// the different compression algorithm available

	/// values to be used as argument of libdar API calls
	/// \ingroup API
    enum compression
    {
	none = 'n', ///< no compression
	zip = 'p',  ///< zip compression (not implemented)
	gzip = 'z', ///< gzip compression
	bzip2 = 'y' ///< bzip2 compression
    };

    extern compression char2compression(char a);
    extern char compression2char(compression c);
    extern std::string compression2string(compression c);

	/// compression class for gzip and bzip2 algorithms
    class compressor : public generic_file
    {
    public :
        compressor(user_interaction & dialog, compression algo, generic_file & compressed_side, U_I compression_level = 9);
            // compressed_side is not owned by the object and will remains
            // after the objet destruction

        compressor(user_interaction & dialog, compression algo, generic_file *compressed_side, U_I compression_level = 9);
            // compressed_side is owned by the object and will be
            // deleted a destructor time

        ~compressor();

        void flush_write(); // flush all data to compressed_side, and reset the compressor
            // for that additional write can be uncompresssed starting at this point.
        void flush_read(); // reset decompression engine to be able to read the next block of compressed data
            // if not called, furthur read return EOF
        void clean_read(); // discard any byte buffered and not yet returned by read()
        void clean_write(); // discard any byte buffered and not yet wrote to compressed_side;

        compression get_algo() const { return current_algo; };

	    /// changes compression algorithm used by the compressor

	    /// \param[in] new_algo defines the new algorithm to use
	    /// \param[in] new_compression_level defines the new compression level to use.
            /// \note valid value for new_compression_level range from 0 (no compression) to
	    /// 9 (maximum compression).
        void change_algo(compression new_algo, U_I new_compression_level);


	    /// changes the compression algorithm keeping the same compression level

        void change_algo(compression new_algo)
	{
	    change_algo(new_algo, current_level);
	};

            // inherited from generic file
        bool skip(const infinint & position) { flush_write(); flush_read(); clean_read(); return compressed->skip(position); };
        bool skip_to_eof()  { flush_write(); flush_read(); clean_read(); return compressed->skip_to_eof(); };
        bool skip_relative(S_I x) { flush_write(); flush_read(); clean_read(); return compressed->skip_relative(x); };
        infinint get_position() { return compressed->get_position(); };

    protected :
        S_I inherited_read(char *a, size_t size) { return (this->*read_ptr)(a, size); };
        S_I inherited_write(const char *a, size_t size) { return (this->*write_ptr)(a, size); };

    private :
        struct xfer
        {
            wrapperlib wrap;
            char *buffer;
            U_I size;

            xfer(U_I sz, wrapperlib_mode mode);
            ~xfer();
        };

        xfer *compr, *decompr;
        generic_file *compressed;
        bool compressed_owner;
        compression current_algo;
	U_I current_level;

        void init(compression algo, generic_file *compressed_side, U_I compression_level);
        void terminate();
        S_I (compressor::*read_ptr) (char *a, size_t size);
        S_I none_read(char *a, size_t size);
        S_I gzip_read(char *a, size_t size);
            // S_I zip_read(char *a, size_t size);
            // S_I bzip2_read(char *a, size_t size); // using gzip_read, same code thanks to wrapperlib

        S_I (compressor::*write_ptr) (const char *a, size_t size);
        S_I none_write(const char *a, size_t size);
        S_I gzip_write(const char *a, size_t size);
            // S_I zip_write(char *a, size_t size);
            // S_I bzip2_write(char *a, size_t size); // using gzip_write, same code thanks to wrapperlib
    };

} // end of namespace

#endif
