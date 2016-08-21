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

namespace libdar
{

    fichier_libcurl::fichier_libcurl(user_interaction & dialog,
				     const std::string & chemin,
				     CURL *ref_handle,
				     gf_mode m,
				     bool force_permission,
				     U_I permission,
				     bool erase): fichier_global(dialog, m),
						  x_ref_handle(ref_handle),
						  easyhandle(nullptr),
						  current_offset(0),
						  has_maxpos(false),
						  maxpos(0),
						  inbuf(0),
						  eof(false)
    {
	CURLcode err;

	try
	{

		// setting x_ref_handle to carry all options that will always be present for this object

	    err = curl_easy_setopt(x_ref_handle, CURLOPT_URL, chemin.c_str());
	    if(err != CURLE_OK)
		throw Erange("","");

	    switch(get_mode())
	    {
	    case gf_read_only:
		err = curl_easy_setopt(x_ref_handle, CURLOPT_WRITEFUNCTION, write_data_callback);
		if(err != CURLE_OK)
		    throw Erange("","");
		err = curl_easy_setopt(x_ref_handle, CURLOPT_WRITEDATA, this);
		if(err != CURLE_OK)
		    throw Erange("","");
		break;
	    case gf_write_only:
		err = curl_easy_setopt(x_ref_handle, CURLOPT_READFUNCTION, read_data_callback);
		if(err != CURLE_OK)
		    throw Erange("","");
		err = curl_easy_setopt(x_ref_handle, CURLOPT_READDATA, this);
		if(err != CURLE_OK)
		    throw Erange("","");
		err = curl_easy_setopt(x_ref_handle, CURLOPT_UPLOAD, 1L);
		if(err != CURLE_OK)
		    throw Erange("","");
		    // should the the CURLOPT_INFILESIZE_LARGE option but file size is not known at this time
		break;
	    case gf_read_write:
		throw Efeature("libcurl does not support read/write mode, either read-only or write-only");
	    default:
		throw SRC_BUG;
	    }

		// setting easyhandle from x_ref_handle

	    try
	    {
		reset_handle();
	    }
	    catch(...)
	    {
		clean();
		throw;
	    }
	}
	catch(Erange & e)
	{
	    throw Erange("entrepot_libcurl::handle_reset", string(gettext("Error met while resetting libcurl handle")));
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
	    reset_handle();
	    if(headers != nullptr)
		curl_slist_free_all(headers);
	    throw;
	}
	reset_handle();
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

	if(!has_maxpos)
	{
	    try
	    {
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
		me->reset_handle();
	    }
	    catch(Erange & e)
	    {
		me->reset_handle();
		throw Erange("fichier_libcurl::change_permission",
			     tools_printf(gettext("Error while reading file size on remote repository: %s"), curl_easy_strerror(err)));
	    }
	}

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
	CURLcode err;
	infinint tmp(pos);
	curl_off_t from = 0;

	if(get_mode() != gf_read_only)
	    sync_write();

	tmp.unstack(from);
	if(!tmp.is_zero())
	    throw Erange("fichier_libcurl::skip",
			 gettext("File too large for skipping at the requested offset in the remote repository"));
	try
	{
	    err = curl_easy_setopt(easyhandle, CURLOPT_RESUME_FROM_LARGE, from);
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
	    reset_handle();
	    throw Erange("fichier_libcurl::fichier_global_inherited_write",
			 tools_printf(gettext("Error while seeking in file on remote repository: %s"), curl_easy_strerror(err)));
	}
	reset_handle();

	current_offset = pos;
	if(get_mode() == gf_write_only && has_maxpos && maxpos < pos)
	    maxpos = pos;
	if(get_mode() != gf_write_only)
	    flush_read();

	return true;
    }

    bool fichier_libcurl::skip_to_eof()
    {
	(void)get_size();
	if(!has_maxpos)
	    throw SRC_BUG; // get_size() would either throw an exception or set maxpos
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
		return false;
	    tmp = current_offset - tmp;
	    return skip(tmp);
	}
    }

    void fichier_libcurl::inherited_read_ahead(const infinint & amount)
    {
	(void)curl_easy_perform(easyhandle);
    }

    void fichier_libcurl::inherited_sync_write()
    {
	eof = true;
	if(inbuf > 0)
	{
	    CURLcode err = curl_easy_perform(easyhandle);
	    if(err != CURLE_OK)
		throw Erange("fichier_libcurl::fichier_global_inherited_write",
			     tools_printf(gettext("Error while completing data writing to remote repository: %s"), curl_easy_strerror(err)));
	    if(inbuf > 0)
		throw SRC_BUG;
	}
    }

    void fichier_libcurl::inherited_flush_read() { inbuf = 0; }

    void fichier_libcurl::inherited_terminate()
    {
	switch(get_mode())
	{
	case gf_write_only:
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
	CURLcode err = CURLE_OK;

	while(wrote < size && err == CURLE_OK)
	{
	    U_I room = CURL_MAX_WRITE_SIZE - inbuf;
	    U_I xfer = size > room ? room : size;
	    memcpy(tampon, a, xfer);
	    wrote += xfer;
	    inbuf += xfer;
	    err = curl_easy_perform(easyhandle);
	}

	if(err != CURLE_OK)
	    throw Erange("fichier_libcurl::fichier_global_inherited_write",
			 tools_printf(gettext("Error while writing data to remote repository: %s"), curl_easy_strerror(err)));

	return wrote;
    }

    bool fichier_libcurl::fichier_global_inherited_read(char *a, U_I size, U_I & read, std::string & message)
    {
	CURLcode err = CURLE_OK;
	read = 0;

	while(read < size && !eof && err == CURLE_OK)
	{
	    if(inbuf > 0)
	    {
		U_I room = size - read;
		U_I xfer = room > inbuf ? inbuf : room;
		(void)memcpy(a + read, tampon, xfer);
		read += xfer;
		if(inbuf > xfer)
		    memmove(tampon, tampon + xfer, inbuf - xfer);
		inbuf -= xfer; // xfer is not greater than inbuf by construction
	    }
	    else // inbuf == 0
	    {
		err = curl_easy_perform(easyhandle);
		if(err == CURLE_OK && inbuf == 0)
		    eof = true;
	    }
	}

	if(err != CURLE_OK)
	    throw Erange("fichier_libcurl::fichier_global_inherited_read",
			 tools_printf(gettext("Error while reading data from remote repository: %s"), curl_easy_strerror(err)));

	return true;
    }

    void fichier_libcurl::reset_handle()
    {
	clean();
	easyhandle = curl_easy_duphandle(x_ref_handle);
    }

    void fichier_libcurl::copy_from(const fichier_libcurl & ref)
    {
	if(ref.x_ref_handle == nullptr)
	    x_ref_handle = nullptr;
	else
	    x_ref_handle = curl_easy_duphandle(ref.x_ref_handle);
	reset_handle();
	current_offset = ref.current_offset;
	has_maxpos = ref.has_maxpos;
	maxpos = ref.maxpos;
	memcpy(tampon, ref.tampon, ref.inbuf);
	inbuf = ref.inbuf;
	eof = ref.eof;
    }

    void fichier_libcurl::copy_parent_from(const fichier_libcurl & ref)
    {
	fichier_global *me = this;
	const fichier_global *you = &ref;
	*me = *you;
    }

    void fichier_libcurl::clean()
    {
	if(easyhandle != nullptr)
	{
	    curl_easy_cleanup(easyhandle);
	    easyhandle = nullptr;
	}
    }

    void fichier_libcurl::detruit()
    {
	clean();
	if(x_ref_handle != nullptr)
	{
	    curl_easy_cleanup(x_ref_handle);
	    x_ref_handle = nullptr;
	}
    }

    size_t fichier_libcurl::write_data_callback(char *buffer, size_t size, size_t nmemb, void *userp)
    {
	size_t ret;
	size_t amount = size * nmemb;
	fichier_libcurl *me = (fichier_libcurl *)(userp);

	if(amount + me->inbuf > CURL_MAX_WRITE_SIZE)
	    if(me->inbuf == 0)
		throw SRC_BUG; // buffer is too short to receive this data
	    else
		ret = CURL_WRITEFUNC_PAUSE;
	else // enough room to store data in tampon
	{
	    (void)memcpy(me->tampon + me->inbuf, buffer, amount);
	    ret = amount;
	    me->inbuf += amount;
	}

	return ret;
    }

    size_t fichier_libcurl::read_data_callback(char *bufptr, size_t size, size_t nitems, void *userp)
    {
	size_t ret;
	size_t room = size * nitems;
	fichier_libcurl *me = (fichier_libcurl *)(userp);
	size_t xfer = room > me->inbuf ? me->inbuf : room;

	if(xfer > 0)
	{
	    memcpy(bufptr, me->tampon, xfer);
	    if(me->inbuf > xfer)
		memmove(me->tampon, me->tampon + xfer, me->inbuf - xfer);
	    me->inbuf -= xfer;
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
