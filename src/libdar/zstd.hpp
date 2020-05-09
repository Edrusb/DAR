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

    /// \file zstd.hpp
    /// \brief libzstd compression interraction
    /// \ingroup Private

#ifndef ZSTD_HPP
#define ZSTD_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_ZSTD_H
#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#endif
}

#include "gf_mode.hpp"
#include "integers.hpp"
#include "generic_file.hpp"

namespace libdar
{


	/// \addtogroup Private
	/// @{

    class zstd
    {
    public:
	zstd(gf_mode x_mode,
	     U_I compression_level,
	     generic_file* x_compressed, //< this object is not owned by zstd but must survive the zstd object
	     U_I workers = 1);

	zstd(const zstd & ref) = delete;
	zstd(zstd && ref) = delete;
	zstd & operator = (const zstd & ref) = delete;
	zstd & operator = (zstd && ref) = delete;
	~zstd();

	U_I read(char *a, U_I size);
	void write(const char *a, U_I size);
	void write_eof_and_flush();
	void reset();

    private:
#if LIBZSTD_AVAILABLE
	ZSTD_CStream *comp;
	ZSTD_DStream *decomp;

	ZSTD_inBuffer inbuf;
	ZSTD_outBuffer outbuf;
	char *below_tampon; // used to hold in-transit data
	U_I below_tampon_size; // allocated size of tampon
	U_I above_tampon_size; // max size of input data

	gf_mode mode;
	generic_file *compressed;
	bool eof;

	static bool check_range(S_I value, const ZSTD_bounds & range);
#endif

	void clear_inbuf();
	void clear_outbuf();
	void release_mem();
	void setup_context(gf_mode mode, U_I compression_level, U_I workers);
    };

	/// @}

} // end of namespace

#endif
