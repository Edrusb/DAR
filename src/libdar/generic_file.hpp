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

    /// \file generic_file.hpp
    /// \brief class generic_file is defined here as well as class fichier
    /// \ingroup Private
    ///
    /// the generic_file interface is widely used in libdar
    /// it defines the standard way of transmitting data between different
    /// part of the library
    /// - compression engine
    /// - encryption engine
    /// - exchange through pipes
    /// - slicing
    /// - dry run operations
    /// - file access
    /// .

#ifndef GENERIC_FILE_HPP
#define GENERIC_FILE_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"

#include "proto_generic_file.hpp"
#include "integers.hpp"
#include "crc.hpp"
#include "infinint.hpp"
#include "gf_mode.hpp"

#include <string>

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// this is the interface class from which all other data transfer classes inherit

	/// it provides mainly read and write operations,
	/// skip operations and few other functions.
	/// \note
	/// the read and write method are similar to the read and write system calls
	/// except that they never return negative values, but throw exception instead.
	/// returning zero means end of generic_file. The call is blocked if
	/// no data is available for reading.
	/// write returns the number of bytes written, and never make partial writtings.
	/// Thus, it is blocked until all bytes are written or occures an exception
	/// inconsequences the returned value is always the value of the argument
	/// "size".
    class generic_file: public proto_generic_file
    {
    public :
	    /// main constructor
        generic_file(gf_mode m) { rw = m; terminated = no_read_ahead = false; enable_crc(false); checksum = nullptr; };

	    /// copy constructor
	generic_file(const generic_file &ref) { copy_from(ref); };

	    /// move constructor
	generic_file(generic_file && ref) noexcept { nullifyptr(); move_from(std::move(ref)); };

	    /// assignment operator
	generic_file & operator = (const generic_file & ref) { destroy(); copy_from(ref); return *this; };

	    /// move operator
	generic_file & operator = (generic_file && ref) noexcept { move_from(std::move(ref)); return *this; };

	    /// virtual destructor,

	    /// \note this let inherited destructor to be called even from a generic_file pointer to an inherited class
        ~generic_file() noexcept(false) { destroy(); };

	    /// destructor-like call, except that it is allowed to throw exceptions
	void terminate();

	    /// compaire the object content (implies skipping and reading)
	    ///
	    /// \note this is a more simple method than diff()
	bool operator == (generic_file & ref);
	bool operator != (generic_file & ref) { return ! (*this == ref); };

	    /// retreive the openning mode for this object
        gf_mode get_mode() const { return rw; };

	    /// read ahead information
	    ///
	    /// \param[in] amount is the expected amount of data the caller will read, if zero is given the
	    /// object shall prepare as much as possible data for reading until a skip request, write request or a new read_ahead request
	    /// \note after sanity checks, the protected inherited_read_ahead() method is called
	void read_ahead(const infinint & amount);

	    /// ignore read ahead requests
	    ///
	    /// \param [in] mode if set to true read_ahead requests are ignored: inherited_read_ahead() of the inherited class
	    /// is not called
	    /// \note read_ahead is useful in multi-thread environement when a slave thread can work in advanced and asynchronously
	    /// to provide read content to reader thread. However, read_ahead is a waste of CPU cycle for in a single threading model
	void ignore_read_ahead(bool mode) { no_read_ahead = mode; };

	    /// read data from the generic_file, inherited from proto_generic_file
        virtual U_I read(char *a, U_I size) override;

	    /// write data to the generic_file, inherited from proto_generic_file
        virtual void write(const char *a, U_I size) override;

	    /// \note throws a exception if not all data could be written as expected
        void write(const std::string & arg);

	    /// skip back one char, read on char and skip back one char
        S_I read_back(char &a);

	    /// read one char
	S_I read_forward(char &a) { if(terminated) throw SRC_BUG; return read(&a, 1); };


	enum skippability { skip_backward, skip_forward };

	    /// whether the implementation is able to skip

	    /// \note the capability to skip does not mean that skip_relative() or
	    /// skip() will succeed, but rather that the inherited class implementation
	    /// does not by construction forbid the requested skip (like inherited class
	    /// providing a generic_file interface of an anonymous pipe for example)
	virtual bool skippable(skippability direction, const infinint & amount) = 0;

	    /// skip at the absolute position

	    /// \param[in] pos the offset in byte where next read/write operation must start
	    /// \return true if operation was successfull and false if the requested position is not valid (after end of file)
	    /// \note if requested position is not valid the reading/writing cursor must be set to the closest valid position
        virtual bool skip(const infinint & pos) = 0;

	    /// skip to the end of file
        virtual bool skip_to_eof() = 0;

	    /// skip relatively to the current position
        virtual bool skip_relative(S_I x) = 0;

	    /// whether the implementation is able to truncate to the given position

	virtual bool truncatable(const infinint & pos) const = 0;

	    /// truncate file at the given offset

	    /// \param[in] pos is the offset of the where to cut the file, in other word the pos - 1 will be the last byte of the
	    /// file (if its size is greater than 1). If pos is greater than EOF the implementation should not enlarge the file
	    /// and may even no skip the file to its current eof. Though this request is not considered an error implementation
	    /// can thus do nothing when asked to truncate after EOF.
	    /// \note running this call on a read-only object will always yeld an exception
	    /// \note if current file position is after the point to truncate, implementation has to skip back the current writing
	    /// offset to the new end of file.
	    /// \note implementation of backward skippability and truncate operation should be coherent: If its not possible to
	    /// skip backward it should not be possible to truncate the file to this position. If truncate a file to requested
	    /// position is not possible, it should not be possible to skip backward to that position. This truncatable() with the
	    /// skip_backward direction *should* indicate whether the truncate() action will succeed or generate an exception
	virtual void truncate(const infinint & pos);

	    /// get the current read/write position
        virtual infinint get_position() const = 0;

	    /// copy all data from current position to the object in argument
        virtual void copy_to(generic_file &ref);

	    /// copy all data from the current position to the object in argument and computes a CRC value of the transmitted data

	    /// \param[in] ref defines where to copy the data to
	    /// \param[in] crc_size tell the width of the crc to compute on the copied data
	    /// \param[out] value points to a newly allocated crc object containing the crc value
	    /// \note value has to be deleted by the caller when no more needed
        virtual void copy_to(generic_file &ref, const infinint & crc_size, crc * & value);

	    /// small copy (up to 4GB) with CRC calculation
        U_32 copy_to(generic_file &ref, U_32 size); // returns the number of byte effectively copied

	    /// copy the given amount to the object in argument
        infinint copy_to(generic_file &ref, infinint size); // returns the number of byte effectively copied

	    /// compares the contents with the object in argument

	    /// \param[in] f is the file to compare the current object with
	    /// \param[in] me_read_ahead is the amount of data to read ahead from "*this" (0 for no limit, up to end of eof)
	    /// \param[in] you_read_ahead is the amount of data to read ahead from f (0 for no limit, up to end of file
	    /// \param[in] crc_size is the width of the CRC to use for calculation
	    /// \param[out] value is the computed checksum, its value can be used for additional
	    /// testing if this method returns false (no difference between files). The given checksum
	    /// has to be set to the expected width by the caller.
	    /// \return true if arg differ from "this"
	    /// \note value has to be deleted by the caller when no more needed
	    /// \note the more simple operator == method may be prefered to diff() when no crc calculation is required
        bool diff(generic_file & f,
		  const infinint & me_read_ahead,
		  const infinint & you_read_ahead,
		  const infinint & crc_size,
		  crc * & value);

	    /// compare the contents with the object in argument, also providing the offset of the first difference met
	    /// \param[in] f is the file to compare the current object with
	    /// \param[in] me_read_ahead is the amount of data to read ahead from "*this" (0 for no limit, up to end of eof)
	    /// \param[in] you_read_ahead is the amount of data to read ahead from f (0 for no limit, up to end of file
	    /// \param[in] crc_size is the width of the CRC to use for calculation
	    /// \param[out] value is the computed checksum, its value can be used for additional
	    /// \param[out] err_offset in case of difference, holds the offset of the first difference met
	    /// testing if this method returns false (no difference between files). The given checksum
	    /// has to be set to the expected width by the caller.
	    /// \return true if arg differ from "this", else false is returned and err_offset is set
	    /// \note value has to be deleted by the caller when no more needed
	    /// \note the more simple operator == method may be prefered to diff() when no crc calculation is required
	bool diff(generic_file & f,
		  const infinint & me_read_ahead,
		  const infinint & you_read_ahead,
		  const infinint & crc_size,
		  crc * & value,
		  infinint & err_offset);

            /// reset CRC on read or writen data

	    /// \param[in] width is the width to use for the CRC
        void reset_crc(const infinint & width);

	    /// to known whether CRC calculation is activated or not
	bool crc_status() const { return active_read == &generic_file::read_crc; };

	    /// get CRC of the transfered date since last reset

	    /// \return a newly allocated crc object, that the caller has the responsibility to delete
	    /// \note does also ends checksum calculation, which if needed again
	    /// have to be re-enabled calling reset_crc() method
        crc *get_crc();

	    /// write any pending data
	void sync_write();

	    /// be ready to read at current position, reseting all pending data for reading, cached and in compression engine for example
	void flush_read();


    protected :
        void set_mode(gf_mode x) { rw = x; };

	    /// tells the object that several calls to read() will follow to probably obtain at least the given amount of data

	    /// \param[in] amount is the maximum expected amount of data that is known to be read
	    /// \note this call may be implemented as a do-nothing call, its presence is only
	    /// to allow optimization when possible, like in multi-threaded environment
	virtual void inherited_read_ahead(const infinint & amount) = 0;


	    /// implementation of read() operation

	    /// \param[in,out] a where to put the data to read
	    /// \param[in] size says how much data to read
	    /// \return the exact amount of data read and put into 'a'
	    /// \note read as much byte as requested, up to end of file
	    /// stays blocked if not enough data is available and EOF not
	    /// yet met. May return less data than requested only if EOF as been reached.
	    /// in other worlds, EOF is reached when returned data is stricly less than the requested data
	    /// Any problem shall be reported by throwing an exception.
        virtual U_I inherited_read(char *a, U_I size) = 0;

	    /// implementation of the write() operation

	    /// \param[in] a what data to write
	    /// \param[in] size amount of data to write
	    /// \note must either write all data or report an error by throwing an exception
        virtual void inherited_write(const char *a, U_I size) = 0;

	    /// truncate file at the give offset

	    /// \note if pos is greater than the current file size, this call may do nothing (not even enlarging the file)
	    /// \note this call should always fail on a read-only generic_file
	    /// \note implementation must throw exception if truncate is not possible for other reason than
	    /// read/write access mode
	virtual void inherited_truncate(const infinint & pos) = 0;


	    /// write down any pending data

	    /// \note called after sanity checks from generic_file::sync_write()
	    /// this method's role is to write down any data pending for writing in the current object
	    /// it has not to be propagated to other gneric_file object this object could rely on
	virtual void inherited_sync_write() = 0;

	    /// reset internal engine, flush caches in order to read the data at current position

	    /// \note when the object relies on external object or system object to fetch the data from for reading,
	    /// when a call to (inherited_)flush_read() occurs, the current object must not assume that any previously read
	    /// data is still valid if it has internal buffers or the like and it should flush them asap. This call must
	    /// not propagate the flush_read to any other gneric_file object it could rely on
	virtual void inherited_flush_read() = 0;

	    /// destructor-like call, except that it is allowed to throw exceptions

	    /// \note this method must never be called directly but using terminate() instead,
	    /// generic_file class manages it to never be called more than once
	virtual void inherited_terminate() = 0;


	    /// is some specific call (skip() & Co.) need to be forbidden when the object
	    /// has been terminated, one can use this call to check the terminated status
	bool is_terminated() const { return terminated; };

    private :
        gf_mode rw;
        crc *checksum;
	bool terminated;
	bool no_read_ahead;
        U_I (generic_file::* active_read)(char *a, U_I size);
        void (generic_file::* active_write)(const char *a, U_I size);

        void enable_crc(bool mode);

        U_I read_crc(char *a, U_I size);
        void write_crc(const char *a, U_I size);
	void destroy();
	void nullifyptr() noexcept { checksum = nullptr; };
	void copy_from(const generic_file & ref);
	void move_from(generic_file && ref) noexcept;
    };

	/// @}

} // end of namespace

#endif
