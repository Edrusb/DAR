/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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

#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )

    fichier_libcurl::fichier_libcurl(const shared_ptr<user_interaction> & dialog,
				     const std::string & chemin,
				     mycurl_protocol proto,
				     const shared_ptr<mycurl_easyhandle_node> & handle,
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
	try
	{
	    if(!ehandle)
		throw SRC_BUG;

		// setting x_ref_handle to carry all options that will always be present for this object

	    ehandle->setopt(CURLOPT_URL, chemin);

	    switch(get_mode())
	    {
	    case gf_read_only:
		ehandle->setopt(CURLOPT_WRITEDATA, this);
		break;
	    case gf_write_only:
		ehandle->setopt(CURLOPT_READDATA, this);
		ehandle->setopt(CURLOPT_UPLOAD, 1L);
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
	struct mycurl_slist headers;
	string order = tools_printf("site CHMOD %o", perm);

	switch_to_metadata(true);
	try
	{
	    headers.append(order);
	    ehandle->setopt(CURLOPT_QUOTE, headers);
	    ehandle->setopt(CURLOPT_NOBODY, (long)1);
	    try
	    {
		ehandle->apply(get_pointer(), wait_delay);
	    }
	    catch(...)
	    {
		ehandle->setopt_default(CURLOPT_QUOTE);
		ehandle->setopt_default(CURLOPT_NOBODY);
		throw;
	    }
	    ehandle->setopt_default(CURLOPT_QUOTE);
	    ehandle->setopt_default(CURLOPT_NOBODY);
	}
	catch(Egeneric & e)
	{
	    e.prepend_message("Error while changing file permission on remote repository");
	    throw;
	}
    }

    infinint fichier_libcurl::get_size() const
    {
	double filesize;
	fichier_libcurl *me = const_cast<fichier_libcurl *>(this);

	if(me == nullptr)
	    throw SRC_BUG;

	if(!has_maxpos || get_mode() != gf_read_only)
	{
	    try
	    {
		me->switch_to_metadata(true);
		me->ehandle->setopt(CURLOPT_NOBODY, (long)1);
		try
		{
		    me->ehandle->apply(get_pointer(), wait_delay);
		    me->ehandle->getinfo(CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
		    if(filesize == -1) // file does not exist
			filesize = 0;
		    me->maxpos = tools_double2infinint(filesize);
		    me->has_maxpos = true;
		}
		catch(...)
		{
		    me->ehandle->setopt_default(CURLOPT_NOBODY);
		    throw;
		}
		me->ehandle->setopt_default(CURLOPT_NOBODY);
	    }
	    catch(Egeneric & e)
	    {
		e.prepend_message("Error while reading file size on a remote repository");
		throw;
	    }
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

    void fichier_libcurl::inherited_truncate(const infinint & pos)
    {
	if(pos != get_position())
	    throw Erange("fichier_libcurl::inherited_truncate", string(gettext("libcurl does not allow truncating at a given position while uploading files")));
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

	    shared_ptr<user_interaction> thread_ui = get_pointer();
	    infinint local_network_block = network_block; // set before unlocking parent thread

	    try
	    {
		if(!thread_ui)
		    throw Ememory("fichier_libcurl::inherited_run");
		subthread_cur_offset = current_offset;
	    }
	    catch(...)
	    {
		initialize_subthread();
		throw;
	    }

		// after next call, the parent thread will be running

	    initialize_subthread();

	    if(local_network_block.is_zero()) // network_block may be non null only in read-only mode
	    {
		ehandle->apply(thread_ui, wait_delay, end_data_mode);
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
			    ehandle->apply(thread_ui, wait_delay);
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
		    while(!end_data_mode);
		}
		while(!cycle_subthread_net_offset.is_zero()     // we just grabbed some data in this ending cycle (not reached eof)
		      && !end_data_mode                   // the current thread has not been asked to stop
		      && !local_network_block.is_zero()); // whe still not have gathered all the requested data
	    }
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

	ehandle->setopt(CURLOPT_RANGE, range);
    }

    void fichier_libcurl::unset_range()
    {
	ehandle->setopt_default(CURLOPT_RANGE);
    }

    void fichier_libcurl::switch_to_metadata(bool mode)
    {
	if(mode == metadatamode)
	    return;

	if(!mode) // data mode
	{
	    infinint resume;
	    curl_off_t cur_pos = 0;
	    long do_append;

	    switch(get_mode())
	    {
	    case gf_read_only:
		ehandle->setopt(CURLOPT_WRITEFUNCTION, (void*)write_data_callback);

		if(network_block.is_zero())
		{

			// setting the offset of the next byte to read / write

		    resume = current_offset;
		    resume.unstack(cur_pos);
		    if(!resume.is_zero())
			throw Erange("fichier_libcurl::switch_to_metadata",
				     gettext("Integer too large for libcurl, cannot skip at the requested offset in the remote repository"));

		    ehandle->setopt(CURLOPT_RESUME_FROM_LARGE, cur_pos);
		}
		    // else (network_block != 0) the subthread will make use of range
		    // this parameter is set back to its default in stop_thread()

		break;
	    case gf_write_only:
		ehandle->setopt(CURLOPT_READFUNCTION, (void*)read_data_callback);

		    // setting the offset of the next byte to read / write

		do_append = (append_write ? 1 : 0);
		ehandle->setopt(CURLOPT_APPEND, do_append);

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
		ehandle->setopt(CURLOPT_WRITEFUNCTION, (void*)write_meta_callback);
		break;
	    case gf_write_only:
		ehandle->setopt(CURLOPT_READFUNCTION, (void*)read_meta_callback);
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
	if(is_running())
	    throw SRC_BUG;

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

	ehandle->setopt_default(CURLOPT_RESUME_FROM_LARGE);
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
		    // of connection an FTP server might see as DoS atempt
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
