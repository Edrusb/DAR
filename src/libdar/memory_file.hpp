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
// $Id: memory_file.hpp,v 1.12.2.1 2012/02/19 17:25:09 edrusb Exp $
//
/*********************************************************************/

    /// \file memory_file.hpp
    /// \brief Memory_file is a generic_file class that only uses virtual memory    /// \ingroup Private
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

	    // Constructors & Destructor
	    //
	    //

	memory_file(gf_mode m) : generic_file(m), data(0) { position = 0; };

	    // virtual method inherited from generic_file
	    //
	    //

	bool skip(const infinint & pos);
	bool skip_to_eof();
	bool skip_relative(S_I x);
	infinint get_position() { if(is_terminated()) throw SRC_BUG; return position; };
	void reset() { if(is_terminated()) throw SRC_BUG; position = 0; data = storage(0); };

	    // raw access to stored data
	    //
	    //

	    /// returns the size taken by the object's raw data
	infinint get_data_size() const { if(is_terminated()) throw SRC_BUG; return data.size(); };

	    /// return the raw data
	const storage & get_raw_data() const { if(is_terminated()) throw SRC_BUG; return data; };
	void set_raw_data(const storage & val) { if(is_terminated()) throw SRC_BUG; data = val; position = 0; };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(memory_file);
#endif

    protected:
	U_I inherited_read(char *a, U_I size);
	void inherited_write(const char *a, U_I size);
	void inherited_sync_write() {};
	void inherited_terminate() {};

    private:
	storage data;
	infinint position;
    };

	/// @}

} // end of namespace

#endif
