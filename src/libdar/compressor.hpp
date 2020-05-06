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

    /// \file compressor.hpp
    /// \brief compression engine implementation
    /// \ingroup Private

#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#include "../my_config.h"

#include "infinint.hpp"
#include "generic_file.hpp"
#include "integers.hpp"
#include "wrapperlib.hpp"
#include "compression.hpp"
#include "zstd.hpp"

namespace libdar
{


	/// \addtogroup Private
	/// @{

	/// compression class for gzip and bzip2 algorithms
    class compressor : public generic_file
    {
    public :
        compressor(compression algo, generic_file & compressed_side, U_I compression_level = 9);
            // compressed_side is not owned by the object and will remains
            // after the objet destruction

        compressor(compression algo, generic_file *compressed_side, U_I compression_level = 9);
            // compressed_side is owned by the object and will be
            // deleted a destructor time

	compressor(const compressor & ref) = delete;
	compressor(compressor && ref) = delete;
	compressor & operator = (const compressor & ref) = delete;
	compressor & operator = (compressor && ref) = delete;
        ~compressor();

        compression get_algo() const { return (current_algo == compression::lzo1x_1_15 || current_algo == compression::lzo1x_1) ? compression::lzo : current_algo; };

	void suspend_compression();
	void resume_compression();
	bool is_compression_suspended() const { return suspended; };


            // inherited from generic file
	virtual bool skippable(skippability direction, const infinint & amount) override { return compressed->skippable(direction, amount); };
        virtual bool skip(const infinint & pos) override { compr_flush_write(); compr_flush_read(); clean_read(); return compressed->skip(pos); };
        virtual bool skip_to_eof() override { compr_flush_write(); compr_flush_read(); clean_read(); return compressed->skip_to_eof(); };
        virtual bool skip_relative(S_I x) override { compr_flush_write(); compr_flush_read(); clean_read(); return compressed->skip_relative(x); };
	virtual bool truncatable(const infinint & pos) const override { return compressed->truncatable(pos); };
        virtual infinint get_position() const override { return compressed->get_position(); };

    protected :
	virtual void inherited_read_ahead(const infinint & amount) override { compressed->read_ahead(amount); };
        virtual U_I inherited_read(char *a, U_I size) override { return (this->*read_ptr)(a, size); };
        virtual void inherited_write(const char *a, U_I size) override { (this->*write_ptr)(a, size); };
	virtual void inherited_truncate(const infinint & pos) override;
	virtual void inherited_sync_write() override { compr_flush_write(); };
	virtual void inherited_flush_read() override { compr_flush_read(); clean_read(); };
	virtual void inherited_terminate() override { local_terminate(); };

    private :
        struct xfer
        {
            wrapperlib wrap;
            char *buffer;
            U_I size;

            xfer(U_I sz, wrapperlib_mode mode);
            ~xfer();
        };

	struct lzo_block_header
	{
	    char type;             ///< let the possibility to extend this architecture (for now type is fixed)
	    infinint size;         ///< size of the following compressed block of data

	    void dump(generic_file & f);
	    void set_from(generic_file & f);
	};


        xfer *compr, *decompr;     ///< datastructure for bzip2 an gzip compression

	char *lzo_read_buffer;     ///< stores clear data (uncompressed) read from the compressed generic_file
	char *lzo_write_buffer;    ///< stores the clear data to be compressed and written to the compressed generic_file
	U_I lzo_read_size;         ///< number of available bytes in the read buffer for lzo decompression
	U_I lzo_write_size;        ///< number of available bytes to compress and next place where to add more data in the wite buffer
	U_I lzo_read_start;        ///< location of the next byte to read out from the read buffer
	bool lzo_write_flushed;    ///< whether write flushing has been done
	bool lzo_read_reached_eof; ///< whether reading reached end of file and the lzo engine has to be reset to uncompress further data
	char *lzo_compressed;      ///< compressed data just read or about to be written
	char *lzo_wrkmem;          ///< work memory for LZO library

	zstd* zstd_ptr;            ///< class that handles libzstd interraction

        generic_file *compressed;
        bool compressed_owner;
        compression current_algo;
	bool suspended;
	compression suspended_compr;
	U_I current_level;

        void init(compression algo, generic_file *compressed_side, U_I compression_level);
        void local_terminate();
        U_I (compressor::*read_ptr) (char *a, U_I size);
        U_I none_read(char *a, U_I size);
        U_I gzip_read(char *a, U_I size);
            // U_I zip_read(char *a, U_I size);
            // U_I bzip2_read(char *a, U_I size); // using gzip_read, same code thanks to wrapperlib
	U_I lzo_read(char *a, U_I size);
	U_I zstd_read(char *a, U_I size);

        void (compressor::*write_ptr) (const char *a, U_I size);
        void none_write(const char *a, U_I size);
        void gzip_write(const char *a, U_I size);
            // void zip_write(char *a, U_I size);
            // void bzip2_write(char *a, U_I size); // using gzip_write, same code thanks to wrapperlib
	void lzo_write(const char *a, U_I size);
	void zstd_write(const char *a, U_I size);

	void lzo_compress_buffer_and_write();
	void lzo_read_and_uncompress_to_buffer();

	    /// changes compression algorithm used by the compressor

	    /// \param[in] new_algo defines the new algorithm to use
	    /// \param[in] new_compression_level defines the new compression level to use.
            /// \note valid value for new_compression_level range from 0 (no compression) to
	    /// 9 (maximum compression).
        void change_algo(compression new_algo, U_I new_compression_level);


	    /// changes the compression algorithm keeping the same compression level

        void change_algo(compression new_algo)
	{ change_algo(new_algo, current_level);	};


        void compr_flush_write(); // flush all data to compressed_side, and reset the compressor
            // for that additional write can be uncompresssed starting at this point.
        void compr_flush_read(); // reset decompression engine to be able to read the next block of compressed data
            // if not called, furthur read return EOF
        void clean_read(); // discard any byte buffered and not yet returned by read()
        void clean_write(); // discard any byte buffered and not yet wrote to compressed_side;
    };

	/// @}

} // end of namespace

#endif
