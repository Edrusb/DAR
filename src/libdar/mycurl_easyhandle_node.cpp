//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

#include "../my_config.h"

#include "mycurl_easyhandle_node.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "secu_string.hpp"

using namespace std;

namespace libdar
{
#if LIBCURL_AVAILABLE


    	/// helper function to handle libcurl error code
	/// wait or throw an exception depending on error condition

	/// \param[in] dialog used to report the reason we are waiting for and how much time we wait
	/// \param[in] err is the curl easy code to examin
	/// \param[in] wait_seconds is the time to wait for recoverable error
	/// \param[in] err_context is the error context message use to prepend waiting message or exception throw
    static void fichier_libcurl_check_wait_or_throw(const std::shared_ptr<user_interaction> & dialog,
						    CURLcode err,
						    U_I wait_seconds,
						    const std::string & err_context);


    bool mycurl_easyhandle_node::defaults_initialized = false;
    mycurl_param_list mycurl_easyhandle_node::defaults;
    constexpr const mycurl_easyhandle_node::opt_asso mycurl_easyhandle_node::association[];


    mycurl_easyhandle_node::mycurl_easyhandle_node(const mycurl_easyhandle_node & ref)
    {
	    // we avoid curl_easy_duphandle to postpone the operation
	    // to the time apply() will be called
	init();
	wanted = ref.current;
	(void) wanted.update_with(ref.wanted);
    }

    mycurl_easyhandle_node::mycurl_easyhandle_node(mycurl_easyhandle_node && ref) noexcept
    {
	handle = ref.handle;
	ref.handle = nullptr;
	current = move(ref.current);
	wanted = move(ref.wanted);
    }

    mycurl_easyhandle_node & mycurl_easyhandle_node::operator = (const mycurl_easyhandle_node & ref)
    {
	    // handle is kept as is
	wanted = ref.current;
	(void) wanted.update_with(ref.wanted);
	return *this;
    }

    mycurl_easyhandle_node & mycurl_easyhandle_node::operator = (mycurl_easyhandle_node && ref) noexcept
    {
	    // this->handle is kept as is
	wanted = std::move(ref.current);
	(void) wanted.update_with(ref.wanted);
	return *this;
    }

    void mycurl_easyhandle_node::setopt_default(CURLoption opt)
    {
	switch(get_opt_type(opt))
	{
	case type_string:
	    set_to_default<string>(opt);
	    break;
	case type_secu_string:
	    set_to_default<secu_string>(opt);
	    break;
	case type_pointer:
	    set_to_default<void*>(opt);
	    break;
	case type_long:
	    set_to_default<long>(opt);
	    break;
	case type_mycurl_slist:
	    set_to_default<mycurl_slist>(opt);
	    break;
	case type_curl_off_t:
	    set_to_default<curl_off_t>(opt);
	    break;
	case eolist:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}
    }

    void mycurl_easyhandle_node::setopt_all_default()
    {
	const opt_asso *asso = association;

	while(asso != nullptr && asso->cast != eolist)
	{
	    setopt_default(asso->opt);
	    ++asso;
	}
    }


    void mycurl_easyhandle_node::apply(const std::shared_ptr<user_interaction> & dialog,
				       U_I wait_seconds,
				       const bool & end_anyway)
    {
	list<CURLoption> changed = current.update_with(wanted);
	list<CURLoption>::iterator it = changed.begin();
	CURLcode err = CURLE_OK;
	const string* t_string = nullptr;
	const secu_string *t_secu_string = nullptr;
	void * const * t_pointer = nullptr;
	const long* t_long = nullptr;
	const mycurl_slist* t_mycurl_slist = nullptr;
	const curl_off_t* t_curl_off_t = nullptr;

	while(it != changed.end())
	{
	    switch(get_opt_type(*it))
	    {
	    case type_string:
		if(!current.get_val(*it, t_string) || t_string == nullptr)
		    throw SRC_BUG;
		err = curl_easy_setopt(handle, *it, t_string->empty() ? nullptr : t_string->c_str());
		if(err != CURLE_OK)
		    throw Erange("mycurl_easyhandle_node::apply",
				 tools_printf(gettext("Error met while setting string option %d on libcurl handle: %s"),
					      *it,
					      curl_easy_strerror(err)));
		break;
	    case type_secu_string:
		if(!current.get_val(*it, t_secu_string) || t_secu_string == nullptr)
		    throw SRC_BUG;
		err = curl_easy_setopt(handle, *it, t_secu_string->empty() ? nullptr : t_secu_string->c_str());
		if(err != CURLE_OK)
		    throw Erange("mycurl_easyhandle_node::apply",
				 tools_printf(gettext("Error met while setting secu_string option %d on libcurl handle: %s"),
					      *it,
					      curl_easy_strerror(err)));
		break;
	    case type_pointer:
		if(!current.get_val(*it, t_pointer) || t_pointer == nullptr)
		    throw SRC_BUG;
		err = curl_easy_setopt(handle, *it, *t_pointer);
		if(err != CURLE_OK)
		    throw Erange("mycurl_easyhandle_node::apply",
				 tools_printf(gettext("Error met while setting pointer option %d on libcurl handle: %s"),
					      *it,
					      curl_easy_strerror(err)));
		break;
	    case type_long:
		if(!current.get_val(*it, t_long) || t_long == nullptr)
		    throw SRC_BUG;
		err = curl_easy_setopt(handle, *it, *t_long);
		if(err != CURLE_OK)
		    throw Erange("mycurl_easyhandle_node::apply",
				 tools_printf(gettext("Error met while setting long option %d on libcurl handle: %s"),
					      *it,
					      curl_easy_strerror(err)));
		break;
	    case type_mycurl_slist:
		if(!current.get_val(*it, t_mycurl_slist) || t_mycurl_slist == nullptr)
		    throw SRC_BUG;
		err = curl_easy_setopt(handle, *it, t_mycurl_slist->empty() ? nullptr: t_mycurl_slist->get_address());
		if(err != CURLE_OK)
		    throw Erange("mycurl_easyhandle_node::apply",
				 tools_printf(gettext("Error met while setting curl_slist option %d on libcurl handle: %s"),
					      *it,
					      curl_easy_strerror(err)));
		break;
	    case type_curl_off_t:
		if(!current.get_val(*it, t_curl_off_t) || t_curl_off_t == nullptr)
		    throw SRC_BUG;
		err = curl_easy_setopt(handle, *it, *t_curl_off_t);
		if(err != CURLE_OK)
		    throw Erange("mycurl_easyhandle_node::apply",
				 tools_printf(gettext("Error met while setting curl_off_t option %d on libcurl handle: %s"),
					      *it,
					      curl_easy_strerror(err)));
		break;
	    case eolist:
		throw SRC_BUG;
	    default:
		throw SRC_BUG;
	    }

	    ++it;
	}

	wanted.clear();
	do
	{
	    err = curl_easy_perform(handle);
	    if(!end_anyway)
		fichier_libcurl_check_wait_or_throw(dialog,
						    err,
						    wait_seconds,
						    "Error met while performing action on libcurl handle");
	}
	while(err != CURLE_OK && !end_anyway);
    }

    void mycurl_easyhandle_node::init()
    {
	handle = curl_easy_init();
	if(handle == nullptr)
	    throw Erange("mycurl_easyhandle_node::mycurl_easyhandle_node",
			 gettext("Error met while creating a libcurl handle"));
    }

    void mycurl_easyhandle_node::init_defaults()
    {
	if(!defaults_initialized)
	{
	    defaults_initialized = true;
	    string arg_string("");
	    secu_string arg_secu_string;
	    void *arg_ptr = nullptr;
	    long arg_long = 0;
	    mycurl_slist arg_mycurl_slist;
	    curl_off_t arg_curl_off_t = 0;

	    const opt_asso *cursor = association;

	    while(cursor->cast != eolist)
	    {
		switch(cursor->cast)
		{
		case type_string:
		    defaults.add(cursor->opt, arg_string); // a string (thus char*), a nullptr else
		    break;
		case type_secu_string:
		    defaults.add(cursor->opt, arg_secu_string);
		    break;
		case type_pointer:
		    defaults.add(cursor->opt, arg_ptr);
		    break;
		case type_long:
		    defaults.add(cursor->opt, arg_long);
		    break;
		case type_mycurl_slist:
		    defaults.add(cursor->opt, arg_mycurl_slist);
		    break;
		case type_curl_off_t:
		    defaults.add(cursor->opt, arg_curl_off_t);
		    break;
		case eolist:
		    throw SRC_BUG;
		default:
		    throw SRC_BUG;
		}

		++cursor;
	    }
	}
    }

    mycurl_easyhandle_node::opttype mycurl_easyhandle_node::get_opt_type(CURLoption opt)
    {
	const opt_asso *ptr = association;

	while(ptr->cast != eolist && ptr->opt != opt)
	    ++ptr;

	if(ptr->cast != eolist)
	    return ptr->cast;
	else
	    throw SRC_BUG; // unknown option type
    }


    static void fichier_libcurl_check_wait_or_throw(const shared_ptr<user_interaction> & dialog,
						    CURLcode err,
						    U_I wait_seconds,
						    const string & err_context)
    {
	switch(err)
	{
	case CURLE_OK:
	case CURLE_BAD_DOWNLOAD_RESUME:
	    break;
	case CURLE_REMOTE_DISK_FULL:
	case CURLE_UPLOAD_FAILED:
	    throw Edata(curl_easy_strerror(err));
	case CURLE_FTP_ACCEPT_FAILED:
	case CURLE_UNSUPPORTED_PROTOCOL:
	case CURLE_FAILED_INIT:
	case CURLE_URL_MALFORMAT:
	case CURLE_NOT_BUILT_IN:
	case CURLE_WRITE_ERROR:
	case CURLE_READ_ERROR:
	case CURLE_OUT_OF_MEMORY:
	case CURLE_RANGE_ERROR:
	case CURLE_FILE_COULDNT_READ_FILE:
	case CURLE_FUNCTION_NOT_FOUND:
	case CURLE_ABORTED_BY_CALLBACK:
	case CURLE_BAD_FUNCTION_ARGUMENT:
	case CURLE_TOO_MANY_REDIRECTS:
	case CURLE_UNKNOWN_OPTION:
	case CURLE_FILESIZE_EXCEEDED:
	case CURLE_REMOTE_FILE_EXISTS:
	case CURLE_REMOTE_FILE_NOT_FOUND:
	case CURLE_PARTIAL_FILE:
	case CURLE_QUOTE_ERROR:
	    throw Erange("entrepot_libcurl::check_wait_or_throw",
			 tools_printf(gettext("%S: %s, aborting"),
				      &err_context,
				      curl_easy_strerror(err)));
	case CURLE_LOGIN_DENIED:
	case CURLE_REMOTE_ACCESS_DENIED:
	case CURLE_PEER_FAILED_VERIFICATION:
	    throw Enet_auth(tools_printf(gettext("%S: %s, aborting"),
					 &err_context,
					 curl_easy_strerror(err)));
	case CURLE_COULDNT_RESOLVE_PROXY:
	case CURLE_COULDNT_RESOLVE_HOST:
	case CURLE_COULDNT_CONNECT:
	case CURLE_FTP_ACCEPT_TIMEOUT:
	case CURLE_FTP_CANT_GET_HOST:
	case CURLE_OPERATION_TIMEDOUT:
	case CURLE_SEND_ERROR:
	case CURLE_RECV_ERROR:
	case CURLE_AGAIN:
	default:
	    if(wait_seconds > 0)
	    {
		dialog->printf(gettext("%S: %s, retrying in %d seconds"),
			      &err_context,
			      curl_easy_strerror(err),
			      wait_seconds);
		sleep(wait_seconds);
	    }
	    else
		dialog->pause(tools_printf(gettext("%S: %s, do we retry network operation?"),
					   &err_context,
					   curl_easy_strerror(err)));
	    break;
	}
    }


#endif
} // end of namespace
