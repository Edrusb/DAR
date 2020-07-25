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
}

#include "parallel_tronconneuse.hpp"
#include "tools.hpp"

using namespace std;

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
	// read_blow class implementation
	//
	//

    void read_below::inherited_run()
    {
	bool end = false;
	unique_ptr<crypto_segment> ptr;
	infinint index_num;
	if(!waiting)
	    throw SRC_BUG;
	else
	    waiting->wait(); // initial sync before starting reading data

	    // initializing clear and encryted buf size
	    // needed by get_ready_for_new_offset
	ptr = tas->get();
	encrypted_buf_size = ptr->crypted_data.get_max_size();
	clear_buf_size = ptr->clear_data.get_max_size();
	tas->put(move(ptr));
	index_num = get_ready_for_new_offset();

	do
	{
	    switch(flag)
	    {
	    case tronco_flags::die:
		send_flag_to_workers();
		end = true;
		break;
	    case tronco_flags::eof:
		send_flag_to_workers();
		waiting->wait();
		if(flag == tronco_flags::eof)
		    throw SRC_BUG; // new order has not been set
		if(flag == tronco_flags::normal)
		    index_num = get_ready_for_new_offset();
		break;
	    case tronco_flags::stop:
		send_flag_to_workers();
		waiting->wait(); // We are suspended here until the caller has set new orders
		if(flag == tronco_flags::die)
		    break;
		if(flag == tronco_flags::normal)
		    index_num = get_ready_for_new_offset();
		else
		    throw SRC_BUG;
		break;
	    case tronco_flags::normal:
		if(ptr)
		    throw SRC_BUG; // show not have a block at this stage

		if(!reof)
		{
		    ptr = tas->get(); // obtaining a new segment from the heap
		    ptr->reset();

		    ptr->crypted_data.set_data_size(encrypted->read(ptr->crypted_data.get_addr(), ptr->crypted_data.get_max_size()));
		    if( ! ptr->crypted_data.is_full())  // we have reached eof
		    {
			if(trailing_clear_data != nullptr) // and we have a callback to remove clear data at eof
			{
			    unique_ptr<crypto_segment> tmp = nullptr;

			    remove_trailing_clear_data_from_encrypted_buf(crypt_offset,
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

		    ptr->block_index = index_num++;
		    crypt_offset += ptr->crypted_data.get_data_size();
		    workers->scatter(ptr, static_cast<int>(flag));
		}
		else
		    flag = tronco_flags::eof;
		break;
	    default:
		throw SRC_BUG;
	    }

	}
	while(!end);
    }

    infinint read_below::get_ready_for_new_offset()
    {
	infinint ret;

	position_clear2crypt(skip_to,
			     crypt_offset,
			     clear_flow_start,
			     pos_in_flow,
			     ret);

	if(!encrypted->skip(crypt_offset + initial_shift))
	    reof = true;
	else
	    reof = false;

	return ret;
    }

    void read_below::send_flag_to_workers()
    {
	unique_ptr<crypto_segment> ptr;

	for(unsigned int i = 0; i < num_w; ++i)
	{
	    ptr = tas->get();
	    ptr->reset();
	    workers->scatter(ptr, static_cast<int>(flag));
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



    	/////////////////////////////////////////////////////
	//
	// read_worker implementation
	//
	//


    void read_worker::inherited_run()
    {
	unique_ptr<crypto_segment> ptr;
	bool end = false;
	unsigned int slot;
	signed int flag;

	waiting->wait(); // initial sync before starting working
	do
	{
	    ptr = reader->worker_get_one(slot, flag);

	    switch(static_cast<tronco_flags>(flag))
	    {
	    case tronco_flags::normal:
		if(!ptr)
		    throw SRC_BUG;

		try
		{
		    ptr->clear_data.set_data_size(crypto->decrypt_data(ptr->block_index,
								       ptr->crypted_data.get_addr(),
								       ptr->crypted_data.get_data_size(),
								       ptr->clear_data.get_addr(),
								       ptr->clear_data.get_max_size()));
		    ptr->clear_data.rewind_read();
		    writer->worker_push_one(slot, ptr, static_cast<int>(flag));
		}
		catch(Erange & e)
		{
		    writer->worker_push_one(slot, ptr, static_cast<int>(tronco_flags::data_error));
		}
		break;
	    case tronco_flags::stop:
	    case tronco_flags::eof:
		writer->worker_push_one(slot, ptr, static_cast<int>(flag));
		waiting->wait();
		break;
	    case tronco_flags::die:
		end = true;
		break;
	    case tronco_flags::data_error:
		break;
	    default:
		throw SRC_BUG;
	    }
	}
	while(!end);

    }

    	/////////////////////////////////////////////////////
	//
	// parallel_tronconneuse implementation
	//
	//

    parallel_tronconneuse::parallel_tronconneuse(U_I workers,
						 U_32 block_size,
						 generic_file & encrypted_side,
						 bool no_initial_shift,
						 const archive_version & ver,
						 std::unique_ptr<crypto_module> & crypto_ptr):
	generic_file(encrypted_side.get_mode() == gf_read_only ? gf_read_only : gf_write_only)
    {
	if(clear_block_size == 0)
	    throw Erange("parallel_tronconneuse::parallel_tronconneuse", tools_printf(gettext("%d is not a valid block size"), block_size));

	initialized = false;
	num_workers = workers;
	clear_block_size = block_size;
	current_position = 0;
	if(! no_initial_shift)
	    initial_shift = encrypted_side.get_position();
	else
	    initial_shift = 0;
	reading_ver = ver;
	crypto = move(crypto_ptr);
	suspended = true; // child threads are waiting us on the barrier
	mycallback = nullptr;
	encrypted = &encrypted_side; // used for further reference, thus the encrypted object must survive "this"
	lus_data.clear();
	lus_flags.clear();
	lus_eof = false;
	check_bytes_to_skip = true;

	if(!crypto)
	    throw SRC_BUG;

	try
	{
	    scatter = make_shared<libthreadar::ratelier_scatter<crypto_segment> >(num_workers);
	    if(!scatter)
		throw Ememory("parallel_tronconneuse::parallel_tronconneuse");

	    gather = make_shared<libthreadar::ratelier_gather<crypto_segment> >(num_workers);
	    if(!gather)
		throw Ememory("parallel_tronconneuse::parallel_tronconneuse");

	    waiter = make_shared<libthreadar::barrier>(num_workers + 2); // +1 for crypto_reade thread, +1 this thread
	    if(!waiter)
		throw Ememory("parallel_tronconneuse::parallel_tronconneuse");

		// tas is created empty, it will be filled by post_constructor_init()
	    tas = make_shared<heap<crypto_segment> >();
	    if(!tas)
		throw Ememory("parallel_tronconneuse::parallel_tronconneuse");

	    for(U_I i = 0; i < workers; ++i)
		travailleur.push_back(read_worker(scatter,
						  gather,
						  waiter,
						  crypto->clone()));

	    crypto_reader = make_unique<read_below>(scatter,
						    waiter,
						    num_workers,
						    encrypted,
						    reading_ver,
						    tas,
						    initial_shift);
	    if(!crypto_reader)
		throw Ememory("parallel_tronconneuse::parallel_tronconneuse");
	}
	catch(std::bad_alloc &)
	{
	    throw Ememory("parallel_tronconneuse::parallel_tronconneuse");
	}
    }

    parallel_tronconneuse::~parallel_tronconneuse()
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
	post_constructor_init();
	send_order(tronco_flags::stop);
	return encrypted->skippable(direction, amount);
    }

    bool parallel_tronconneuse::skip(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;
	else
	    post_constructor_init();

	if(encrypted->get_mode() != gf_read_only)
	    throw SRC_BUG;

	send_order(tronco_flags::stop);
	current_position = pos;
	lus_eof = false;
	return true;
    }

    bool parallel_tronconneuse::skip_to_eof()
    {
	bool ret;

	if(is_terminated())
	    throw SRC_BUG;
	else
	    post_constructor_init();

	if(encrypted->get_mode() != gf_read_only)
	    throw SRC_BUG;

	send_order(tronco_flags::stop);

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
		tas->put(move(aux));
		throw;
	    }
	    tas->put(move(aux));
	}

	return ret;
    }

    bool parallel_tronconneuse::skip_relative(S_I x)
    {
	bool ret;

	if(is_terminated())
	    throw SRC_BUG;

	if(encrypted->get_mode() != gf_read_only)
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
	else
	    post_constructor_init();

	if(!suspended)
	    send_order(tronco_flags::stop);
	initial_shift = x;
	crypto_reader->set_initial_shift(x);
    }

    void parallel_tronconneuse::inherited_read_ahead(const infinint & amount)
    {
	    // nothing special to do, the below thread
	    // will read as much as possible until it
	    // reaches eof or is interrupted by the
	    // master thread

	if(is_terminated())
	    throw SRC_BUG;
	else
	{
	    post_constructor_init();
	    go_read();
	}
    }

    U_I parallel_tronconneuse::inherited_read(char *a, U_I size)
    {
	U_I ret = 0;

	post_constructor_init();
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
		    tas->put(move(lus_data.front()));
		    lus_data.pop_front();
		    lus_flags.pop_front();
		}
		break;
	    case tronco_flags::stop:
		throw SRC_BUG; // should receive a stop flag only upon stop request
	    case tronco_flags::eof:
		lus_eof = true;
		purge_ratelier_up_to(tronco_flags::eof);
		tas->put(lus_data);
		lus_data.clear();
		lus_flags.clear();
		suspended = true;
		break;
	    case tronco_flags::die:
		throw SRC_BUG; // should never receive a die flag
	    case tronco_flags::data_error:
		if(lus_data.empty())
		    throw SRC_BUG; // not the same amount of item in lus_data than in lus_flags!
		if(!lus_data.front())
		    throw SRC_BUG; // front does not point to any crypto_segment pointed object

		if(mycallback != nullptr)
		{
		    unique_ptr<crypto_segment> & current = lus_data.front();
		    if(!current)
			throw SRC_BUG;

		    try
		    {
			infinint crypt_offset = current->block_index * crypto->encrypted_block_size_for(clear_block_size) + initial_shift;

			lus_data.pop_front();
			lus_flags.pop_front();
			read_refill(); // to be sure we will have the next available segment

			if(lus_flags.front() == static_cast<int>(tronco_flags::normal)
			   || lus_flags.front() == static_cast<int>(tronco_flags::data_error))
			{
			    if(lus_data.empty())
				throw SRC_BUG; // an element should be as lus_flags is not empty
			    unique_ptr<crypto_segment> & opt = lus_data.front(); // this is a reference on an unique_ptr

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
			current->clear_data.set_data_size(crypto->decrypt_data(current->block_index,
									       current->crypted_data.get_addr(),
									       current->crypted_data.get_data_size(),
									       current->clear_data.get_addr(),
									       current->clear_data.get_max_size()));
			current->clear_data.rewind_read();
		    }
		    catch(...)
		    {
			tas->put(move(current));
			throw;
		    }
		    lus_data.push_front((move(current)));
		    lus_flags.push_front(static_cast<int>(tronco_flags::normal));
		}
		else
		{
		    unique_ptr<crypto_segment> & current = lus_data.front(); // current is here a reference to an unique_ptr

			// this should throw the exception met by the worker
		    current->clear_data.set_data_size(crypto->decrypt_data(current->block_index,
									   current->crypted_data.get_addr(),
									   current->crypted_data.get_data_size(),
									   current->clear_data.get_addr(),
									   current->clear_data.get_max_size()));
		    current->clear_data.rewind_read();
		}
		break;
	    default:
		throw SRC_BUG;
	    }
	}

	return ret;
    }

    void parallel_tronconneuse::inherited_write(const char *a, U_I size)
    {
	throw Efeature("To be implemented soon");
    }

    void parallel_tronconneuse::inherited_sync_write()
    {
	throw Efeature("To be implemented soon");
    }

    void parallel_tronconneuse::inherited_flush_read()
    {
	post_constructor_init();
	send_order(tronco_flags::stop);
	purge_ratelier_up_to(tronco_flags::stop);
    }

    void parallel_tronconneuse::inherited_terminate()
    {
	deque<read_worker>::iterator it = travailleur.begin();

	post_constructor_init();
	send_order(tronco_flags::die);
	crypto_reader->join(); // may propagate exception thrown in child thread
	while(it != travailleur.end())
	{
	    it->join(); // may propagate exception thrown in child thread
	    ++it;
	}
    }

    void parallel_tronconneuse::post_constructor_init()
    {
	if(!initialized)
	{
	    U_32 crypted_block_size = crypto->encrypted_block_size_for(clear_block_size);
	    U_I heap_size = num_workers * 3 + 3;
		// each ratelier can be full of crypto_segment and at the same
		// time, each worker could hold a crypto_segment the below thread
		// as well and the main thread for parallel_tronconneuse could hold
		// 2 more crypto_segments

	    initialized = true;

	    try
	    {
		for(U_I i = 0; i < heap_size; ++i)
		    tas->put(make_unique<crypto_segment>(crypted_block_size, clear_block_size));
	    }
	    catch(std::bad_alloc &)
	    {
		throw Ememory("tronconneuse::post_constructor_init");
	    }

		// launching all subthreads
	    if(!crypto_reader)
		throw SRC_BUG;
	    else
		crypto_reader->run();

	    for(deque<read_worker>::iterator it = travailleur.begin(); it != travailleur.end(); ++it)
		it->run();
	}
    }

    void parallel_tronconneuse::send_order(tronco_flags order)
    {
	crypto_reader->set_flag(order);
	if(suspended)
	    waiter->wait();
	purge_ratelier_up_to(order);
	suspended = true;
    }

    void parallel_tronconneuse::go_read()
    {
	if(suspended)
	{
	    crypto_reader->set_pos(current_position);
	    crypto_reader->set_flag(tronco_flags::normal);
	    waiter->wait(); // this should release the workers and the crypto_reader thread
	    suspended = false;
	    check_bytes_to_skip = true;
	}
    }

    void parallel_tronconneuse::read_refill()
    {
	if(lus_data.empty())
	{
	    if(!lus_flags.empty())
		throw SRC_BUG;
	    gather->gather(lus_data, lus_flags);

	    if(lus_flags.empty() || lus_data.empty())
		throw SRC_BUG; // show receive some blocks with either flag eof, error or data

	    if(check_bytes_to_skip)
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
		    switch(static_cast<tronco_flags>(lus_flags.front()))
		    {
		    case tronco_flags::normal:
			break;
		    case tronco_flags::stop:
			throw SRC_BUG;
		    case tronco_flags::eof:
			throw SRC_BUG;
		    case tronco_flags::die:
			throw SRC_BUG;
		    case tronco_flags::data_error:
			break;
		    default:
			throw SRC_BUG;
		    }
		}
	    }
	}
    }

    void parallel_tronconneuse::purge_ratelier_up_to(tronco_flags order)
    {
	U_I num = travailleur.size(); // the number of worker
	deque<signed int>::iterator it;

	switch(order)
	{
	case tronco_flags::normal:
	    throw SRC_BUG;
	case tronco_flags::stop:
	case tronco_flags::eof:
	    break; // OK
	case tronco_flags::die:
	    throw SRC_BUG;
	case tronco_flags::data_error:
	    throw SRC_BUG;
	default:
	    break; // expected cases
	}

	do
	{
	    read_refill();

	    while(!lus_flags.empty() && num > 0)
	    {
		if(static_cast<tronco_flags>(lus_flags.front()) == order)
		    --num;
		lus_flags.pop_front();
		tas->put(move(lus_data.front()));
		lus_data.pop_front();
	    }
	}
	while(num > 0);
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
							      bool & reof)

    {
	infinint clear_offset = 0;
	memory_file tmp;

	if(callback == nullptr)
	    throw SRC_BUG;

	if(!first)
	    throw SRC_BUG;

	tmp.write(first->crypted_data.get_addr(), first->crypted_data.get_data_size());
	if(opt_next)
	    tmp.write(opt_next->crypted_data.get_addr(), opt_next->crypted_data.get_data_size());
	clear_offset = (*callback)(tmp, reading_ver);


	if(clear_offset >= initial_shift)
	    clear_offset -= initial_shift;
	    // now clear_offset can be compared to read_offset
	else
	    return;

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
		throw SRC_BUG; // more encrypted data than could be read so far!
	}
    }

} // end of namespace
