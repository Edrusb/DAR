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

#include "parallel_compressor.hpp"
#include "erreurs.hpp"

using namespace std;
using namespace libthreadar;

namespace libdar
{


    	/////////////////////////////////////////////////////
	//
	// zip_below_write class implementation
	//
	//


    zip_below_write::zip_below_write(const shared_ptr<ratelier_gather<crypto_segment> > & source,
				     generic_file *dest,
				     const shared_ptr<heap<crypto_segment> > & xtas,
				     U_I num_workers,
				     const infinint & uncompressed_block_size):
	src(source),
	dst(dest),
	tas(xtas),
	num_w(num_workers)
    {
	if(!src)
	    throw SRC_BUG;
	if(dst == nullptr)
	    throw SRC_BUG;
	if(!tas)
	    throw SRC_BUG;
	if(num_w < 1)
	    throw SRC_BUG;

	    // dropping the initial uncompressed_block_size info

	reset(uncompressed_block_size);
    }

    void zip_below_write::reset(const infinint & uncompressed_block_size)
    {
	error = false;
	ending = num_w;
	data.clear();
	flags.clear();
	uncompressed_block_size.dump(*dst);
    }

    void zip_below_write::inherited_run()
    {
	try
	{
	    work();
	}
	catch(...)
	{
	    error = true;
		// this should trigger the parallel_compressor
		// to push eof_die flag leading work() and zip_workers threads
		// to complete as properly as possible (minimizing dead-lock
		// situation).

	    try
	    {
		work();
	    }
	    catch(...)
	    {
		    // do nothing
	    }

	    throw; // relaunching the exception now as we end the thread
	}
    }

    void zip_below_write::work()
    {
	infinint aux;

	do
	{
	    if(data.empty())
	    {
		if(!flags.empty())
		{
		    if(!error)
			throw SRC_BUG;
			// does not hurt in error context
			// we must survive at any cost
		}
		src->gather(data, flags);
	    }

	    while(!data.empty() && ending > 0)
	    {
		if(flags.empty())
		{
		    if(!error)
			throw SRC_BUG;
		}
		else
		{
		    switch(static_cast<compressor_block_flags>(flags.front()))
		    {
		    case compressor_block_flags::data:
			if(!error)
			{
			    aux = data.front()->crypted_data.get_data_size();
			    aux.dump(*dst);
			    dst->write(data.front()->crypted_data.get_addr(),
				       data.front()->crypted_data.get_data_size());
			    pop_front();
			}
			else // do nothing (avoid generating a new exception)
			    pop_front();
			break;
		    case compressor_block_flags::eof_die:
			--ending;
			pop_front();
			break;
		    case compressor_block_flags::worker_error: // error received from a zip_worker
			error = true; // this will trigger the main thread to terminate all threads
			break;
		    case compressor_block_flags::error:
			if(!error)
			    throw SRC_BUG;
			break;
		    default:
			if(!error)
			    throw SRC_BUG;
		    }
		}
	    }
	}
	while(ending > 0);
    }





    	/////////////////////////////////////////////////////
	//
	// zip_below_read class implementation
	//
	//

    zip_below_read::zip_below_read(generic_file *source,
				   const shared_ptr<ratelier_scatter<crypto_segment> > & dest,
				   const shared_ptr<heap<crypto_segment> > & xtas,
				   U_I num_workers):
	src(source),
	dst(dest),
	tas(xtas),
	num_w(num_workers)
    {
	if(src == nullptr)
	    throw SRC_BUG;
	if(!dst)
	    throw SRC_BUG;
	if(!tas)
	    throw SRC_BUG;
	if(num_w < 1)
	    throw SRC_BUG;

	reset();
    }

    void zip_below_read::reset()
    {
	should_i_stop = false;
	if(ptr)
	    tas->put(move(ptr));
	uncomp_block_size.read(*src);
    }

    void zip_below_read::inherited_run()
    {
	try
	{
	    work();
	}
	catch(...)
	{
	    if(ptr)
		tas->put(move(ptr));
	    push_flag_to_all_workers(compressor_block_flags::error);

	    throw; // relaunching the exception now as we end the thread
	}
    }


    void zip_below_read::work()
    {
	bool end = false;
	infinint aux_i;
	U_I aux;

	do
	{
		// reading the compressed block size

	    if(!should_i_stop)
	    {
		aux_i.read(*src);
		aux = 0;
		aux_i.unstack(aux);
		if(!aux_i.is_zero())
		{
			// the read infinint does not hold in an a U_I assuming
			// data corruption occured

		    push_flag_to_all_workers(compressor_block_flags::error);
		    end = true;
		}
	    }
	    else // we are asked to stop by the parallel_compressor thread
		aux = 0; // simulating an end of file

	    if(aux == 0)
	    {
		    // eof

		push_flag_to_all_workers(compressor_block_flags::eof_die);
		end = true;
	    }
	    else
	    {
		if(!ptr)
		{
		    ptr = tas->get();
		    ptr->reset();
		}

		if(aux > ptr->crypted_data.get_max_size())
		{
		    tas->put(move(ptr));
		    push_flag_to_all_workers(compressor_block_flags::error);
		    end = true;
		}
		else
		{
		    ptr->crypted_data.set_data_size(src->read(ptr->crypted_data.get_addr(), aux));

		    if(ptr->crypted_data.get_data_size() < aux)
		    {
			    // we should have the whole data filled, not less than aux!

			tas->put(move(ptr));
			push_flag_to_all_workers(compressor_block_flags::error);
			end = true;
		    }
		    else
		    {
			ptr->crypted_data.rewind_read();
			dst->scatter(ptr, static_cast<signed int>(compressor_block_flags::data));
		    }
		}
	    }
	}
	while(!end);
    }

    void zip_below_read::push_flag_to_all_workers(compressor_block_flags flag)
    {
	for(U_I i = 0; i < num_w; ++i)
	{
	    if(!ptr)
		ptr = tas->get();
	    dst->scatter(ptr, static_cast<signed int>(flag));
	}
    }


	/////////////////////////////////////////////////////
	//
	// zip_worker class implementation
	//
	//


    zip_worker::zip_worker(shared_ptr<ratelier_scatter <crypto_segment> > & read_side,
			   shared_ptr<ratelier_gather <crypto_segment > > & write_side,
			   unique_ptr<compress_module> && ptr,
			   bool compress):
	reader(read_side),
	writer(write_side),
	compr(move(ptr)),
	do_compress(compress)
    {
	if(!reader)
	    throw SRC_BUG;
	if(!writer)
	    throw SRC_BUG;
	if(!compr)
	    throw SRC_BUG;

	error = false;
    }



    void zip_worker::inherited_run()
    {
	try
	{
	    work();
	}
	catch(...)
	{
	    error = true;
		// this should trigger the parallel_compressor
		// to push eof_die flag leading work() and zip_workers threads
		// to complete as properly as possible (minimizing dead-lock
		// situation).

	    try
	    {
		signed int flag;

		if(!transit)
		    transit = reader->worker_get_one(transit_slot, flag);
		writer->worker_push_one(transit_slot,
					transit,
					static_cast<signed int>(compressor_block_flags::worker_error));
		work();
	    }
	    catch(...)
	    {
		    // do nothing
	    }

	    throw; // relaunching the exception now as we end the thread
	}
    }

    void zip_worker::work()
    {
	bool ending = false;
	signed int flag;

	do
	{
	    if(!transit)
		transit = reader->worker_get_one(transit_slot, flag);

	    switch(static_cast<compressor_block_flags>(flag))
	    {
	    case compressor_block_flags::data:
		if(!error)
		{
		    if(do_compress)
		    {
			transit->crypted_data.set_data_size(compr->compress_data(transit->clear_data.get_addr(),
										 transit->clear_data.get_data_size(),
										 transit->crypted_data.get_addr(),
										 transit->crypted_data.get_max_size()));
			transit->crypted_data.rewind_read();
		    }
		    else
		    {
			transit->clear_data.set_data_size(compr->uncompress_data(transit->crypted_data.get_addr(),
										 transit->crypted_data.get_data_size(),
										 transit->clear_data.get_addr(),
										 transit->clear_data.get_max_size()));
			transit->clear_data.rewind_read();
		    }

		}
		writer->worker_push_one(transit_slot, transit, flag);
		break;
	    case compressor_block_flags::eof_die:
		ending = true;
		writer->worker_push_one(transit_slot, transit, flag);
		break;
	    case compressor_block_flags::error:
		if(!error)
		    writer->worker_push_one(transit_slot, transit, flag);
		if(!do_compress)
		    ending = true;
		    // in read mode (uncompressing) in case the zip_below_write
		    // thread sends an error message, it ends, so we propagate
		    // and terminate too, but we do not throw exception ourselves
		else
		{
		    if(!error)
			throw SRC_BUG;
			// in write mode (compressing) we should never receive a error
			// flag from the main thread, but could generate one and upon
			// exception raised in our code (managed by inherited_run()
			// that called us
			// this will set the error field in the  zip_below_read thread
			// that will be notified by the parallel_compressor which will
			// trigger eof_die message, then we inherited_run() will relaunch
			// the exception to be caught while parallel_compressor will join()
			// on us.
		}
		break;
	    case compressor_block_flags::worker_error:
		if(!error)
		    throw SRC_BUG;
		    // only a worker should set this error and we
		    // do not communicate with other workers
		break;
	    default:
		if(!error)
		    throw SRC_BUG;
		break;
	    }
	}
	while(!ending);
    }


    	/////////////////////////////////////////////////////
	//
	// parallel_compressor class implementation
	//
	//


    parallel_compressor::parallel_compressor(U_I num_workers,
					     unique_ptr<compress_module> & block_zipper,
					     generic_file & compressed_side,
					     U_I uncompressed_bs):
	proto_compressor(compressed_side.get_mode()),
	zipper(move(block_zipper)),
	compressed(&compressed_side),
	we_own_compressed_side(false),
	uncompressed_block_size(uncompressed_bs)
    {
	init_fields();
    }


    parallel_compressor::parallel_compressor(U_I num_workers,
					     unique_ptr<compress_module> & block_zipper,
					     generic_file *compressed_side,
					     U_I uncompressed_bs):
	proto_compressor(compressed_side->get_mode()),
	zipper(move(block_zipper)),
	compressed(compressed_side),
	we_own_compressed_side(true),
	uncompressed_block_size(uncompressed_bs)
    {
	init_fields();
    }

    parallel_compressor::~parallel_compressor()
    {
	try
	{
	    terminate();
	}
	catch(...)
	{
		// ignore all exceptions
	}

	if(we_own_compressed_side && compressed != nullptr)
	    delete compressed;
    }

    void parallel_compressor::suspend_compression()
    {
	if(get_mode() == gf_read_only)
	    stop_read_threads();
	else
	    stop_write_threads();
    }

    void parallel_compressor::resume_compression()
    {
	if(get_mode() == gf_read_only)
	    run_read_threads();
	else
	    run_write_threads();
    }

    bool parallel_compressor::skippable(skippability direction, const infinint & amount)
    {
	bool ret;

	if(is_terminated())
	    throw SRC_BUG;

	if(get_mode() == gf_read_only)
	{
	    if(reader && reader->is_running())
		stop_read_threads();
		// not restarting threads

	    ret = compressed->skippable(direction, amount);
	}
	else
	{
	    if(writer && writer->is_running())
		stop_write_threads();
		    // not restarting threads

	    ret = compressed->skippable(direction, amount);
	}

	return ret;
    }

    bool parallel_compressor::skip(const infinint & pos)
    {
	bool ret;

	if(is_terminated())
	    throw SRC_BUG;

	if(get_mode() == gf_read_only)
	{
	    if(reader && reader->is_running())
	    {
		stop_read_threads();
		ret = compressed->skip(pos);
		    // not restarting threads
	    }
	    else
		ret = compressed->skip(pos);

	}
	else
	{
	    if(writer && writer->is_running())
	    {
		stop_write_threads();
		ret = compressed->skip(pos);
		    // not restarting threads
	    }
	    else
		ret = compressed->skip(pos);
	}

	return ret;
    }

    bool parallel_compressor::skip_to_eof()
    {
	bool ret;

	if(is_terminated())
	    throw SRC_BUG;

	if(get_mode() == gf_read_only)
	{
	    if(reader && reader->is_running())
	    {
		stop_read_threads();
		ret = compressed->skip_to_eof();
		    // not restarting threads
	    }
	    else
		ret = compressed->skip_to_eof();

	}
	else
	{
	    if(writer && writer->is_running())
	    {
		stop_write_threads();
		ret = compressed->skip_to_eof();
		    // not restarting threads
	    }
	    else
		ret = compressed->skip_to_eof();
	}

	return ret;
    }

        bool parallel_compressor::skip_relative(S_I x)
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


    bool parallel_compressor::truncatable(const infinint & pos) const
    {
	bool ret;
	parallel_compressor *me = const_cast<parallel_compressor *>(this);
	if(me == nullptr)
	    throw SRC_BUG;

	if(is_terminated())
	    throw SRC_BUG;

	if(get_mode() == gf_read_only)
	{
	    if(reader && reader->is_running())
	    {
		me->stop_read_threads();
		ret = compressed->truncatable(pos);
		me->run_read_threads();
	    }
	    else
		ret = compressed->truncatable(pos);

	}
	else
	{
	    if(writer && writer->is_running())
	    {
		me->stop_write_threads();
		ret = compressed->truncatable(pos);
		me->run_write_threads();
	    }
	    else
		ret = compressed->truncatable(pos);
	}

	return ret;
    }


    infinint parallel_compressor::get_position() const
    {
	parallel_compressor *me = const_cast<parallel_compressor *>(this);
	if(me == nullptr)
	    throw SRC_BUG;

	if(is_terminated())
	    throw SRC_BUG;

	if(get_mode() == gf_read_only)
	    return current_position;
	else
	    if(writer && writer->is_running())
	    {
		infinint ret;

		    // this situation when one needs to
		    // get the current position (= current position of the compressed side)
		    // durung a compression process should not occur
		    // frequently of not at all in libdar, thus stopping the
		    // threads and restarting them afterward to read the position
		    // has been found a good compromise compared to more complex
		    // interaction with the zip_below_write thread and the required
		    // use of mutex to access the position information

		me->stop_write_threads();
		ret = compressed->get_position();
		me->run_write_threads();

		return ret;
	    }
	    else
		return compressed->get_position();
    }


    U_I parallel_compressor::inherited_read(char *a, U_I size)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(suspended)
	{
	    if(!reof)
		return compressed->read(a, size);
	    else
		return 0;
	}
	else
	{
	    U_I ret = 0;

	    run_read_threads();

	    while(ret < size && ! reof)
	    {
		if(lus_data.empty())
		    rassemble->gather(lus_data, lus_flags);

		while(!lus_data.empty() && ret < size && ! reof)
		{
		    if(lus_flags.empty())
			throw SRC_BUG;

		    switch(static_cast<compressor_block_flags>(lus_flags.front()))
		    {
		    case compressor_block_flags::data:
			ret += lus_data.front()->clear_data.read(a + ret, size - ret);
			if(lus_data.front()->clear_data.all_is_read())
			{
			    tas->put(move(lus_data.front()));
			    lus_data.pop_front();
			    lus_flags.pop_front();
			}
			break;
		    case compressor_block_flags::eof_die:
			reof = true;
			stop_read_threads(); // this may throw a pending exception
			break;
		    case compressor_block_flags::error: // error received from the zip_below_read thread
			    // the thread has terminated and all worker have been
			    // asked to terminate by the zip_below_read thread
			stop_read_threads(); // this should relaunch the exception from the worker
			throw SRC_BUG; // if not this is a bug
		    case compressor_block_flags::worker_error:
			    // a single worker have reported an error and is till alive
			stop_read_threads(); // we stop all threads which will relaunch the worker exception
			throw SRC_BUG; // if not this is a bug
		    default:
			throw SRC_BUG;
		    }

		}

	    }

	    return ret;
	}
    }

    void parallel_compressor::inherited_write(const char *a, U_I size)
    {
	U_I wrote = 0;

	if(is_terminated())
	    throw SRC_BUG;

	if(suspended)
	    compressed->write(a, size);
	else
	{
	    run_write_threads();

	    while(wrote < size && !writer->exception_pending())
	    {
		if(!curwrite)
		{
		    curwrite = tas->get();
		    curwrite->reset();
		}
		else
		{
		    if(curwrite->clear_data.is_full())
			throw SRC_BUG;
		}
		wrote += curwrite->clear_data.write(a + wrote, size - wrote);
		if(curwrite->clear_data.is_full())
		{
		    curwrite->clear_data.rewind_read();
		    disperse->scatter(curwrite, static_cast<signed int>(compressor_block_flags::data));
		}
	    }

	    if(writer->exception_pending())
	    {
		stop_write_threads();
		    // this should throw an exception
		    // else we do it now:
		throw SRC_BUG;
	    }
	}
    }

    void parallel_compressor::inherited_truncate(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

	stop_write_threads();
	compressed->truncate(pos);
    }

    void parallel_compressor::inherited_sync_write()
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(suspended && curwrite)
	{
	    run_write_threads();
	    disperse->scatter(curwrite, static_cast<signed int>(compressor_block_flags::data));
	}
    }

    void parallel_compressor::inherited_terminate()
    {
	switch(get_mode())
	{
	case gf_read_only:
	    stop_read_threads();
	    break;
	case gf_write_only:
	    inherited_sync_write();
	    stop_write_threads();
	    break;
	case gf_read_write:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}
    }

    void parallel_compressor::init_fields()
    {


	    // sanity checks on fields set by constructors

	if(get_mode() == gf_read_write)
	    throw SRC_BUG; // mode not suported for this type of object
	if(num_w < 1)
	    throw SRC_BUG;
	if(!zipper)
	    throw SRC_BUG;
	if(compressed == nullptr)
	    throw SRC_BUG;
	if(uncompressed_block_size < min_uncompressed_block_size)
	    throw SRC_BUG;


	    // initializing simple fields not set by constructors

	suspended = false;
	reof = false;
	current_position = compressed->get_position();


	    // creating inter thread communication structures

	disperse = make_shared<ratelier_scatter<crypto_segment> >(get_ratelier_size(num_w));
	rassemble = make_shared<ratelier_gather<crypto_segment> >(get_ratelier_size(num_w));
	tas = make_shared<heap<crypto_segment> >(); // created empty


	    // creating the zip_below_* thread object

	if(get_mode() == gf_write_only)
	{
	    writer = make_unique<zip_below_write>(rassemble,
						  compressed,
						  tas,
						  num_w,
						  uncompressed_block_size);
	}
	else
	{
	    reader = make_unique<zip_below_read>(compressed,
						 disperse,
						 tas,
						 num_w);
	    if(reader)
	    {
		infinint tmp = reader->get_uncomp_block_size();

		uncompressed_block_size = 0;
		tmp.unstack(uncompressed_block_size);
		if(!tmp.is_zero())
		    throw Edata("uncompressed block size too large for, data corruption occurred?");
	    }
	}


	    // filling the head that was created empty now we know the block size

	for(U_I i = 0 ; i < get_heap_size(num_w) ; ++i)
	    tas->put(make_unique<crypto_segment>(uncompressed_block_size,
						 uncompressed_block_size));


	    // creating the worker threads objects

	for(U_I i = 0 ; i < num_w ; ++i)
	    travailleurs.push_back(zip_worker(disperse,
					      rassemble,
					      zipper->clone(),
					      get_mode() == gf_write_only));

	    // no other thread than the one executing this code is running at this point!!!
    }

    void parallel_compressor::send_flag_to_workers(compressor_block_flags flag)
    {
	unique_ptr<crypto_segment> ptr;

	if(get_mode() != gf_write_only)
	    throw SRC_BUG;

	for(U_I i = 0; i < num_w; ++i)
	{
	    ptr = tas->get();
	    disperse->scatter(ptr, static_cast<signed int>(flag));
	}
    }

    void parallel_compressor::stop_read_threads()
    {
	if(!suspended)
	{
	    if(!reader)
		throw SRC_BUG;

	    reader->do_stop();
	    purge_ratelier_for(compressor_block_flags::eof_die);
	    reader->join();
	    for(deque<zip_worker>::iterator it = travailleurs.begin(); it !=travailleurs.end(); ++it)
		it->join();
	    suspended = true;
	}
    }

    void parallel_compressor::stop_write_threads()
    {
	if(!suspended)
	{
	    send_flag_to_workers(compressor_block_flags::eof_die);

	    if(!writer)
		throw SRC_BUG;
	    writer->join();
	    for(deque<zip_worker>::iterator it = travailleurs.begin(); it !=travailleurs.end(); ++it)
		it->join();
	    suspended = true;
	}
    }

    void parallel_compressor::run_read_threads()
    {
	if(suspended)
	{
	    if(!reader)
		throw SRC_BUG;
	    if(reader->is_running())
		throw SRC_BUG;
	    reader->reset();
	    reader->run();
	    for(deque<zip_worker>::iterator it = travailleurs.begin(); it !=travailleurs.end(); ++it)
		it->run();
	    suspended = false;
	}
    }

    void parallel_compressor::run_write_threads()
    {
	if(suspended)
	{
	    if(!writer)
		throw SRC_BUG;
	    if(writer->is_running())
		throw SRC_BUG;
	    writer->reset(uncompressed_block_size);
	    writer->run();
	    for(deque<zip_worker>::iterator it = travailleurs.begin(); it !=travailleurs.end(); ++it)
		it->run();
	    suspended = false;
	}
    }

    void parallel_compressor::purge_ratelier_for(compressor_block_flags flag)
    {
	S_I expected = num_w;
	tas->put(lus_data);
	lus_data.clear();
	lus_flags.clear();

	if(get_mode() != gf_read_only)
	    throw SRC_BUG;

	while(expected > 0)
	{
	    rassemble->gather(lus_data, lus_flags);
	    while(!lus_flags.empty())
	    {
		if(lus_data.empty())
		    throw SRC_BUG;
		if(lus_flags.front() == static_cast<signed int>(flag))
		    --expected;
		tas->put(move(lus_data.front()));
		lus_data.pop_front();
		lus_flags.pop_front();
	    }
	}
    }


    U_I parallel_compressor::get_heap_size(U_I num_workers)
    {
	U_I ratelier_size = get_ratelier_size(num_workers);
	U_I heap_size = ratelier_size * 2 + num_workers + 1 + ratelier_size + 2;
	    // each ratelier can be full of crypto_segment and at the same
	    // time, each worker could hold a crypto_segment, the below thread
	    // as well and the main thread for parallel_compressor could hold
	    // a deque of the size of the ratelier plus 2 more crypto_segments
	return heap_size;
    }

} // end of namespace

