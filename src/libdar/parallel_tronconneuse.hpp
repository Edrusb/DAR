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

    /// \file parallel_tronconneuse.hpp
    /// \brief defines a block structured file.
    /// \ingroup Private

    /// Several classes are defined here:
    /// - class parallel_tronconneuse, which has similar interface and behavior as class tronconneuse
    /// - class read_below, is used internally by parallel_tronconneuse in read-only mode to feed workers with encrypted data of the underlay
    /// - class write_below, is used internally by parallel_tronconneuse in write-only mode to write down the encrypted data produced by the workers
    /// - class crypto_workers, that transform data between parallel_tronconneuse and the read_below class or the write_below class
    /// depending on the context. Several objects of this class executes in parallel the crypto_module routine, but there is
    /// only one read_below or write_below thread, the class parallel_tronconneuse stays executed by the main/original/calling thread
    /// .

#ifndef PARALLEL_TRONCONNEUSE_HPP
#define PARALLEL_TRONCONNEUSE_HPP

#include "../my_config.h"
#include <string>

#include "infinint.hpp"
#include "archive_version.hpp"
#include "crypto_segment.hpp"
#include "heap.hpp"
#include "crypto_module.hpp"
#include "proto_tronco.hpp"

#include <libthreadar/libthreadar.hpp>

namespace libdar
{

        /// \addtogroup Private
        /// @{

        // those class are used by the parallel_tronconneuse class to wrap the different
        // type of threads. They are defined just after the parallel_tronconneuse definition
    class read_below;
    class write_below;
    class crypto_worker;

        /// status flags used between parallel_tronconneuse and its sub-threads

    enum class tronco_flags { normal = 0, stop = 1, eof = 2, die = 3, data_error = 4, exception_below = 5, exception_worker = 6, exception_error = 7 };


        /////////////////////////////////////////////////////
        //
        // the parallel_tronconneuse class that orchestrate all that
        //
        //


        /// this is a partial implementation of the generic_file interface to cypher/decypher data block by block.

        /// This class is a pure virtual one, as several calls have to be defined by inherited classes
        /// - encrypted_block_size_for
        /// - clear_block_allocated_size_for
        /// - encrypt_data
        /// - decrypt_data
        /// .
        /// parallel_tronconneuse is either read_only or write_only, read_write is not allowed.
        /// The openning mode is defined by encrypted_side's mode.
        /// In write_only no skip() is allowed, writing is sequential from the beginning of the file to the end
        /// (like writing to a pipe).
        /// In read_only all skip() functions are available.

    class parallel_tronconneuse : public proto_tronco
    {
    public:
            /// This is the constructor

            /// \param[in] workers is the number of worker threads
            /// \param[in] block_size is the size of block encryption (the size of clear data encrypted toghether).
            /// \param[in] encrypted_side where encrypted data are read from or written to.
            /// \param[in] reading_ver version of the archive format
            /// \param[in] ptr pointer to an crypto_module object that will be passed to the parallel_tronconneuse
            /// \note that encrypted_side is not owned and destroyed by tronconneuse, it must exist during all the life of the
            /// tronconneuse object, and is not destroyed by the tronconneuse's destructor
        parallel_tronconneuse(U_I workers,
                              U_32 block_size,
                              generic_file & encrypted_side,
                              const archive_version & reading_ver,
                              std::unique_ptr<crypto_module> & ptr);

            /// copy constructor
        parallel_tronconneuse(const parallel_tronconneuse & ref) = delete;

            /// move constructor
        parallel_tronconneuse(parallel_tronconneuse && ref) = default;

            /// assignment operator
        parallel_tronconneuse & operator = (const parallel_tronconneuse & ref) = delete;

            /// move operator
        parallel_tronconneuse & operator = (parallel_tronconneuse && ref) noexcept = default;

            /// destructor
        ~parallel_tronconneuse() noexcept;

            /// inherited from generic_file
        virtual bool skippable(skippability direction, const infinint & amount) override;
            /// inherited from generic_file
        virtual bool skip(const infinint & pos) override;
            /// inherited from generic_file
        virtual bool skip_to_eof() override;
            /// inherited from generic_file
        virtual bool skip_relative(S_I x) override;
            /// inherited from generic_file
        virtual bool truncatable(const infinint & pos) const override { return false; };
            /// inherited from generic_file
        virtual infinint get_position() const override { if(is_terminated()) throw SRC_BUG; return current_position; };

            /// in write_only mode indicate that end of file is reached

            /// this call must be called in write mode to purge the
            /// internal cache before deleting the object (else some data may be lost)
            /// no further write call is allowed
            /// \note this call cannot be used from the destructor, because it relies on pure virtual methods
        virtual void write_end_of_file() override { if(is_terminated()) throw SRC_BUG; sync_write(); };


            /// this method to modify the initial shift. This overrides the constructor "no_initial_shift" of the constructor

        virtual void set_initial_shift(const infinint & x) override;

            /// let the caller give a callback function that given a generic_file with cyphered data, is able
            /// to return the offset of the first clear byte located *after* all the cyphered data, this
            /// callback function is used (if defined by the following method), when reaching End of File.
        virtual void set_callback_trailing_clear_data(trailing_clear_data_callback call_back) override;

            /// returns the block size given to constructor
        virtual U_32 get_clear_block_size() const override { return clear_block_size; };

    private:

            // inherited from generic_file

            /// this protected inherited method is now private for inherited classes of tronconneuse
        virtual void inherited_read_ahead(const infinint & amount) override;

            /// this protected inherited method is now private for inherited classes of tronconneuse
        virtual U_I inherited_read(char *a, U_I size) override;

            /// inherited from generic_file

            /// this protected inherited method is now private for inherited classes of tronconneuse
        virtual void inherited_write(const char *a, U_I size) override;

            /// this prorected inherited method is now private for inherited classed of tronconneuse

            /// \note no skippability in write mode, so no truncate possibility neither, this call
            /// will always throw a bug exception
        virtual void inherited_truncate(const infinint & pos) override { throw SRC_BUG; };

            /// this protected inherited method is now private for inherited classes of tronconneuse
        virtual void inherited_sync_write() override;


            /// this protected inherited method is now private for inherited classes of tronconneuse
        virtual void inherited_flush_read() override;

            /// this protected inherited method is now private for inherited classes of tronconneuse
        virtual void inherited_terminate() override;

        const archive_version & get_reading_version() const { return reading_ver; };

            // internal data structure
        enum class thread_status { running, suspended, dead };

            // the fields

        U_I num_workers;               ///< number of worker threads
        U_32 clear_block_size;         ///< size of a clear block
        infinint current_position;     ///< current position for the upper layer perspective (modified by skip*, inherited_read/write, find_offset_in_lus_data)
        infinint initial_shift;        ///< the offset in the "encrypted" below layer at which starts the encrypted data
        archive_version reading_ver;   ///< archive format we follow
        std::unique_ptr<crypto_module> crypto;  ///< the crypto module use to cipher / uncipher block of data
        infinint (*mycallback)(generic_file & below, const archive_version & reading_ver);
        generic_file* encrypted;

            // fields used to represent possible status of subthreads and communication channel (the pipe)

        U_I ignore_stop_acks;          ///< how much stop ack still to be read (aborted stop order context)
        thread_status t_status;        ///< wehther child thread are waiting us on the barrier


            // the following stores data from the ratelier_gather to be provided for read() operation
            // the lus_data/lus_flags is what is extracted from the ratelier_gather, both constitute
            // the feedback channel from sub-threads to provide order acks and normal data

        std::deque<std::unique_ptr<crypto_segment> > lus_data;
        std::deque<signed int> lus_flags;
        bool lus_eof;
        bool check_bytes_to_skip; ///< whether to check for bytes to skip

            // the following stores data going to ratelier_scatter for the write() operation

        std::unique_ptr<crypto_segment> tempo_write;
        infinint block_num;

            // the datastructures shared among threads

        std::shared_ptr<libthreadar::ratelier_scatter<crypto_segment> > scatter;
        std::shared_ptr<libthreadar::ratelier_gather<crypto_segment> > gather;
        std::shared_ptr<libthreadar::barrier> waiter;
        std::shared_ptr<heap<crypto_segment> > tas;

            // the child threads

        std::deque<std::unique_ptr<crypto_worker> > travailleur;
        std::unique_ptr<read_below> crypto_reader;
        std::unique_ptr<write_below> crypto_writer;



            /// send and order to subthreads and gather acks from them

            /// \param[in] order is the order to send to the subthreads
            /// \param[in] for_offset is not zero is the offset we want to skip to
            /// (it is only taken into account when order is stop)
            /// \note with order stop and non zero for_offset, if the
            /// data at for_offset is found while purging the
            /// ratelier_gather, the purge is stopped, false is returned and
            /// the ignore_stop field is set to true, meaning that a stop order
            /// has not been purged from the pipe and should be ignored when its
            /// acknolegements will be met. In any other case true is returned,
            /// the subthreaded got the order and the ratelier has been purged.
        bool send_read_order(tronco_flags order, const infinint & for_offset = 0);

            /// send order in write mode
        void send_write_order(tronco_flags order);

            /// wake up threads in read mode when necessary
        void go_read();

            /// fill lus_data/lus_flags from ratelier_gather if these are empty
        void read_refill();

            /// purge the ratelier from the next order which is provided as returned value

            /// \param[in] pos if pos is not zero, the normal data located at
            /// pos offset is also looked for. If it is found before any order
            /// the call returns tronco_flags::normal and the order is not purged
            /// but the ignore_stop_acks fields is set to true for further
            /// reading to skip over this order acknolegments.
        tronco_flags purge_ratelier_from_next_order(infinint pos = 0);

            /// removing the ignore_stop_acks pending on the pipe

            /// \param[in] pos if pos is not zero and normal data is found at
            /// pos offset before all stop acks could get read, the call stops
            /// and return false. Else true is returned meaning all stop acks
            /// has been read and removed from the pipe
            /// \note this method acts the same pas purge_ratelier_from_next_order but fetch
            /// from ratelier up to data offset met of the pending ack to be all read
            /// but it then stops, it does not look for a real order after
            /// the pending stop acks of an aborted stop order
        bool purge_unack_stop_order(const infinint & pos = 0);

            /// flush lus_data/lus_flags up to requested pos offset to be found or all data has been removed

            /// \param[in] pos the data offset we look for
            /// \return true if the offset has been found and is
            /// ready for next inherited_read() call, false else
            /// and lus_data/lus_flags have been empties of
            /// the first non-order blocks found (data blocks)
            /// \note current_position is set upon success else
            /// it is unchanged but will not match what may still
            /// remain in the pipe
        bool find_offset_in_lus_data(const infinint & pos);

            /// reset the interthread datastructure and launch the threads
        void run_threads();

            /// end threads taking into account the fact they may be suspended on the barrier
        void stop_threads();

	    /// call by join_threads() below just code simplification around exception handling
	void join_workers_only();

            /// wait for threads to finish and eventually rethrow their exceptions in current thread
        void join_threads();


        static U_I get_ratelier_size(U_I num_worker) { return num_worker + num_worker/2; };
        static U_I get_heap_size(U_I num_worker);
    };


        /////////////////////////////////////////////////////
        //
        // read_below subthread used by parallel_tronconneuse
        // to dispatch chunk of encrypted data to the workers
        //

    class read_below: public libthreadar::thread
    {
    public:
        read_below(const std::shared_ptr<libthreadar::ratelier_scatter<crypto_segment> > & to_workers, ///< where to send chunk of crypted data
                   const std::shared_ptr<libthreadar::barrier> & waiter, ///< barrier used for synchronization with workers and the thread that called us
                   U_I num_workers,              ///< how much workers have to be informed when special condition occurs
                   U_I clear_block_size,         ///< clear block size used
                   generic_file* encrypted_side, ///< the encrypted file we fetch data from and slice in chunks for the workers
                   const std::shared_ptr<heap<crypto_segment> > xtas, ///< heap of pre-allocated memory for the chunks
                   infinint init_shift           ///< the offset at which the encrypted data is expected to start
	    );

        ~read_below() { if(ptr) tas->put(std::move(ptr)); cancel(); join(); };

            /// let the caller give a callback function that given a generic_file with mixed cyphered and clear data, is able
            /// to return the offset of the first clear byte located *after* all the cyphered data, this
            /// callback function is used (if defined by the following method), when reaching End of File.
        void set_callback_trailing_clear_data(trailing_clear_data_callback call_back) { trailing_clear_data = call_back; };

            // *** //
            // *** the method above should not be used anymore once the thread is running *** //
            // *** //

            /// set the initial shift must be called right after set_flag(stop) to take effect
        void set_initial_shift(const infinint & x) { initial_shift = x; };

            /// available for the parent thread to skip at another positon (read mode)

            /// \param[in] pos is the offset (from the upper layer / clear data point
            /// of view) at which we should seek
            /// \note changes asked by this call do not take effect until set_flag(stop)
            /// is invoked
        void set_pos(const infinint & pos) { skip_to = pos; };

            /// method for the parent thread to send a control order

            /// \note the parent thread must cleanup the gather object, it should
            /// expect to receive N block with the flag given to set_flag (N being
            /// the number of workers. Then this parent thread by a call the the
            /// waiter.Wait() barrier method will
            /// release all the worker and this below thread that will feed the
            /// pipe with new data taken at the requested offset provided by
            /// mean of the above set_pos() method.
            /// \note example: ask to stop before skipping
        void set_flag(tronco_flags val) { flag = val; };

	    /// method available for the parent thread to know the clear offset of the new flow

	    /// \note after set_pos()+set_flag() the new flow may start
	    /// a bit before the position requested by set_pos() due to
	    /// block of encryption boundaries. So the caller should ignore
	    /// some first bytes of data received
	const infinint & get_clear_flow_start() const { return clear_flow_start; };

	    /// method available for the parent thread to know the amount of bytes to skip in the new flow

	    /// \note this is the difference between the argument provided
	    /// to set_pos() and the value returned by get_clear_flow_start()
	    /// in other words this is the amount of bytes to skip in the
	    /// new flow, in order to
	const infinint & get_pos_in_flow() const { return pos_in_flow; };


    protected:
	virtual void inherited_run() override;

    private:
	std::shared_ptr<libthreadar::ratelier_scatter <crypto_segment> > workers; ///< object used to scatter data to workers
	std::shared_ptr<libthreadar::barrier> waiting;                            ///< barrier used to synchronize with worker and parent thread
	U_I num_w;                                                                ///< number of worker thread we are feeding through the ratelier_scatter
	U_I clear_buf_size;                                                       ///< amount of clear data per encrypted chunk
	generic_file* encrypted;                                                  ///< the encrypted data
	archive_version version;                                                  ///< archive version format
	std::shared_ptr<heap<crypto_segment> > tas;                               ///< where to fetch from blocks of data
	infinint initial_shift;                                                   ///< initial shift
	bool reof;                                                                ///< whether we reached eof while reading
	trailing_clear_data_callback trailing_clear_data;                         ///< callback function that gives the amount of clear data found at the end of the given file
	std::unique_ptr<crypto_segment> ptr;                                      ///< current segment we are setting up
	infinint index_num;                                                       ///< current crypto block index


	    // initialized by inherited_run() / get_ready_for_new_offset()

	U_I encrypted_buf_size;                                                   ///< size of the encrypted chunk of data

	    // fields accessible by both the caller and the read_below thread

	infinint skip_to;          ///< modification to this field is only done by the parent thread at a time it is not read by this thread
	tronco_flags flag;         ///< modification to this type is atomic so we do not use mutex
	infinint clear_flow_start; ///< modification of this field is only done by this thread a at time the parent thread does not read it
	infinint pos_in_flow;      ///< modification of this field is only done by this thread a at time the parent thread does not read it

	void work();
	infinint get_ready_for_new_offset();
	void send_flag_to_workers(tronco_flags theflag);

	    // same function as the tronconneuse::position_clear2crypt
	void position_clear2crypt(const infinint & pos,
				  infinint & file_buf_start,
				  infinint & clear_buf_start,
				  infinint & pos_in_buf,
				  infinint & block_num);

    };


	/////////////////////////////////////////////////////
	//
	// write_below subthread used by parallel_tronconneuse
	// to gather and write down encrypted data work from workers
	//

    class write_below: public libthreadar::thread
    {
    public:
	write_below(const std::shared_ptr<libthreadar::ratelier_gather<crypto_segment> > & from_workers,
		    const std::shared_ptr<libthreadar::barrier> & waiter,
		    U_I num_workers,
		    generic_file* encrypted_side,
		    const std::shared_ptr<heap<crypto_segment> > xtas);

	~write_below() { cancel(); join(); };

	bool exception_pending() const { return error; };
	const infinint & get_error_block() const { return error_block; };

    protected:
	virtual void inherited_run() override;

    private:
	std::shared_ptr<libthreadar::ratelier_gather<crypto_segment> > workers;
	std::shared_ptr<libthreadar::barrier> waiting;
	U_I num_w;
	U_I cur_num_w;
	generic_file* encrypted; ///< the encrypted data
	std::shared_ptr<heap<crypto_segment> > tas;
	bool error;
	infinint error_block; // last crypto block before error
	std::deque<std::unique_ptr<crypto_segment> >ones;
	std::deque<signed int> flags;

	void work();
    };


    	/////////////////////////////////////////////////////
	//
	// the crypto_worker threads performing ciphering/deciphering
	// of many data blocks in parallel
	//


    class crypto_worker: public libthreadar::thread
    {
    public:
	crypto_worker(std::shared_ptr<libthreadar::ratelier_scatter <crypto_segment> > & read_side,
		    std::shared_ptr<libthreadar::ratelier_gather <crypto_segment> > & write_side,
		    std::shared_ptr<libthreadar::barrier> waiter,
		    std::unique_ptr<crypto_module> && ptr,
		    bool encrypt);

	virtual ~crypto_worker() { cancel(); join(); };

    protected:
	virtual void inherited_run() override;

    private:
	enum class status { fine, inform, sent };

	std::shared_ptr<libthreadar::ratelier_scatter <crypto_segment> > & reader;
	std::shared_ptr<libthreadar::ratelier_gather <crypto_segment> > & writer;
	std::shared_ptr<libthreadar::barrier> waiting;
	std::unique_ptr<crypto_module> crypto;
	bool do_encrypt; // if false do decrypt
	std::unique_ptr<crypto_segment> ptr;
	unsigned int slot;
	status abort;

	void work();
    };


	/// @}

} // end of namespace

#endif
