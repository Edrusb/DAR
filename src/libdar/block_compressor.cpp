/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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

extern "C"
{
}

#include "block_compressor.hpp"
#include "erreurs.hpp"
#include "compress_block_header.hpp"

using namespace std;

namespace libdar
{


    block_compressor::block_compressor(unique_ptr<compress_module> block_zipper,
							 generic_file & compressed_side,
							 U_I uncompressed_bs):
	proto_compressor((compressed_side.get_mode() == gf_read_only)? gf_read_only: gf_write_only),
	zipper(move(block_zipper)),
	compressed(&compressed_side),
	uncompressed_block_size(uncompressed_bs)
    {
	U_I compr_bs = zipper->get_min_size_to_compress(uncompressed_block_size);

	    // sanity checks on fields set by constructors

	if(get_mode() == gf_read_write)
	    throw SRC_BUG; // mode not suported for this type of object
	if(!zipper)
	    throw SRC_BUG;
	if(compressed == nullptr)
	    throw SRC_BUG;
	if(uncompressed_block_size < min_uncompressed_block_size)
	    throw SRC_BUG;


	    // initializing simple fields not set by constructors

	suspended = false;
	need_eof = false;
	current = make_unique<crypto_segment>(compr_bs, uncompressed_block_size);
	reof = false;
    }


    block_compressor::~block_compressor()
    {
	try
	{
	    terminate();
	}
	catch(...)
	{
		// ignore all exceptions
	}
    }

    void block_compressor::suspend_compression()
    {
	if(!suspended)
	{
	    inherited_sync_write();
	    inherited_flush_read();
	    suspended = true;
	}
    }

    void block_compressor::resume_compression()
    {
	suspended = false;
    }

    bool block_compressor::skippable(skippability direction, const infinint & amount)
    {
	if(is_terminated())
	    throw SRC_BUG;

	return compressed->skippable(direction, amount);
    }

    bool block_compressor::skip(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

	current->reset();
	reof = false;
	need_eof = false;
	return compressed->skip(pos);
    }

    bool block_compressor::skip_to_eof()
    {
	if(is_terminated())
	    throw SRC_BUG;

	current->reset();
	reof = false;
	need_eof = false;
	return compressed->skip_to_eof();
    }

    bool block_compressor::skip_relative(S_I x)
    {
	if(is_terminated())
	    throw SRC_BUG;

	current->reset();
	reof = false;
	need_eof = false;
	return skip_relative(x);
    }

    bool block_compressor::truncatable(const infinint & pos) const
    {
	return compressed->truncatable(pos);
    }


    infinint block_compressor::get_position() const
    {
	switch(get_mode())
	{
	case gf_read_only:
	    if(!current->clear_data.all_is_read())
		throw SRC_BUG;
	    break;
	case gf_write_only:
	    if(!current->clear_data.is_empty())
		throw SRC_BUG;
	    break;
	case gf_read_write:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}

	return compressed->get_position();
    }


    U_I block_compressor::inherited_read(char *a, U_I size)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(suspended)
	{
	    if(!reof)
		return compressed->read(a, size);
	    else
		return 0;
	}
	else
	{
	    U_I ret = 0;

	    while(ret < size && ! reof)
	    {
		if(current->clear_data.all_is_read())
		    read_and_uncompress_current();

		ret += current->clear_data.read(a + ret, size - ret);
	    }

	    return ret;
	}
    }

    void block_compressor::inherited_write(const char *a, U_I size)
    {
	U_I wrote = 0;

	if(is_terminated())
	    throw SRC_BUG;

	if(suspended)
	{
	    compressed->write(a, size);
	}
	else
	{
	    while(wrote < size)
	    {
		wrote += current->clear_data.write(a + wrote, size - wrote);
		if(current->clear_data.is_full())
		    compress_and_write_current();
	    }
	    if(size > 0)
		need_eof = true;
	}
    }

    void block_compressor::inherited_truncate(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

	compressed->truncate(pos);
    }

    void block_compressor::inherited_sync_write()
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(get_mode() == gf_read_only)
	    return;

	compress_and_write_current();
	if(need_eof)
	{
	    compress_block_header bh;

	    bh.type = compress_block_header::H_EOF;
	    bh.size = 0;
	    bh.dump(*compressed); // adding EOF null block size
	    need_eof = false;
	}
    }

    void block_compressor::inherited_terminate()
    {
	switch(get_mode())
	{
	case gf_read_only:
	    break;
	case gf_write_only:
	    inherited_sync_write();
	    break;
	case gf_read_write:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}
    }

    void block_compressor::compress_and_write_current()
    {
	compress_block_header bh;

	if(current->clear_data.get_data_size() > 0)
	{
	    current->crypted_data.set_data_size(zipper->compress_data(current->clear_data.get_addr(),
								      current->clear_data.get_data_size(),
								      current->crypted_data.get_addr(),
								      current->crypted_data.get_max_size()));
	    if(current->crypted_data.get_data_size() > 0)
	    {
		bh.type = compress_block_header::H_DATA;
		bh.size = current->crypted_data.get_data_size();
	    }
	    else
		throw SRC_BUG;
		// it is not expected that compressed data, that to say some information
		// get compressed as a result of no information at all

	    try
	    {
		bh.dump(*compressed);
		compressed->write(current->crypted_data.get_addr(), current->crypted_data.get_data_size());
		current->reset();
	    }
	    catch(Ethread_cancel & e)
	    {
		current->reset();
		throw;
	    }
	    catch(Euser_abort & e)
	    {
		current->reset();
		throw;
	    }
	}
    }

    void block_compressor::read_and_uncompress_current()
    {
	U_I bs;
	compress_block_header bh;

	if(!bh.set_from(*compressed))
	{
	    current->reset();
	    reof = true;
	    return;
	}

	switch(bh.type)
	{
	case compress_block_header::H_DATA:
	    bs = 0;
	    bh.size.unstack(bs);
	    if(!bh.size.is_zero())
		throw Erange("zip_below_read::work", gettext("incoherent compressed block structure, compressed data corruption"));

	    if(bs > current->crypted_data.get_max_size())
		throw Erange("zip_below_read::work", gettext("incoherent compressed block structure, compressed block size in archive too large"));

	    if(bs > 0)
	    {
		current->crypted_data.set_data_size(compressed->read(current->crypted_data.get_addr(), bs));
		current->clear_data.set_data_size(zipper->uncompress_data(current->crypted_data.get_addr(),
									  current->crypted_data.get_data_size(),
									  current->clear_data.get_addr(),
									  current->clear_data.get_max_size()));
		current->clear_data.rewind_read();
	    }
	    else
	    	throw Erange("zip_below_read::work", gettext("incoherent compressed block structure, compressed data corruption"));
	    break;
	case compress_block_header::H_EOF:
	    if(!bh.size.is_zero())
		throw Erange("zip_below_read::work", gettext("incoherent compressed block structure, compressed data corruption"));
	    current->reset();
	    reof = true;
	    break;
	default:
	    throw Erange("zip_below_read::work", gettext("incoherent compressed block structure, compressed data corruption"));
	}
    }


} // end of namespace

