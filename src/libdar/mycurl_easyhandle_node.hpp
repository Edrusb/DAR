/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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

    /// \file mycurl_easyhandle_node.hpp
    /// \brief used to optimize session creation done by libcurl
    /// \ingroup Private

#ifndef MYCURL_EASYHANDLE_NODE_HPP
#define MYCURL_EASYHANDLE_NODE_HPP

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

namespace libdar
{
        /// \addtogroup Private
	/// @{

#if LIBCURL_AVAILABLE


	/// structure managing libcurl CURL* easy_handle

    class mycurl_easyhandle_node
    {
    public:
	    /// create a new easyhandle
	mycurl_easyhandle_node();

	    /// copy constructor
	mycurl_easyhandle_node(const mycurl_easyhandle_node & ref);

	    /// move constructor
	mycurl_easyhandle_node(mycurl_easyhandle_node && ref) noexcept;

	    /// assignment operator
	mycurl_easyhandle_node & operator = (const mycurl_easyhandle_node & ref) = delete;

	    /// move operator
	mycurl_easyhandle_node & operator = (mycurl_easyhandle_node && ref) noexcept = delete;

	    /// destructor
	~mycurl_easyhandle_node() { if(handle != nullptr) curl_easy_cleanup(handle); };

	void set_used_mode(bool mode) { used = mode; };
	bool get_used_mode() const { return used; };

	CURL *get_handle() const { return handle; };

    private:
	CURL *handle;
	bool used;
    };

#endif

   	/// @}

} // end of namespace

#endif
