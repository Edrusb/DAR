/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
#include <typeinfo>
#include "mycurl_param_list.hpp"
#include "user_interaction.hpp"
#include "mycurl_slist.hpp"
#include "tools.hpp"

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
	mycurl_easyhandle_node() { init(); };

	    /// copy constructor
	mycurl_easyhandle_node(const mycurl_easyhandle_node & ref);

	    /// move constructor
	mycurl_easyhandle_node(mycurl_easyhandle_node && ref) noexcept;

	    /// assignment operator
	mycurl_easyhandle_node & operator = (const mycurl_easyhandle_node & ref);

	    /// move operator
	mycurl_easyhandle_node & operator = (mycurl_easyhandle_node && ref) noexcept;

	    /// destructor
	~mycurl_easyhandle_node() { if(handle != nullptr) curl_easy_cleanup(handle); };

	    /// set options
	template<class T> void setopt(CURLoption opt, const T & val) { check_for_type(opt, val); wanted.add(opt, val); }

	    /// adds a set of options
	void setopt_list(const mycurl_param_list & listing) { (void) wanted.update_with(listing); };

	    /// set back to default
	void setopt_default(CURLoption opt);

	    /// set back all options to their default
	void setopt_all_default();

	    /// apply changed options since last call to apply, then execute curl_perform()
	void apply(const std::shared_ptr<user_interaction> & dialog,
		   U_I wait_seconds,
		   const bool & end_anyway = false);

	    /// get informations on the previous apply

	template<class T> void getinfo(CURLINFO info, T* val)
	{
	    CURLcode err = curl_easy_getinfo(handle, info, val);
	    if(err != CURLE_OK)
		throw Erange("mycurl_easyhandle_node::getinfo",
			     tools_printf(gettext("Error met while fetching info %d: %s"),
					  (S_I)info,
					  curl_easy_strerror(err)));
	}

	static void init_defaults(); // must be called once libgcrypt has been initialized (due to secu_string presence in the defaults)
	static void release_defaults() { defaults.clear(); }; // must be called before libgcrypt is cleaned up

    private:

	    ////////////////////////////////////
	    // object level fields and methods
	    //

	enum opttype
	{
	    type_string,
	    type_secu_string,
	    type_pointer,
	    type_long,
	    type_mycurl_slist,
	    type_curl_off_t,
	    eolist ///< this is not a type just for flagging the end of a list
	};

	CURL *handle;
	mycurl_param_list current;
	mycurl_param_list wanted;

	void init();
	template<class T>void set_to_default(CURLoption opt)
	{
	    const T* ptr;

	    if(current.get_val(opt, ptr))
	    {
		if(defaults.get_val(opt, ptr))
		    wanted.add(opt, *ptr);
		else
		    throw SRC_BUG;
	    }
	    else
		wanted.clear(opt);
	}


	    ////////////////////////////////////
	    // class level fields and methods
	    //

	struct opt_asso
	{
	    CURLoption opt;
	    opttype cast;
	};

	static constexpr const opt_asso association[] =
	{
	  { CURLOPT_APPEND, type_long },
	  { CURLOPT_DIRLISTONLY, type_long },
	  { CURLOPT_NETRC, type_long },
	  { CURLOPT_NOBODY, type_long },
	  { CURLOPT_SSH_KNOWNHOSTS, type_string },
	  { CURLOPT_SSH_PUBLIC_KEYFILE, type_string },
	  { CURLOPT_SSH_PRIVATE_KEYFILE, type_string },
	  { CURLOPT_SSH_AUTH_TYPES, type_long },
	  { CURLOPT_QUOTE, type_mycurl_slist },
	  { CURLOPT_RANGE, type_string },
	  { CURLOPT_READDATA, type_pointer },
	  { CURLOPT_READFUNCTION, type_pointer },
	  { CURLOPT_RESUME_FROM_LARGE, type_curl_off_t },
	  { CURLOPT_UPLOAD, type_long },
	  { CURLOPT_URL, type_string },
	  { CURLOPT_USERNAME, type_string },
	  { CURLOPT_USERPWD, type_secu_string },
	  { CURLOPT_VERBOSE, type_long },
	  { CURLOPT_WRITEDATA, type_pointer },
	  { CURLOPT_WRITEFUNCTION, type_pointer },
	  { CURLOPT_NEW_DIRECTORY_PERMS, type_long },
	  { CURLOPT_FTP_CREATE_MISSING_DIRS, type_long },
	      // eolist is needed to flag the end of list, option type does not matter
	  { CURLOPT_APPEND, eolist }
	};

	static opttype get_opt_type(CURLoption opt);

	static bool defaults_initialized;
	static mycurl_param_list defaults;

	template <class T> void check_for_type(CURLoption opt, const T & val)
	{
	    switch(get_opt_type(opt))
	    {
	    case type_string:
		if(typeid(val) != typeid(std::string))
		    throw SRC_BUG;
		break;
	    case type_secu_string:
		if(typeid(val) != typeid(secu_string))
		    throw SRC_BUG;
		break;
	    case type_pointer:
		if(typeid(val) != typeid(void *))
		    throw SRC_BUG;
		break;
	    case type_long:
		if(typeid(val) != typeid(long))
		    throw SRC_BUG;
		break;
	    case type_mycurl_slist:
		if(typeid(val) != typeid(mycurl_slist))
		    throw SRC_BUG;
		break;
	    case type_curl_off_t:
		if(typeid(val) != typeid(curl_off_t))
		    throw SRC_BUG;
		break;
	    case eolist:
		throw SRC_BUG;
	    default:
		throw SRC_BUG;
	    }
       	}
    };

#endif

   	/// @}

} // end of namespace

#endif
