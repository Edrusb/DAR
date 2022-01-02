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

    /// \file mycurl_slist.hpp
    /// \brief wrapper of the curl_slist struct to allow usual copy/move on C++ object
    /// \ingroup Private

#ifndef MYCURL_SLIST_H
#define MYCURL_SLIST_H

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
#include <deque>

namespace libdar
{
        /// \addtogroup Private
	/// @{

#if LIBCURL_AVAILABLE


	/// wrapper class around curl_slist

    class mycurl_slist
    {
    public:
	mycurl_slist() { header = nullptr; };
	mycurl_slist(const mycurl_slist & ref): appended(ref.appended) { header = rebuild(appended); };
	mycurl_slist(mycurl_slist && ref) noexcept: appended(std::move(ref.appended)) { header = ref.header; ref.header = nullptr; };
	mycurl_slist & operator = (const mycurl_slist & ref) { release(header); appended = ref.appended; header = rebuild(appended); return *this; };
	mycurl_slist & operator = (mycurl_slist && ref) noexcept { std::swap(header, ref.header); std::swap(appended, ref.appended); return *this; };
	~mycurl_slist() { release(header); };

	bool operator == (const mycurl_slist & ref) const;
	bool operator != (const mycurl_slist & ref) const { return ! (*this == ref); };

	void append(const std::string & s);
	const curl_slist *get_address() const { return header; };
	void clear() { release(header); appended.clear(); };
	bool empty() const { return appended.empty(); };

    private:
	struct curl_slist* header;
	std::deque<std::string> appended;

	static curl_slist* rebuild(const std::deque<std::string> & ap);
	static void release(curl_slist* & ptr) { curl_slist_free_all(ptr); ptr = nullptr; }
    };

#endif

   	/// @}

} // end of namespace

#endif
