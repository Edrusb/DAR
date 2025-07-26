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
    ///
    /// the compressed block structure is a sequence of 2 types of data
    /// - data to compress/uncompress
    /// - eof
    /// .
    ///
    /// COMPRESSION PROCESS
    ///
    /// *** in the archive:
    ///
    /// the block stream is an arbitrary length sequence of data blocks followed by an eof.
    /// Each data block consists of an infinint determining the length of following field
    /// that stores the compressed data of variable length
    /// the compressed data comes from block of uncompressed data of constant size (except the
    /// last one that is smaller. THis information is needed to prepare memory allocation
    /// and is stored in the archive header (archive reading) or provided by the user
    /// (archive creation). Providing 0 for the block size leads to the classical/historical
    /// but impossible to parallelize compression algorithm (gzip, bzip2, etc.)
    ///
    ///
    ///
    /// *** between threads:
    ///
    /// when compressing, the inherited_write call produces a set of data block to the ratelier_scatter
    /// treated by a set of zip_workers which in turn passe the resulting compressed
    /// data blocks to the zip_below_write thread.
    ///
    /// last when eof is reached or during terminate() method,  N eof blocks are sent
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


	/// the different flags used to communicate between threads hold by parallel_block_compressor class

    enum class compressor_block_flags { data = 0, eof_die = 1, error = 2, worker_error = 3 };

	// the following classes hold the subthreads of class parallel_block_compressor
	// and are defined just after it below

    class zip_below_read;
    class zip_below_write;
    class zip_worker;


	/////////////////////////////////////////////////////
	//
	// paralle_compressor class, which holds the sub-threads
	//
	//

    class parallel_block_compressor: public proto_compressor
    {
    public:
	    /// \note if uncompressed_bs is too small the uncompressed data
	    /// will not be able to be stored in the datastructure and decompression
	    /// wil fail. This metadata should be stored in the archive header
	    /// and passed to the parallel_block_compressor object at
	    /// construction time both while reading and writing an archive

	parallel_block_compressor(U_I num_workers,
				  std::unique_ptr<compress_module> block_zipper,
				  generic_file & compressed_side,
				  U_I uncompressed_bs = default_uncompressed_block_size);
	    // compressed_side is not owned by the object and will remains
            // after the objet destruction

	parallel_block_compressor(const parallel_block_compressor & ref) = delete;
	parallel_block_compressor(parallel_block_compressor && ref) noexcept = delete;
	parallel_block_compressor & operator = (const parallel_block_compressor & ref) = delete;
	parallel_block_compressor & operator = (parallel_block_compressor && ref) noexcept = delete;
	~parallel_block_compressor();

	    // inherited from proto_compressor

	virtual compression get_algo() const override { return suspended? compression::none : zipper->get_algo(); };
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

    protected :
	virtual void inherited_read_ahead(const infinint & amount) override;
        virtual U_I inherited_read(char *a, U_I size) override;
        virtual void inherited_write(const char *a, U_I size) override;
	virtual void inherited_truncate(const infinint & pos) override;
	virtual void inherited_sync_write() override;
	virtual void inherited_flush_read() override { stop_read_threads(); reof = false; };
	virtual void inherited_terminate() override;

    private:

	    // the local fields

	U_I num_w;                                             ///< number of worker threads
	std::unique_ptr<compress_module> zipper;               ///< compress_module for the requested compression algo
	generic_file *compressed;                              ///< where to read from / write to, compressed data
	U_I uncompressed_block_size;                           ///< the max block size of un compressed data used
	bool suspended;                                        ///< whether compression is suspended or not
	bool running_threads;                                  ///< whether subthreads are running
	std::unique_ptr<crypto_segment> curwrite;              ///< aggregates a block before compression and writing
	std::deque<std::unique_ptr<crypto_segment> > lus_data; ///< uncompressed data from workers in read mode
	std::deque<signed int> lus_flags;                      ///< uncompressed data flags from workers in read mode
	bool reof;                                             ///< whether we have hit the end of file while reading


	    // inter-thread data structure

	std::shared_ptr<libthreadar::ratelier_scatter<crypto_segment> > disperse;
	std::shared_ptr<libthreadar::ratelier_gather<crypto_segment> > rassemble;
	std::shared_ptr<heap<crypto_segment> > tas;


	    // the subthreads

	std::unique_ptr<zip_below_read> reader;
	std::unique_ptr<zip_below_write> writer;
	std::deque<std::unique_ptr<zip_worker> > travailleurs;


	    // private methods

	void send_flag_to_workers(compressor_block_flags flag);
	void stop_threads();
	void stop_read_threads();
	void stop_write_threads();
	void run_threads();
	void run_read_threads();
	void run_write_threads();
	compressor_block_flags purge_ratelier_up_to_non_data();


	    // static methods

	static U_I get_ratelier_size(U_I num_workers) { return num_workers + num_workers/2; };
	static U_I get_heap_size(U_I num_workers);
    };



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
			U_I num_workers);

	~zip_below_write() { cancel(); join(); };


	    /// consulted by the main thread, set to true by the zip_below_write thread
	    /// when an exception has been caught or an error flag has been seen from a
	    /// worker
	bool exception_pending() const { return error; };

	    /// reset the thread objet ready for a new compression run, but does not launch it
	void reset();

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

	~zip_below_read() { cancel(); join(); };

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

	~zip_worker() { cancel(); join(); };

    protected:
	virtual void inherited_run() override;

    private:
	std::shared_ptr<libthreadar::ratelier_scatter <crypto_segment> > & reader;
	std::shared_ptr<libthreadar::ratelier_gather <crypto_segment> > & writer;
	std::unique_ptr<compress_module> compr;
	bool do_compress;
	bool error;
	std::unique_ptr<crypto_segment> transit;
	unsigned int transit_slot;

	void work();
    };


	/// @}

} // end of namespace


#endif

