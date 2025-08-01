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

#include "parallel_block_compressor.hpp"
#include "erreurs.hpp"
#include "compress_block_header.hpp"

using namespace std;
using namespace libthreadar;

namespace libdar
{

	/////////////////////////////////////////////////////
        //
        // parallel_block_compressor class implementation
        //
        //


    parallel_block_compressor::parallel_block_compressor(U_I num_workers,
                                                         unique_ptr<compress_module> block_zipper,
                                                         generic_file & compressed_side,
                                                         U_I uncompressed_bs):
        proto_compressor((compressed_side.get_mode() == gf_read_only)? gf_read_only: gf_write_only),
        num_w(num_workers),
        zipper(std::move(block_zipper)),
        compressed(&compressed_side),
        uncompressed_block_size(uncompressed_bs)
    {
        U_I compr_bs = zipper->get_min_size_to_compress(uncompressed_block_size);

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
        running_threads = false;
        reof = false;


            // creating inter thread communication structures

	try
	{
	    disperse = make_shared<ratelier_scatter<crypto_segment> >(get_ratelier_size(num_w));
	    rassemble = make_shared<ratelier_gather<crypto_segment> >(get_ratelier_size(num_w));
	    tas = make_shared<heap<crypto_segment> >(); // created empty
	}
	catch(std::bad_alloc & e)
	{
	    throw Ememory("parallel_block_compressor::parallel_block_compressor");
	}

            // now filling the head that was created empty


        for(U_I i = 0 ; i < get_heap_size(num_w) ; ++i)
            tas->put(make_unique<crypto_segment>(compr_bs, uncompressed_block_size));

            // creating the zip_below_* thread object

        if(get_mode() == gf_write_only)
        {
            writer = make_unique<zip_below_write>(rassemble,
                                                  compressed,
                                                  tas,
                                                  num_w);
        }
        else
        {
            reader = make_unique<zip_below_read>(compressed,
                                                 disperse,
                                                 tas,
                                                 num_w);
        }

            // creating the worker threads objects

        for(U_I i = 0 ; i < num_w ; ++i)
            travailleurs.push_back(make_unique<zip_worker>(disperse,
							   rassemble,
							   zipper->clone(),
							   get_mode() == gf_write_only));

            // no other thread than the one executing this code is running at this point!!!
    }

    parallel_block_compressor::~parallel_block_compressor()
    {
        try
        {
            terminate();
        }
        catch(...)
        {
                // ignore all exceptions
        }
    }

    void parallel_block_compressor::suspend_compression()
    {
        if(get_mode() != gf_read_only)
            inherited_sync_write();

	if(get_mode() != gf_write_only)
	    inherited_flush_read();

        suspended = true;
    }

    void parallel_block_compressor::resume_compression()
    {
        suspended = false;
    }

    bool parallel_block_compressor::skippable(skippability direction, const infinint & amount)
    {
        if(is_terminated())
            throw SRC_BUG;

        stop_threads();

        return compressed->skippable(direction, amount);
    }

    bool parallel_block_compressor::skip(const infinint & pos)
    {
        if(is_terminated())
            throw SRC_BUG;

        stop_threads();
        reof = false;
        return compressed->skip(pos);
    }

    bool parallel_block_compressor::skip_to_eof()
    {
        if(is_terminated())
            throw SRC_BUG;

        stop_threads();
        reof = false;
        return compressed->skip_to_eof();
    }

    bool parallel_block_compressor::skip_relative(S_I x)
    {
        if(is_terminated())
            throw SRC_BUG;

        stop_threads();
        reof = false;
        return skip_relative(x);
    }


    bool parallel_block_compressor::truncatable(const infinint & pos) const
    {
        parallel_block_compressor *me = const_cast<parallel_block_compressor *>(this);
        if(me == nullptr)
            throw SRC_BUG;

        if(is_terminated())
            throw SRC_BUG;

        me->stop_threads();
        return compressed->truncatable(pos);
    }


    infinint parallel_block_compressor::get_position() const
    {
        parallel_block_compressor *me = const_cast<parallel_block_compressor *>(this);
        if(me == nullptr)
            throw SRC_BUG;

        if(is_terminated())
            throw SRC_BUG;

        me->stop_threads();
        return compressed->get_position();
    }

    void parallel_block_compressor::inherited_read_ahead(const infinint & amount)
    {
	if(!suspended)
	{
	    if(!running_threads)
	    {
		compressed->read_ahead(amount);
		run_read_threads();
	    }
	}
	else // compression is disabled
	{
	    if(running_threads)
		throw SRC_BUG;
		// no thread should be running as
		// compression is disabled for now
	    compressed->read_ahead(amount);
	}
    }

    U_I parallel_block_compressor::inherited_read(char *a, U_I size)
    {
        if(is_terminated())
            throw SRC_BUG;

        if(suspended)
        {
            stop_threads();

            if(!reof)
                return compressed->read(a, size);
            else
                return 0;
        }
        else
        {
            U_I ret = 0;

            if(! reof)
                run_threads();

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
                            tas->put(std::move(lus_data.front()));
                            lus_data.pop_front();
                            lus_flags.pop_front();
                        }
                        break;
                    case compressor_block_flags::eof_die:
                        reof = true;
                        stop_threads(); // this may throw a pending exception
                        break;
                    case compressor_block_flags::error:
                            // error received from the zip_below_read thread
                            // the thread has terminated and all worker have been
                            // asked to terminate by the zip_below_read thread
                        stop_threads(); // this should relaunch the exception from the worker
                        throw SRC_BUG; // if not this is a bug
                    case compressor_block_flags::worker_error:
                            // a single worker have reported an error and is till alive
                            // we drop this single order
                            // to cleanly stop threads
                        tas->put(std::move(lus_data.front()));
                        lus_data.pop_front();
                        lus_flags.pop_front();
                            // no we can stop the threads poperly
                        stop_threads(); // we stop all threads which will relaunch the worker exception
                        throw SRC_BUG;  // if not this is a bug
                    default:
                        throw SRC_BUG;
                    }

                }

            }

            return ret;
        }
    }

    void parallel_block_compressor::inherited_write(const char *a, U_I size)
    {
        U_I wrote = 0;

        if(is_terminated())
            throw SRC_BUG;

        if(suspended)
        {
            stop_threads();
            compressed->write(a, size);
        }
        else
        {
            run_threads();

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
                stop_threads();
                    // this should throw an exception
                    // else we do it now:
                throw SRC_BUG;
            }
        }
    }

    void parallel_block_compressor::inherited_truncate(const infinint & pos)
    {
        if(is_terminated())
            throw SRC_BUG;

        stop_threads();
        compressed->truncate(pos);
    }

    void parallel_block_compressor::inherited_sync_write()
    {
        if(is_terminated())
            throw SRC_BUG;

        if(curwrite && curwrite->clear_data.get_data_size() > 0)
        {
            run_threads();
            disperse->scatter(curwrite, static_cast<signed int>(compressor_block_flags::data));
        }

            // this adds the eof mark (zero block size)
        stop_threads();
    }

    void parallel_block_compressor::inherited_terminate()
    {
        switch(get_mode())
        {
        case gf_read_only:
            break;
        case gf_write_only:
            inherited_sync_write();
            break;
        case gf_read_write:
            throw SRC_BUG;
        default:
            throw SRC_BUG;
        }

        stop_threads();
    }

    void parallel_block_compressor::send_flag_to_workers(compressor_block_flags flag)
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

    void parallel_block_compressor::stop_threads()
    {
        switch(get_mode())
        {
        case gf_read_only:
            stop_read_threads();
            break;
        case gf_write_only:
            stop_write_threads();
            break;
        case gf_read_write:
            throw SRC_BUG;
        default:
            throw SRC_BUG;
        }
    }

    void parallel_block_compressor::stop_read_threads()
    {
        if(running_threads)
        {
            if(!reader)
                throw SRC_BUG;

            reader->do_stop();
            switch(purge_ratelier_up_to_non_data())
            {
            case compressor_block_flags::data:
                throw SRC_BUG;
            case compressor_block_flags::eof_die:
                break;
            case compressor_block_flags::error:
                break;
            case compressor_block_flags::worker_error:
                (void)purge_ratelier_up_to_non_data();
                    // need to purge further as this is
                    // not a global order that equals the
                    // number of workers
                break;
            default:
                throw SRC_BUG;
            }

            running_threads = false;
                // we must set this before join()
                // to avoid purging a second time
                // in case join() would throw an
                // exception

            reader->join();
            for(deque<unique_ptr<zip_worker> >::iterator it = travailleurs.begin(); it != travailleurs.end(); ++it)
	    {
		if((*it) != nullptr)
		    (*it)->join();
		else
		    throw SRC_BUG;
	    }
        }
    }

    void parallel_block_compressor::stop_write_threads()
    {
        if(curwrite && curwrite->clear_data.get_data_size() > 0)
            inherited_sync_write();

        if(running_threads)
        {
            if(!writer)
                throw SRC_BUG;

            running_threads = false;
                // change the flag before calling join()
                // as they may trigger an exception

            if(writer->is_running())
            {
                send_flag_to_workers(compressor_block_flags::eof_die);

                writer->join();
                for(deque<unique_ptr<zip_worker> >::iterator it = travailleurs.begin(); it !=travailleurs.end(); ++it)
		{
		    if((*it) != nullptr)
			(*it)->join();
		    else
			throw SRC_BUG;
		}
            }
        }
    }

    void parallel_block_compressor::run_threads()
    {
        switch(get_mode())
        {
        case gf_read_only:
            run_read_threads();
            break;
        case gf_write_only:
            run_write_threads();
            break;
        case gf_read_write:
            throw SRC_BUG;
        default:
            throw SRC_BUG;
        }
    }

    void parallel_block_compressor::run_read_threads()
    {
        if(!running_threads)
        {
            if(!reader)
                throw SRC_BUG;
            if(reader->is_running())
                throw SRC_BUG;
            reader->reset();
            reader->run();
            for(deque<unique_ptr<zip_worker> >::iterator it = travailleurs.begin(); it !=travailleurs.end(); ++it)
	    {
		if((*it) != nullptr)
		    (*it)->run();
		else
		    throw SRC_BUG;
	    }
            running_threads = true;
        }
    }

    void parallel_block_compressor::run_write_threads()
    {
        if(!running_threads)
        {
            if(!writer)
                throw SRC_BUG;
            if(writer->is_running())
                throw SRC_BUG;
            writer->reset();
            writer->run();
            for(deque<unique_ptr<zip_worker> >::iterator it = travailleurs.begin(); it !=travailleurs.end(); ++it)
	    {
		if((*it) != nullptr)
		    (*it)->run();
		else
		    throw SRC_BUG;
	    }
            running_threads = true;
        }
    }

    compressor_block_flags parallel_block_compressor::purge_ratelier_up_to_non_data()
    {
        S_I expected = num_w;
        compressor_block_flags ret = compressor_block_flags::data;

        if(get_mode() != gf_read_only)
            throw SRC_BUG;

        while(expected > 0)
        {
            if(lus_data.empty())
            {
                if(!lus_flags.empty())
                    throw SRC_BUG;
                rassemble->gather(lus_data, lus_flags);
            }

            while(!lus_flags.empty() && expected > 0)
            {
                if(lus_data.empty())
                    throw SRC_BUG;
                if(ret == compressor_block_flags::data
                   && lus_flags.front() != static_cast<signed int>(compressor_block_flags::data))
                    ret = static_cast<compressor_block_flags>(lus_flags.front());
                if(lus_flags.front() == static_cast<signed int>(ret) && ret != compressor_block_flags::data)
                {
                    --expected;
                    if(ret == compressor_block_flags::worker_error)
                        expected = 0;
                }
                tas->put(std::move(lus_data.front()));
                lus_data.pop_front();
                lus_flags.pop_front();
            }
        }

        return ret;
    }

    U_I parallel_block_compressor::get_heap_size(U_I num_workers)
    {
        U_I ratelier_size = get_ratelier_size(num_workers);
        U_I heap_size = ratelier_size * 2 + num_workers + 1 + ratelier_size + 2;
            // each ratelier can be full of crypto_segment and at the same
            // time, each worker could hold a crypto_segment, the below thread
            // as well and the main thread for parallel_block_compressor could hold
            // a deque of the size of the ratelier plus 2 more crypto_segments
        return heap_size;
    }


        /////////////////////////////////////////////////////
	//
	// zip_below_write class implementation
	//
	//


    zip_below_write::zip_below_write(const shared_ptr<ratelier_gather<crypto_segment> > & source,
				     generic_file *dest,
				     const shared_ptr<heap<crypto_segment> > & xtas,
				     U_I num_workers):
	src(source),
	dst(dest),
	tas(xtas),
	num_w(num_workers)
    {
#ifdef LIBTHREADAR_STACK_FEATURE_AVAILABLE
	set_stack_size(LIBDAR_DEFAULT_STACK_SIZE);
#endif

	if(!src)
	    throw SRC_BUG;
	if(dst == nullptr)
	    throw SRC_BUG;
	if(!tas)
	    throw SRC_BUG;
	if(num_w < 1)
	    throw SRC_BUG;

	    // dropping the initial uncompressed_block_size info

	reset();
    }

    void zip_below_write::reset()
    {
	error = false;
	ending = num_w;
	tas->put(data);
	data.clear();
	flags.clear();
    }

    void zip_below_write::inherited_run()
    {
	try
	{
	    work();
	}
	catch(cancel_except &)
	{
	    throw;
	}
	catch(...)
	{
	    error = true;
		// this should trigger the parallel_block_compressor
		// to push eof_die flag leading work() and zip_workers threads
		// to complete as properly as possible (minimizing dead-lock
		// situation).

	    try
	    {
		work();
	    }
	    catch(cancel_except &)
	    {
		throw;
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
	do
	{
	    cancellation_checkpoint();
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
		    compress_block_header bh;

		    switch(static_cast<compressor_block_flags>(flags.front()))
		    {
		    case compressor_block_flags::data:
			if(!error)
			{
			    bh.type = compress_block_header::H_DATA;
			    bh.size = data.front()->crypted_data.get_data_size();
			    bh.dump(*dst);
			    dst->write(data.front()->crypted_data.get_addr(),
				       data.front()->crypted_data.get_data_size());
			}
			// else do nothing (avoid generating a new exception)
			pop_front();
			break;
		    case compressor_block_flags::eof_die:
			--ending;
			pop_front();
			if(ending == 0)
			{
			    bh.type = compress_block_header::H_EOF;
			    bh.size = 0;
			    bh.dump(*dst);
			}
			break;
		    case compressor_block_flags::worker_error: // error received from a zip_worker
			error = true; // this will trigger the main thread to terminate all threads
			pop_front();
			break;
		    case compressor_block_flags::error:
			pop_front();
			if(!error)
			    throw SRC_BUG;
			break;
		    default:
			pop_front();
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
#ifdef LIBTHREADAR_STACK_FEATURE_AVAILABLE
	set_stack_size(LIBDAR_DEFAULT_STACK_SIZE);
#endif

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
	    tas->put(std::move(ptr));
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
		tas->put(std::move(ptr));
	    push_flag_to_all_workers(compressor_block_flags::error);

	    throw; // relaunching the exception now as we end the thread
	}
    }


    void zip_below_read::work()
    {
	bool end = false;
	U_I aux;
	compress_block_header bh;

	do
	{
	    cancellation_checkpoint();

		// reading compressed block's header

	    if(!should_i_stop)
	    {
		bh.set_from(*src);
		aux = 0;
		bh.size.unstack(aux);
		if(!bh.size.is_zero())
		{
			// the read infinint does not hold in an a U_I assuming
			// data corruption occured

		    throw Erange("zip_below_read::work", gettext("incoherent compressed block structure, compressed data corruption"));
		}
	    }
	    else // we are asked to stop by the parallel_block_compressor thread
	    {
		aux = 0; // simulating an end of file
		bh.type = compress_block_header::H_EOF;
	    }

	    switch(bh.type)
	    {
	    case compress_block_header::H_EOF:
		push_flag_to_all_workers(compressor_block_flags::eof_die);
		if(aux != 0)
		    throw Erange("zip_below_read::work", gettext("incoherent compressed block structure, compressed data corruption"));
		end = true;
		break;
	    case compress_block_header::H_DATA:
		if(!ptr)
		{
		    ptr = tas->get();
		    ptr->reset();
		}

		if(aux > ptr->crypted_data.get_max_size())
		{
		    tas->put(std::move(ptr));
		    push_flag_to_all_workers(compressor_block_flags::error);
		    end = true;
		}
		else
		{
		    ptr->crypted_data.set_data_size(src->read(ptr->crypted_data.get_addr(), aux));

		    if(ptr->crypted_data.get_data_size() < aux)
		    {
			    // we should have the whole data filled, not less than aux!

			tas->put(std::move(ptr));
			push_flag_to_all_workers(compressor_block_flags::error);
			end = true;
		    }
		    else
		    {
			ptr->crypted_data.rewind_read();
			dst->scatter(ptr, static_cast<signed int>(compressor_block_flags::data));
		    }
		}
		break;
	    default:
		throw Erange("zip_below_read::work", gettext("incoherent compressed block structure, compressed data corruption"));
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
	compr(std::move(ptr)),
	do_compress(compress)
    {
#ifdef LIBTHREADAR_STACK_FEATURE_AVAILABLE
	set_stack_size(LIBDAR_DEFAULT_STACK_SIZE);
#endif

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
	catch(cancel_except &)
	{
	    throw;
	}
	catch(...)
	{
	    error = true;
		// this should trigger the parallel_block_compressor
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
	    catch(cancel_except &)
	    {
		throw;
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
	    cancellation_checkpoint();

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
		if(!do_compress)
		{
		    writer->worker_push_one(transit_slot, transit, flag);
		    ending = true;
			// in read mode (uncompressing) in case the zip_below_write
			// thread sends an error message, it ends, so we propagate
			// and terminate too, but we do not throw exception ourselves
		}
		else
		{
		    if(!error)
			throw SRC_BUG;
			// in write mode (compressing) we should never receive a error
			// flag from the main thread
		}
		break;
	    case compressor_block_flags::worker_error:
		if(!error)
		    throw SRC_BUG;
		    // only a worker should set this error and we
		    // do not communicate with other workers
		else
		    writer->worker_push_one(transit_slot, transit, flag);
		    // we now stay as much transparent as possible
		break;
	    default:
		if(!error)
		    throw SRC_BUG;
		else
		    writer->worker_push_one(transit_slot, transit, flag);
		    // we now stay as much transparent as possible
		break;
	    }
	}
	while(!ending);
    }



} // end of namespace

