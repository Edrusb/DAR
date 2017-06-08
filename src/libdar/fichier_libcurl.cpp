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
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"

#include "tools.hpp"
#include "erreurs.hpp"
#include "fichier_libcurl.hpp"

using namespace std;

namespace libdar
{

#if LIBCURL_AVAILABLE

    fichier_libcurl::fichier_libcurl(user_interaction & dialog,
				     const std::string & chemin,
				     mycurl_protocol proto,
				     const shared_handle & handle,
				     gf_mode m,
				     U_I waiting,
				     bool force_permission,
				     U_I permission,
				     bool erase): fichier_global(dialog, m),
						  end_data_mode(false),
						  sub_is_dying(false),
						  ehandle(handle),
						  metadatamode(false),
						  current_offset(0),
						  has_maxpos(false),
						  maxpos(0),
						  append_write(!erase),
						  meta_inbuf(0),
						  wait_delay(waiting),
						  interthread(10, tampon_size),
						  synchronize(2),
						  x_proto(proto)
    {
	CURLcode err;

	try
	{

		// setting x_ref_handle to carry all options that will always be present for this object

	    err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_URL, chemin.c_str());
	    if(err != CURLE_OK)
		throw Erange("fichier_libcurl::fichier_libcurl",
			     tools_printf(gettext("Error met while resetting URL to handle: %s"),
					  curl_easy_strerror(err)));

	    switch(get_mode())
	    {
	    case gf_read_only:
		err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_WRITEDATA, this);
		if(err != CURLE_OK)
		    throw Erange("fichier_libcurl::fichier_libcurl",
				 tools_printf(gettext("Error met while setting libcurl for reading data file: %s"),
					      curl_easy_strerror(err)));
		break;
	    case gf_write_only:
		err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_READDATA, this);
		if(err != CURLE_OK)
		    throw Erange("fichier_libcurl::fichier_libcurl",
				 tools_printf(gettext("Error met while setting libcurl for writing data file: %s"),
					      curl_easy_strerror(err)));
		err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_UPLOAD, 1L);
		if(err != CURLE_OK)
		    throw Erange("fichier_libcurl::fichier_libcurl",
				 tools_printf(gettext("Error met while setting libcurl for writing data file: %s"),
					      curl_easy_strerror(err)));
		break;
	    case gf_read_write:
		throw Efeature("read-write mode for fichier libcurl");
	    default:
		throw SRC_BUG;
	    }

	    switch_to_metadata(true);
	    if(append_write && m != gf_read_only)
		current_offset = get_size();
	}
	catch(...)
	{
	    detruit();
	    throw;
	}
    }

    void fichier_libcurl::change_permission(U_I perm)
    {
	const char * errmsg = "Error while changing file permission on remote repository";
	CURLcode err;
	struct curl_slist *headers = nullptr;
	string order = tools_printf("site CHMOD %o", perm);

	switch_to_metadata(true);
	try
	{
	    try
	    {
		headers = curl_slist_append(headers, order.c_str());
		err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_QUOTE, headers);
		if(err != CURLE_OK)
		    throw Erange("","");
		err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_NOBODY, 1);
		if(err != CURLE_OK)
		    throw Erange("","");
	    }
	    catch(Erange & e)
	    {
		throw Erange("fichier_libcurl::change_permission",
			     tools_printf(gettext("%s: %s"), errmsg, curl_easy_strerror(err)));
	    }

	    do
	    {
		err = curl_easy_perform(ehandle.get_handle());
		fichier_libcurl_check_wait_or_throw(get_ui(),
						    err,
						    wait_delay,
						    tools_printf(gettext("%s: %s"),
								 errmsg,
								 curl_easy_strerror(err)));
	    }
	    while(err != CURLE_OK);
	}
	catch(...)
	{
	    (void)curl_easy_setopt(ehandle.get_handle(), CURLOPT_QUOTE, nullptr);
	    (void)curl_easy_setopt(ehandle.get_handle(), CURLOPT_NOBODY, 0);
	    if(headers != nullptr)
		curl_slist_free_all(headers);
	    throw;
	}
	(void)curl_easy_setopt(ehandle.get_handle(), CURLOPT_QUOTE, nullptr);
	(void)curl_easy_setopt(ehandle.get_handle(), CURLOPT_NOBODY, 0);
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
	    me->switch_to_metadata(true);

	    err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_NOBODY, 1);
	    if(err != CURLE_OK)
		throw Erange("fichier_libcurl::get_size()",
			     tools_printf(gettext("Error met while fetching file size: %s"),
					  curl_easy_strerror(err)));
	    do
	    {
		err = curl_easy_perform(ehandle.get_handle());
		fichier_libcurl_check_wait_or_throw(get_ui(),
						    err,
						    wait_delay,
						    tools_printf(gettext("Error met while fetching file size: %s"),
								 curl_easy_strerror(err)));
	    }
	    while(err != CURLE_OK);

	    err = curl_easy_getinfo(ehandle.get_handle(), CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
	    if(filesize == -1) // file does not exist
		filesize = 0;

	    if(err != CURLE_OK)
		throw Erange("fichier_libcurl::get_size()",
			     tools_printf(gettext("Error met while fetching file size: %s"),
					  curl_easy_strerror(err)));
	    me->maxpos = tools_double2infinint(filesize);
	    me->has_maxpos = true;
	    (void)curl_easy_setopt(ehandle.get_handle(), CURLOPT_NOBODY, 0);
	}

	return maxpos;
    }

    bool fichier_libcurl::skippable(skippability direction, const infinint & amount)
    {
	if(get_mode() == gf_read_only)
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
	else
	    return false;
    }

    bool fichier_libcurl::skip(const infinint & pos)
    {
	if(pos == current_offset)
	    return true;

	switch(get_mode())
	{
	case gf_read_only:
	    switch_to_metadata(true); // necessary to stop current subthread and change easy_handle offset
	    current_offset = pos;
	    if(get_mode() == gf_write_only)
	    {
		if(has_maxpos)
		{
		    if(maxpos < pos)
			maxpos = pos;
		}
		else
		    maxpos = pos;
	    }
	    if(get_mode() != gf_write_only)
		flush_read();
	    break;
	case gf_write_only:
	    throw Erange("fichier_libcurl::skip", string(gettext("libcurl does not allow skipping in write mode")));
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
	if(get_mode() == gf_write_only)
	    return true;
	else
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
	relaunch_thread(amount);
    }

    void fichier_libcurl::inherited_sync_write()
    {
	    // nothing to do because there is no data in transit
	    // except in interthread but it cannot be flushed faster
	    // than the normal multi-thread process does
    }

    void fichier_libcurl::inherited_flush_read()
    {
	switch_to_metadata(true);
	interthread.reset();
    }

    void fichier_libcurl::inherited_terminate()
    {
	switch(get_mode())
	{
	case gf_write_only:
	    switch_to_metadata(true);
	    break;
	case gf_read_only:
	    switch_to_metadata(true);
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
	bool full = false;
	char *ptr;
	unsigned int ptr_size;

	switch_to_metadata(false);

	while(wrote < size && !full)
	{
	    try
	    {
		if(!is_running() || sub_is_dying)
		{
		    join();
		    throw SRC_BUG;
		    // inherited_run() should throw an exception
		    // as this is not a normal condition:
		    // we have not yet finished writing
		    // data and child thread has already ended
		}
	    }
	    catch(Edata & e)
	    {
		    // remote disk is full
		full = true;
	    }

	    if(!full)
	    {
		U_I toadd = size - wrote;

		interthread.get_block_to_feed(ptr, ptr_size);

		if(toadd <= ptr_size)
		{
		    memcpy(ptr, a + wrote, toadd);
		    interthread.feed(ptr, toadd);
		    wrote = size;
		}
		else
		{
		    memcpy(ptr, a + wrote, ptr_size);
		    interthread.feed(ptr, ptr_size);
		    wrote += ptr_size;
		}
	    }
	}

	current_offset += wrote;
	if(current_offset > 0)
	    append_write = true; // we can now ignore the request to erase data
	    // and we now need to swap in write append mode

	return wrote;
    }

    bool fichier_libcurl::fichier_global_inherited_read(char *a, U_I size, U_I & read, std::string & message)
    {
	char *ptr;
	unsigned int ptr_size;
	U_I room;
	U_I delta;
	bool maybe_eof = false;

	set_subthread(size);

	read = 0;
	do
	{
	    delta = 0;
 	    while(read + delta < size && (!sub_is_dying || interthread.is_not_empty()))
	    {
		interthread.fetch(ptr, ptr_size);

		room = size - read - delta;
		if(room >= ptr_size)
		{
		    memcpy(a + read + delta, ptr, ptr_size);
		    interthread.fetch_recycle(ptr);
		    delta += ptr_size;
		}
		else
		{
		    memcpy(a + read + delta, ptr, room);
		    delta += room;
		    ptr_size -= room;
		    memmove(ptr, ptr + room, ptr_size);
		    interthread.fetch_push_back(ptr, ptr_size);
		}
	    }
	    current_offset += delta;
	    read += delta;

	    if(read < size                      // we requested more data than what we got so far
	       && (!has_maxpos                  // we don't know where is EOF
		   || current_offset < maxpos)  // or we have not yet reached EOF
	       && !maybe_eof)                   // avoid looping endelessly
	    {
		maybe_eof = (delta == 0);
		U_I remaining = size - read;

		    // if interthread is empty and thread has not been launched at least once
		    // we can only now switch to data mode because current_offset is now correct.
		    // This will (re-)launch the thread that should fill interthread pipe with data
		set_subthread(remaining);
		size = read + remaining;
	    }
	}
	while(read < size && (is_running() || interthread.is_not_empty()));

	return true;
    }

    void fichier_libcurl::inherited_run()
    {

	try
	{
		// parent thread is still suspended

	    CURLcode err;
	    user_interaction *thread_ui = nullptr;
	    infinint local_network_block = network_block; // set before unlocking parent thread

	    try
	    {
		thread_ui = get_ui().clone();
		subthread_cur_offset = current_offset;
	    }
	    catch(...)
	    {
		initialize_subthread();
		throw;
	    }

		// after next call, the parent thread will be running

	    initialize_subthread();

	    try
	    {
		if(local_network_block.is_zero()) // network_block may be non null only in read-only mode
		{
		    do
		    {
			err = curl_easy_perform(ehandle.get_handle());
			if(!end_data_mode)
			    fichier_libcurl_check_wait_or_throw(*thread_ui,
								err,
								wait_delay,
								gettext("Error met during network transfer: "));
		    }
		    while(err != CURLE_OK && !end_data_mode);
		}
		else // reading by block to avoid having interrupting libcurl
		{
		    infinint cycle_subthread_net_offset;

		    do
		    {
			cycle_subthread_net_offset = 0; // how many bytes have will we read in the next do/while loop
			do
			{
			    subthread_net_offset = 0; // keeps trace of the amount of bytes sent to main thread by callback
			    set_range(subthread_cur_offset, local_network_block);
			    try
			    {
				err = curl_easy_perform(ehandle.get_handle());
				if(err == CURLE_BAD_DOWNLOAD_RESUME)
				    err = CURLE_OK;
				fichier_libcurl_check_wait_or_throw(*thread_ui,
								    err,
								    wait_delay,
								    tools_printf(gettext("Error met while reading a block of data: %s"),
										 curl_easy_strerror(err)));
				subthread_cur_offset += subthread_net_offset;
				if(local_network_block < subthread_net_offset)
				    throw SRC_BUG; // we acquired more data from libcurl than expected!
				local_network_block -= subthread_net_offset;
				cycle_subthread_net_offset += subthread_net_offset;
			    }
			    catch(...)
			    {
				unset_range();
				throw;
			    }
			    unset_range();
			}
			while(err != CURLE_OK && !end_data_mode);
		    }
		    while(!cycle_subthread_net_offset.is_zero()     // we just grabbed some data in this ending cycle (not reached eof)
			  && !end_data_mode                   // the current thread has not been asked to stop
			  && !local_network_block.is_zero()); // whe still not have gathered all the requested data
		}
	    }
	    catch(...)
	    {
		delete thread_ui;
		thread_ui = nullptr;
		throw;
	    }
	    delete thread_ui;
	    thread_ui = nullptr;
	}
	catch(...)
	{
	    finalize_subthread();
	    throw;
	}
	finalize_subthread();
    }

    void fichier_libcurl::initialize_subthread()
    {
	sub_is_dying = false;
	synchronize.wait(); // release calling thread as we, as child thread, do now exist
    }

    void fichier_libcurl::finalize_subthread()
    {
	sub_is_dying = true;
	if(!end_data_mode) // natural death, main thread has not required our death
	{
	    char *ptr;
	    unsigned int ptr_size;

	    switch(get_mode())
	    {
	    case gf_write_only:
		    // making room in the pile to toggle main thread if
		    // it was suspended waiting for a block to feed
		interthread.fetch(ptr, ptr_size);
		interthread.fetch_recycle(ptr);
		break;
	    case gf_read_only:
		    // sending a zero length block to toggle main thread
		    // if it was suspended waiting for a block to fetch
		interthread.get_block_to_feed(ptr, ptr_size);
		interthread.feed(ptr, 0); // means eof to main thread
		break;
	    case gf_read_write:
		throw SRC_BUG;
	    default:
		throw SRC_BUG;
	    }
	}
    }

    void fichier_libcurl::set_range(const infinint & begin, const infinint & range_size)
    {
	infinint end_range = begin + range_size - 1;
	string range = tools_printf("%i-%i", &begin, &end_range);

	    // setting the block size if necessary

	CURLcode err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_RANGE, range.c_str());
	if(err != CURLE_OK)
	    throw Erange("fichier_libcurl::set_range",
			 tools_printf(gettext("Error while seeking in file on remote repository: %s"), curl_easy_strerror(err)));
    }

    void fichier_libcurl::unset_range()
    {
	CURLcode err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_RANGE, nullptr);
	if(err != CURLE_OK)
	    throw Erange("fichier_libcurl::unset_range",
			 tools_printf(gettext("Error while seeking in file on remote repository: %s"), curl_easy_strerror(err)));
    }

    void fichier_libcurl::switch_to_metadata(bool mode)
    {
	CURLcode err;

	if(mode == metadatamode)
	    return;

	if(!mode) // data mode
	{
	    infinint resume;
	    curl_off_t cur_pos = 0;

	    switch(get_mode())
	    {
	    case gf_read_only:
		err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_WRITEFUNCTION, write_data_callback);
		if(err != CURLE_OK)
		    throw Erange("fichier_libcurl::switch_to_metadata",
				 tools_printf(gettext("Error met while setting libcurl for reading data file: %s"),
					      curl_easy_strerror(err)));

		if(network_block.is_zero())
		{

			// setting the offset of the next byte to read / write

		    resume = current_offset;
		    resume.unstack(cur_pos);
		    if(!resume.is_zero())
			throw Erange("fichier_libcurl::switch_to_metadata",
				     gettext("Integer too large for libcurl, cannot skip at the requested offset in the remote repository"));

		    err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_RESUME_FROM_LARGE, cur_pos);
		    if(err != CURLE_OK)
			throw Erange("fichier_libcurl::switch_to_metadata",
				     tools_printf(gettext("Error while seeking in file on remote repository: %s"), curl_easy_strerror(err)));
		}
		    // else (network_block != 0) the subthread will make use of range
		    // this parameter is set back to its default in stop_thread()

		break;
	    case gf_write_only:
		err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_READFUNCTION, read_data_callback);
		if(err != CURLE_OK)
		    throw Erange("fichier_libcurl::switch_to_metadata",
				 tools_printf(gettext("Error met while setting libcurl for writing data file: %s"),
					      curl_easy_strerror(err)));

		    // setting the offset of the next byte to read / write

		err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_APPEND, append_write ? 1 : 0);
		if(err != CURLE_OK)
		    throw Erange("fichier_libcur::switch_to_metadata",
				 tools_printf(gettext("Error while setting write append mode for libcurl: %s"), curl_easy_strerror(err)));

		    // should also set the CURLOPT_INFILESIZE_LARGE option but file size is not known at this time
		break;
	    case gf_read_write:
		throw SRC_BUG;
	    default:
		throw SRC_BUG;
	    }
	    run_thread();
	}
	else // metadata mode
	{
	    stop_thread();
	    meta_inbuf = 0; // we don't care existing metadata remaining in transfer

	    switch(get_mode())
	    {
	    case gf_read_only:
		err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_WRITEFUNCTION, write_meta_callback);
		if(err != CURLE_OK)
		    throw Erange("fichier_libcurl::switch_to_metadata",
				 tools_printf(gettext("Error met while setting libcurl for reading data file: %s"),
					      curl_easy_strerror(err)));
		break;
	    case gf_write_only:
		err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_READFUNCTION, read_meta_callback);
		if(err != CURLE_OK)
		    throw Erange("fichier_libcurl::switch_to_metadata",
				 tools_printf(gettext("Error met while setting libcurl for writing data file: %s"),
					      curl_easy_strerror(err)));
		break;
	    case gf_read_write:
		throw SRC_BUG;
	    default:
		throw SRC_BUG;
	    }
	}
	metadatamode = mode;
    }

    void fichier_libcurl::detruit()
    {
	try
	{
	    terminate();
	}
	catch(...)
	{
		// ignore all errors
	}
    }

    void fichier_libcurl::run_thread()
    {
	if(interthread.is_not_empty())
	{
	    char *ptr;
	    unsigned int ptr_size;
	    bool bug = false;

		// the interthread may keep
		// a single empty block pending
		// to be fetched.
	    interthread.fetch(ptr, ptr_size);
	    if(ptr_size != 0)
		bug = true;
	    interthread.fetch_recycle(ptr);
	    if(bug)
		throw SRC_BUG;

		// now interthread should be empty
	    if(interthread.is_not_empty())
		bug = true;
	    if(bug)
		throw SRC_BUG;
		// interthread should have been purged when
		// previous thread had ended

	}

	end_data_mode = false;
	if(is_running())
	    throw SRC_BUG;
	run();
	synchronize.wait(); // waiting for child thread to be ready
    }

    void fichier_libcurl::stop_thread()
    {
	if(is_running())
	{
	    char *ptr;
	    unsigned int ptr_size;

	    end_data_mode = true;
	    switch(get_mode())
	    {
	    case gf_write_only:
		interthread.get_block_to_feed(ptr, ptr_size);
		interthread.feed(ptr, 0); // trigger the thread if it was waiting for data from interthread
		break;
	    case gf_read_only:
		if(interthread.is_full())
		{
		    interthread.fetch(ptr, ptr_size);
		    interthread.fetch_recycle(ptr); // trigger the thread if it was waiting for a free block to fill
		}
		break;
	    case gf_read_write:
		throw SRC_BUG;
	    default:
		throw SRC_BUG;
	    }
	}
	join();

	CURLcode err;
	curl_off_t cur_pos = 0;
	err = curl_easy_setopt(ehandle.get_handle(), CURLOPT_RESUME_FROM_LARGE, cur_pos);
	if(err != CURLE_OK)
	    throw Erange("fichier_libcurl::switch_to_metadata",
			 tools_printf(gettext("Error while seeking in file on remote repository: %s"), curl_easy_strerror(err)));
    }

    void fichier_libcurl::relaunch_thread(const infinint & block_size)
    {
	if(metadatamode)
	{
	    if(x_proto == proto_ftp)
		network_block = 0;
	    else
		network_block = block_size;
	    switch_to_metadata(false);
	}
	else
	{
	    if(sub_is_dying)
	    {
		stop_thread();
		if(x_proto == proto_ftp)
		    network_block = 0;
		else
		    network_block = block_size;
		run_thread();
	    }

		// else thread is still running so
		// we cannot change the network_block size
	}
    }

    size_t fichier_libcurl::write_data_callback(char *buffer, size_t size, size_t nmemb, void *userp)
    {
	size_t remain = size * nmemb;
	size_t lu = 0;
	fichier_libcurl *me = (fichier_libcurl *)(userp);
	char *ptr;
	unsigned int ptr_size;

	if(me == nullptr)
	    throw SRC_BUG;

	while(!me->end_data_mode && remain > 0)
	{
	    me->interthread.get_block_to_feed(ptr, ptr_size);

	    if(remain <= ptr_size)
	    {
		memcpy(ptr, buffer + lu, remain);
		me->interthread.feed(ptr, remain);
		lu += remain;
		remain = 0;
	    }
	    else
	    {
		memcpy(ptr, buffer + lu, ptr_size);
		me->interthread.feed(ptr, ptr_size);
		remain -= ptr_size;
		lu += ptr_size;
	    }
	}

	if(me->network_block > 0)
	    me->subthread_net_offset += lu;

	if(me->end_data_mode)
	{
	    if(me->network_block == 0)
	    {
		if(remain > 0) // not all data could be sent to main thread
		    lu = 0; // to force easy_perform() that called us, to return
	    }
	    else
	    {
		if(remain > 0)
		    throw SRC_BUG;
		    // main thread should not ask us to stop
		    // until we have provided all the requested data
	    }
	}

	return lu;
    }

    size_t fichier_libcurl::read_data_callback(char *bufptr, size_t size, size_t nitems, void *userp)
    {
	size_t ret;
	size_t room = size * nitems;
	fichier_libcurl *me = (fichier_libcurl *)(userp);
	char *ptr;
	unsigned int ptr_size;

	if(me == nullptr)
	    throw SRC_BUG;

	me->interthread.fetch(ptr, ptr_size);

	if(ptr_size <= room)
	{
	    memcpy(bufptr, ptr, ptr_size);
	    me->interthread.fetch_recycle(ptr);
	    ret = ptr_size;
	}
	else
	{
	    memcpy(bufptr, ptr, room);
	    ptr_size -= room;
	    memmove(ptr, ptr + room, ptr_size);
	    me->interthread.fetch_push_back(ptr, ptr_size);
	    ret = room;
	}

	return ret;
    }

    size_t fichier_libcurl::write_meta_callback(char *buffer, size_t size, size_t nmemb, void *userp)
    {
	return size * nmemb;
    }

    size_t fichier_libcurl::read_meta_callback(char *bufptr, size_t size, size_t nitems, void *userp)
    {
	return 0;
    }

    void fichier_libcurl_check_wait_or_throw(user_interaction & dialog,
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
	case CURLE_REMOTE_ACCESS_DENIED:
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
		dialog.printf(gettext("%S: %s, retrying in %d seconds"),
			      &err_context,
			      curl_easy_strerror(err),
			      wait_seconds);
		sleep(wait_seconds);
	    }
	    else
		dialog.pause(tools_printf(gettext("%S: %s, do we retry network operation?"),
					  &err_context,
					  curl_easy_strerror(err)));
	    break;
	}
    }

    void fichier_libcurl::set_subthread(U_I & needed_bytes)
    {
	if(interthread.is_empty())
	{
		// cannot switch to data mode if some data are
		// in transit because current_offset would be
		// wrongly positionned in the requested to libcurl
	    if(metadatamode)
	    {
		if(x_proto == proto_ftp)
		    network_block = 0;
		    // because reading by block lead control session to
		    // be reset when ftp is used, leading a huge amount
		    // of connection a FTP server might see as DoS atempt
		else
		{
		    if(has_maxpos && maxpos <= current_offset + needed_bytes)
		    {
			infinint tmp = maxpos - current_offset;

			    // this sets size the value of tmp:
			needed_bytes = 0;
			tmp.unstack(needed_bytes);
			if(!tmp.is_zero())
			    throw SRC_BUG;

			network_block = 0;
		    }
		    else
			network_block = needed_bytes;
		}
		switch_to_metadata(false);
	    }
	    else
	    {
		if(sub_is_dying)
		    relaunch_thread(needed_bytes);
	    }
	}
    }

#endif

} // end of namespace
