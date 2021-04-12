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

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif
}

#include "lz4_module.hpp"
#include "tools.hpp"

#include <utility>

using namespace std;

namespace libdar
{

    lz4_module::lz4_module(U_I compression_level)
    {
#if LIBLZ4_AVAILABLE
	if(compression_level > 9 || compression_level < 1)
	    throw Erange("lz4_module::lz4_module", tools_printf(gettext("out of range LZ4 compression level: %d"), compression_level));
	acceleration = 10 - compression_level;

	state = new (nothrow) char[LZ4_sizeofState()];
	if(state == nullptr)
	    throw Ememory("lz4_module::lz4_module");
#else
	throw Ecompilation(gettext("lz4 compression"));
#endif
    }

    lz4_module::lz4_module(const lz4_module & ref)
    {
#if LIBLZ4_AVAILABLE
	state = new(nothrow) char[LZ4_sizeofState()];
	if(state == nullptr)
	    throw Ememory("lz4_module::lz4_module");
	    // no need to copy the content of state

	acceleration = ref.acceleration;
#else
	throw Ecompilation(gettext("lz4 compression"));
#endif
    }

    lz4_module::lz4_module(lz4_module && ref) noexcept
    {
	state = nullptr;
	swap(state, ref.state);
	acceleration = move(ref.acceleration);
    }

    lz4_module & lz4_module::operator = (const lz4_module & ref)
    {
	acceleration = ref.acceleration;
	    // the content of the *state* field does not
	    // matter, it is only used during
	    // the LZ4_compress_fast_extState()
	    // and its content is not used as input
	    // for this function.
	return *this;
    }

    lz4_module & lz4_module::operator = (lz4_module && ref) noexcept
    {
	acceleration = move(ref.acceleration);
	swap(state, ref.state);

	return *this;
    }

    lz4_module::~lz4_module() noexcept
    {
	if(state != nullptr)
	    delete [] state;
    }

    U_I lz4_module::get_max_compressing_size() const
    {
#if LIBLZ4_AVAILABLE
	return LZ4_MAX_INPUT_SIZE;
#else
	throw Ecompilation(gettext("lz4 compression"));
#endif
    }

    U_I lz4_module::get_min_size_to_compress(U_I clear_size) const
    {
#if LIBLZ4_AVAILABLE
	if(clear_size > get_max_compressing_size() || clear_size < 1)
	    throw Erange("lz4_module::get_min_size_to_compress", "out of range block size submitted to lz4_module::get_min_size_to_compress");

	return LZ4_compressBound(clear_size);
#else
	throw Ecompilation(gettext("lz4 compression"));
#endif
    }


    U_I lz4_module::compress_data(const char *normal,
				  const U_I normal_size,
				  char *zip_buf,
				  U_I zip_buf_size) const

    {
#if LIBLZ4_AVAILABLE
	S_I ret;

	if(normal_size > get_max_compressing_size())
	    throw Erange("lz4_module::compress_data", "oversized uncompressed data given to LZ4 compression engine");

	ret = LZ4_compress_fast_extState((void *)state,
					 normal,
					 zip_buf,
					 normal_size,
					 zip_buf_size,
					 acceleration);
	if(ret <= 0)
	    throw Erange("lz4_module::compress_data", "undersized compressed buffer given to LZ4 compression engine");

	return ret;
#else
	throw Ecompilation(gettext("lz4 compression"));
#endif
    }

    U_I lz4_module::uncompress_data(const char *zip_buf,
				    const U_I zip_buf_size,
				    char *normal,
				    U_I normal_size) const
    {
#if LIBLZ4_AVAILABLE
	S_I ret = LZ4_decompress_safe(zip_buf, normal, zip_buf_size, normal_size);

	if(ret < 0)
	    throw Edata(gettext("corrupted compressed data met"));

	return ret;
#else
	throw Ecompilation(gettext("lz4 compression"));
#endif
    }


    unique_ptr<compress_module> lz4_module::clone() const
    {
#if LIBLZ4_AVAILABLE
	try
	{
	    return std::make_unique<lz4_module>(*this);
	}
	catch(bad_alloc &)
	{
	    throw Ememory("lz4_module::clone");
	}
#else
	throw Ecompilation(gettext("lz4 compression"));
#endif
    }


} // end of namespace
