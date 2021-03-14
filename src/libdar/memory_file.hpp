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

    /// \file memory_file.hpp
    /// \brief Memory_file is a generic_file class that only uses virtual memory
    /// \ingroup Private

#ifndef MEMORY_FILE_HPP
#define MEMORY_FILE_HPP

#include "generic_file.hpp"
#include "storage.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// generic_file stored in memory

    class memory_file : public generic_file
    {
    public:

	    /// Constructors & Destructor
	memory_file() : generic_file(gf_read_write), data(0) { position = 0; };
	memory_file(const memory_file & ref) = default;
	memory_file(memory_file && ref) noexcept = default;
	memory_file & operator = (const memory_file & ref) = default;
	memory_file & operator = (memory_file && ref) noexcept = default;
	~memory_file() = default;

	    // memory_storage specific methods

	void reset() { if(is_terminated()) throw SRC_BUG; position = 0; data = storage(0); };
	infinint size() const { return data.size(); };


	    // virtual method inherited from generic_file
	virtual bool skippable(skippability direction, const infinint & amount) override { return true; };
	virtual bool skip(const infinint & pos) override;
	virtual bool skip_to_eof() override;
	virtual bool skip_relative(S_I x) override;
	virtual bool truncatable(const infinint & pos) const override { return true; };
	virtual infinint get_position() const  override { if(is_terminated()) throw SRC_BUG; return position; };


    protected:

	    // virtual method inherited from generic_file
	virtual void inherited_read_ahead(const infinint & amount) override {}; // no optimization can be done here, we rely on the OS here
	virtual U_I inherited_read(char *a, U_I size) override;
	virtual void inherited_write(const char *a, U_I size) override;
	virtual void inherited_truncate(const infinint & pos) override;
	virtual void inherited_sync_write() override {};
	virtual void inherited_flush_read() override {};
	virtual void inherited_terminate() override {};

    private:
	storage data;
	infinint position;
    };

	/// @}

} // end of namespace

#endif
