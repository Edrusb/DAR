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

    /// \file archive_listing_callback.hpp
    /// \brief definition of the user defined callback function used for archive listing
    /// \ingroup API

#include "list_entry.hpp"

#ifndef ARCHIVE_LISTING_CALLBACK_HPP
#define ARCHIVE_LISTING_CALLBACK_HPP

#include "../my_config.h"

namespace libdar
{

	/// callback function type expected for archive::op_listing and archive::get_children_of()
    using archive_listing_callback = void (*)(const std::string & the_path,
					      const list_entry & entry,
					      void *context);

} // end of namespace

#endif
