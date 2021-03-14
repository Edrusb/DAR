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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file trontextual.hpp
    /// \brief class trontextual is a contextual variant of class tronc
    /// \ingroup Private

#ifndef TRONTEXTUAL_HPP
#define TRONTEXTUAL_HPP

#include "../my_config.h"

#include "tronc.hpp"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "contextual.hpp"

namespace libdar
{

        /// \addtogroup Private
        /// @{

	/// trontextual class is a contextual class tronc, that's all.

    class trontextual : public tronc, public contextual
    {
    public:
	trontextual(generic_file *f, const infinint & offset, const infinint & size, bool own_f = false);
	trontextual(generic_file *f, const infinint & offset, const infinint & size, gf_mode mode, bool own_f = false);
	trontextual(const trontextual & ref) = delete;
	trontextual(trontextual && ref) noexcept = delete;
	trontextual & operator = (const trontextual & ref) = delete;
	trontextual & operator = (trontextual && ref) = delete;
	~trontextual() = default;

	virtual bool is_an_old_start_end_archive() const override { if(ref == nullptr) throw SRC_BUG; return ref->is_an_old_start_end_archive(); };
	virtual const label & get_data_name() const override { if(ref == nullptr) throw SRC_BUG; return ref->get_data_name(); };

    private:
	contextual *ref;   ///< this is just a pointer to data owned by the inherited class tronc part of this object

	void init(generic_file *f);
    };

	/// @}
}

#endif
