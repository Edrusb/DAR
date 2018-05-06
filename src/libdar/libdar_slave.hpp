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

    /// \file libdar_slave.hpp
    /// \brief API for dar_slave functionnality
    /// \ingroup API

#ifndef LIBDAR_SLAVE_HPP
#define LIBDAR_SLAVE_HPP

#include "../my_config.h"
#include "entrepot_local.hpp"
#include "slave_zapette.hpp"
#include "infinint.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

    class libdar_slave
    {
    public:
	libdar_slave(std::shared_ptr<user_interaction> & dialog,
		     const std::string & folder,
		     const std::string & basename,
		     const std::string & extension,
		     const std::string & input_pipe,
		     const std::string & output_pipe,
		     const std::string & execute,
		     const infinint & min_digits);
	libdar_slave(const libdar_slave & ref) = delete;
	libdar_slave(libdar_slave && ref) noexcept = default;
	libdar_slave & operator = (const libdar_slave & ref) = delete;
	libdar_slave & operator = (libdar_slave && ref) noexcept = default;
	~libdar_slave() { zap.reset(); entrep.reset(); }; // the order is important

	void run() { zap->action(); };

    private:
	std::shared_ptr<entrepot_local> entrep; // must be deleted last
	std::unique_ptr<slave_zapette> zap; // must be deleted first
    };

	/// @}

} // end of namespace

#endif
