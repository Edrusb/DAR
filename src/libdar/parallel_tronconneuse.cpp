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

#include "../my_config.h"

extern "C"
{
}

#include "parallel_tronconneuse.hpp"
#include "tools.hpp"

using namespace std;
using namespace libthreadar;

namespace libdar
{

    	/////////////////////////////////////////////////////
	//
	// static routines declaration
	//
	//


    static void remove_trailing_clear_data_from_encrypted_buf(const infinint & read_offset,    ///< offset of the first byte of the 'first' segment
							      const archive_version & reading_ver, ///< read archive format version
							      const infinint & initial_shift,  ///< amount of free bytes before encrypted ones
							      infinint (*callback)(generic_file & below, const archive_version& reading_version),
							      unique_ptr<crypto_segment> & first, ///< pointer to a existing segment
							      unique_ptr<crypto_segment> & opt_next, ///< in option pointer to the segment following the first in the crypted data
							      bool & reof);


    	/////////////////////////////////////////////////////
	//
	// parallel_tronconneuse implementation
	//
	//

    parallel_tronconneuse::parallel_tronconneuse(U_I workers,
						 U_32 block_size,
						 generic_file & encrypted_side,
						 const archive_version & ver,
						 std::unique_ptr<crypto_module> & crypto_ptr):
	proto_tronco(encrypted_side.get_mode() == gf_read_only ? gf_read_only : gf_write_only)
    {
	if(block_size == 0)
	    throw Erange("parallel_tronconneuse::parallel_tronconneuse", tools_printf(gettext("%d is not a valid block size"), block_size));

	num_workers = workers;
	clear_block_size = block_size;
	current_position = 0;
	initial_shift = 0;
	reading_ver = ver;
	crypto = std::move(crypto_ptr);
	t_status = thread_status::dead;
	ignore_stop_acks = 0;
	mycallback = nullptr;
	encrypted = &encrypted_side; // used for further reference, thus the encrypted object must survive "this"
	lus_data.clear();
	lus_flags.clear();
	lus_eof = false;
	check_bytes_to_skip = true;
	block_num = 0;

	if(!crypto)
	    throw SRC_BUG;

	    // creating the inter-thread communication structures

	try
	{
	    U_I tmp_bs1, tmp_bs2;

	    scatter.reset(new (nothrow) ratelier_scatter<crypto_segment>(get_ratelier_size(num_workers)));
	    if(!scatter)
		throw Ememory("parallel_tronconneuse::parallel_tronconneuse");

	    gather.reset(new (nothrow) ratelier_gather<crypto_segment>(get_ratelier_size(num_workers)));
	    if(!gather)
		throw Ememory("parallel_tronconneuse::parallel_tronconneuse");

	    waiter.reset(new (nothrow) barrier(num_workers + 2));
	    if(!waiter)
		throw Ememory("parallel_tronconneuse::parallel_tronconneuse");

		// tas is created empty

	    tas.reset(new (nothrow) heap<crypto_segment>());
	    if(!tas)
		throw Ememory("parallel_tronconneuse::parallel_tronconneuse");

		// filling the heap (tas) with preallocated crypto_segments

	    tmp_bs1 = crypto->encrypted_block_size_for(clear_block_size);
	    tmp_bs2 = crypto->clear_block_allocated_size_for(clear_block_size);
	    for(U_I i = 0; i < get_heap_size(num_workers); ++i)
		tas->put(make_unique<crypto_segment>(tmp_bs1,tmp_bs2));


		// creating and running the sub-thread objects

	    for(U_I i = 0; i < workers; ++i)
		travailleur.push_back(make_unique<crypto_worker>(scatter,
								 gather,
								 waiter,
								 crypto->clone(),
								 get_mode() == gf_write_only)
		    );

	    switch(get_mode())
	    {
	    case gf_read_only:
		crypto_reader = make_unique<read_below>(scatter,
							waiter,
							num_workers,
							clear_block_size,
							encrypted,
							tas,
							initial_shift);
		if(!crypto_reader)
		    throw Ememory("parallel_tronconneuse::parallel_tronconneuse");
		break;
	    case gf_write_only:
		crypto_writer = make_unique<write_below>(gather,
							 waiter,
							 num_workers,
							 encrypted,
							 tas);
		if(!crypto_writer)
		    throw Ememory("parallel_tronconneuse::parallel_tronconneuse");
		break;
	    case gf_read_write:
		throw SRC_BUG;
	    default:
		throw SRC_BUG;
	    }

	    run_threads(); // threads starts in a suspended state (pending on the waiter barrier)

	}
	catch(std::bad_alloc &)
	{
	    throw Ememory("parallel_tronconneuse::parallel_tronconneuse");
	}
    }

    parallel_tronconneuse::~parallel_tronconneuse() noexcept
    {
	try
	{
	    terminate();
	}
	catch(...)
	{
		// not throwing exception from destructor
	}
    }

    bool parallel_tronconneuse::skippable(skippability direction, const infinint & amount)
    {
	if(get_mode() != gf_read_only)
	    return false;

	send_read_order(tronco_flags::stop);
	return encrypted->skippable(direction, amount);
    }

    bool parallel_tronconneuse::skip(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(get_mode() != gf_read_only)
	    throw SRC_BUG;

	if(pos == current_position)
	    return true;

	    // looking in the pipe for data
	    // before sending the stop order

	if(!find_offset_in_lus_data(pos))
	{
	    if(ignore_stop_acks == 0)
	    {
		if(send_read_order(tronco_flags::stop, pos))
		{
			// the stop order completed and
			// threads are now stopped
		    current_position = pos;
		    lus_eof = false;
		}
		    // else we are in the same situation as
		    // if the offset was found in lus_data
		    // but a stop ack order are pending in
		    // the ratelier_gather, for that reason
		    // ignore_stop_acks field has been set to the number of
		    // workers which is the number of expected pending stop orders
		    // by send_read_order() to ignore these acks
		    // during further readings

		    // current_position has been set by send_read_order() (calling find_offset_in_lus_data())

	    }
	    else // some unacknowledged stop order are pending in the pipe, we can read further up to them, subthreads are *maybe* already suspended
	    {
		    // we do not need to send a stop order
		if(purge_unack_stop_order(pos))
		{
			// pending stop found or eof found
		    current_position = pos;
		    lus_eof = false;
		}

		    // current_position has been set by purge_unack_stop_order (calling find_offset_in_lus_data())
	    }
	}
	    // else current_position has been set by find_offset_in_lus_data

	    // offset has been found in lus_data, no stop order needed to be sent to skip()

	return true;
    }

    bool parallel_tronconneuse::skip_to_eof()
    {
	bool ret;

	if(is_terminated())
	    throw SRC_BUG;

	if(get_mode() != gf_read_only)
	    throw SRC_BUG;

	send_read_order(tronco_flags::stop);

	ret = encrypted->skip_to_eof();
	if(ret)
	{
	    infinint residu;
	    infinint block_num;
	    U_32 encrypted_buf_size = crypto->encrypted_block_size_for(clear_block_size);
	    unique_ptr<crypto_segment> aux = tas->get();

	    try
	    {
		if(encrypted->get_position() < initial_shift)
		    throw SRC_BUG; // eof is before the first encrypted byte
		euclide(encrypted->get_position() - initial_shift, encrypted_buf_size, block_num, residu);
		current_position = block_num * infinint(clear_block_size);
		if(residu > 0)
		{
		    go_read();
			// we will read the last block and uncipher it to know the exact
			// position of eof from clear side point of view
		    while(read(aux->clear_data.get_addr(), aux->clear_data.get_max_size()) == aux->clear_data.get_max_size())
			; // nothing in the while loop
		}
	    }
	    catch(...)
	    {
		tas->put(std::move(aux));
		throw;
	    }
	    tas->put(std::move(aux));
	}

	return ret;
    }

    bool parallel_tronconneuse::skip_relative(S_I x)
    {
	bool ret;

	if(is_terminated())
	    throw SRC_BUG;

	if(get_mode() != gf_read_only)
	    throw SRC_BUG;
	if(x >= 0)
	    ret = skip(current_position + x);
	else
	{
	    x = -x;
	    if(current_position >= x)
		ret = skip(current_position - infinint(x));
	    else
	    {
		skip(0);
		ret = false;
	    }
	}
	return ret;
    }

    void parallel_tronconneuse::set_initial_shift(const infinint & x)
    {
	if(is_terminated())
	    throw SRC_BUG;

	initial_shift = x;
	if(get_mode() == gf_read_only)
	{
	    send_read_order(tronco_flags::stop);
	    crypto_reader->set_initial_shift(x);
	}
    }

    void parallel_tronconneuse::set_callback_trailing_clear_data(trailing_clear_data_callback call_back)
    {
        if(crypto_reader)
        {
            mycallback = call_back;
            crypto_reader->set_callback_trailing_clear_data(call_back);
        }
        else
	    throw SRC_BUG;
    }


    void parallel_tronconneuse::inherited_read_ahead(const infinint & amount)
    {
	if(get_mode() != gf_read_only)
	    throw SRC_BUG;

	if(crypto_reader)
	    crypto_reader->read_ahead_up_to(current_position + amount);
	else
	    throw SRC_BUG;

	if(is_terminated())
	    throw SRC_BUG;
	else
	    go_read();
    }

    U_I parallel_tronconneuse::inherited_read(char *a, U_I size)
    {
	U_I ret = 0;
	U_I added_to_current_pos = 0;

	if(get_mode() != gf_read_only)
	    throw SRC_BUG;

	if(lus_eof)
	    return ret;

	go_read();
	while(ret < size && ! lus_eof)
	{
	    read_refill();

	    switch(static_cast<tronco_flags>(lus_flags.front()))
	    {
	    case tronco_flags::normal:
		if(lus_data.empty())
		    throw SRC_BUG; // not the same amount of item in lus_data than in lus_flags!
		if(!lus_data.front())
		    throw SRC_BUG; // front pointer does not point to any crypto_segment object
		ret += lus_data.front()->clear_data.read(a + ret, size - ret);
		if(lus_data.front()->clear_data.all_is_read())
		{
		    tas->put(std::move(lus_data.front()));
		    lus_data.pop_front();
		    lus_flags.pop_front();
		}
		break;
	    case tronco_flags::stop:
		if(ignore_stop_acks > 0)
		{
		    --ignore_stop_acks;
		    tas->put(std::move(lus_data.front()));
		    lus_data.pop_front();
		    lus_flags.pop_front();
		    if(ignore_stop_acks == 0)
		    {
			t_status = thread_status::suspended;
			current_position += (ret - added_to_current_pos);
			added_to_current_pos = ret;
			go_read();
		    }
		}
		else
		    throw SRC_BUG;
		break;
	    case tronco_flags::eof:
		lus_eof = true;
		if(purge_ratelier_from_next_order() != tronco_flags::eof)
		    throw SRC_BUG;
		break;
	    case tronco_flags::die:
		throw SRC_BUG; // should never receive a die flag without a first sollicition
	    case tronco_flags::data_error:
		if(lus_data.empty())
		    throw SRC_BUG; // not the same amount of item in lus_data than in lus_flags!
		if(!lus_data.front())
		    throw SRC_BUG; // front does not point to any crypto_segment pointed object

		    // we must check that the deciphering error
		    // is not due to the fact some clear data (archive terminator)
		    // has been mixed with the encrypted data and tried
		    // to be decrypted

		if(mycallback != nullptr)
		{
		    unique_ptr<crypto_segment> current(std::move(lus_data.front()));
		    if(!current)
			throw SRC_BUG;

		    try
		    {
			lus_data.pop_front();
			lus_flags.pop_front();

			infinint crypt_offset = current->block_index * crypto->encrypted_block_size_for(clear_block_size) + initial_shift;

			    // now fetching the next segment following 'current' if any
			    // in order to append it to current and remove trailing clear data at the tail

			read_refill(); // this is be sure we will have the next available segment

			try
			{
			    if(lus_flags.size() > 0
			       &&
			       (lus_flags.front() == static_cast<int>(tronco_flags::normal)
				|| lus_flags.front() == static_cast<int>(tronco_flags::data_error)
				   )
				)
			    {
				if(lus_data.empty())
				    throw SRC_BUG; // an element should be present, as lus_flags is not empty
				unique_ptr<crypto_segment> & opt = lus_data.front();
				    // this is a reference on an unique_ptr
				    // the unique_ptr pointed to by 'opt' stays managed by lus_data

				remove_trailing_clear_data_from_encrypted_buf(crypt_offset,
									      reading_ver,
									      initial_shift,
									      mycallback,
									      current,
									      opt,
									      lus_eof);
			    }
			    else // we ignore this next segment (eof reached at an exact segment size boundary)
			    {
				unique_ptr<crypto_segment> opt = nullptr;
				remove_trailing_clear_data_from_encrypted_buf(crypt_offset,
									      reading_ver,
									      initial_shift,
									      mycallback,
									      current,
									      opt,
									      lus_eof);
			    }
			}
			catch(Erange & e)
			{
				// we must change this exception message
				// to something more relevant to the context
			    throw Erange("parallel_tronconneuse::inherited_read",
					 gettext("data deciphering failed"));
			}

			    // now current should have possible clear data removed
			    // from the tail and only contain encrypted data
			    // retrying the deciphering that failed in the worker:

			current->clear_data.set_data_size(crypto->decrypt_data(current->block_index,
									       current->crypted_data.get_addr(),
									       current->crypted_data.get_data_size(),
									       current->clear_data.get_addr(),
									       current->clear_data.get_max_size()));
			current->clear_data.rewind_read();
		    }
		    catch(...)
		    {
			tas->put(std::move(current));
			throw;
		    }

			// we reached this statement means no exception took place
			// thus deciphering succeeded and we can reinsert the 'current'
			// crypto_segment at the front of lus_data with a normal flag:

		    lus_data.push_front((std::move(current)));
		    lus_flags.push_front(static_cast<int>(tronco_flags::normal));
		}
		else // mycallback == nullptr
		{
			// without callback we cannot remove possible clear data at the tail
			// so we try to reproduce the exception met by the worker for it
			// propagates to our caller:

		    unique_ptr<crypto_segment> & current = lus_data.front();
			// 'current' is here a reference to an unique_ptr
			// and stays managed by lus_data

			// this should throw the exception met by the worker
		    current->clear_data.set_data_size(crypto->decrypt_data(current->block_index,
									   current->crypted_data.get_addr(),
									   current->crypted_data.get_data_size(),
									   current->clear_data.get_addr(),
									   current->clear_data.get_max_size()));

			// if no exception arose, we probably hit a bug
		    throw SRC_BUG;
		}
		break;
	    case tronco_flags::exception_below:
		join_threads();
		throw SRC_BUG; // the previous call should throw an exception if not this is a bug
	    case tronco_flags::exception_worker:
		    // a worker has a problem, the other as well as the below thread are not
		    // aware of that, for that reason the faulty worker is still alive just
		    // waiting for the order to die, which purge_ratelier will do on purpose
		    // when cleaning up this exception_worker block and throw the worker exception
		(void)purge_ratelier_from_next_order();
		throw SRC_BUG; // else this is a bug condition
	    default:
		throw SRC_BUG;
	    }
	}

	current_position += (ret - added_to_current_pos);
	return ret;
    }

    void parallel_tronconneuse::inherited_write(const char *a, U_I size)
    {
	U_I wrote = 0;
	U_I remain;

	if(get_mode() != gf_write_only)
	    throw SRC_BUG;

	if(t_status == thread_status::dead)
	    run_threads();


	while(wrote < size)
	{
	    if(crypto_writer->exception_pending())
	    {
		try
		{
		    stop_threads();
		    join_threads();
		    throw SRC_BUG; // if join() has not thrown an exception
		}
		catch(...)
		{
			// we have to reset the object position
			// to where the error took place
		    block_num = crypto_writer->get_error_block();
		    current_position = block_num*clear_block_size;
			// truncate below if possible
		    try
		    {
			encrypted->truncate(initial_shift
					    + block_num*crypto->encrypted_block_size_for(clear_block_size));
		    }
		    catch(...)
		    {
			encrypted->skip(initial_shift
					+ block_num*crypto->encrypted_block_size_for(clear_block_size));
		    }
		    throw;
		}
	    }

	    if(!tempo_write) // no crypto_segment pointed to by tempo_write
	    {
		tempo_write = tas->get();
		tempo_write->reset();
		tempo_write->block_index = block_num++;
		if(tempo_write->clear_data.get_max_size() < clear_block_size)
		    throw SRC_BUG;
	    }

	    remain = size - wrote;
	    if(remain + tempo_write->clear_data.get_data_size() > clear_block_size)
		remain = clear_block_size - tempo_write->clear_data.get_data_size();

	    wrote += tempo_write->clear_data.write(a + wrote, remain);
	    if(tempo_write->clear_data.get_data_size() == clear_block_size)
		scatter->scatter(tempo_write, static_cast<int>(tronco_flags::normal));
	}

	current_position += wrote;
    }

    void parallel_tronconneuse::inherited_sync_write()
    {
	if(get_mode() == gf_write_only)
	{
	    if(tempo_write)
		scatter->scatter(tempo_write, static_cast<int>(tronco_flags::normal));
	}
    }

    void parallel_tronconneuse::inherited_flush_read()
    {
	if(get_mode() == gf_read_only)
	    send_read_order(tronco_flags::stop);
    }

    void parallel_tronconneuse::inherited_terminate()
    {
	if(get_mode() == gf_write_only)
	    sync_write();
	if(get_mode() == gf_read_only)
	    flush_read();

	switch(t_status)
	{
	case thread_status::running:
	case thread_status::suspended:
	    stop_threads(); // stop threads first if relevant
		/* no break */
	case thread_status::dead:
	    join_threads(); // may throw exception
	    break;
	default:
	    throw SRC_BUG;
	}

	    // sanity checks

	if(tas->get_size() != get_heap_size(num_workers))
	    throw SRC_BUG;
    }

    bool parallel_tronconneuse::send_read_order(tronco_flags order, const infinint & for_offset)
    {
	bool ret = true;
	tronco_flags val;

	if(get_mode() != gf_read_only)
	    throw SRC_BUG;

	if(t_status == thread_status::dead)
	    throw SRC_BUG;

	switch(order)
	{
	case tronco_flags::die:
	    crypto_reader->set_flag(order);
	    if(t_status == thread_status::suspended)
	    {
		waiter->wait();
		t_status = thread_status::running;
	    }

	    val = purge_ratelier_from_next_order();

	    switch(val)
	    {
	    case tronco_flags::die:
		    // expected normal condition
		break;
	    case tronco_flags::eof:
		    // eof met threads are stopped
		    /* no break */
	    case tronco_flags::stop:
		    // pending stop was not yet read
		    // now that threads are suspended we can
		    // send the order again
		crypto_reader->set_flag(order);
		if(t_status == thread_status::suspended)
		{
		    waiter->wait(); // awaking thread pending on waiter for they take the order into account
		    t_status = thread_status::running;
		}
		val = purge_ratelier_from_next_order();
		if(val != order)
		    throw SRC_BUG;
		break;
	    case tronco_flags::normal:
		throw SRC_BUG; // purge_ratelier_from_next_order() should have drop those
	    case tronco_flags::data_error:
		throw SRC_BUG; // purge_ratelier_from_next_order() should have drop those
	    case tronco_flags::exception_below:
		    // unexpected but possible, has the same outcome as "die", so we are good
		break;
	    case tronco_flags::exception_worker:
		throw SRC_BUG;
		    // purge_ratelier_from_next_order() should handle that
		    // an trigger the launch of worker exception
	    default:
		throw SRC_BUG;
	    }
	    break;
	case tronco_flags::eof:
	    throw SRC_BUG;
	case tronco_flags::stop:
	    if(t_status == thread_status::suspended)
		return ret; // nothing to do
	    crypto_reader->set_flag(order);
	    val = purge_ratelier_from_next_order(for_offset);
	    if(val != tronco_flags::stop
	       && val != tronco_flags::eof
	       && (val != tronco_flags::normal || for_offset.is_zero()))
		throw SRC_BUG;
	    if(!for_offset.is_zero() && val == tronco_flags::normal)
		ret = false;
		// order not purged, not yet subthreaded will be considered suspended
		// after the order completion (ignore_stop_order back to zero)
	    break;
	case tronco_flags::normal:
	    throw SRC_BUG;
	case tronco_flags::data_error:
	    throw SRC_BUG;
	case tronco_flags::exception_below:
	    throw SRC_BUG;
	case tronco_flags::exception_worker:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}

	return ret;
    }

    void parallel_tronconneuse::send_write_order(tronco_flags order)
    {
	if(t_status == thread_status::dead)
	    throw SRC_BUG;

	switch(order)
	{
	case tronco_flags::normal:
	case tronco_flags::stop:
	case tronco_flags::eof:
	case tronco_flags::data_error:
	case tronco_flags::exception_below:
	case tronco_flags::exception_worker:
	    throw SRC_BUG;
	case tronco_flags::die:
	    break;
	default:
	    throw SRC_BUG;
	}

	sync_write();
	if(tempo_write)
	    throw SRC_BUG;

	for(U_I i = 0; i < num_workers; ++i)
	{
	    tempo_write = tas->get();
	    scatter->scatter(tempo_write, static_cast<int>(order));
	}
	if(order != tronco_flags::die) // OK OK, today only "die" is expected
	    waiter->wait(); // to release threads once order has been received
    }

    void parallel_tronconneuse::go_read()
    {
	if(t_status == thread_status::dead)
	    run_threads();

	if(t_status == thread_status::suspended)
	{
	    crypto_reader->set_pos(current_position);
	    crypto_reader->set_flag(tronco_flags::normal);
	    waiter->wait(); // this should release the workers and the crypto_reader thread
	    t_status = thread_status::running;
	    check_bytes_to_skip = true;
	}
    }

    void parallel_tronconneuse::read_refill()
    {
	if(lus_data.empty() && t_status != thread_status::dead)
	{
	    if(!lus_flags.empty())
		throw SRC_BUG;
	    gather->gather(lus_data, lus_flags);

	    if(lus_flags.empty() || lus_data.empty())
		throw SRC_BUG; // show receive some blocks with either flag eof, error or data

	    if(check_bytes_to_skip && static_cast<tronco_flags>(lus_flags.front()) == tronco_flags::normal)
	    {
		infinint bytes_to_skip = crypto_reader->get_pos_in_flow();
		check_bytes_to_skip = false;

		if(!bytes_to_skip.is_zero())
		{
		    U_I to_skip = 0;

		    bytes_to_skip.unstack(to_skip);
		    if(!bytes_to_skip.is_zero())
			throw SRC_BUG; // should be castable to an U_I
		    if(lus_data.front()->clear_data.get_data_size() >= to_skip)
			lus_data.front()->clear_data.rewind_read(to_skip);
		    else
			throw SRC_BUG; // offset to skip should be within the segment
		}
	    }
	}
    }

    tronco_flags parallel_tronconneuse::purge_ratelier_from_next_order(infinint pos)
    {
	U_I num = travailleur.size(); // the number of worker
	deque<signed int>::iterator it;
	tronco_flags ret = tronco_flags::normal;

	if(t_status == thread_status::dead)
	    throw SRC_BUG;

	do
	{
	    read_refill();

		// only if no order block has been found and if pos is given

	    if(!pos.is_zero() && ret == tronco_flags::normal)
	    {
		if(find_offset_in_lus_data(pos))
		{
		    ignore_stop_acks = num;
		    num = 0; // current method will return tronco_flags::normal

			// when reading further data later on the order acks
			// we were here to purge and we have not yet reached
			// because we first found the data, will be hit.
			// however the ignore_stop_acks set above will let
			// further operation ot skip over and ignore them.
		}
	    }

	    while(!lus_flags.empty() && num > 0)
	    {
		switch(static_cast<tronco_flags>(lus_flags.front()))
		{
		case tronco_flags::stop:
		case tronco_flags::die:
		case tronco_flags::eof:
		case tronco_flags::exception_below:
		    if(ret == tronco_flags::normal) // first order found
		    {
			ret = static_cast<tronco_flags>(lus_flags.front());

			if(ret != tronco_flags::stop
			   && ret != tronco_flags::eof
			   && ignore_stop_acks > 0)
			{
			    ignore_stop_acks = 0;
				// this situation occurs when skip() triggered
				// a stop order which was not purged because
				// the requested data was found in the pipe
				// from subthreads, while the subthreads reached
				// eof and were thus suspended. In that case
				// the stop order did not triggered any ack
				// we just have to consider that the subthreads
				// are actually suspended (which status would
				// else have been set after the purge of the pending
				// stop orders)
			}
		    }

		    if(static_cast<tronco_flags>(lus_flags.front()) == ret)
		    {
			if(ignore_stop_acks > 0)
				// only when ret is tronco_flags::stop or tronco_flags::eof we can have this condition
				// (ignore_stop_acks is reset to zero in the code just above for other cases)
			{
			    --ignore_stop_acks; // first purging aborted stop acks
			    if(ignore_stop_acks == 0)
			    {
				    // now that the last stop/eof pending acks
				    // is about to be purged we can set the current thread status
				t_status = thread_status::suspended;

				if(ret != tronco_flags::eof)
				{
				    go_read();
					// yes we trigger the sub thread to push their data
					// to the ratelier up to the current order acknowledgment

				    ret = tronco_flags::normal;
					// getting ready to read the "first" order
					// now that all aborted stop order have been purged

				    pos = 0;
					// but we must ignore the pos lookup
					// as this ending purge of stop order
					// may have dropped some interleaved data
					// (normal) blocks, thus the offset of
					// data from the pipe is now unknown
				}
				else // we are done
				    num = 0;

			    }
			}
			else
			{
			    --num;
			    if(num == 0)
			    {
				if(ret == tronco_flags::die)
				    t_status = thread_status::dead;
				else
				    t_status = thread_status::suspended;

				if(ret == tronco_flags::exception_below)
				{
				    t_status = thread_status::dead;
					// all threads should have ended
					// below thread ended with an exception
				    join_threads();
					// we thus relaunch the exception in the curren thread
					// else this is a bug:
				    throw SRC_BUG;
				}
			    }

			}
		    }
		    else
			throw SRC_BUG; // melted orders in the pipe!
			// the N order should follow each other before and
			// not be mixed with other orders
		    break;
		case tronco_flags::exception_worker:
			// unlike other orders this one could be single in the pipe
			// we just have to drive the threads to die to
			// get the worker exception thrown in the current thread
		    lus_flags.pop_front();
		    tas->put(std::move(lus_data.front()));
		    lus_data.pop_front();
		    send_read_order(tronco_flags::die);	// this leads to a recursive call !!!
		    join_threads(); // this should propagate the worker exception
		    throw SRC_BUG; // else this is a bug condition
		case tronco_flags::normal:
			// we get here only if a order block
			// has been found or if pos equals zero
			// and we drop any further data block now
		    break;
		case tronco_flags::data_error:
		    break;
		default:
		    throw SRC_BUG;
		}

		lus_flags.pop_front();
		tas->put(std::move(lus_data.front()));
		lus_data.pop_front();
	    }
	}
	while(num > 0);

	return ret;
    }

    bool parallel_tronconneuse::purge_unack_stop_order(const infinint & pos)
    {
	bool ret = pos.is_zero() || !find_offset_in_lus_data(pos);
	bool loop = ignore_stop_acks > 0 && ret;
	    // we loop only if there is some pending acks to purge
	    // and if either we do not have to look for a given offset
	    // or we could not find the pos position in lus_data
	bool lookup = true;
	    // if lookup becomes false, we ignore pos and the offset lookup

	if(t_status == thread_status::dead)
	    throw SRC_BUG;

	while(loop)
	{
	    read_refill();

	    while(!lus_flags.empty() && loop)
	    {
		switch(static_cast<tronco_flags>(lus_flags.front()))
		{
		case tronco_flags::stop:
		case tronco_flags::eof: // eof can be found
		    if(ignore_stop_acks == 0)
			throw SRC_BUG;
		    else
			--ignore_stop_acks;

		    if(ignore_stop_acks == 0)
			loop = false;
			// some data block could be present in lus_data
			// after this last stop block and could match
			// the expected offset, but we would have to retur
			// true as purge order completed but would
			// also to return false at the same time as the
			// expected offset has been found... so we
			// ignore any data past the last stop block
			// even if it is accessible and could match
			// our lookup
		    break;
		case tronco_flags::die:
		    throw SRC_BUG;
		case tronco_flags::normal:
		case tronco_flags::data_error:
		    if(lookup && ! pos.is_zero() && find_offset_in_lus_data(pos))
		    {
			loop = false;
			ret = false;
		    }
		    else
		    {
			    // different fates car take place due to find_offset_in_lus_data() exec:
			    // - the original data block is in place (requested offset before current)
			    // - original block has been removed (lus_data eventually void) if
			    //   the requested offset if past all data block that could be found
			    //   in the pipe (but for an non data block is hit), same situation as if
			    //   find_offset_in_lus_data() has not been executed (pos == 0)
			    // - the front() block in lus_data is now a non-data block
			if( ! lus_flags.empty())
			{
			    if(static_cast<tronco_flags>(lus_flags.front()) != tronco_flags::data_error
			       && static_cast<tronco_flags>(lus_flags.front()) != tronco_flags::normal)
				continue;
				// find_offset_in_lus_data has purged all the data entry before a non-data
				// one, we must not purge this block before inspecting it (code below) but
				// have to restart the while loop from the beginning (so we "continue")
				// ELSE we exit the switch construct (break) and execute the purge code
				// after it (leading data block has to be removed)
			    else
				lookup = false;
				// the code aftet this switch construct will "manually" remove the
				// leading data block without current_offset update,
				// which will make the current offset info wrong
				// se must not more look for an offset (it has no use,
				// because find_offset_in_lus_data()) would have remove all data block
				// eventually hitting a non-data block if there was chance to find the
				// requested offset looking forward.
			}
		    }
		    break;
		case tronco_flags::exception_below:
		    join_threads();
		    throw SRC_BUG;
		case tronco_flags::exception_worker:
		    purge_ratelier_from_next_order();
		    throw SRC_BUG;
		default:
		    throw SRC_BUG;
		}

		if(loop || ret)
		{
		    if(!lus_flags.empty())
		    {
			lus_flags.pop_front();
			if(lus_data.empty())
			    throw SRC_BUG;
			tas->put(std::move(lus_data.front()));
			lus_data.pop_front();
		    }
		}
	    }
	}

	if(ret)
	    t_status = thread_status::suspended;

	return ret;
    }

    bool parallel_tronconneuse::find_offset_in_lus_data(const infinint & pos)
    {
	bool search = true;
	bool found = false;

		    // checking first whether we do not already have the requested data

	while(search && !found)
	{
	    if(lus_data.empty())
	    {
		search = false;
		continue;
	    }

	    if(lus_flags.empty())
		throw SRC_BUG;

	    if(static_cast<tronco_flags>(lus_flags.front()) != tronco_flags::normal)
	    {
		search = false;
		continue;
	    }

	    if(pos < current_position)
	    {
		infinint lu = lus_data.front()->clear_data.get_read_offset();

		if(current_position <= pos + lu)
		{
		    U_I lu_tmp = 0;

		    lu -= current_position - pos;
		    lu.unstack(lu_tmp);
		    if(!lu.is_zero())
			throw SRC_BUG; // we should be in the range of a U_I
		    lus_data.front()->clear_data.rewind_read(lu_tmp);
		    current_position = pos;
		    found = true;
		}
		else
		    search = false;
		    // else we need to send_read_order()
		    // requested offset is further before
		    // the earliest data we have in lus_data
	    }
	    else // pos > current_position (the case pos == current_position has already been seen)
	    {
		infinint alire = lus_data.front()->clear_data.get_data_size()
		    - lus_data.front()->clear_data.get_read_offset();

		if(pos < current_position + alire)
		{
		    U_I alire_tmp = 0;

		    alire = infinint(lus_data.front()->clear_data.get_read_offset()) + pos - current_position;
		    alire.unstack(alire_tmp);
		    if(!alire.is_zero())
			throw SRC_BUG; // we should be in the range of a U_I
		    lus_data.front()->clear_data.rewind_read(alire_tmp);
		    current_position = pos;
		    found = true;
		}
		else
		{
		    current_position += alire;
		    tas->put(std::move(lus_data.front()));
		    lus_data.pop_front();
		    lus_flags.pop_front();
		    if(current_position == pos && !lus_data.empty())
			found = true;
		}
	    }
	}

	return found;
    }

    void parallel_tronconneuse::run_threads()
    {
	if(t_status != thread_status::dead)
	    throw SRC_BUG;

	if(!scatter)
	    throw SRC_BUG;
	scatter->reset();

	if(!gather)
	    throw SRC_BUG;
	gather->reset();

	tas->put(lus_data);
	lus_data.clear();
	lus_flags.clear();
	lus_eof = false;
	check_bytes_to_skip = true;

	deque<unique_ptr<crypto_worker> >::iterator it = travailleur.begin();
	while(it != travailleur.end())
	{
	    if((*it) != nullptr)
		(*it)->run();
	    else
		throw SRC_BUG;
	    ++it;
	}

	switch(get_mode())
	{
	case gf_read_only:
	    if(!crypto_reader)
		throw Ememory("parallel_tronconneuse::parallel_tronconneuse");
	    else
		crypto_reader->run();
	    break;
	case gf_write_only:
	    if(!crypto_writer)
		throw Ememory("parallel_tronconneuse::parallel_tronconneuse");
	    else
		crypto_writer->run();
	    waiter->wait(); // release all threads
		// in write mode sub-threads
		// are not pending for an order
		// but for data to come and tread
	    break;
	case gf_read_write:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}

	    // all subthreads are pending on waiter barrier

	t_status = thread_status::suspended;
    }

    void parallel_tronconneuse::stop_threads()
    {
	if(t_status == thread_status::dead)
	    return;

	if(ignore_stop_acks > 0)
	{
	    if(!purge_unack_stop_order())
		throw SRC_BUG;
		// we just need to remove the stop pending order
		// to be sure the threads are stop and in condition
		// to receive the die order
	}

	if(get_mode() == gf_read_only)
	    send_read_order(tronco_flags::die);
	else
	    send_write_order(tronco_flags::die);
    }

    void parallel_tronconneuse::join_workers_only()
    {
	deque<unique_ptr<crypto_worker> >::iterator it = travailleur.begin();

	while(it != travailleur.end())
	{
	    if((*it)  != nullptr)
		(*it)->join();
	    else
		throw SRC_BUG;
	    ++it;
	}
    }

    void parallel_tronconneuse::join_threads()
    {
	try
	{
	    if(get_mode() == gf_read_only)
		crypto_reader->join(); // may propagate exception thrown in child thread
	    else
		crypto_writer->join(); // may propagate exception thrown in child thread
	}
	catch(...)
	{
	    join_workers_only();
	    t_status = thread_status::dead;
	    throw;
	}
	join_workers_only();
	t_status = thread_status::dead;
    }

    U_I parallel_tronconneuse::get_heap_size(U_I num_workers)
    {
	U_I ratelier_size = get_ratelier_size(num_workers);
	U_I heap_size = ratelier_size * 2 + num_workers + 1 + ratelier_size + 2;
	    // each ratelier can be full of crypto_segment and at the same
	    // time, each worker could hold a crypto_segment, the below thread
	    // as well and the main thread for parallel_tronconneuse could hold
	    // a deque of the size of the ratelier plus 2 more crypto_segments
	return heap_size;
    }



	/////////////////////////////////////////////////////
	//
	// read_blow class implementation
	//
	//

    read_below::read_below(const std::shared_ptr<libthreadar::ratelier_scatter<crypto_segment> > & to_workers,
			   const std::shared_ptr<libthreadar::barrier> & waiter,
			   U_I num_workers,
			   U_I clear_block_size,
			   generic_file* encrypted_side,
			   const std::shared_ptr<heap<crypto_segment> > xtas,
			   infinint init_shift):
	workers(to_workers),
	waiting(waiter),
	num_w(num_workers),
	clear_buf_size(clear_block_size),
	encrypted(encrypted_side),
	tas(xtas),
	initial_shift(init_shift),
	reof(false),
	trailing_clear_data(nullptr),
	read_ahead_set(false)
    {
#ifdef LIBTHREADAR_STACK_FEATURE_AVAILABLE
	set_stack_size(LIBDAR_DEFAULT_STACK_SIZE);
#endif

	flag = tronco_flags::normal;
    }

    void read_below::read_ahead_up_to(const infinint & offset)
    {
	ra_cntrl.lock();

	try
	{
		// we ignore previously set read-ahead because
		// if we ask for another one this means we could
		// get the data to read before this previous
		// read-ahead request could be taken into account
	    read_ahead_set = true;
	    read_ahead_offset = offset;
	}
	catch(...)
	{
	    ra_cntrl.unlock();
	    throw;
	}
	ra_cntrl.unlock();
    }

    void read_below::inherited_run()
    {
	try
	{
	    if(!waiting)
		throw SRC_BUG;
	    else
		waiting->wait(); // initial sync before starting reading data

		// initializing clear and encryted buf size
		// needed by get_ready_for_new_offset

	    ptr = tas->get();
	    if(ptr->clear_data.get_max_size() < clear_buf_size)
	    {
		tas->put(std::move(ptr));
		throw SRC_BUG;
	    }
	    encrypted_buf_size = ptr->crypted_data.get_max_size();
	    tas->put(std::move(ptr));
	    index_num = get_ready_for_new_offset();

	    work();
	}
	catch(...)
	{
	    send_flag_to_workers(tronco_flags::exception_below);
	    throw; // this will end the current thread with an exception
	}
    }

    void read_below::work()
    {
	bool end = false;

	do
	{
	    cancellation_checkpoint();
	    switch(flag)
	    {

		    // flag can be modifed anytime by the main thread, we must not assume
		    // its value in a given case to be equal to the what the switch directive
		    // pointed to
	    case tronco_flags::die:
		if(reof) // eof collided with a received order, continuing the eof process
		    flag = tronco_flags::eof;
		else
		{
		    send_flag_to_workers(tronco_flags::die);
		    end = true;
		}
		break;
	    case tronco_flags::eof:
		send_flag_to_workers(tronco_flags::eof);
		waiting->wait();
		if(flag == tronco_flags::eof)
		    throw SRC_BUG; // new order has not been set
		if(flag == tronco_flags::normal)
		    index_num = get_ready_for_new_offset();
		reof = false; // assuming condition has changed
		break;
	    case tronco_flags::stop:
		if(reof) // eof collided with a received order, continuing the eof process
		    flag = tronco_flags::eof;
		else
		{
		    send_flag_to_workers(tronco_flags::stop);
		    waiting->wait(); // We are suspended here until the caller has set new orders
		    switch(flag)
		    {
		    case tronco_flags::die:
			break;
		    case tronco_flags::normal:
			index_num = get_ready_for_new_offset();
			break;
		    case tronco_flags::stop:
			break; // possible if partial purging the lus_data by the main thread
		    case tronco_flags::data_error:
			throw SRC_BUG;
		    case tronco_flags::eof:
			throw SRC_BUG;
		    default:
			throw SRC_BUG;
		    }
		}
		break;
	    case tronco_flags::normal:
		if(ptr)
		    throw SRC_BUG; // we should not have a block at this stage

		check_read_ahead();

		if(!reof)
		{
		    infinint local_crypt_offset = encrypted->get_position();

		    ptr = tas->get(); // obtaining a new segment from the heap
		    ptr->reset();

		    ptr->crypted_data.set_data_size(encrypted->read(ptr->crypted_data.get_addr(), ptr->crypted_data.get_max_size()));
		    if( ! ptr->crypted_data.is_full())  // we have reached eof
		    {
			if(trailing_clear_data != nullptr && ptr->crypted_data.get_data_size() > 0) // and we have a callback to remove clear data at eof
			{
			    unique_ptr<crypto_segment> tmp = nullptr;

			    remove_trailing_clear_data_from_encrypted_buf(local_crypt_offset,
									  version,
									  initial_shift,
									  trailing_clear_data,
									  ptr,
									  tmp,
									  reof);
			}
			reof = true;
			    // whatever the callback modified, we could not read the whole block
			    // so we are at end of file, reof should be set to true in any case
		    }

		    if(ptr->crypted_data.get_data_size() > 0)
		    {
			ptr->block_index = index_num++;
			workers->scatter(ptr, static_cast<int>(tronco_flags::normal));
		    }
		    else // we could not read anything from encrypted (or it has been trailed down to zero)
		    {
			tas->put(std::move(ptr));
			reof = true;
		    }
		}
		else
		    flag = tronco_flags::eof;
		break;
	    case tronco_flags::data_error:
		throw SRC_BUG;
	    case tronco_flags::exception_below:
		throw SRC_BUG;
	    case tronco_flags::exception_worker:
		throw SRC_BUG;
	    default:
		throw SRC_BUG;
	    }

	}
	while(!end);
    }

    infinint read_below::get_ready_for_new_offset()
    {
	infinint ret;
	infinint local_crypt_offset;

	position_clear2crypt(skip_to,
			     local_crypt_offset,
			     clear_flow_start,
			     pos_in_flow,
			     ret);

	if(!encrypted->skip(local_crypt_offset + initial_shift))
	    reof = true;
	else
	    reof = false;

	return ret;
    }

    void read_below::send_flag_to_workers(tronco_flags theflag)
    {
	unique_ptr<crypto_segment> ptr;

	for(unsigned int i = 0; i < num_w; ++i)
	{
	    ptr = tas->get();
	    ptr->reset();
	    workers->scatter(ptr, static_cast<int>(theflag));
	}
    }

    void read_below::position_clear2crypt(const infinint & pos,
					  infinint & file_buf_start,
					  infinint & clear_buf_start,
					  infinint & pos_in_buf,
					  infinint & block_num)
    {
	euclide(pos, clear_buf_size, block_num, pos_in_buf);
	file_buf_start = block_num * infinint(encrypted_buf_size);
	clear_buf_start = block_num * infinint(clear_buf_size);
    }


    void read_below::check_read_ahead()
    {
	if(read_ahead_set)
	{
	    ra_cntrl.lock();

	    try
	    {
		read_ahead_set = false;

		if(!reof)
		{
		    infinint current = encrypted->get_position();

		    if(current < read_ahead_offset)
			encrypted->read_ahead(read_ahead_offset - current);
		}
	    }
	    catch(...)
	    {
		ra_cntrl.unlock();
		throw;
	    }
	    ra_cntrl.unlock();
	}
    }

	/////////////////////////////////////////////////////
	//
	// below_writer implementation
	//
	//

    write_below::write_below(const std::shared_ptr<libthreadar::ratelier_gather<crypto_segment> > & from_workers,
			     const std::shared_ptr<libthreadar::barrier> & waiter,
			     U_I num_workers,
			     generic_file* encrypted_side,
			     const std::shared_ptr<heap<crypto_segment> > xtas):
	workers(from_workers),
	waiting(waiter),
	num_w(num_workers),
	cur_num_w(0),
	encrypted(encrypted_side),
	tas(xtas),
	error(false),
	error_block(0)
    {
#ifdef LIBTHREADAR_STACK_FEATURE_AVAILABLE
	set_stack_size(LIBDAR_DEFAULT_STACK_SIZE);
#endif

	if(encrypted == nullptr) throw SRC_BUG;
    }


    void write_below::inherited_run()
    {
	error = false;
	error_block = 0;
	cur_num_w = num_w;
	try
	{
	    if(!waiting || !workers)
		throw SRC_BUG;
	    else
		waiting->wait(); // initial sync with other threads

	    work();
	}
	catch(...)
	{
	    error = true;
	    try
	    {
		work();
	    }
	    catch(...)
	    {
		    // ignore further exceptions
	    }
	    throw;
	}
    }

    void write_below::work()
    {
	bool end = false;

	do
	{
	    cancellation_checkpoint();

	    if(ones.empty())
	    {
		if(!flags.empty())
		    throw SRC_BUG;
		workers->gather(ones, flags);
	    }

	    if(ones.empty() || flags.empty())
	    {
	       if(!error)
		   throw SRC_BUG;
	       else
		   end = true; // avoiding endless loop
	    }

	    switch(static_cast<tronco_flags>(flags.front()))
	    {
	    case tronco_flags::normal:
		if(!error)
		{
		    error_block = ones.front()->block_index;
		    encrypted->write(ones.front()->crypted_data.get_addr(),
				     ones.front()->crypted_data.get_data_size());
		}
		break;
	    case tronco_flags::stop:
		if(!error)
		    throw SRC_BUG;
		break;
	    case tronco_flags::eof:
		if(!error)
		    throw SRC_BUG;
		break;
	    case tronco_flags::data_error:
		    // error reported by a worker while encrypting
		    // we ignore it and set the flag
		error = true;
		break;
	    case tronco_flags::die:
		    // read num dies and push them back to tas
		--cur_num_w;
		if(cur_num_w == 0)
		    end = true;
		break;
	    case tronco_flags::exception_below:
		if(!error)
		    throw SRC_BUG;
		break;
	    case tronco_flags::exception_worker:
		error = true;
		break;
	    default:
		if(!error)
		    throw SRC_BUG;
	    }

	    tas->put(std::move(ones.front()));
	    ones.pop_front();
	    flags.pop_front();
	}
	while(!end);
    }


    	/////////////////////////////////////////////////////
	//
	// crypto_worker implementation
	//
	//

    crypto_worker::crypto_worker(std::shared_ptr<libthreadar::ratelier_scatter <crypto_segment> > & read_side,
				 std::shared_ptr<libthreadar::ratelier_gather <crypto_segment> > & write_side,
				 std::shared_ptr<libthreadar::barrier> waiter,
				 std::unique_ptr<crypto_module> && ptr,
				 bool encrypt):
	reader(read_side),
	writer(write_side),
	waiting(waiter),
	crypto(std::move(ptr)),
	do_encrypt(encrypt),
	abort(status::fine)
    {
#ifdef LIBTHREADAR_STACK_FEATURE_AVAILABLE
	set_stack_size(LIBDAR_DEFAULT_STACK_SIZE);
#endif

	if(!reader || !writer || !waiting || !crypto)
	    throw SRC_BUG;
    }


    void crypto_worker::inherited_run()
    {
	try
	{
	    waiting->wait(); // initial sync before starting working
	    work();
	}
	catch(...)
	{
	    try
	    {
		abort = status::inform;
		work();
	    }
	    catch(...)
	    {
		    // we ignore exception here
	    }

	    throw; // propagating the original exception
	}
    }

    void crypto_worker::work()
    {
	bool end = false;
	signed int flag;

	do
	{
	    cancellation_checkpoint();

	    ptr = reader->worker_get_one(slot, flag);

	    switch(static_cast<tronco_flags>(flag))
	    {
	    case tronco_flags::normal:
		switch(abort)
		{
		case status::fine:
		    if(!ptr)
			throw SRC_BUG;

		    try
		    {
			if(do_encrypt)
			{
			    ptr->crypted_data.set_data_size(crypto->encrypt_data(ptr->block_index,
										 ptr->clear_data.get_addr(),
										 ptr->clear_data.get_data_size(),
										 ptr->clear_data.get_max_size(),
										 ptr->crypted_data.get_addr(),
										 ptr->crypted_data.get_max_size()));
			    ptr->crypted_data.rewind_read();
			}
			else
			{
			    ptr->clear_data.set_data_size(crypto->decrypt_data(ptr->block_index,
									       ptr->crypted_data.get_addr(),
									       ptr->crypted_data.get_data_size(),
									       ptr->clear_data.get_addr(),
									       ptr->clear_data.get_max_size()));
			    ptr->clear_data.rewind_read();
			}
		    }
		    catch(Erange & e)
		    {
			flag = static_cast<int>(tronco_flags::data_error);
			    // we will push the block with this flag data_error
		    }
		    break;
		case status::inform:
		    flag = static_cast<int>(tronco_flags::exception_worker);
		    abort = status::sent;
		    break;
		case status::sent:
		    break;
		default:
			// we can't report this problem
			// because we can trigger an exception
			// only when abort equals status fine
		    break;
		}
		writer->worker_push_one(slot, ptr, flag);
		break;
	    case tronco_flags::stop:
	    case tronco_flags::eof:
		writer->worker_push_one(slot, ptr, flag);
		waiting->wait();
		break;
	    case tronco_flags::die:
		end = true;
		writer->worker_push_one(slot, ptr, flag);
		break;
	    case tronco_flags::data_error:
		if(abort == status::fine)
		    throw SRC_BUG; // should not receive a block with that flag
		break;
	    case tronco_flags::exception_below:
		end = true;
		writer->worker_push_one(slot, ptr, flag);
		break;
	    case tronco_flags::exception_worker:
		if(abort == status::fine)
		    throw SRC_BUG;
		break;
	    default:
		throw SRC_BUG;
	    }
	}
	while(!end);
    }

    	/////////////////////////////////////////////////////
	//
	// static routines implementation
	//
	//


   static void remove_trailing_clear_data_from_encrypted_buf(const infinint & read_offset,    ///< offset of the first byte of the 'first' segment
							     const archive_version & reading_ver, ///< read archive format version
							     const infinint & initial_shift,  ///< amount of free bytes before encrypted ones
							     infinint (*callback)(generic_file & below, const archive_version& reading_version),
							     unique_ptr<crypto_segment> & first, ///< pointer to a existing segment
							     unique_ptr<crypto_segment> & opt_next, ///< in option pointer to the segment following the first in the crypted data
							     bool & reof) ///< may be set to true if clear data is found after encrypted one

    {
	infinint clear_offset = 0;
	memory_file tmp;

	if(!first)
	    throw SRC_BUG;

	tmp.write(first->crypted_data.get_addr(), first->crypted_data.get_data_size());
	if(opt_next)
	    tmp.write(opt_next->crypted_data.get_addr(), opt_next->crypted_data.get_data_size());
	if(callback == nullptr)
	    throw SRC_BUG;
	try
	{
	    clear_offset = (*callback)(tmp, reading_ver);
	    if(clear_offset >= initial_shift)
		clear_offset -= initial_shift;
		// now clear_offset can be compared to read_offset
	    else
		return;
	}
	catch(Egeneric & e)
	{
	    return; // we no nothing
	}

	if(read_offset >= clear_offset) // all data in encrypted_buf is clear data
	{
	    first->reset();
	    if(opt_next)
		opt_next->reset(); // empty the second segment
	    reof = true;
	}
	else
	{
		// finding where clear_data is located in the providing buffer(s)

	    U_I nouv_buf_data = 0;

	    clear_offset -= read_offset;
	    clear_offset.unstack(nouv_buf_data);
	    if(!clear_offset.is_zero())
		throw SRC_BUG; // cannot handle that integer as U_32 while this number should be less than encrypted_buf_size which is a U_32

	    if(nouv_buf_data <= first->crypted_data.get_data_size())
	    {
		first->crypted_data.set_data_size(nouv_buf_data);
		first->crypted_data.rewind_read();
		if(opt_next)
		    opt_next->reset();
		reof = true;
	    }
	    else
	    {
		nouv_buf_data -= first->crypted_data.get_data_size();
		if(opt_next)
		{
		    if(nouv_buf_data <= opt_next->crypted_data.get_data_size())
		    {
			opt_next->crypted_data.set_data_size(nouv_buf_data);
			opt_next->crypted_data.rewind_read();
			reof = true;
		    }
		    else
			throw SRC_BUG; // more encrypted data than could be read so far!
		}
		else
		    throw SRC_BUG; // more encrypted data than could be read so far!
	    }
	}
    }

} // end of namespace
