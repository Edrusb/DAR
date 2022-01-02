/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
// to contact the author, see the AUTHOR file
/*********************************************************************/

    /// \file mycurl_easyhandle_sharing.hpp
    /// \brief used to optimize network session establised by libcurl
    /// \ingroup Private

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

#include <deque>
#include "erreurs.hpp"
#include "mycurl_easyhandle_node.hpp"

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
	mycurl_easyhandle_sharing(const mycurl_easyhandle_sharing & ref): global_params(ref.global_params) { table.clear(); };
	mycurl_easyhandle_sharing(mycurl_easyhandle_sharing && ref) noexcept: table(std::move(ref.table)), global_params(std::move(ref.global_params)) {};
	mycurl_easyhandle_sharing & operator = (const mycurl_easyhandle_sharing & ref) = delete;
	mycurl_easyhandle_sharing & operator = (mycurl_easyhandle_sharing && ref) noexcept = delete;
	~mycurl_easyhandle_sharing() = default;


	    /// global options are applied to all mycurl_easyhandle_node provided by this object
	template<class T> void setopt_global(CURLoption opt, const T & val) { global_params.add(opt, val); }

	    /// obtain a libcurl handle wrapped in the adhoc classes to allow copy and cloning

	    /// \note once no more needed reset() the shared_ptr to have this object recycled
	std::shared_ptr<mycurl_easyhandle_node> alloc_instance();


    private:
	std::deque<std::shared_ptr<mycurl_easyhandle_node> > table;
	mycurl_param_list global_params;
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
