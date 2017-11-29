/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2052 Denis Corbin
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file secu_memory_file.hpp
    /// \brief secu_memory_file is a generic_file class that only uses secured memory (not swappable and zeroed after use)
    /// \ingroup Private

#ifndef SECU_MEMORY_FILE_HPP
#define SECU_MEMORY_FILE_HPP

#include "generic_file.hpp"
#include "storage.hpp"
#include "secu_string.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class secu_memory_file : public generic_file
    {
    public:

	    // Constructors & Destructor
	secu_memory_file(U_I storage_size) : generic_file(gf_read_write), data(storage_size) { position = 0; };
	secu_memory_file(const secu_memory_file & ref) = default;
	secu_memory_file(secu_memory_file && ref) noexcept = default;
	secu_memory_file & operator = (const secu_memory_file & ref) = default;
	secu_memory_file & operator = (secu_memory_file && ref) noexcept = default;
	~secu_memory_file() = default;

	    // memory_storage specific methods

	    /// reset the storage size and empty object content
	void reset(U_I size) { if(is_terminated()) throw SRC_BUG; position = 0; data.resize(size); };

	    /// the size of the data in the object
	infinint get_size() const { return data.get_size(); };

	    /// the allocated size of the object
	infinint get_allocated_size() const { return data.get_allocated_size(); };

	    /// set the content to a random string of size bytes
	void randomize(U_I size) { if(size > data.get_allocated_size()) reset(size); data.randomize(size); };


	    // virtual method inherited from generic_file
	virtual bool skippable(skippability direction, const infinint & amount) override { return true; };
	virtual bool skip(const infinint & pos) override;
	virtual bool skip_to_eof() override;
	virtual bool skip_relative(S_I x) override;
	virtual infinint get_position() const override { if(is_terminated()) throw SRC_BUG; return position; };

	const secu_string & get_contents() const { return data; };

    protected:

	    // virtual method inherited from generic_file
	virtual void inherited_read_ahead(const infinint & amount) override {};
	virtual U_I inherited_read(char *a, U_I size) override;
	virtual void inherited_write(const char *a, U_I size) override;
	virtual void inherited_sync_write() override {};
	virtual void inherited_flush_read() override {};
	virtual void inherited_terminate() override {};

    private:
	secu_string data;
	U_I position;
    };

	/// @}

} // end of namespace

#endif
