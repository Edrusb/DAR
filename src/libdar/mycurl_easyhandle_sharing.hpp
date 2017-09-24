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

#ifndef MYCURL_EASYHANDLE_SHARING_HPP
#define MYCURL_EASYHANDLE_SHARING_HPP

#include "../my_config.h"

extern "C"
{
#if LIBCURL_AVAILABLE
#if HAVE_CURL_CURL_H
#include <curl/curl.h>
#endif
#endif
} // end extern "C"

#include <string>
#include "erreurs.hpp"
#include "mycurl_shared_handle.hpp"

namespace libdar
{
        /// \addtogroup Private
	/// @{

#if LIBCURL_AVAILABLE

	/// manage libcurl easy_handle and its duplications

    class mycurl_easyhandle_sharing
    {
    public:
	mycurl_easyhandle_sharing() = default;
	mycurl_easyhandle_sharing(const mycurl_easyhandle_sharing & ref): root(ref.root) { clone_table.clear(); };
	mycurl_easyhandle_sharing & operator = (const mycurl_easyhandle_sharing & ref) = delete;
	~mycurl_easyhandle_sharing() = default;

	CURL *get_root_handle() const { return root.get_handle(); };

	mycurl_shared_handle alloc_instance() const;

    private:
	mycurl_easyhandle_node root;
	std::list<smart_pointer<mycurl_easyhandle_node> > clone_table;
    };

#else

    class mycurl_easyhandle_sharing
    {
    public:
	mycurl_easyhandle_sharing() { throw Ecompilation("remote repository"); };
    };

#endif

   	/// @}

} // end of namespace

#endif
