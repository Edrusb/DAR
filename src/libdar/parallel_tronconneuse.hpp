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

    /// \file parallel_tronconneuse.hpp
    /// \brief defines a block structured file.
    /// \ingroup Private
    ///
    /// Mainly used for strong encryption.

#ifndef PARALLEL_TRONCONNEUSE_HPP
#define PARALLEL_TRONCONNEUSE_HPP

#include "../my_config.h"
#include <string>

#include "infinint.hpp"
#include "generic_file.hpp"
#include "archive_version.hpp"
#include "crypto_segment.hpp"
#include "heap.hpp"
#include "crypto_module.hpp"

#if HAVE_LIBTHREADAR_LIBTHREADAR_HPP
#include <libthreadar/libthreadar.hpp>
#endif


namespace libdar
{

	/// \addtogroup Private
	/// @{

    enum class tronco_flags { normal = 0, stop = 1, eof = 2, die = 3, data_error = 4 };

    class read_below: public libthreadar::thread
    {
    public:
	read_below(const std::shared_ptr<libthreadar::ratelier_scatter<crypto_segment> > & to_workers, ///< where to send chunk of crypted data
		   const std::shared_ptr<libthreadar::barrier> & waiter, ///< barrier used for synchronization with workers and the thread that called us
		   U_I num_workers,              ///< how much workers have to be informed when special condition occurs
		   U_I clear_block_size,         ///< clear block size used
		   generic_file* encrypted_side, ///< the encrypted file we fetch data from and slice in chunks for the workers
		   const std::shared_ptr<heap<crypto_segment> > xtas, ///< heap of pre-allocated memory for the chunks
		   infinint init_shift):         ///< the offset at which the encrypted data is expected to start
	    workers(to_workers),
	    waiting(waiter),
	    num_w(num_workers),
	    clear_buf_size(clear_block_size),
	    encrypted(encrypted_side),
	    tas(xtas),
	    initial_shift(init_shift),
	    reof(false),
	    trailing_clear_data(nullptr)
	{ flag = tronco_flags::normal; };

	    /// let the caller give a callback function that given a generic_file with mixed cyphered and clear data, is able
	    /// to return the offset of the first clear byte located *after* all the cyphered data, this
	    /// callback function is used (if defined by the following method), when reaching End of File.
	void set_callback_trailing_clear_data(infinint (*call_back)(generic_file & below, const archive_version & reading_ver)) { trailing_clear_data = call_back; };

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
	std::shared_ptr<libthreadar::barrier> waiting;  ///< barrier used to synchronize with worker and parent thread
	U_I num_w;            ///< number of worker thread we are feeding through the ratelier_scatter
	U_I clear_buf_size;   ///< amount of clear data per encrypted chunk
	generic_file* encrypted; ///< the encrypted data
	archive_version version; ///< archive version format
	std::shared_ptr<heap<crypto_segment> > tas; ///< where to fetch from blocks of data
	infinint initial_shift;                     ///< initial shift
	bool reof;                                  ///< whether we reached eof while reading
	infinint (*trailing_clear_data)(generic_file & below, const archive_version & reading_ver); ///< callback function that gives the amount of clear data found at the end of the given file

	    // initialized by inherited_run() / get_ready_for_new_offset()
	infinint crypt_offset;                     ///< position to skip to in 'below' to start reading crypted data
	U_I encrypted_buf_size;                    ///< size of the encrypted chunk of data

	    // fields accessible by both the caller and the running thread

	infinint skip_to;  // modification to this field is only done by the parent thread at a time it is not read by this thread
	tronco_flags flag; // modification to this type is atomic so we do not use mutex
	infinint clear_flow_start; // modification of this field is only done by this thread a at time the parent thread does not read it
	infinint pos_in_flow; // modification of this field is only done by this thread a at time the parent thread does not read it

	infinint get_ready_for_new_offset();
	void send_flag_to_workers(tronco_flags theflag);

	    // same function as the tronconneuse::position_clear2crypt
	void position_clear2crypt(const infinint & pos,
				  infinint & file_buf_start,
				  infinint & clear_buf_start,
				  infinint & pos_in_buf,
				  infinint & block_num);

    };


    class write_below: public libthreadar::thread
    {
    public:
	write_below(const std::shared_ptr<libthreadar::ratelier_gather<crypto_segment> > & from_workers,
		    const std::shared_ptr<libthreadar::barrier> & waiter,
		    U_I num_workers,
		    generic_file* encrypted_side,
		    const std::shared_ptr<heap<crypto_segment> > xtas):
	    workers(from_workers),
	    waiting(waiter),
	    num_w(num_workers),
	    encrypted(encrypted_side),
	    tas(xtas) { if(encrypted == nullptr) throw SRC_BUG; };

    protected:
	virtual void inherited_run() override;

    private:
	std::shared_ptr<libthreadar::ratelier_gather<crypto_segment> > workers;
	std::shared_ptr<libthreadar::barrier> waiting;
	U_I num_w;
	generic_file* encrypted; ///< the encrypted data
	std::shared_ptr<heap<crypto_segment> > tas;


    };

    class crypto_worker: public libthreadar::thread
    {
    public:
	crypto_worker(std::shared_ptr<libthreadar::ratelier_scatter <crypto_segment> > & read_side,
		    std::shared_ptr<libthreadar::ratelier_gather <crypto_segment> > & write_side,
		    std::shared_ptr<libthreadar::barrier> waiter,
		    std::unique_ptr<crypto_module> && ptr,
		    bool encrypt):
	    reader(read_side),
	    writer(write_side),
	    waiting(waiter),
	    crypto(move(ptr)),
	    do_encrypt(encrypt)
	{ if(!reader || !writer || !waiting || !crypto) throw SRC_BUG; };

    protected:
	virtual void inherited_run() override;

    private:
	std::shared_ptr<libthreadar::ratelier_scatter <crypto_segment> > & reader;
	std::shared_ptr<libthreadar::ratelier_gather <crypto_segment> > & writer;
	std::shared_ptr<libthreadar::barrier> waiting;
	std::unique_ptr<crypto_module> crypto;
	bool do_encrypt; // if false do decrypt
    };

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

    class parallel_tronconneuse : public generic_file
    {
    public:
	    /// This is the constructor

	    /// \param[in] workers is the number of worker threads
	    /// \param[in] block_size is the size of block encryption (the size of clear data encrypted toghether).
	    /// \param[in] encrypted_side where encrypted data are read from or written to.
  	    /// \param[in] no_initial_shift assume that no unencrypted data is located at the begining of the underlying file, else this is the
	    /// position of the encrypted_side at the time of this call that is used as initial_shift
	    /// \param[in] reading_ver version of the archive format
	    /// \param[in] ptr pointer to an crypto_module object that will be passed to the parallel_tronconneuse
	    /// \note that encrypted_side is not owned and destroyed by tronconneuse, it must exist during all the life of the
	    /// tronconneuse object, and is not destroyed by the tronconneuse's destructor
	parallel_tronconneuse(U_I workers,
			      U_32 block_size,
			      generic_file & encrypted_side,
			      bool no_initial_shift,
			      const archive_version & reading_ver,
			      std::unique_ptr<crypto_module> & ptr);

	    /// copy constructor
	parallel_tronconneuse(const parallel_tronconneuse & ref) = delete;

	    /// move constructor
	parallel_tronconneuse(parallel_tronconneuse && ref) noexcept = default;

	    /// assignment operator
 	parallel_tronconneuse & operator = (const parallel_tronconneuse & ref) = delete;

	    /// move operator
	parallel_tronconneuse & operator = (parallel_tronconneuse && ref) noexcept = default;

	    /// destructor
	~parallel_tronconneuse();

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
	void write_end_of_file() { throw SRC_BUG; }; /* A IMPLEMENTER */


	    /// this method to modify the initial shift. This overrides the constructor "no_initial_shift" of the constructor

	void set_initial_shift(const infinint & x);

	    /// let the caller give a callback function that given a generic_file with cyphered data, is able
	    /// to return the offset of the first clear byte located *after* all the cyphered data, this
	    /// callback function is used (if defined by the following method), when reaching End of File.
	void set_callback_trailing_clear_data(infinint (*call_back)(generic_file & below, const archive_version & reading_ver))
	{ if(crypto_reader) { mycallback = call_back; crypto_reader->set_callback_trailing_clear_data(call_back); } else throw SRC_BUG; };

	    /// returns the block size given to constructor
	U_32 get_clear_block_size() const { return clear_block_size; };

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

	    // the fields

	bool initialized;          ///< whether post_constructor_init() has been run
	U_I num_workers;
	U_32 clear_block_size;
	infinint current_position;
	infinint initial_shift;
	archive_version reading_ver;
	std::unique_ptr<crypto_module> crypto;
	bool suspended;              ///< wehther child thread are waiting us on the barrier
	infinint (*mycallback)(generic_file & below, const archive_version & reading_ver);
	generic_file* encrypted;

	    // the following stores data from the ratelier_gather to be provided for read() operation
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
	std::deque<crypto_worker> travailleur;
	std::unique_ptr<read_below> crypto_reader;
	std::unique_ptr<write_below> crypto_writer;

	    /// initialize fields that could be not be from constructor
	    /// then run the child threads
	void post_constructor_init();
	void send_read_order(tronco_flags order);
	void send_write_order(tronco_flags order);
	void go_read();
	void read_refill();
	tronco_flags purge_ratelier_from_next_order();

	static U_I get_heap_size(U_I num_worker);
    };

	/// @}

} // end of namespace

#endif
