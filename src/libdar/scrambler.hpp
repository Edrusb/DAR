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

    /// \file scrambler.hpp
    /// \brief contains the definition of the scrambler class, a very weak encryption scheme
    /// \ingroup Private

#ifndef SCRAMBLER_HPP
#define SCRAMBLER_HPP

#include "../my_config.h"
#include <string>
#include "infinint.hpp"
#include "generic_file.hpp"
#include "erreurs.hpp"
#include "secu_string.hpp"

namespace libdar
{

	/// \addtogroup Private
        /// @{


	/// \brief scrambler is a very weak encryption scheme

    class scrambler : public generic_file
    {
    public:
        scrambler(const secu_string & pass, generic_file & hidden_side);
	scrambler(const scrambler & ref) = delete;
	scrambler(scrambler && ref) noexcept = delete;
	scrambler & operator = (const scrambler & ref) = delete;
	scrambler & operator = (scrambler && ref) noexcept = delete;
        ~scrambler() { if(buffer != nullptr) delete [] buffer; };


	virtual bool skippable(skippability direction, const infinint & amount) override { if(ref == nullptr) throw SRC_BUG; return ref->skippable(direction, amount); };
        virtual bool skip(const infinint & pos) override { if(ref == nullptr) throw SRC_BUG; return ref->skip(pos); };
        virtual bool skip_to_eof() override { if(ref == nullptr) throw SRC_BUG; return ref->skip_to_eof(); };
        virtual bool skip_relative(S_I x) override { if(ref == nullptr) throw SRC_BUG; return ref->skip_relative(x); };
	virtual bool truncatable(const infinint & pos) const override { if(ref == nullptr) throw SRC_BUG; return ref->truncatable(pos); };
        virtual infinint get_position() const override { if(ref == nullptr) throw SRC_BUG; return ref->get_position(); };

    protected:
	virtual void inherited_read_ahead(const infinint & amount) override { if(ref == nullptr) throw SRC_BUG; ref->read_ahead(amount); };
        virtual U_I inherited_read(char *a, U_I size) override;
        virtual void inherited_write(const char *a, U_I size) override;
	virtual	void inherited_truncate(const infinint & pos) override { if(ref == nullptr) throw SRC_BUG; ref->truncate(pos); };
	virtual void inherited_sync_write() override {}; // nothing to do
	virtual void inherited_flush_read() override {}; // nothing to do
	virtual void inherited_terminate() override {};  // nothing to do

    private:
        secu_string key;
        U_I len;
        generic_file *ref;
        unsigned char *buffer;
        U_I buf_size;
    };

	/// @}

} // end of namespace

#endif
