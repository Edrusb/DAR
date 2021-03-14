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

    /// \file mycurl_shared_handle.hpp
    /// \brief structure used to optimize network session creation done by libcurl
    /// \ingroup Private

#ifndef MYCURL_SHARED_HANDLE_HPP
#define MYCURL_SHARED_HANDLE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include <string>
#include "erreurs.hpp"
#include "smart_pointer.hpp"
#include "mycurl_easyhandle_node.hpp"

namespace libdar
{
        /// \addtogroup Private
	/// @{

#if LIBCURL_AVAILABLE


	/// structure returned to fichier_libcurl by entrepot_libcurl for libcurl easyhandle access

	/// structure used to share an easyhandle between a fichier_libcurl and entrepot_libcurl
	/// in order to recycle it instead of duphandle it at new fichier_libcurl creation
	/// \note once created a mycurl_shared_handle can only be moved, never copied nor duplicated

    class mycurl_shared_handle
    {
    public:
	    /// constructor
	mycurl_shared_handle(const smart_pointer<mycurl_easyhandle_node> & node);

	    /// no copy constructor
	mycurl_shared_handle(const mycurl_shared_handle & ref) = delete;

	    /// only a move constructor
	mycurl_shared_handle(mycurl_shared_handle && arg) noexcept;

	    /// no assignment operator
	mycurl_shared_handle & operator = (const mycurl_shared_handle & ref) = delete;

	    /// only move assignment one
	mycurl_shared_handle & operator = (mycurl_shared_handle && arg) noexcept;

	    /// destructor
	~mycurl_shared_handle() { if(!ref.is_null()) ref->set_used_mode(false); };

	    /// get access to the managed CURL easy handle
	CURL *get_handle() const { return ref->get_handle(); };

    private:
	smart_pointer<mycurl_easyhandle_node> ref;

    };

#endif

   	/// @}

} // end of namespace

#endif
