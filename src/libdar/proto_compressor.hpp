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

    /// \file proto_compressor.hpp
    /// \brief abstracted ancestor class for compressor and parallel_compressor classes
    /// \ingroup Private

#ifndef PROTO_COMPRESSOR_HPP
#define PROTO_COMPRESSOR_HPP

#include "../my_config.h"

#include "infinint.hpp"
#include "generic_file.hpp"
#include "compression.hpp"

namespace libdar
{


	/// \addtogroup Private
	/// @{

    class proto_compressor : public generic_file
    {
    public :
	using generic_file::generic_file;

	proto_compressor(const proto_compressor & ref) = default;
	proto_compressor(proto_compressor && ref) noexcept = default;
	proto_compressor & operator = (const proto_compressor & ref) = default;
	proto_compressor & operator = (proto_compressor && ref) noexcept = default;
        virtual ~proto_compressor() = default;

        virtual compression get_algo() const = 0;
	virtual void suspend_compression() = 0;
	virtual void resume_compression() = 0;
	virtual bool is_compression_suspended() const = 0;
    };

	/// @}

} // end of namespace

#endif