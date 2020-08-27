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

    /// \file compressor_zstd.hpp
    /// \brief streaming compression implementation for zstd algorithm
    /// \ingroup Private

#ifndef COMPRESSOR_ZSTD_HPP
#define COMPRESSOR_ZSTD_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_ZSTD_H
#include <zstd.h>
#endif
}

#include "proto_compressor.hpp"

namespace libdar
{


	/// \addtogroup Private
	/// @{

	/// compression class for zstd algorithms

    class compressor_zstd : public proto_compressor
    {
    public :
        compressor_zstd(generic_file & compressed_side, U_I compression_level = 9);
            // compressed_side is not owned by the object and will remains
            // after the objet destruction

	compressor_zstd(const compressor_zstd & ref) = delete;
	compressor_zstd(compressor_zstd && ref) noexcept = delete;
	compressor_zstd & operator = (const compressor_zstd & ref) = delete;
	compressor_zstd & operator = (compressor_zstd && ref) noexcept = delete;
        ~compressor_zstd();

        virtual compression get_algo() const override;

	virtual void suspend_compression() override;
	virtual void resume_compression() override;
	virtual bool is_compression_suspended() const override { return suspended; };


            // inherited from generic file
	virtual bool skippable(skippability direction, const infinint & amount) override { return compressed->skippable(direction, amount); };
        virtual bool skip(const infinint & pos) override { compr_flush_write(); compr_flush_read(); clean_read(); return compressed->skip(pos); };
        virtual bool skip_to_eof() override { compr_flush_write(); compr_flush_read(); clean_read(); return compressed->skip_to_eof(); };
        virtual bool skip_relative(S_I x) override { compr_flush_write(); compr_flush_read(); clean_read(); return compressed->skip_relative(x); };
	virtual bool truncatable(const infinint & pos) const override { return compressed->truncatable(pos); };
        virtual infinint get_position() const override { return compressed->get_position(); };

    protected :
	virtual void inherited_read_ahead(const infinint & amount) override { compressed->read_ahead(amount); };
        virtual U_I inherited_read(char *a, U_I size) override;
        virtual void inherited_write(const char *a, U_I size) override;
	virtual void inherited_truncate(const infinint & pos) override;
	virtual void inherited_sync_write() override { compr_flush_write(); };
	virtual void inherited_flush_read() override { compr_flush_read(); clean_read(); };
	virtual void inherited_terminate() override;

    private :
        generic_file *compressed;
	bool suspended;

#if LIBZSTD_AVAILABLE
	ZSTD_CStream *comp;
	ZSTD_DStream *decomp;

	ZSTD_inBuffer inbuf;
	ZSTD_outBuffer outbuf;
	char *below_tampon;    ///< used to hold in-transit data
	U_I below_tampon_size; ///< allocated size of tampon
	U_I above_tampon_size; ///< max size of input data

	bool flueof;           ///< is EOF in read mode and flushed in write mode
	bool no_comp_data;     ///< EOF in underlying layer in read mode
#endif



	void reset_compr_engine();    ///< reset the compression engine ready for use

        void compr_flush_write(); // flush all data to compressed_side, and reset the compressor_zstd
            // for that additional write can be uncompresssed starting at this point.
        void compr_flush_read(); // reset decompression engine to be able to read the next block of compressed data
            // if not called, furthur read return EOF
        void clean_read(); // discard any byte buffered and not yet returned by read()
        void clean_write(); // discard any byte buffered and not yet wrote to compressed_side;

	    /// from zstd

	void clear_inbuf();
	void clear_outbuf();
	void release_mem();
	void setup_context(U_I compression_level);

    };

	/// @}

} // end of namespace

#endif
