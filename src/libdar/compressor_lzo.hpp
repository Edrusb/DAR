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

    /// \file compressor_lzo.hpp
    /// \brief streaming compression engine implementation for lzo algorithm
    /// \ingroup Private

#ifndef COMPRESSOR_LZO_HPP
#define COMPRESSOR_LZO_HPP

#include "../my_config.h"

#include "proto_compressor.hpp"
#include "compress_module.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// streaming compression class for lzo algorithm

    class compressor_lzo : public proto_compressor
    {
    public :
        compressor_lzo(compression algo, generic_file & compressed_side, U_I compression_level = 9);
            // compressed_side is not owned by the object and will remains (must survive)
            // after the objet destruction

	compressor_lzo(const compressor_lzo & ref) = delete;
	compressor_lzo(compressor_lzo && ref) noexcept = delete;
	compressor_lzo & operator = (const compressor_lzo & ref) = delete;
	compressor_lzo & operator = (compressor_lzo && ref) noexcept = delete;
        ~compressor_lzo();

        virtual compression get_algo() const override;

	virtual void suspend_compression() override;
	virtual void resume_compression() override;
	virtual bool is_compression_suspended() const override { return suspended; };


            // inherited from generic file
	virtual bool skippable(skippability direction, const infinint & amount) override { return compressed->skippable(direction, amount); };
        virtual bool skip(const infinint & pos) override { inherited_sync_write(); inherited_flush_read(); return compressed->skip(pos); };
        virtual bool skip_to_eof() override { inherited_sync_write(); inherited_flush_read(); return compressed->skip_to_eof(); };
        virtual bool skip_relative(S_I x) override { inherited_sync_write(); inherited_flush_read(); return compressed->skip_relative(x); };
	virtual bool truncatable(const infinint & pos) const override { return compressed->truncatable(pos); };
        virtual infinint get_position() const override { return compressed->get_position(); };

    protected :
	virtual void inherited_read_ahead(const infinint & amount) override { compressed->read_ahead(amount); };
        virtual U_I inherited_read(char *a, U_I size) override;
        virtual void inherited_write(const char *a, U_I size) override;
	virtual void inherited_truncate(const infinint & pos) override;
	virtual void inherited_sync_write() override;
	virtual void inherited_flush_read() override;
	virtual void inherited_terminate() override;

    private :
	struct lzo_block_header
	{
	    char type;             ///< let the possibility to extend this architecture (for now type is fixed)
	    infinint size;         ///< size of the following compressed block of data

	    void dump(generic_file & f);
	    void set_from(generic_file & f);
	};

	char *lzo_read_buffer;     ///< stores clear data (uncompressed) read from the compressed generic_file
	U_I lzo_read_size;         ///< number of available bytes in the read buffer for lzo decompression
	U_I lzo_read_start;        ///< location of the next byte to read out from the read buffer
	bool lzo_read_reached_eof; ///< whether reading reached end of file and the lzo engine has to be reset to uncompress further data

	char *lzo_write_buffer;    ///< stores the clear data to be compressed and written to the compressed generic_file
	U_I lzo_write_size;        ///< number of available bytes to compress and next place where to add more data in the wite buffer
	bool lzo_write_flushed;    ///< whether write flushing has been done

	char *lzo_compressed;      ///< compressed data just read or about to be written
	char *lzo_wrkmem;          ///< work memory for LZO library

	std::unique_ptr<compress_module> module; ///< handles the per block compression/decompression

        generic_file *compressed;
        compression current_algo;
	bool suspended;
	U_I current_level;

	void reset_compr_engine(); ///< reset the compression engine ready for use

	void lzo_compress_buffer_and_write();
	void lzo_read_and_uncompress_to_buffer();

        void clean_write(); // discard any byte buffered and not yet wrote to compressed_side;
    };

	/// @}

} // end of namespace

#endif
