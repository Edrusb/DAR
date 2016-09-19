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

#include "../my_config.h"

extern "C"
{

} // end extern "C"

#include "tools.hpp"
#include "erreurs.hpp"
#include "fichier_libcurl.hpp"

using namespace std;

#ifdef LIBDAR_NO_OPTIMIZATION
#define CURLDEBUG 1
#else
#define CURLDEBUG 0
#endif

namespace libdar
{

    fichier_libcurl::fichier_libcurl(user_interaction & dialog,
				     const std::string & chemin,
				     CURL *ref_handle,
				     gf_mode m,
				     bool force_permission,
				     U_I permission,
				     bool erase): fichier_global(dialog, m),
						  easyhandle(nullptr),
						  multihandle(nullptr),
						  multimode(false),
						  current_offset(0),
						  has_maxpos(false),
						  maxpos(0),
						  inbuf(0),
						  eof(false),
						  append_write(false),
						  easy_inbuf(0),
						  ptr_tampon(nullptr),
						  ptr_inbuf(nullptr)
    {
	CURLcode err;

	try
	{

		// setting x_ref_handle to carry all options that will always be present for this object

	    easyhandle = curl_easy_duphandle(ref_handle);
	    if(easyhandle == nullptr)
		throw Erange("fichier_libcurl::fichier_libcurl", string(gettext("libcurl_easy_duphandle() failed")));

	    err = curl_easy_setopt(easyhandle, CURLOPT_URL, chemin.c_str());
	    if(err != CURLE_OK)
		throw Erange("entrepot_libcurl::entreport_libcurl",
			     tools_printf(gettext("Error met while resetting URL to handle: %s"),
					  curl_easy_strerror(err)));

	    err = curl_easy_setopt(easyhandle, CURLOPT_VERBOSE, CURLDEBUG);
	    if(err != CURLE_OK)
		throw Erange("entrepot_libcurl::entrepot_libcurl",
			     tools_printf(gettext("Error met while setting verbosity on handle: %s"),
					  curl_easy_strerror(err)));

	    switch(get_mode())
	    {
	    case gf_read_only:
		try
		{
			// yes, reading data from libcurl means for libcurl to write data to the application

		    err = curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, write_data_callback);
		    if(err != CURLE_OK)
			throw Erange("","");
		    err = curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, this);
		    if(err != CURLE_OK)
			throw Erange("","");
		}
		catch(Erange & e)
		{
		    throw Erange("entrepot_libcurl::handle_reset",
				 tools_printf(gettext("Error met while setting libcurl for reading data file: %s"),
					      curl_easy_strerror(err)));
		}
		break;
	    case gf_write_only:
		try
		{
		    err = curl_easy_setopt(easyhandle, CURLOPT_READFUNCTION, read_data_callback);
		    if(err != CURLE_OK)
			throw Erange("","");
		    err = curl_easy_setopt(easyhandle, CURLOPT_READDATA, this);
		    if(err != CURLE_OK)
			throw Erange("","");
		    err = curl_easy_setopt(easyhandle, CURLOPT_UPLOAD, 1L);
		    if(err != CURLE_OK)
			throw Erange("","");
			// should the CURLOPT_INFILESIZE_LARGE option but file size is not known at this time
		}
		catch(Erange & e)
		{
		    throw Erange("entrepot_libcurl::handle_reset",
				 tools_printf(gettext("Error met while setting libcurl for writing data file: %s"),
					      curl_easy_strerror(err)));
		}
		break;
	    case gf_read_write:
		throw Efeature("libcurl does not support read/write mode, either read-only or write-only");
	    default:
		throw SRC_BUG;
	    }

	    multihandle = curl_multi_init();
	    if(multihandle == nullptr)
		throw Erange("entrepot_libcur::handle_reset", string(gettext("multhandle initialization failed")));
	    switch_to_multi(true);
	}
	catch(...)
	{
	    detruit();
	    throw;
	}
    }

    void fichier_libcurl::change_permission(U_I perm)
    {
	CURLcode err;
	struct curl_slist *headers = NULL;
	string order = tools_printf("site CHMOD %o", perm);

	try
	{
	    try
	    {
		switch_to_multi(false);
		headers = curl_slist_append(headers, order.c_str());
		err = curl_easy_setopt(easyhandle, CURLOPT_QUOTE, headers);
		if(err != CURLE_OK)
		    throw Erange("","");
		err = curl_easy_setopt(easyhandle, CURLOPT_NOBODY, 1);
		if(err != CURLE_OK)
		    throw Erange("","");
		err = curl_easy_perform(easyhandle);
		if(err != CURLE_OK)
		    throw Erange("","");
	    }
	    catch(Erange & e)
	    {
		throw Erange("fichier_libcurl::change_permission",
			     tools_printf(gettext("Error while changing file permission on remote repository: %s"), curl_easy_strerror(err)));
	    }
	}
	catch(...)
	{
	    (void)curl_easy_setopt(easyhandle, CURLOPT_QUOTE, nullptr);
	    (void)curl_easy_setopt(easyhandle, CURLOPT_NOBODY, 0);
	    switch_to_multi(true);
	    if(headers != nullptr)
		curl_slist_free_all(headers);
	    throw;
	}

	(void)curl_easy_setopt(easyhandle, CURLOPT_QUOTE, nullptr);
	(void)curl_easy_setopt(easyhandle, CURLOPT_NOBODY, 0);
	switch_to_multi(true);
	if(headers != nullptr)
	    curl_slist_free_all(headers);
    }

    infinint fichier_libcurl::get_size() const
    {
	CURLcode err;
	double filesize;
	fichier_libcurl *me = const_cast<fichier_libcurl *>(this);

	if(me == nullptr)
	    throw SRC_BUG;

	if(!has_maxpos || get_mode() != gf_read_only)
	{
	    try
	    {
		me->switch_to_multi(false);
		err = curl_easy_setopt(easyhandle, CURLOPT_NOBODY, 1);
		if(err != CURLE_OK)
		    throw Erange("","");
		err = curl_easy_perform(easyhandle);
		if(err != CURLE_OK)
		    throw Erange("","");
		err = curl_easy_getinfo(easyhandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
		if(err != CURLE_OK)
		    throw Erange("","");
		me->maxpos = tools_double2infinint(filesize);
		me->has_maxpos = true;
	    }
	    catch(Erange & e)
	    {
		(void)curl_easy_setopt(easyhandle, CURLOPT_NOBODY, 0);
		me->switch_to_multi(true);
		throw Erange("fichier_libcurl::change_permission",
			     tools_printf(gettext("Error while reading file size on remote repository: %s"), curl_easy_strerror(err)));
	    }
	    (void)curl_easy_setopt(easyhandle, CURLOPT_NOBODY, 0);
	}
	me->switch_to_multi(true);

	return maxpos;
    }

    bool fichier_libcurl::skippable(skippability direction, const infinint & amount)
    {
	switch(direction)
	{
	case skip_backward:
	    return amount <= current_offset;
	case skip_forward:
	    if(!has_maxpos)
		(void)get_size();
	    if(!has_maxpos)
		throw SRC_BUG;
	    return current_offset + amount < maxpos;
	default:
	    throw SRC_BUG;
	}
    }

    bool fichier_libcurl::skip(const infinint & pos)
    {
	if(get_mode() != gf_read_only)
	    sync_write();

	switch(get_mode())
	{
	case gf_read_only:
	    switch_to_multi(false);
	    try
	    {
		current_offset = pos;
		if(get_mode() == gf_write_only && has_maxpos && maxpos < pos)
		    maxpos = pos;
		if(get_mode() != gf_write_only)
		    flush_read();
	    }
	    catch(...)
	    {
		switch_to_multi(true);
		throw;
	    }
	    switch_to_multi(true);
	    break;
	case gf_write_only:
	    if(pos != current_offset)
	    {
		append_write = true;
		(void)get_size();
		    // get_size() calls switch_to_multi(true) which sets the CURLOPT_ option to append when append_write is set
		if(!has_maxpos)
		    throw SRC_BUG;
		if(pos != maxpos)
		    throw Erange("fichier_libcurl::skip", string(gettext("libcurl does not allow skipping in write mode, except to end of file (append mode)")));
	    }
	    break;
	case gf_read_write:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}

	return true;
    }

    bool fichier_libcurl::skip_to_eof()
    {
	(void)get_size();
	if(!has_maxpos)
	    throw SRC_BUG; // get_size() sould either throw an exception or set maxpos
	return skip(maxpos);
    }

    bool fichier_libcurl::skip_relative(S_I x)
    {
	if(x >= 0)
	{
	    infinint tmp(x);
	    tmp += current_offset;
	    return skip(tmp);
	}
	else
	{
	    infinint tmp(-x);

	    if(tmp > current_offset)
	    {
		skip(0);
		return false;
	    }
	    else
	    {
		tmp = current_offset - tmp;
		return skip(tmp);
	    }
	}
    }

    void fichier_libcurl::inherited_read_ahead(const infinint & amount)
    {
	throw Efeature("fichier_libcurl::inherited_read_ahead");
    }

    void fichier_libcurl::inherited_sync_write()
    {
	CURLMcode errm = CURLM_OK;
	int running = 1;

	if(!multimode)
	    throw SRC_BUG;

	while((inbuf > 0 || eof) && running == 1)
	{
	    errm = curl_multi_perform(multihandle, &running);
	    if(errm != CURLM_OK)
		throw Erange("fichier_libcurl::fichier_global_inherited_write",
			     tools_printf(gettext("Error while completing data writing to remote repository: %s"), curl_multi_strerror(errm)));
	}

	if(inbuf > 0)
	    throw SRC_BUG; // multi has finished but data remain in transit
    }

    void fichier_libcurl::inherited_flush_read() { inbuf = 0; }

    void fichier_libcurl::inherited_terminate()
    {
	char tmp;

	switch(get_mode())
	{
	case gf_write_only:
	    eof = true;
	    fichier_global_inherited_write(&tmp, 0);
	    sync_write();
	    break;
	case gf_read_only:
	    break;
	case gf_read_write:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}
    }

    U_I fichier_libcurl::fichier_global_inherited_write(const char *a, U_I size)
    {
	U_I wrote = 0;
	CURLMcode errm = CURLM_OK;
	int running = 1;

	if(!multimode)
	    throw SRC_BUG;

	while((wrote < size || inbuf > 0 || eof)
	      && (errm == CURLM_OK || errm == CURLM_CALL_MULTI_PERFORM)
	      && running > 0)
	{
	    if(wrote < size)
	    {
		U_I room = tampon_size - inbuf;
		U_I toadd = size - wrote;
		U_I xfer = toadd > room ? room : toadd;
		memcpy(tampon + inbuf, a + wrote, xfer);
		wrote += xfer;
		inbuf += xfer;
	    }
	    errm = curl_multi_perform(multihandle, &running);
	}

	if(errm != CURLM_OK)
	    throw Erange("fichier_libcurl::fichier_global_inherited_write",
			 tools_printf(gettext("Error while writing data to remote repository: %s"), curl_multi_strerror(errm)));
	if(wrote < size)
	    throw SRC_BUG; // curl has finished transfer but data remain in pipe
	current_offset += wrote;

	return wrote;
    }

    bool fichier_libcurl::fichier_global_inherited_read(char *a, U_I size, U_I & read, std::string & message)
    {
	CURLMcode errm = CURLM_OK;
	int running = 1;

	if(!multimode)
	    throw SRC_BUG;

	read = 0;
	while(read < size
	      && (!eof || inbuf > 0)
	      && (errm == CURLM_OK || errm == CURLM_CALL_MULTI_PERFORM))
	{
	    if(inbuf > 0)
	    {
		U_I room = size - read;
		U_I xfer = room > inbuf ? inbuf : room;
		(void)memcpy(a + read, tampon, xfer);
		read += xfer;
		if(inbuf > xfer)
		{
		    inbuf -= xfer; // xfer is not greater than inbuf by construction
		    memmove(tampon, tampon + xfer, inbuf);
		}
		else
		    inbuf = 0;
	    }
	    else // inbuf == 0
	    {
		errm = curl_multi_perform(multihandle, &running);
		switch(errm)
		{
		case CURLM_OK:
		    check_info_after_multi_perform();
		    if(running == 0)
			eof = true;
		    break;
		default:
		    check_info_after_multi_perform();
		    throw Erange("fichier_libcurl::fichier_global_inherited_read",
				 tools_printf(gettext("Error wile reading data from repote repository: %s"),
					      curl_multi_strerror(errm)));
		}
	    }
	}

		// checking the status of the multihandle

	if(errm != CURLM_OK && errm != CURLM_CALL_MULTI_PERFORM)
	    throw Erange("fichier_libcurl::fichier_global_inherited_read",
			 tools_printf(gettext("Error while reading data from remote repository: %s"), curl_multi_strerror(errm)));

	if(read < size && !eof)
	    throw SRC_BUG; // we should not return before having provided the requested data amount
	current_offset += read;

	return true;
    }

    void fichier_libcurl::switch_to_multi(bool mode)
    {
	static const char *err_msg_add = gettext("Error while adding an easyahandle to a multihandle");
	static const char *err_msg_rem = gettext("Error while removing an easyhandle from a multihandle");
	const char *msgptr = nullptr;
	CURLMcode errm = CURLM_OK;

	if(mode == multimode)
	    return;

	if(multihandle == nullptr)
	    throw SRC_BUG;
	if(easyhandle == nullptr)
	    throw SRC_BUG;

	if(mode) // multi mode
	{

		// setting the offset of the next byte to read / write

	    CURLcode erre;
	    infinint resume;
	    infinint iinbuf = inbuf;
	    curl_off_t cur_pos = 0;
	    bool action = false;

	    switch(get_mode())
	    {
	    case gf_read_only:
		resume = current_offset + iinbuf;
		action = true;
		break;
	    case gf_write_only:
		resume = maxpos;
		action = append_write;
		break;
	    case gf_read_write:
		throw SRC_BUG;
	    default:
		throw SRC_BUG;
	    }

	    if(action)
	    {
		resume.unstack(cur_pos);
		if(!resume.is_zero())
		    throw Erange("fichier_libcurl::switch_to_multi",
				 gettext("Integer too large for libcurl, cannot skip at the requested offset in the remote repository"));
		erre = curl_easy_setopt(easyhandle, CURLOPT_RESUME_FROM_LARGE, cur_pos);
		if(erre != CURLE_OK)
		    throw Erange("fichier_libcurl::fichier_global_inherited_write",
				 tools_printf(gettext("Error while seeking in file on remote repository: %s"), curl_easy_strerror(erre)));
	    }

		// now moving to multi mode

	    errm = curl_multi_add_handle(multihandle, easyhandle);
	    msgptr = err_msg_add;
	    ptr_tampon = tampon;
	    ptr_inbuf = &inbuf;
	}
	else // easy mode
	{
	    ptr_tampon = easy_tampon;
	    ptr_inbuf = &easy_inbuf;
	    easy_inbuf = 0; // we don't care about existing data remaining in easy_tampon
	    errm = curl_multi_remove_handle(multihandle, easyhandle);
	    msgptr = err_msg_rem;
	}

	if(errm != CURLM_OK)
	    throw Erange("entrepot_libcur::switch_to_multi", tools_printf("%s: %s",
									  msgptr,
									  curl_multi_strerror(errm)));
	multimode = mode;
    }

    void fichier_libcurl::copy_from(const fichier_libcurl & ref)
    {
	if(ref.easyhandle == nullptr)
	    easyhandle = nullptr;
	else
	    easyhandle = curl_easy_duphandle(ref.easyhandle);

	if(ref.multihandle != nullptr)
	{
	    multimode = false;
	    ptr_tampon = easy_tampon;
	    ptr_inbuf = & easy_inbuf;
	    multihandle = curl_multi_init();
	    if(multihandle == nullptr)
		throw Erange("entrepot_libcur::handle_reset", string(gettext("multhandle initialization failed")));
	    if(ref.multimode)
	    {
		if(easyhandle == nullptr)
		    throw SRC_BUG;
		else
		    switch_to_multi(true);
	    }
	}
	else
	{
	    multihandle = nullptr;
	    multimode = ref.multimode;
	}

	current_offset = ref.current_offset;
	has_maxpos = ref.has_maxpos;
	maxpos = ref.maxpos;
	memcpy(tampon, ref.tampon, ref.inbuf);
	inbuf = ref.inbuf;
	eof = ref.eof;
	easy_inbuf = 0; // don't care data in easy_tampon
    }

    void fichier_libcurl::copy_parent_from(const fichier_libcurl & ref)
    {
	fichier_global *me = this;
	const fichier_global *you = &ref;
	*me = *you;
    }

    void fichier_libcurl::check_info_after_multi_perform()
    {
	string msg;
	int msgs_in_queue;
	CURLMsg *ptr = nullptr;
	bool err = false;

	while((ptr = curl_multi_info_read(multihandle, &msgs_in_queue)) != nullptr)
	{
	    if(ptr->easy_handle != easyhandle)
		throw SRC_BUG;
	    if(ptr->msg != CURLMSG_DONE)
		throw SRC_BUG;
	    if(ptr->data.result != CURLE_OK)
	    {
		err = true;
		if(msg.size() == 0)
		    msg = curl_easy_strerror(ptr->data.result);
		else
		    msg += string(" ; ") + curl_easy_strerror(ptr->data.result);
	    }
	}

	if(err)
	    throw Erange("fichier_libcurl::check_info_after_multi_perform", tools_printf(gettext("Error met at the end of file transfer: %S"), &msg));
    }

    void fichier_libcurl::detruit()
    {
	if(get_mode() == gf_write_only)
	{
	    eof = true;
	    sync_write();
	}
	switch_to_multi(false);
	if(easyhandle != nullptr)
	{
	    curl_easy_cleanup(easyhandle);
	    easyhandle = nullptr;
	}

	if(multihandle != nullptr)
	{
	    curl_multi_cleanup(multihandle);
	    multihandle = nullptr;
	}
    }

    size_t fichier_libcurl::write_data_callback(char *buffer, size_t size, size_t nmemb, void *userp)
    {
	size_t ret;
	size_t amount = size * nmemb;
	fichier_libcurl *me = (fichier_libcurl *)(userp);

	if(amount + *(me->ptr_inbuf) > tampon_size)
	    if(me->ptr_inbuf == 0)
		throw SRC_BUG; // buffer is too short to receive this data
	    else
		ret = CURL_WRITEFUNC_PAUSE;
	else // enough room to store data in tampon
	{
	    (void)memcpy(me->ptr_tampon + *(me->ptr_inbuf), buffer, amount);
	    ret = amount;
	    *(me->ptr_inbuf) += amount;
	}

	return ret;
    }

    size_t fichier_libcurl::read_data_callback(char *bufptr, size_t size, size_t nitems, void *userp)
    {
	size_t ret;
	size_t room = size * nitems;
	fichier_libcurl *me = (fichier_libcurl *)(userp);
	size_t xfer = room > *(me->ptr_inbuf) ? *(me->ptr_inbuf) : room;

	if(xfer > 0)
	{
	    memcpy(bufptr, me->ptr_tampon, xfer);
	    if(*(me->ptr_inbuf) > xfer)
	    {
		*(me->ptr_inbuf) -= xfer;
		memmove(me->ptr_tampon, me->ptr_tampon + xfer, *(me->ptr_inbuf));
	    }
	    else
		*(me->ptr_inbuf) = 0;
	    ret = xfer;
	}
	else
	    if(me->eof)
		ret = 0;
	    else
		ret = CURL_READFUNC_PAUSE;

	return ret;
    }

} // end of namespace
