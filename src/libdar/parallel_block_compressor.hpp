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

    /// \file parallel_block_compressor.hpp
    /// \brief provide per block and parallel compression/decompression
    /// \ingroup Private

    /// Several classes are defined here:
    /// - class parallel_block_compressor, which has similar interface and behavior as class compressor but compresses a
    ///   data of a file per block of given size which allows parallel compression of a given file's data at the
    ///   cost of memory requirement and probably a less good compression ratio (depends on the size of the blocks)
    /// - class zip_below_write which gather the compressed data works of workers toward the filesystem
    /// - class zip_below_read which provides block of compressed data to workers comming from the filesystem
    /// - class zip_worker which instanciates worker objects to compute in parallel the compression of block of data
    /// .



#ifndef PARALLEL_BLOCK_COMPRESSOR_HPP
#define PARALLEL_BLOCK_COMPRESSOR_HPP

#include "../my_config.h"

#include "infinint.hpp"
#include "crypto_segment.hpp"
#include "heap.hpp"
#include "compress_module.hpp"
#include "proto_compressor.hpp"
#include <libthreadar/libthreadar.hpp>

namespace libdar
{

	/// \addtogroup Private
	/// @{


    enum class compressor_block_flags { data = 0, eof_die = 1, error = 2, worker_error = 3 };

	/// the compressed block structure is a sequence of 3 types of data
	/// - uncompressed block size
	/// - data to compress/uncompress
	/// - eof
	/// .
	///
	/// COMPRESSION PROCESS
	///
	/// *** in the archive:
	///
	/// the block streams starts with the uncompressed block size, followed by any
	/// number of data blocks, completed by an single eof block. Each data block is
	/// structured with a infinint field followed by a array of random (compresssed) bytes which
	/// lenght is the value of the first field.
	///
	/// *** between threads:
	///
	/// when compressing, the parallel_block_compressor class's sends gives the uncompressed
	/// block size at constructor time to the zip_below_write thread which drops the
	/// the information to the archive at that time.
	/// then the inherited_write call produces a set of data block to the ratelier_scatter
	/// treated by a set of zip_workers which are in turn passed to the zip_below_write
	/// thread.
	/// last when eof is reached or during terminate() method, the N eof blocks are sent
	/// where N is the number of zip_workers, either with flag eof, or die (termination).
	/// the eof flag is just passed by the workers but the zip_below_write thread collects
	/// them and awakes the main thread once all are collected and thus all data written.
	/// the die flag drives the thread termination, worker first pass them to the
	/// zip_below_thread which collect them and terminates too.
	///
	/// blocks used are of type crypto_segment the clear_data containes the uncompressed
	/// data, the crypted_data block contains the compressed data. The block_index is not used.
	///
	/// upon receiption of data blocks, each zip_workers read the clear_data and produce the
	/// compressed data to the crypted_data mem_block and pushes that to the ratelier_gather
	/// which passed to and write down by the zip_below_write.
	///
	///
	///
	/// UNCOMPRESSON PROCESS
	///
	/// the zip_below_read expectes to find the clear_block size at construction time and
	/// provides a method to read it by the parallel_block_compressor thread which allocate the
	/// mem_blocks accordingly in a pool (same as parallel_tronconneuse). The zip_below
	/// thread once started reading the blocks and push them into the ratelier_scatter
	/// (without the initial U_32 telling the size of the compressed data that follows).
	/// Upon error (incoherent strucuture ....) the thread pushes N error block in the
	/// ratelier_scatter and terminates
	///
	/// the zip_workers fetch a block and put the corresponding uncompressed data to the
	/// clear_data mem_block then push the crypto_segment to the ratelier_gather. Upon
	/// uncompression error, the error flag is set with the crypto_segment, the zip_worker
	/// continues to work anyway until it reads a crypto_segment with the eof_die flag from
	/// the ratelier_scatter.
	/// Upon reception of the error flag from the ratelier_gather the parallel_compression
	/// thread invoke a method of the zip_below_read that triggers the thread to terminate
	/// after having pushed N error blocks to the ratelier_scatter which in turns triggers
	/// the termination of the zip_workers. The parallel_block_compressor thread can then gather
	/// the N error block from the ratelier_gather, join() the threads and report the
	/// compression error


	/// the below_feed_workers class read the compressed block structure and
	/// feed it per block to the ratelier_scatter


	/////////////////////////////////////////////////////
	//
	// zip_below_write class/sub-thread
	//
	//


    class zip_below_write: public libthreadar::thread
    {
    public:
	zip_below_write(const std::shared_ptr<libthreadar::ratelier_gather<crypto_segment> > & source,
			generic_file *dest,
			const std::shared_ptr<heap<crypto_segment> > & xtas,
			U_I num_workers,
			const infinint & uncompressed_block_size);

	    /// consulted by the main thread, set to true by the zip_below_write thread
	    /// when an exception has been caught or an error flag has been seen from a
	    /// worker
	bool exception_pending() const { return error; };

	    /// reset the thread objet ready for a new compression run, but does not launch it
	void reset(const infinint & uncompressed_block_size);

    protected:
	virtual void inherited_run() override;

    private:
	std::shared_ptr<libthreadar::ratelier_gather<crypto_segment> > src;
	generic_file *dst;
	std::shared_ptr<heap<crypto_segment> > tas;
	U_I num_w;
	bool error;
	U_I ending;
	std::deque<std::unique_ptr<crypto_segment> > data;
	std::deque<signed int> flags;
	libthreadar::mutex get_pos; ///< to manage concurrent access to current_position
	infinint current_position;

	void work();
	void pop_front() { tas->put(std::move(data.front())); data.pop_front(); flags.pop_front(); };
    };


    	/////////////////////////////////////////////////////
	//
	// zip_below_read class/sub-thread
	//
	//



    class zip_below_read: public libthreadar::thread
    {
    public:
	zip_below_read(generic_file *source,
		       const std::shared_ptr<libthreadar::ratelier_scatter<crypto_segment> > & dest,
		       const std::shared_ptr<heap<crypto_segment> > & xtas,
		       U_I num_workers);

	const infinint & get_uncomp_block_size() const { return uncomp_block_size; };

	    /// this will trigger the sending of N eof_die blocks and thread termination
	void do_stop() { should_i_stop = true; };

	    /// will read the uncompr block from the source generic_file but will not launch the thread
	void reset();

    protected:
	virtual void inherited_run() override;

    private:
	generic_file *src;
	const std::shared_ptr<libthreadar::ratelier_scatter<crypto_segment> > & dst;
	const std::shared_ptr<heap<crypto_segment> > & tas;
	U_I num_w;
	infinint uncomp_block_size;
	std::unique_ptr<crypto_segment> ptr;
	bool should_i_stop;


	void work();
	void push_flag_to_all_workers(compressor_block_flags flag);
    };



     	/////////////////////////////////////////////////////
	//
	// zip_worker class/sub-thread
	//
	//



    class zip_worker: public libthreadar::thread
    {
    public:
	zip_worker(std::shared_ptr<libthreadar::ratelier_scatter <crypto_segment> > & read_side,
		   std::shared_ptr<libthreadar::ratelier_gather <crypto_segment> > & write_size,
		   std::unique_ptr<compress_module> && ptr,
		   bool compress);

    protected:
	virtual void inherited_run() override;

    private:
	std::shared_ptr<libthreadar::ratelier_scatter <crypto_segment> > & reader;
	std::shared_ptr<libthreadar::ratelier_gather <crypto_segment> > & writer;
	std::unique_ptr<compress_module> && compr;
	bool do_compress;
	bool error;
	std::unique_ptr<crypto_segment> transit;
	unsigned int transit_slot;

	void work();
    };



    	/////////////////////////////////////////////////////
	//
	// paralle_compressor class, which holds the sub-threads
	//
	//



    class parallel_block_compressor: public proto_compressor
    {
    public:
	static constexpr const U_I default_uncompressed_block_size = 102400;

	    /// \note uncompressed_bs is not used in read mode,
	    /// the block size is fetched from the compressed data

	parallel_block_compressor(U_I num_workers,
				  std::unique_ptr<compress_module> & block_zipper,
				  generic_file & compressed_side,
				  U_I uncompressed_bs = default_uncompressed_block_size);
	    // compressed_side is not owned by the object and will remains
            // after the objet destruction

	parallel_block_compressor(U_I num_workers,
				  std::unique_ptr<compress_module> & block_zipper,
				  generic_file *compressed_side,
				  U_I uncompressed_bs = default_uncompressed_block_size);
            // compressed_side is owned by the object and will be
            // deleted a destructor time

	parallel_block_compressor(const parallel_block_compressor & ref) = delete;
	parallel_block_compressor(parallel_block_compressor && ref) noexcept = delete;
	parallel_block_compressor & operator = (const parallel_block_compressor & ref) = delete;
	parallel_block_compressor & operator = (parallel_block_compressor && ref) noexcept = delete;
	~parallel_block_compressor();

	    // inherited from proto_compressor

	virtual compression get_algo() const override { return zipper->get_algo(); };
	virtual void suspend_compression() override;
	virtual void resume_compression() override;
	virtual bool is_compression_suspended() const override { return suspended; };

	    // inherited from generic file

	virtual bool skippable(skippability direction, const infinint & amount) override;
        virtual bool skip(const infinint & pos) override;
        virtual bool skip_to_eof() override;
        virtual bool skip_relative(S_I x) override;
	virtual bool truncatable(const infinint & pos) const override;
        virtual infinint get_position() const override;

	    // my own methods

	U_I get_uncompressed_block_size() const { return uncompressed_block_size; };
	void change_uncompressed_block_size(U_I bs) { uncompressed_block_size = bs; };

    protected :
	virtual void inherited_read_ahead(const infinint & amount) override { run_read_threads(); };
        virtual U_I inherited_read(char *a, U_I size) override;
        virtual void inherited_write(const char *a, U_I size) override;
	virtual void inherited_truncate(const infinint & pos) override;
	virtual void inherited_sync_write() override;
	virtual void inherited_flush_read() override { stop_read_threads(); };
	virtual void inherited_terminate() override;

    private:
	static constexpr const U_I min_uncompressed_block_size = 100;

	    // initialized from constructors
	U_I num_w;
	std::unique_ptr<compress_module> zipper;
	generic_file *compressed;
	bool we_own_compressed_side;
	U_I uncompressed_block_size;

	    // initialized by init_fields method

	bool suspended;
	std::unique_ptr<crypto_segment> curwrite;
	std::deque<std::unique_ptr<crypto_segment> > lus_data;
	std::deque<signed int> lus_flags;
	bool reof;
	infinint current_position;

	std::shared_ptr<libthreadar::ratelier_scatter<crypto_segment> > disperse;
	std::shared_ptr<libthreadar::ratelier_gather<crypto_segment> > rassemble;
	std::shared_ptr<heap<crypto_segment> > tas;


	    // the subthreads

	std::unique_ptr<zip_below_read> reader;
	std::unique_ptr<zip_below_write> writer;
	std::deque<zip_worker> travailleurs;

	void init_fields();
	void send_flag_to_workers(compressor_block_flags flag);
	void stop_read_threads();
	void stop_write_threads();
	void run_read_threads();
	void run_write_threads();
	void purge_ratelier_for(compressor_block_flags flag);

	static U_I get_ratelier_size(U_I num_workers) { return num_workers + num_workers/2; };
	static U_I get_heap_size(U_I num_workers);
    };

	/// @}

} // end of namespace


#endif

