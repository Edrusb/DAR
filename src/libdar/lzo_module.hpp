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
// to contact the author, see the AUTHOR file
/*********************************************************************/

    /// \file lzo_module.hpp
    /// \brief per block encryption using LZO algorithm/library
    /// \ingroup Private
    ///

#ifndef LZO_MODULE_HPP
#define LZO_MODULE_HPP

extern "C"
{
}

#include "../my_config.h"
#include <string>
#include <memory>

#include "compress_module.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class lzo_module: public compress_module
    {
    public:
	lzo_module(compression algo, U_I compression_level = 9) { init(algo, compression_level); };
	lzo_module(const lzo_module & ref) { init(ref.lzo_algo, ref.level); };
	lzo_module(lzo_module && ref) noexcept = default;
	lzo_module & operator = (const lzo_module & ref) { init(ref.lzo_algo, ref.level); return *this; };
	lzo_module & operator = (lzo_module && ref) noexcept = default;
	virtual ~lzo_module() noexcept = default;

	    // inherited from compress_module interface

	virtual compression get_algo() const override { return compression::lzo; };

	virtual U_I get_max_compressing_size() const override;

	virtual U_I get_min_size_to_compress(U_I clear_size) const override;

	virtual U_I compress_data(const char *normal,
				   const U_I normal_size,
				   char *zip_buf,
				   U_I zip_buf_size) const override;

	virtual U_I uncompress_data(const char *zip_buf,
				     const U_I zip_buf_size,
				     char *normal,
				     U_I normal_size) const override;


	virtual std::unique_ptr<compress_module> clone() const override;

    private:
	compression lzo_algo;
	U_I level;
	std::unique_ptr<char[]> wrkmem_decompr;
	std::unique_ptr<char[]> wrkmem_compr;

	void init(compression algo, U_I compression_level);

    };
	/// @}

} // end of namespace

#endif
