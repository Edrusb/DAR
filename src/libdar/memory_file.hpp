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

    class memory_file : public generic_file
    {
    public:

	    /// Constructors & Destructor
	memory_file() : generic_file(gf_read_write), data(0) { position = 0; };
	memory_file(const memory_file & ref) = default;
	memory_file & operator = (const memory_file & ref) = default;
	~memory_file() = default;

	    // memory_storage specific methods

	void reset() { if(is_terminated()) throw SRC_BUG; position = 0; data = storage(0); };
	infinint size() const { return data.size(); };


	    // virtual method inherited from generic_file
	bool skippable(skippability direction, const infinint & amount) { return true; };
	bool skip(const infinint & pos);
	bool skip_to_eof();
	bool skip_relative(S_I x);
	infinint get_position() const { if(is_terminated()) throw SRC_BUG; return position; };


    protected:

	    // virtual method inherited from generic_file
	void inherited_read_ahead(const infinint & amount) {}; // no optimization can be done here, we rely on the OS here
	U_I inherited_read(char *a, U_I size);
	void inherited_write(const char *a, U_I size);
	void inherited_sync_write() {};
	void inherited_flush_read() {};
	void inherited_terminate() {};

    private:
	storage data;
	infinint position;
    };

	/// @}

} // end of namespace

#endif
