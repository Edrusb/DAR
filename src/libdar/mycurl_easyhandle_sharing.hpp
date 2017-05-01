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

#ifdef LIBTHREADAR_AVAILABLE
#include <libthreadar/libthreadar.hpp>
#endif

#include "smart_pointer.hpp"

namespace libdar
{
        /// \addtogroup Private
	/// @{

#if LIBCURL_AVAILABLE && LIBTHREADAR_AVAILABLE


	/// structure managing libcurl CURL* easy_handle

    class mycurl_easyhandle_node
    {
    public:
	mycurl_easyhandle_node();
	mycurl_easyhandle_node(const mycurl_easyhandle_node & ref);
	    // assignment operator made private and forbidden
	~mycurl_easyhandle_node() { curl_easy_cleanup(handle); };

	void set_used_mode(bool mode) { used = mode; };
	bool get_used_mode() const { return used; };

	CURL *get_handle() const { return handle; };

    private:
	CURL *handle;
	bool used;

	const mycurl_easyhandle_node & operator = (const mycurl_easyhandle_node & ref) { throw SRC_BUG; };
    };

	/// return to fichier_libcurl by entrepot_libcurl
	///
	/// structure used to share an easyhandle between a fichier_libcurl and entrepot_libcurl
	/// in order to recycle it instead of duphandle it at new fichier_libcurl creation

    class shared_handle
    {
    public:
	shared_handle(const smart_pointer<mycurl_easyhandle_node> & node) : ref(node)
	{
	    if(node.is_null())
		throw SRC_BUG;
	    if(node->get_used_mode())
		throw SRC_BUG;
	    ref->set_used_mode(true);
	}
	    // default copy constructor is allowed
	~shared_handle() { ref->set_used_mode(false); };

	CURL *get_handle() const { return ref->get_handle(); };

    private:
	smart_pointer<mycurl_easyhandle_node> ref;

	const shared_handle & operator = (const shared_handle & re) { throw SRC_BUG; };
    };


	/// manage libcurl easy_handle and its duplications

    class mycurl_easyhandle_sharing
    {
    public:
	mycurl_easyhandle_sharing() {};
	mycurl_easyhandle_sharing(const mycurl_easyhandle_sharing & ref): root(ref.root) { clone_table.clear(); };
	    // assignement operator made privater and forbidden
	    // default destructor OK

	CURL *get_root_handle() const { return root.get_handle(); };

	shared_handle alloc_instance() const;

    private:
	mycurl_easyhandle_node root;
	std::list<smart_pointer<mycurl_easyhandle_node> > clone_table;

	const mycurl_easyhandle_sharing & operator = (const mycurl_easyhandle_sharing & ref) { throw SRC_BUG; };
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
