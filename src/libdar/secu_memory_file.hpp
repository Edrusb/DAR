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

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class secu_memory_file : public generic_file
    {
    public:

	    /// Constructors & Destructor
	secu_memory_file(U_I size, bool randomize) : generic_file(gf_read_write), data(size) { position = 0; if(randomize) data.randomize(size); };


	    // memory_storage specific methods

	void reset(U_I size) { if(is_terminated()) throw SRC_BUG; position = 0; data.resize(size); };
	infinint size() const { return data.get_size(); };


	    // virtual method inherited from generic_file
	bool skippable(skippability direction, const infinint & amount) { return true; };
	bool skip(const infinint & pos);
	bool skip_to_eof();
	bool skip_relative(S_I x);
	infinint get_position() const { if(is_terminated()) throw SRC_BUG; return position; };

	const secu_string & get_contents() const { return data; };

    protected:

	    // virtual method inherited from generic_file
	void inherited_read_ahead(const infinint & amount) {};
	U_I inherited_read(char *a, U_I size);
	void inherited_write(const char *a, U_I size);
	void inherited_sync_write() {};
	void inherited_flush_read() {};
	void inherited_terminate() {};

    private:
	secu_string data;
	U_I position;
    };

	/// @}

} // end of namespace

#endif
