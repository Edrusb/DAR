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
// to contact the author, see the AUTHOR file
/*********************************************************************/

    /// \file block_compressor.hpp
    /// \brief provide per block compression/decompression independant from libthreadar but single threaded
    /// \ingroup Private


#ifndef BLOCK_COMPRESSOR_HPP
#define BLOCK_COMPRESSOR_HPP

#include "../my_config.h"

#include "infinint.hpp"
#include "crypto_segment.hpp"
#include "heap.hpp"
#include "compress_module.hpp"
#include "proto_compressor.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


    class block_compressor: public proto_compressor
    {
    public:
	block_compressor(std::unique_ptr<compress_module> block_zipper,
			 generic_file & compressed_side,
			 U_I uncompressed_bs = default_uncompressed_block_size);
	    // compressed_side is not owned by the object and will remains
            // after the objet destruction

	block_compressor(const block_compressor & ref) = delete;
	block_compressor(block_compressor && ref) noexcept = delete;
	block_compressor & operator = (const block_compressor & ref) = delete;
	block_compressor & operator = (block_compressor && ref) noexcept = delete;
	~block_compressor();

	    // inherited from proto_compressor

	virtual compression get_algo() const override { return suspended? compression::none : zipper->get_algo(); };
	virtual void suspend_compression() override;
	virtual void resume_compression() override;
	virtual bool is_compression_suspended() const override { return suspended; };

	    // inherited from generic file

	virtual bool skippable(skippability direction, const infinint & amount) override;
        virtual bool skip(const infinint & pos) override;
        virtual bool skip_to_eof() override;
        virtual bool skip_relative(S_I x) override;
	virtual bool truncatable(const infinint & pos) const override;
        virtual infinint get_position() const override;

    protected :
	virtual void inherited_read_ahead(const infinint & amount) override { compressed->read_ahead(amount); };
        virtual U_I inherited_read(char *a, U_I size) override;
        virtual void inherited_write(const char *a, U_I size) override;
	virtual void inherited_truncate(const infinint & pos) override;
	virtual void inherited_sync_write() override;
	virtual void inherited_flush_read() override { if(get_mode() != gf_write_only) current->reset(); };
	virtual void inherited_terminate() override;

    private:
	static constexpr const U_I min_uncompressed_block_size = 100;


	    // the local fields

	std::unique_ptr<compress_module> zipper;  ///< compress_module for the requested compression algo
	generic_file *compressed;                 ///< where to read from / write to, compressed data
	U_I uncompressed_block_size;              ///< the max block size of un compressed data used
	bool suspended;                           ///< whether compression is suspended or not
	bool need_eof;                            ///< whether a zero size block need to be added
	std::unique_ptr<crypto_segment> current;  ///< current block under construction or exploitation
	bool reof;                                ///< whether we have hit the end of file while reading

	    // private methods

	void compress_and_write_current();
	void read_and_uncompress_current();

    };

	/// @}

} // end of namespace


#endif
